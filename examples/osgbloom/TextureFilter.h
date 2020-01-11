#ifndef OSG_1_FILTER_H
#define OSG_1_FILTER_H

#include <osg/Group>
#include <osg/Texture2D>

namespace galaxy
{

class TextureFilter : public osg::Group
{
public:
    using TextureInitList = std::initializer_list<osg::Texture2D*>;

    TextureFilter( TextureInitList inputs, TextureInitList outputs );

    // Set texture inputs, frame buffer attachments. Note that you still need to
    // setup texture uniforms.
    void setup( TextureInitList inputs, TextureInitList outputs );

    using TextureList = std::vector<osg::ref_ptr<osg::Texture2D>>;

    osg::Texture2D* getFinalOutput() { return _outputs.back(); }
    osg::Texture* getOutput( int i ) { return _outputs[i]; }

    osg::Program* getProgram() const { return _program; }
    void setProgram( osg::Program* v );

private:
    osg::ref_ptr<osg::Geometry> _quad;
    osg::ref_ptr<osg::Geode> _leaf;
    osg::ref_ptr<osg::Camera> _camera;
    osg::ref_ptr<osg::Program> _program;
    TextureList _inputs;
    TextureList _outputs; // also used as ping
};

class BrightnessFilter : public TextureFilter
{
public:
    BrightnessFilter( osg::Texture2D* input, osg::Texture2D* output );

    float getThreshold() const { return _threshold; }
    void setThreshold( float v );

private:
    float _threshold = 0.95f;
};

class BloomFilter : public TextureFilter
{
public:
    BloomFilter( osg::Texture2D* colorInput, osg::Texture2D* brightnessInput,
        osg::Texture2D* output );

    float getExposure() const { return _exposure; }
    void setExposure(float v);

private:
    float _exposure = 1.0f;
};

} // namespace galaxy

#endif // OSG_1_FILTER_H
