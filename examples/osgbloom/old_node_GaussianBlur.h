#ifndef OSG_1_GAUSSIANBLUR_H
#define OSG_1_GAUSSIANBLUR_H

#include <osg/Texture2D>
#include <osg/Group>

namespace galaxy
{

class GaussianBlur : public osg::Group
{
public:
    GaussianBlur( osg::Texture2D* input, osg::Texture2D* output, int times );

    // you must setup internal format and texture size for output.
    void setup( osg::Texture2D* input, osg::Texture2D* output, int times );

    int getTimes() const { return _times; }

    osg::Program* getProgram() { return _program; }
    const osg::Program* getProgram() const { return _program; }

    const std::vector<float>& getWeights() const { return _weights; }
    void setWeights(const std::vector<float>& v);

    const osg::Texture2D* getInput() const { return _input; }
    osg::Texture2D* getInput() { return _input; }

    // Can be the same as input.
    const osg::Texture2D* getOutput() const { return _output; }
    osg::Texture2D* getOutput() { return _output; }

    const osg::Texture2D* getPong() const { return _pong; }
    osg::Texture2D* getPong() { return _pong; }

private:

    osg::StateSet* createCameraStateSet(const osg::Vec2& step, osg::Texture* tex);

    int _times = 2;
    std::vector<float> _weights;
    osg::ref_ptr<osg::Program> _program;
    osg::ref_ptr<osg::Geometry> _quad;
    osg::ref_ptr<osg::Geode> _leaf;
    osg::ref_ptr<osg::Texture2D> _input;
    osg::ref_ptr<osg::Texture2D> _output; // also used as ping
    osg::ref_ptr<osg::Texture2D> _pong;   // internal pong
    osg::ref_ptr<osg::StateSet> _inputStateSet;
    osg::ref_ptr<osg::StateSet> _pingStateSet;
    osg::ref_ptr<osg::StateSet> _pongStateSet;
};

} // namespace galaxy

#endif // OSG_1_GAUSSIANBLUR_H
