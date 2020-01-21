#include "GaussianBlur.h"

#include <cassert>
#include <iterator>

#include <osgDB/ReadFile>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>

namespace galaxy
{

class GaussianBlurTechnique : public osgFX::Technique
{

public:
    GaussianBlurTechnique();

    void define_passes() override;

    osg::Node* getOverrideChild(int) override;

    GaussianBlur* getEffect() const { return _effect; }
    void setEffect( GaussianBlur* v ) { _effect = v; }

private:
    GaussianBlur* _effect = 0;
};

GaussianBlur::GaussianBlur(
    osg::Texture2D* input, osg::Texture2D* output, int times )
{
    setName( effectName() );

    _quad = new osg::Geode;
    _quad->setCullingActive( false );

    _quadGeom = osg::createTexturedQuadGeometry(
        osg::Vec3( -1, -1, 0 ), osg::Vec3( 2, 0, 0 ), osg::Vec3( 0, 2, 0 ) );
    _quad->addDrawable( _quadGeom );
    _quadGeom->setName( "GaussianBlurQuad" );
    _quadGeom->setUseDisplayList( false );
    _quadGeom->setUseVertexBufferObjects( true );
    _quadGeom->setCullingActive( false );

    _program = new osg::Program;
    _program->setName( "GaussianBlur" );
    auto vertShader =
        osgDB::readShaderFile( osg::Shader::VERTEX, "shader/quad.vert" );
    auto fragShader = osgDB::readShaderFile(
        osg::Shader::FRAGMENT, "shader/gaussianblur.frag" );
    _program->addShader( vertShader );
    _program->addShader( fragShader );

    auto ss = getOrCreateStateSet();
    ss->setAttributeAndModes( _program );
    ss->addUniform( new osg::Uniform( "quad_map", 0 ) );
    ss->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

    setup(input, output, times);
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

void GaussianBlur::setup(
    osg::Texture2D* input, osg::Texture2D* output, int times )
{
    _times = times;
    _input = input;
    _output = output;

    if ( !_input || !_output ) return;

    if ( _input == _output )
    {
        OSG_WARN
            << __FUNCTION__
            << " : you can't use the same Texture2D for both input and output"
            << std::endl;
        return;
    }

    if (_output->getTextureWidth() == 0 || _output->getTextureHeight() == 0)
    {
        OSG_WARN << __FUNCTION__ << " : 0 sized output texture " << std::endl;
        return;
    }

    _pong = new osg::Texture2D;
    _pong->setInternalFormat( _output->getInternalFormat() );
    _pong->setTextureSize(
        output->getTextureWidth(), output->getTextureHeight() );

    auto ss = getOrCreateStateSet();
    ss->setAttributeAndModes( new osg::Viewport(
        0, 0, output->getTextureWidth(), output->getTextureHeight() ) );

    dirtyTechniques();
}

bool GaussianBlur::define_techniques()
{
    auto tech = new GaussianBlurTechnique();
    tech->setEffect( this );
    addTechnique( tech );
    return true;
}

GaussianBlurTechnique::GaussianBlurTechnique() {}

void GaussianBlurTechnique::define_passes()
{
    assert( _effect );

    auto input = _effect->getInput();
    auto ping = _effect->getOutput();
    auto pong = _effect->getPong();
    auto ustep = 1.0f / ping->getTextureWidth();
    auto vstep = 1.0f / ping->getTextureHeight();

    // create ping pong frame buffer object
    auto fboPing =
        osg::ref_ptr<osg::FrameBufferObject>( new osg::FrameBufferObject );
    fboPing->setAttachment(
        osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment( pong, 0 ) );

    auto fboPong =
        osg::ref_ptr<osg::FrameBufferObject>( new osg::FrameBufferObject );
    fboPong->setAttachment(
        osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment( ping, 0 ) );

    // horizontal and vertical blur n times
    for ( auto i = 0; i < _effect->getTimes() * 2; ++i )
    {
        // you can't reuse StateSet in Technique, everything you call addPass,
        // it'ss RenderBin detail is changed.
        auto ss = new osg::StateSet;
        auto step = i % 2 == 0 ? osg::Vec2( ustep, 0 ) : osg::Vec2( 0, vstep );
        auto inputMap = i == 0 ? input : i % 2 == 0 ? ping : pong;
        auto fbo = i % 2 == 0 ? fboPing : fboPong;

        ss->addUniform( new osg::Uniform( "texture_step", step ) );
        ss->setTextureAttributeAndModes( 0,  inputMap);
        ss->setAttributeAndModes(fbo);

        addPass(ss);
    }
}

osg::Node* GaussianBlurTechnique::getOverrideChild(int) 
{
    return getEffect()->getQuad();
}

} // namespace galaxy
