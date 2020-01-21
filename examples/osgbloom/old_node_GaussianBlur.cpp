#include "GaussianBlur.h"

#include <cassert>
#include <sstream>
#include <iterator>

#include <osgDB/ReadFile>
#include <osg/Geometry>
#include <osg/Texture2D>

namespace galaxy
{

GaussianBlur::GaussianBlur(
    osg::Texture2D* input, osg::Texture2D* output, int times )
{
    _leaf = new osg::Geode;

    _quad = osg::createTexturedQuadGeometry(
        osg::Vec3( -1, -1, 0 ), osg::Vec3( 2, 0, 0 ), osg::Vec3( 0, 2, 0 ) );
    _quad->setName( "quad" );
    _quad->setUseDisplayList( false );
    _leaf->addDrawable( _quad );

    _program = new osg::Program;
    _program->setName( "GaussianBlur" );
    auto vertShader =
        osgDB::readShaderFile( osg::Shader::VERTEX, "shader/quad.vert" );
    auto fragShader = osgDB::readShaderFile(
        osg::Shader::FRAGMENT, "shader/gaussianblur.frag" );
    _program->addShader( vertShader );
    _program->addShader( fragShader );

    // mean = 0, standard deviation = 2, gaussian_step = 1, step = 5
    setWeights( {0.204164, 0.180174, 0.123832, 0.0662822, 0.0276306} );

    auto ss = getOrCreateStateSet();
    ss->setAttributeAndModes( _program );
    ss->addUniform( new osg::Uniform( "quad_map", 0 ) );

    setup( input, output, times );
}

void GaussianBlur::setup(
    osg::Texture2D* input, osg::Texture2D* output, int times )
{
    _times = times;
    _input = input;
    _output = output;

    if ( !output || output->getInternalFormat() == 0 ||
         output->getTextureWidth() == 0 || output->getTextureHeight() == 0 )
    {
        OSG_WARN << __func__ << " : invalid output " << std::endl;
        return;
    }

    if ( _input == _output )
    {
        OSG_WARN
            << __func__
            << " : you can't use the same Texture2D for both input and output"
            << std::endl;
        return;
    }

    // RenderStage set texture size to viewport size for 0 sized texture
    _pong = new osg::Texture2D;
    _pong->setInternalFormat( _output->getInternalFormat() );

    auto ustep = 1.0f / output->getTextureWidth();
    auto vstep = 1.0f / output->getTextureHeight();

    _inputStateSet = createCameraStateSet( osg::Vec2( ustep, 0 ), _input );
    _pingStateSet = createCameraStateSet( osg::Vec2( ustep, 0 ), _output );
    _pongStateSet = createCameraStateSet( osg::Vec2( 0, vstep ), _pong );

    removeChild( 0, getNumChildren() );

    //  CullVisitor reset RenderStage the for cached RenderStage in Pre or Post
    //  render camera, you also can't render the same RenderStage twice,
    //  RenderStage::_stageDrawnThisFrame won't let you do that, so I have to
    //  use one camera for one post processing.
    for ( auto i = 0; i < times * 2; ++i )
    {
        auto camera = new osg::Camera;
        camera->setStateSet( i == 0
                                 ? _inputStateSet
                                 : i % 2 == 0 ? _pingStateSet : _pongStateSet );
        camera->setRenderTargetImplementation(
            osg::Camera::FRAME_BUFFER_OBJECT );
        camera->setRenderOrder( osg::Camera::PRE_RENDER );
        camera->setClearMask( 0 );
        camera->setViewport(
            0, 0, _output->getTextureWidth(), _output->getTextureHeight() );
        camera->attach(
            osg::Camera::COLOR_BUFFER0, i % 2 == 0 ? _pong : _output );
        camera->addChild( _leaf );
        addChild( camera );
    }
}

void GaussianBlur::setWeights( const std::vector<float>& v )
{
    _weights = v;
    auto ss = getOrCreateStateSet();
    ss->setDefine( "RADIUS", std::to_string( v.size() - 1 ) );

    std::stringstream os;
    os << "float[RADIUS + 1]( ";
    std::copy( _weights.begin(), _weights.end() - 1,
        std::ostream_iterator<float>( os, ", " ) );
    if ( _weights.size() > 1 )
    {
        os << _weights.back();
    }
    os << " )";
    ss->setDefine( "WEIGHTS", os.str() );
}

osg::StateSet* GaussianBlur::createCameraStateSet(
    const osg::Vec2& step, osg::Texture* tex )
{
    auto ss = new osg::StateSet;
    ss->addUniform( new osg::Uniform( "texture_step", step ) );
    ss->setTextureAttributeAndModes( 0, tex );
    return ss;
}

} // namespace galaxy
