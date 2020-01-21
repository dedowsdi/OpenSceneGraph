#include "OsgFactory.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osg/Program>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>
#include <osg/Drawable>
#include <cassert>

namespace osgf
{

osg::Geometry* getNdcQuad()
{
    static osg::ref_ptr<osg::Geometry> quad;
    if ( !quad )
    {
        quad = osg::createTexturedQuadGeometry( osg::Vec3( -1, -1, 0 ),
            osg::Vec3( 2, 0, 0 ), osg::Vec3( 0, 2, 0 ) );
        quad->setUseDisplayList( false );
        quad->setUseVertexBufferObjects( true );
        quad->setCullingActive( false );
        quad->setName( "NdcQuad" );
    }
    return quad;
}

osg::Geode* createNdcQuadLeaf()
{
    auto leaf = new osg::Geode;
    leaf->addDrawable( getNdcQuad() );
    leaf->setCullingActive( false );
    return leaf;
}

osg::Program* createProgram(
    const std::string& vertFile, const std::string& fragFile )
{
    auto prg = new osg::Program;
    auto vertShader = osgDB::readShaderFile( osg::Shader::VERTEX, vertFile );
    auto fragShader = osgDB::readShaderFile( osg::Shader::FRAGMENT, fragFile );
    prg->addShader( vertShader );
    prg->addShader( fragShader );
    return prg;
}

osg::Program* createProgram( const std::string& vertFile,
    const std::string& geomFile, const std::string& fragFile, int inputType,
    int outputType, int maxVertices )
{
    auto prg = new osg::Program;
    auto vertShader = osgDB::readShaderFile( osg::Shader::VERTEX, vertFile );
    auto geomShader = osgDB::readShaderFile( osg::Shader::GEOMETRY, geomFile );
    auto fragShader = osgDB::readShaderFile( osg::Shader::FRAGMENT, fragFile );
    prg->addShader( vertShader );
    prg->addShader( geomShader );
    prg->addShader( fragShader );

    prg->setParameter( GL_GEOMETRY_INPUT_TYPE_EXT, inputType );
    prg->setParameter( GL_GEOMETRY_OUTPUT_TYPE_EXT, outputType );
    prg->setParameter( GL_GEOMETRY_VERTICES_OUT_EXT, maxVertices );
    return prg;
}

osg::Camera* createPrerenderCamera( int x, int y, int width, int height )
{
    auto camera = new osg::Camera;
    camera->setViewport( x, y, width, height );
    camera->setRenderOrder( osg::Camera::PRE_RENDER );
    camera->setClearMask( 0 );
    return camera;
}

osg::Camera* createRttCamera(
    int x, int y, int width, int height, int implementation )
{
    auto camera = createPrerenderCamera( x, y, width, height );
    camera->setRenderTargetImplementation(
        static_cast<osg::Camera::RenderTargetImplementation>(
            implementation ) );
    return camera;
}

osg::Camera* createFilterCamera( int x, int y, int width, int height )
{
    auto camera = createRttCamera(
        x, y, width, height, osg::Camera::FRAME_BUFFER_OBJECT );
    camera->getOrCreateStateSet()->setMode(
        GL_DEPTH_TEST, osg::StateAttribute::OFF );
    return camera;
}

osg::Texture2D* createTexture(
    int internalFormat, int width, int height, int minFilter, int magFilter )
{
    auto tex = new osg::Texture2D;
    tex->setInternalFormat( internalFormat );
    tex->setTextureSize( width, height );
    tex->setFilter( osg::Texture::MIN_FILTER,
        static_cast<osg::Texture::FilterMode>( minFilter ) );
    tex->setFilter( osg::Texture::MAG_FILTER,
        static_cast<osg::Texture::FilterMode>( magFilter ) );
    return tex;
}

osg::FrameBufferObject* createFboRtt( osg::StateSet* ss, osg::Texture2D* tex0,
    osg::Texture2D* tex1, osg::Texture2D* tex2 )
{
    assert( tex0 );

    auto fbo = new osg::FrameBufferObject();
    fbo->setAttachment(
        osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment( tex0 ) );
    ss->setAttributeAndModes( fbo );
    ss->setAttributeAndModes( new osg::Viewport(
        0, 0, tex0->getTextureWidth(), tex0->getTextureHeight() ) );

    if ( tex1 )
    {
        if ( tex0->getTextureWidth() != tex1->getTextureWidth() ||
             tex0->getTextureHeight() != tex1->getTextureHeight() )
        {
            OSG_WARN << "inconsistent texture size." << std::endl;
        }
        fbo->setAttachment(
            osg::Camera::COLOR_BUFFER1, osg::FrameBufferAttachment( tex1 ) );
    }

    if ( tex2 )
    {
        if ( tex0->getTextureWidth() != tex2->getTextureWidth() ||
             tex0->getTextureHeight() != tex2->getTextureHeight() )
        {
            OSG_WARN << "inconsistent texture size." << std::endl;
        }
        fbo->setAttachment(
            osg::Camera::COLOR_BUFFER2, osg::FrameBufferAttachment( tex2 ) );
    }

    return fbo;
}

class BlankDrawable : public osg::Drawable
{
public:
    BlankDrawable() = default;

    BlankDrawable( DrawableDrawFunc func ) : _drawFunc( func )
    {
        setName( "BlankDrawable" );
    }

    BlankDrawable( const osg::Drawable& drawable,
        const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY )
        : Drawable( drawable, copyop )
    {
    }

    META_Node( galaxy, BlankDrawable );

    virtual void drawImplementation( osg::RenderInfo& renderInfo ) const
    {
        if ( _drawFunc )
        {
            _drawFunc( renderInfo, this );
        }
        else
        {
            OSG_WARN << "Zero draw func" << std::endl;
        }
    }

    DrawableDrawFunc getDrawFunc() const { return _drawFunc; }
    void setDrawFunc( DrawableDrawFunc v ) { _drawFunc = v; }

private:
    DrawableDrawFunc _drawFunc;
};

osg::Drawable* createDrawable( DrawableDrawFunc func )
{
    return new BlankDrawable( func );
}

osg::Drawable* createClearDrawable(
    int mask, const osg::Vec4& clearColor, float depth )
{
    auto func = [mask, clearColor, depth](
                    osg::RenderInfo&, const osg::Drawable* ) -> void {
        if ( mask & GL_COLOR_BUFFER_BIT )
        {
            glClearColor( clearColor.x(), clearColor.y(), clearColor.z(),
                clearColor.w() );
            glClear( GL_COLOR_BUFFER_BIT );
        }

        if ( mask & GL_DEPTH_BUFFER_BIT )
        {
            glClearDepth( depth );
            glClear( GL_DEPTH_BUFFER_BIT );
        }
    };

    auto drawable = new BlankDrawable( func );
    drawable->setName( "ClearDrawable" );
    return drawable;
}

osg::Drawable* createCopyTex2DDrawable( osg::Texture2D* tex, int xoffset,
    int yoffset, int x, int y, int width, int height )
{
    if ( width == 0 ) width = tex->getTextureWidth();

    if ( height == 0 ) height = tex->getTextureHeight();

    if ( width == 0 || height == 0 )
        OSG_WARN << __FUNCTION__ << " : 0 width or height" << std::endl;

    auto func = [=]( osg::RenderInfo& renderInfo,
                    const osg::Drawable* ) -> void {
        tex->copyTexSubImage2D(
            *renderInfo.getState(), xoffset, yoffset, x, y, width, height );
    };

    auto drawable = new BlankDrawable( func );
    drawable->setName( "CopyTex2DDrawable" );
    return drawable;
}

} // namespace osgf
