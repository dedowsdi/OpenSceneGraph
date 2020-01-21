#ifndef OSG_1_GAUSSIANBLUREFFECT_H
#define OSG_1_GAUSSIANBLUREFFECT_H

#include <osgFX/Effect>
#include <osg/Texture2D>

namespace galaxy
{

class GaussianBlur : public osgFX::Effect
{
public:
    GaussianBlur();

    const char* effectName() const override { return "Gaussian blur."; }

    const char* effectDescription() const override { return "Gaussian blur."; }

    const char* effectAuthor() const override { return "dedowsdi"; }

    void setup( osg::Texture2D* input, osg::Texture2D* output, int times );

    int getTimes() const { return _times; }

    const osg::Texture2D* getInput() const { return _input; }
    osg::Texture2D* getInput() { return _input; }

    // Can be the same as input.
    const osg::Texture2D* getOutput() const { return _output; }
    osg::Texture2D* getOutput() { return _output; }

    const osg::Texture2D* getPong() const { return _pong; }
    osg::Texture2D* getPong() { return _pong; }

    osg::Camera* getCamera( int i ) { return _cameras[i]; }

protected:
    bool define_techniques() override;

private:
    osg::Camera* createCamera();

    int _times = 2;
    osg::Program* _program = 0;
    osg::Geometry* _quad = 0;
    osg::Texture2D* _input = 0;
    osg::Texture2D* _output = 0; // also used as ping
    osg::Geode* _leaf = 0;
    osg::ref_ptr<osg::Texture2D> _pong; // internal pong

    //  CullVisitor reset RenderStage the for cached RenderStage in Pre or Post
    //  render camera, you also can't render the same RenderStage twice,
    //  RenderStage::_stageDrawnThisFrame won't let you do that, so I have to
    //  use one camera for one post processing.
    using CameraList = std::vector<osg::ref_ptr<osg::Camera>>;
    CameraList _cameras;
};

} // namespace galaxy

#endif // OSG_1_GAUSSIANBLUREFFECT_H
