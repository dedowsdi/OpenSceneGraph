#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include "GaussianBlur.h"
#include <osgDB/ReadFile>
#include "OsgHelper.h"
#include "TextureFilter.h"

osg::Node* createCube()
{
    auto leaf = new osg::Geode;
    auto box = new osg::Box(osg::Vec3(), 2, 2, 2);
    auto drawable = new osg::ShapeDrawable(box);
    leaf->addDrawable(drawable);
    return leaf;
}

osg::Texture2D* createOutputTexture(GLenum format, int width, int height)
{
    auto output = new osg::Texture2D;
    output->setInternalFormat( format );
    output->setTextureSize( width, height );
    return output;
}


int main( int argc, char* argv[] )
{
    auto root = new osg::Group;
    auto rootSS = root->getOrCreateStateSet();

    auto tf = new osg::MatrixTransform;
    root->addChild(tf);

    auto texture = new osg::Texture2D;
    auto img = osgDB::readImageFile("Images/osg64.png");
    texture->setImage(img);

    auto brightnessOutput =
        createOutputTexture( texture->getInternalFormat(), img->s(), img->t() );
    auto blurOutput = createOutputTexture(texture->getInternalFormat(), img->s(), img->t());
    auto bloomOutput = createOutputTexture(texture->getInternalFormat(), img->s(), img->t());

    auto brightnessFilter =
        new galaxy::BrightnessFilter( texture, brightnessOutput );
    brightnessFilter->setThreshold(0.8f);
    auto blurFilter = new galaxy::GaussianBlur(brightnessOutput, blurOutput, 3);
    auto bloomFilter = new galaxy::BloomFilter(texture, blurOutput, bloomOutput);
    bloomFilter->setExposure(0.8);
    tf->addChild(brightnessFilter);
    tf->addChild(blurFilter);
    tf->addChild(bloomFilter);

    auto cube = createCube();

    // use rtt texture
    tf->addChild(cube);
    auto ss = tf->getOrCreateStateSet();
    ss->setTextureAttributeAndModes( 0, bloomOutput );

    osgViewer::Viewer viewer;
    viewer.setSceneData( root );
    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgGA::StateSetManipulator( rootSS ) );
    auto handler = new galaxy::OsgDebugHandler;
    handler->setRoot(root);
    handler->setCamera(viewer.getCamera());
    viewer.addEventHandler(handler);

    return viewer.run();
}
