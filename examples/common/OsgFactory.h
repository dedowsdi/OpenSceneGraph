#ifndef UFO_OSGFACTORY_H
#define UFO_OSGFACTORY_H

#include <string>
#include <functional>
#include <osg/Vec4>

namespace osg
{
class Geometry;
class Geode;
class Node;
class Program;
class Camera;
class Texture2D;
class StateSet;
class FrameBufferObject;
class Drawable;
class RenderInfo;
} // namespace osg

namespace osgf
{

osg::Geometry* getNdcQuad();

osg::Geode* createNdcQuadLeaf();

osg::Program* createProgram(
    const std::string& vertFile, const std::string& fragFile );

osg::Program* createProgram( const std::string& vertFile,
    const std::string& geomFile, const std::string& fragFile, int inputType,
    int outputType, int maxVertices );

// 0 clear mask.
osg::Camera* createPrerenderCamera( int x, int y, int width, int height );

// 0 clear mask, pre render.
osg::Camera* createRttCamera(
    int x, int y, int width, int height, int implementation );

// 0 clear mask, pre render, framebufferobject, no depth test.
osg::Camera* createFilterCamera( int x, int y, int width, int height );

osg::Texture2D* createTexture(
    int internalFormat, int width, int height, int minFilter, int magFilter );

// tex0 must not be empty
osg::FrameBufferObject* createFboRtt( osg::StateSet* ss, osg::Texture2D* tex0,
    osg::Texture2D* tex1 = 0, osg::Texture2D* tex2 = 0 );

using DrawableDrawFunc =
    std::function<void( osg::RenderInfo&, const osg::Drawable* )>;
osg::Drawable* createDrawable( DrawableDrawFunc func );

osg::Drawable* createClearDrawable( int mask,
    const osg::Vec4& clearColor = osg::Vec4( 0, 0, 0, 1 ), float depth = 1.0f );

// tex is not stored. Use texture width height as copy width height if they are
// 0.
osg::Drawable* createCopyTex2DDrawable( osg::Texture2D* tex, int xoffset = 0,
    int yoffset = 0, int x = 0, int y = 0, int width = 0, int height = 0 );

} // namespace osgf

#endif // UFO_OSGFACTORY_H
