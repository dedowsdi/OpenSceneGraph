#include "GaussianBlur.h"

#include <iterator>

#include <osg/Camera>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgUtil/RenderStage>

namespace galaxy
{

class GaussianDrawcallback : public osg::Drawable::DrawCallback
{
public:
    GaussianDrawcallback( osg::Texture2D* input, osg::Texture2D* output,
        int times, osg::Program* program )
        : _times( times ), _input( input ), _output( output ),
          _program( program )
    {
        _pong = dynamic_cast<osg::Texture2D*>(
            output->clone( osg::CopyOp::SHALLOW_COPY ) );
    }

    virtual void drawImplementation(
        osg::RenderInfo& renderInfo, const osg::Drawable* drawable ) const
    {

        auto renderBinStack = renderInfo.getRenderBinStack();
        if ( renderBinStack.empty() )
        {
            // it's empty when compiling display list
            return drawable->drawImplementation( renderInfo );
        }

        auto stage =
            dynamic_cast<osgUtil::RenderStage*>( renderBinStack.back() );
        if ( !stage ) return;

        auto fbo = stage->getFrameBufferObject();
        if ( !fbo ) return;

        auto state = renderInfo.getState();
        auto textureStepLocation =
            _program->getPCP( *state )->getUniformLocation( "texture_step" );
        auto cid = state->getContextID();

        // Force create texture object.
        if ( !_output->getTextureObject( cid ) ) _output->apply( *state );

        if ( !_pong->getTextureObject( cid ) ) _pong->apply( *state );

        // get ids, steps
        auto pingTexId = _output->getTextureObject( cid )->_id;
        auto pongTexId = _pong->getTextureObject( cid )->_id;
        auto inputTexId = _input->getTextureObject( cid )->_id;

        auto sstep = osg::Vec2( 1.0 / _output->getTextureWidth(), 0 );
        auto tstep = osg::Vec2( 0, 1.0 / _output->getTextureHeight() );

        auto ext = state->get<osg::GLExtensions>();
        for ( auto i = 0; i < _times * 2; ++i )
        {
            auto step = i % 2 == 0 ? sstep.ptr() : tstep.ptr();
            auto renderTarget = i % 2 == 0 ? pongTexId : pingTexId;
            auto inputMap =
                i == 0 ? inputTexId : i % 2 == 0 ? pingTexId : pongTexId;

            ext->glUniform2fv( textureStepLocation, 1, step );
            ext->glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTarget, 0 );
            glBindTexture( GL_TEXTURE_2D, inputMap );

            drawable->drawImplementation( renderInfo );
        }
    }

private:
    int _times = 1;
    osg::Texture2D* _input = 0;
    osg::Texture2D* _output = 0;
    osg::Program* _program = 0;
    osg::ref_ptr<osg::Texture2D> _pong;
};

GaussianBlur::GaussianBlur(osg::Texture2D* input, osg::Texture2D* output, int times ):
    _times(times)
{
    _camera = new osg::Camera;
    _camera->setName("GaussianBlur");
    addChild(_camera);
    _camera->setClearMask( 0 );
    _camera->setClearColor( osg::Vec4() );
    _camera->setRenderOrder(osg::Camera::PRE_RENDER);
    _camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    _camera->attach( osg::Camera::COLOR_BUFFER0, output );
    _camera->setViewport(
        0, 0, output->getTextureWidth(), output->getTextureHeight() );

    auto quad = new osg::Geode;
    _camera->addChild( quad );
    auto quadGraph = osg::createTexturedQuadGeometry(
        osg::Vec3( -1, -1, -1 ), osg::Vec3( 2, 0, 0 ), osg::Vec3( 0, 2, 0 ) );
    quad->addDrawable( quadGraph );
    quadGraph->setUseDisplayList( false );
    quadGraph->setUseVertexBufferObjects( true );
    quadGraph->setName("GaussianBlurQuad");

    auto program = new osg::Program;
    program->setName( "GaussianBlur" );
    auto vertShader =
        osgDB::readShaderFile( osg::Shader::VERTEX, "shader/quad.vert" );
    auto fragShader = osgDB::readShaderFile(
        osg::Shader::FRAGMENT, "shader/gaussianblur.frag" );
    program->addShader( vertShader );
    program->addShader( fragShader );

    auto ss = _camera->getOrCreateStateSet();
    ss->setAttributeAndModes( program );
    ss->setTextureAttributeAndModes( 0, input );
    ss->addUniform( new osg::Uniform( "quad_map", 0 ) );
    ss->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );
    auto stepUniform = new osg::Uniform( "texture_step", osg::Vec2( 0, 0 ) );
    ss->addUniform( stepUniform );

    setWeights( {0.134598, 0.127325, 0.107778, 0.081638, 0.055335,
        0.0335624, 0.0182159, 0.00884695} );

    quadGraph->setDrawCallback(
        new GaussianDrawcallback( input, output, times, program ) );
}

void GaussianBlur::setWeights( const std::vector<float>& v )
{
    auto ss = getOrCreateStateSet();
    ss->setDefine( "RADIUS", std::to_string( v.size() - 1 ) );

    std::stringstream os;
    os << "float[RADIUS + 1]( ";
    std::copy(
        v.begin(), v.end() - 1, std::ostream_iterator<float>( os, ", " ) );
    if ( v.size() > 1 )
    {
        os << v.back();
    }
    os << " )";
    ss->setDefine( "WEIGHTS", os.str() );
}

}
