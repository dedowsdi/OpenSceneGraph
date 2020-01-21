#include "Lightning.h"

#include <cassert>
#include <vector>
#include <random>
#include <functional>
#include <algorithm>

#include <osg/Geometry>
#include <osg/io_utils>
#include <osgDB/ReadFile>
#include <osg/BlendFunc>
#include <osg/BlendEquation>
#include <osg/Depth>

namespace galaxy
{

// [0,1)
inline float unitrand()
{
    static auto gen = []() {
        std::random_device rd;
        return std::mt19937( rd() );
    }();
    static std::uniform_real_distribution<> dist( 0.0f, 1.0f );
    return dist( gen );
}

// [a, b)
inline float linearRand( float a, float b )
{
    return a + unitrand() * ( b - a );
}

inline osg::Vec3 sphericalRand( float radius )
{
    auto theta = unitrand() * osg::PI;
    auto phi = unitrand() * osg::PI * 2;
    auto sinTheta = sin( theta );
    return osg::Vec3( sinTheta * std::cos( phi ), sinTheta * std::sin( phi ),
               cos( theta ) ) *
           radius;
}

// Assume ndir normalized
inline osg::Vec3 randomOrghogonal( const osg::Vec3& ndir )
{
    // find a reference vector that's not parallel to ndir
    auto ref = sphericalRand( 1.0 );
    while ( std::abs( ndir * ref ) > 0.999f )
    {
        ref = sphericalRand( 1.0 );
    }

    auto perp = ndir ^ ref;
    return perp;
}

} // namespace galaxy

namespace galaxy
{

class LightningSoftwareUpdater : public osg::Callback
{
public:
    LightningSoftwareUpdater( LightningSoftware& lightning )
        : _lightning( &lightning )
    {
    }

    bool run( osg::Object* object, osg::Object* data ) override
    {
        _lightning->resetLightning();
        return traverse( object, data );
    }

private:
    LightningSoftware* _lightning;
};

LightningSoftware::LightningSoftware()
{
    // _geom = new osg::Geometry;
    setUseDisplayList( false );
    setDataVariance( osg::Object::DYNAMIC );
    addPrimitiveSet( new osg::DrawArrays( GL_LINES, 0, 0 ) );

    _vertices = new osg::Vec3Array( osg::Array::BIND_PER_VERTEX );
    setVertexArray( _vertices );

    setUpdateCallback( new LightningSoftwareUpdater( *this ) );

    auto ss = getOrCreateStateSet();
    ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

    // Closing gap block lines behind it, disable depth write for blending. If
    // you need depth, you must write this again without color mask.
    ss->setAttributeAndModes( new osg::Depth( osg::Depth::LESS, 0, 1, false ) );
    ss->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ONE));
    ss->setAttributeAndModes(
        new osg::BlendEquation( osg::BlendEquation::RGBA_MAX ) );

    // render it as billboard, create faces along lines
    _billboardProgram = new osg::Program;

    auto vertShader = osgDB::readShaderFile(
        osg::Shader::VERTEX, "shader/lightning_billboard.vert" );
    auto geomShader = osgDB::readShaderFile(
        osg::Shader::GEOMETRY, "shader/lightning_billboard.geom" );
    auto fragShader = osgDB::readShaderFile(
        osg::Shader::FRAGMENT, "shader/lightning_billboard.frag" );
    _billboardProgram->addShader( vertShader );
    _billboardProgram->addShader( fragShader );

    // 3 rect = 12 vertices
    _billboardProgram->setParameter( GL_GEOMETRY_VERTICES_OUT_EXT, 12 );
    _billboardProgram->setParameter( GL_GEOMETRY_INPUT_TYPE_EXT, GL_LINES );
    _billboardProgram->setParameter( GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP );
    _billboardProgram->addShader( geomShader );

}

void LightningSoftware::add(
    const std::string& pattern, const osg::Vec3& p0, const osg::Vec3& p1 )
{
    _bolts.push_back( Bolt{pattern, p0, p1} );
}

void LightningSoftware::resetLightning()
{
    // if ( !_vertices->empty() ) return;

    _vertices->clear();

    auto totalVeritces = std::accumulate( _bolts.begin(), _bolts.end(), 0ul,
        []( int sum, Bolt& b ) { return sum + b.getNumVertices(); } );

    _vertices->reserve( totalVeritces );

    for ( auto& bolt : _bolts )
    {
        auto boltVertices = createLightning( bolt );
        std::copy( boltVertices.begin(), boltVertices.end(),
            std::back_inserter( *_vertices ) );
    }

    assert( _vertices->size() <= totalVeritces );

    _vertices->dirty();
    static_cast<osg::DrawArrays*>( getPrimitiveSet( 0 ) )
        ->setCount( _vertices->size() );
    dirtyBound();
}

void LightningSoftware::setBillboardWidth( float v )
{
    _billboardWidth = v;
    getOrCreateStateSet()->addUniform(
        new osg::Uniform( "billboard_width", v ) );
}

void LightningSoftware::setCenterColor( const osg::Vec4& v )
{
    _centerColor = v;
    getOrCreateStateSet()->addUniform( new osg::Uniform( "center_color", v ) );
}

void LightningSoftware::setBorderColor( const osg::Vec4& v )
{ _borderColor = v;
    getOrCreateStateSet()->addUniform( new osg::Uniform( "border_color", v ) );
}

void LightningSoftware::setBillboard( bool v )
{
    _billboard = v;
    auto ss = getOrCreateStateSet();
    if ( _billboard )
    {
        ss->setAttributeAndModes( _billboardProgram );
        setBillboardWidth( _billboardWidth );
        setCenterColor( _centerColor );
        setBorderColor( _borderColor );
        setExponent( _exponent );
    }
    else
    {
        ss->removeAttribute( osg::StateAttribute::PROGRAM );
        ss->removeUniform( "billboard_width" );
        ss->removeUniform( "color" );
    }
}

void LightningSoftware::setExponent(float v)
{
    _exponent = v;
    getOrCreateStateSet()->addUniform( new osg::Uniform( "exponent", v ) );
}

std::vector<osg::Vec3> LightningSoftware::createLightning( const Bolt& bolt )
{
    // subdivide bolt recursively with a ping pong vector :
    //      subdivide ping into pong, swap ping pong.
    // The result is in the ping.

    auto ping = std::vector<osg::Vec3>();
    auto pong = std::vector<osg::Vec3>();
    auto boltVertices = bolt.getNumVertices();
    ping.reserve( boltVertices );
    pong.reserve( ping.capacity() );

    ping.push_back( bolt.start );
    ping.push_back( bolt.end );

    GLfloat segmentLength = ( bolt.end - bolt.start ).length();
    if ( segmentLength == 0 )
    {
        OSG_WARN << "0 length bolt, ignred" << std::endl;
        return std::vector<osg::Vec3>{};
    }

    auto direction = bolt.end - bolt.start;
    direction /= segmentLength;

    for ( auto letter : bolt.pattern )
    {
        if ( letter != 'j' && letter != 'f' ) continue;

        segmentLength *= 0.5f;

        pong.clear();
        auto pongIter = std::back_inserter( pong );
        for ( auto pingIter = ping.begin(); pingIter != ping.end(); )
        {
            // jitter
            auto& p0 = *pingIter++;
            auto& p1 = *pingIter++;
            auto p2 = ( p0 + p1 ) * 0.5f;
            auto v01 = p1 - p0;
            v01.normalize();

            auto v01Perp = randomOrghogonal( v01 );
            v01Perp.normalize();
            p2 += v01Perp * unitrand() * _maxJitter * segmentLength;

            // segment 0
            *pongIter++ = p0;
            *pongIter++ = p2;

            // segment 1
            *pongIter++ = p2;
            *pongIter++ = p1;

            // fork
            if ( letter == 'f' && unitrand() < _forkRate )
            {
                // fork len should be the same as normal segment?
                GLfloat theta = linearRand( -_maxForkAngle, _maxForkAngle );

                auto directionPerp = randomOrghogonal( direction );
                directionPerp.normalize();
                auto forkDir =
                    direction * cos( theta ) + directionPerp * sin( theta );

                // segment 2
                *pongIter++ = p2;
                *pongIter++ = p2 + forkDir * segmentLength;
            }
        }

        ping.swap( pong );
    }

    assert( ping.size() <= static_cast<unsigned>( boltVertices ) );
    return ping;
}

int LightningSoftware::Bolt::getNumVertices() const
{
    auto numVertices = 2;
    for ( auto& letter : pattern )
    {
        if ( letter == 'j' )
        {
            numVertices *= 2;
        }
        else if ( letter == 'f' )
        {
            numVertices *= 3;
        }
    }
    return numVertices;
}

} // namespace galaxy
