#include <iterator>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osg/AnimationPath>
#include <osg/LineWidth>
#include <osg/Geometry>
#include <osg/Viewport>
#include <osgViewer/Renderer>
#include <osgUtil/RenderStage>
#include <osgGA/StateSetManipulator>
#include <osgDB/ReadFile>
#include <osg/Depth>
#include <osg/BlendFunc>
#include "Lightning.h"
#include "../osgbloom/GaussianBlur.cpp"
#include "../common/OsgHelper.h"
#include "../common/OsgFactory.h"

osg::Node* createSphere()
{
    auto leaf = new osg::Geode;
    auto shape = new osg::Sphere( osg::Vec3( 0, 0, 0 ), 1 );
    auto drawable = new osg::ShapeDrawable( shape );
    leaf->addDrawable( drawable );
    return leaf;
}

osg::Node* createLightning()
{
    auto lightning = new osg::Geode;
    auto lightningGraph = new galaxy::LightningSoftware;
    lightningGraph->setName("Lightning");
    lightningGraph->setBillboard( true );
    // lightningGraph->add(
    //     "jfjfjf", osg::Vec3( -5, 0, 0 ), osg::Vec3( 5, 0, 0 ) );
    // lightningGraph->add(
    //     "jfjfjf", osg::Vec3( 0, 50, 50 ), osg::Vec3( 0, 10, 0 ) );
    lightningGraph->add( "ffjjjf", osg::Vec3( -5, 0, 0 ), osg::Vec3( 5, 0, 0 ) );
    lightningGraph->add( "jjjf", osg::Vec3( -5, 0, 0 ), osg::Vec3( 5, 0, 0 ) );
    lightning->addChild( lightningGraph );
    lightningGraph->setBillboardWidth( 0.2f );
    return lightning;
}

osg::Node* createBrightnessQuad( osg::Texture2D* colorMap )
{
    auto quad = osgf::createNdcQuadLeaf();

    auto prg = new osg::Program;
    auto vertShader =
        osgDB::readShaderFile( osg::Shader::VERTEX, "shader/quad.vert" );
    auto fragShader = osgDB::readShaderFile(
        osg::Shader::FRAGMENT, "shader/brightness.frag" );
    prg->addShader( vertShader );
    prg->addShader( fragShader );

    auto ss = quad->getOrCreateStateSet();
    ss->setAttributeAndModes(prg);
    ss->setTextureAttributeAndModes(0, colorMap);
    ss->addUniform( new osg::Uniform( "quad_map", 0 ) );
    ss->addUniform( new osg::Uniform( "threshold", 0.8f ));

    return quad;
}

osg::Node* createBlendQuad( osg::Texture2D* colorMap, osg::Texture2D* bluredMap ) {
    auto quad = osgf::createNdcQuadLeaf();

    auto prg = new osg::Program;
    auto vertShader = osgDB::readShaderFile( osg::Shader::VERTEX, "shader/quad.vert" );
    auto fragShader =
        osgDB::readShaderFile( osg::Shader::FRAGMENT, "shader/lightning.frag" );
    prg->addShader( vertShader );
    prg->addShader( fragShader );

    auto ss = quad->getOrCreateStateSet();
    ss->setAttributeAndModes( prg );
    ss->setTextureAttributeAndModes( 0, colorMap );
    ss->setTextureAttributeAndModes( 1, bluredMap );
    ss->addUniform(new osg::Uniform("color_map", 0));
    ss->addUniform(new osg::Uniform("blured_color_map", 1));

    return quad;
}

// one post camera + 1 bin per render target
osg::Node* createBloomBin( int width, int height )
{
    auto root = new osg::Camera;
    root->setClearMask( 0 );
    root->setRenderOrder( osg::Camera::POST_RENDER );
    root->setName( "PostProcessingCamera" );

    int binNum = 0;

    // copy color map
    auto colorMap = osgf::createTexture(
        GL_RGBA, width, height, osg::Texture2D::LINEAR, osg::Texture2D::LINEAR );
    {
        auto leaf = new osg::Geode;
        root->addChild( leaf );

        auto copyNode = osgf::createCopyTex2DDrawable(
            static_cast<osg::Texture2D*>( colorMap ) );
        leaf->addDrawable( copyNode );

        auto ss = leaf->getOrCreateStateSet();
        ss->setRenderBinDetails( ++binNum, "RenderBin" );
    }

    // get brightness
    auto blurWidth = width * 0.25;
    auto blurHeight = height * 0.25;

    auto brightnessMap = osgf::createTexture( GL_RGBA, blurWidth, blurHeight,
        osg::Texture2D::LINEAR, osg::Texture2D::LINEAR );
    {
        auto group = new osg::Group;
        root->addChild( group );

        // new framebuffer
        auto groupSS = group->getOrCreateStateSet();
        osgf::createFboRtt(groupSS, brightnessMap);
        groupSS->setRenderBinDetails( ++binNum, "RenderBin" );
        groupSS->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

        auto quad = createBrightnessQuad( colorMap );
        group->addChild( quad );
        // quad->getOrCreateStateSet()->setRenderBinDetails(2, "RenderBin");
    }

    // blur brightness
    auto blurMap = osgf::createTexture( GL_RGBA, blurWidth, blurHeight,
        osg::Texture2D::LINEAR, osg::Texture2D::LINEAR );
    {
        auto blurNode = new galaxy::GaussianBlur( brightnessMap, blurMap, 2 );
        root->addChild( blurNode );
        blurNode->setWeights( {0.103153, 0.0999789, 0.0910319, 0.0778637,
            0.0625652, 0.0472267, 0.0334888, 0.0223083, 0.0139602} );
        blurNode->getOrCreateStateSet()->setRenderBinDetails(
            ++binNum, "RenderBin" );
    }

    // add blured result back to color map
    {
        auto quad = createBlendQuad( colorMap, blurMap );
        root->addChild( quad );
        quad->getOrCreateStateSet()->setRenderBinDetails(
            ++binNum, "RenderBin" );
    }

    return root;
}

// one camera per rtt
osg::Node* createBloomCamera(int width, int height)
{
    auto root = new osg::Camera;
    root->setClearMask( 0 );
    root->setRenderOrder( osg::Camera::POST_RENDER );
    root->setName( "PostProcessingCamera" );

    // copy color map
    auto colorCamera = osgf::createPrerenderCamera(0, 0, width, height );
    colorCamera->setName( "CopyColorMapCamera");
    root->addChild( colorCamera );
    colorCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER );
    auto colorMap = osgf::createTexture( GL_RGBA, width, height,
        osg::Texture2D::LINEAR, osg::Texture2D::LINEAR );
    colorCamera->attach( osg::Camera::COLOR_BUFFER, colorMap, 0 );

    // copy brightness
    auto brightnessCamera = osgf::createRttCamera(
        0, 0, width * 0.25, height * 0.25, osg::Camera::FRAME_BUFFER_OBJECT );
    brightnessCamera->setName( "BrightnessCamera");
    root->addChild( brightnessCamera );
    brightnessCamera->setClearMask(GL_COLOR_BUFFER_BIT);
    brightnessCamera->setClearColor( osg::Vec4(0, 0, 0, 1 ) );
    auto brightnessMap = osgf::createTexture( GL_RGBA, width * 0.25,
        height * 0.25, osg::Texture::LINEAR, osg::Texture::LINEAR );
    brightnessCamera->addChild( createBrightnessQuad(colorMap) );
    brightnessCamera->attach( osg::Camera::COLOR_BUFFER0, brightnessMap );
    brightnessCamera->getOrCreateStateSet()->setMode(
        GL_DEPTH_TEST, osg::StateAttribute::OFF );

    // blur
    auto blurCamera =
        osgf::createPrerenderCamera( 0, 0, width * 0.25, height * 0.25 );
    blurCamera->setName( "BlurCamera");
    root->addChild(blurCamera);
    auto blurredMap = osgf::createTexture( GL_RGBA, width * 0.25,
        height * 0.25, osg::Texture2D::LINEAR, osg::Texture2D::LINEAR );
    auto blurNode = new galaxy::GaussianBlur( brightnessMap, blurredMap, 2 );
    blurNode->setWeights( {0.103153, 0.0999789, 0.0910319, 0.0778637, 0.0625652,
        0.0472267, 0.0334888, 0.0223083, 0.0139602} );

    blurCamera->addChild(blurNode);

    // add blured result back to color map
    auto blendCamera =
        osgf::createRttCamera( 0, 0, width, height, osg::Camera::FRAME_BUFFER );
    blendCamera->setName( "BlurCamera");
    root->addChild(blendCamera);
    blendCamera->addChild( createBlendQuad( colorMap, blurredMap ) );

    return root;
}

int main( int argc, char* argv[] )
{
    osgViewer::Viewer viewer;
    viewer.realize();
    auto vp = viewer.getCamera()->getViewport();
    auto width = vp->width();
    auto height = vp->height();

    auto root = new osg::Group;

    auto lightning = createLightning();
    root->addChild(lightning);
    lightning->getOrCreateStateSet()->setRenderBinDetails(99, "RenderBin");

    // create a sphere in world center
    auto sphere = new osg::MatrixTransform;
    root->addChild( sphere );
    sphere->addChild( createSphere() );

    // root->addChild( createBloomCamera(width, height) );
    root->addChild( createBloomBin(width, height) );

    // viewer.getCamera()->setClearMask( 0 );
    viewer.getCamera()->setName( "main" );
    viewer.setSceneData( root );
    viewer.addEventHandler( new osgViewer::StatsHandler );
    auto debugHandler =  new galaxy::OsgDebugHandler;
    debugHandler->setCamera( viewer.getCamera() );
    debugHandler->setRoot( viewer.getCamera() );
    viewer.addEventHandler(debugHandler);

    return viewer.run();
}
