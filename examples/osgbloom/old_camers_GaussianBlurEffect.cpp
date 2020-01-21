#include "GaussianBlurEffect.h"

#include <cassert>

#include <osgDB/ReadFile>
#include <osg/Geometry>
#include <osg/Texture2D>

namespace galaxy
{

class GaussianBlurTechnique : public osgFX::Technique
{

public:
    GaussianBlurTechnique();

    void define_passes() override;

    osg::Node* getOverrideChild( int i ) override;

    GaussianBlur* getEffect() const { return _effect; }
    void setEffect( GaussianBlur* v ) { _effect = v; }

private:
    GaussianBlur* _effect = 0;
};

GaussianBlur::GaussianBlur()
{
    _leaf = new osg::Geode;

    _quad = osg::createTexturedQuadGeometry(
        osg::Vec3( -1, -1, 0 ), osg::Vec3( 2, 0, 0 ), osg::Vec3( 0, 2, 0 ) );
    _quad->setName( "quad" );
    _quad->setUseDisplayList( false );
    _quad->setUseVertexBufferObjects( true );
    _leaf->addDrawable( _quad );

    _program = new osg::Program;
    _program->setName( "GaussianBlur" );
    auto vertShader =
        osgDB::readShaderFile( osg::Shader::VERTEX, "data/shader/quad.vert" );
    auto fragShader = osgDB::readShaderFile(
        osg::Shader::FRAGMENT, "data/shader/gaussianblur.frag" );
    _program->addShader( vertShader );
    _program->addShader( fragShader );
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

    if ( _input->getTextureWidth() != _output->getTextureWidth() ||
         _output->getTextureHeight() != _output->getTextureHeight() )
    {
        OSG_WARN << __FUNCTION__ << " : Different input and output size"
                 << std::endl;
    }

    // _pong = dynamic_cast<osg::Texture2D*>(
    //     _input->clone( osg::CopyOp::SHALLOW_COPY ) );
    // _pong->setImage( 0 );
    // _pong->setDataVariance(osg::ObjectDynamic);

    _pong = new osg::Texture2D;
    _pong->setInternalFormat( _output->getInternalFormat() );

    _cameras.clear();
    for ( auto i = 0; i < times * 2; ++i )
    {
        auto camera = createCamera();
        camera->attach(
            osg::Camera::COLOR_BUFFER0, i % 2 == 0 ? _pong.get() : _output );
        camera->setViewport(
            0, 0, _input->getTextureWidth(), _input->getTextureHeight() );
        _cameras.push_back( camera );
    }

    dirtyTechniques();
}

bool GaussianBlur::define_techniques()
{
    auto tech = new GaussianBlurTechnique();
    tech->setEffect( this );
    addTechnique( tech );
    return true;
}

osg::Camera* GaussianBlur::createCamera()
{
    auto camera = new osg::Camera;
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    camera->setRenderOrder( osg::Camera::PRE_RENDER );
    camera->getOrCreateStateSet()->setAttributeAndModes( _program );
    camera->getOrCreateStateSet()->addUniform(
        new osg::Uniform( "quadMap", 0 ) );
    camera->addChild( _leaf );
    return camera;
}

GaussianBlurTechnique::GaussianBlurTechnique() {}

void GaussianBlurTechnique::define_passes()
{
    assert( _effect );

    auto input = _effect->getInput();
    auto ping = _effect->getOutput();
    auto pong = _effect->getPong();
    auto ustep = 1.0f / input->getTextureWidth();
    auto vstep = 1.0f / input->getTextureHeight();

    // Start horizontal ping pong
    for ( auto i = 0; i < _effect->getTimes(); ++i )
    {
        auto ss = new osg::StateSet();
        ss->setTextureAttributeAndModes( 0, i == 0 ? input : ping );
        ss->addUniform(
            new osg::Uniform( "textureStep", osg::Vec2( ustep, 0 ) ) );
        addPass( ss );
        std::swap( ping, pong );
    }

    // Start vertical ping pong
    for ( auto i = 0; i < _effect->getTimes(); ++i )
    {
        auto ss = new osg::StateSet();
        ss->setTextureAttributeAndModes( 0, ping );
        ss->addUniform(
            new osg::Uniform( "textureStep", osg::Vec2( 0, vstep ) ) );
        addPass( ss );
        std::swap( ping, pong );
    }

    // We are doing n*2 times rendering, the final result is always in the
    // required output.
}

osg::Node* GaussianBlurTechnique::getOverrideChild( int i )
{
    return _effect->getCamera( i );
}

} // namespace galaxy
