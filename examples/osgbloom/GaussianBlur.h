#ifndef OSGBLOOM_GAUSSIANBLUREFFECT2_H
#define OSGBLOOM_GAUSSIANBLUREFFECT2_H

#include <osgFX/Effect>
#include <osg/Texture2D>

namespace galaxy
{

// Draw ndc quad with ping pong fbo, render to ping pong texture.
class GaussianBlur : public osgFX::Effect
{
public:
    GaussianBlur( osg::Texture2D* input, osg::Texture2D* output, int times );

    const char* effectName() const override { return "Gaussian blur."; }

    const char* effectDescription() const override { return "Gaussian blur."; }

    const char* effectAuthor() const override { return "dedowsdi"; }

    const std::vector<float>& getWeights() const { return _weights; }
    void setWeights(const std::vector<float>& v);

    // You must at least setup internal format and texture size for output
    void setup( osg::Texture2D* input, osg::Texture2D* output, int times );

    int getTimes() const { return _times; }

    const osg::Texture2D* getInput() const { return _input; }
    osg::Texture2D* getInput() { return _input; }

    // Can be the same as input.
    const osg::Texture2D* getOutput() const { return _output; }
    osg::Texture2D* getOutput() { return _output; }

    const osg::Texture2D* getPong() const { return _pong; }
    osg::Texture2D* getPong() { return _pong; }

    osg::Geode* getQuad() const { return _quad; }

protected:
    bool define_techniques() override;

private:
    int _times = 2;
    std::vector<float> _weights;
    osg::Program* _program = 0;
    osg::Geometry* _quadGeom = 0;
    osg::Texture2D* _input = 0;
    osg::Texture2D* _output = 0; // also used as ping
    osg::ref_ptr<osg::Geode> _quad;
    osg::ref_ptr<osg::Texture2D> _pong; // internal pong
};

} // namespace galaxy

#endif // OSGBLOOM_GAUSSIANBLUREFFECT2_H
