#ifndef OSGLIGHTNING_GAUSSIANBLUR_H
#define OSGLIGHTNING_GAUSSIANBLUR_H

#include <osg/Group>
#include <osg/Texture2D>

namespace galaxy
{

// The internal camera is NESTED, becareful.
class GaussianBlur : public osg::Group
{
public:
    GaussianBlur(osg::Texture2D* input, osg::Texture2D* output, int times );

    const int& getTimes() const { return _times; }
    void setTimes(const int& v){ _times = v; }

    void setWeights( const std::vector<float>& v );

private:

    int _times;
    osg::Camera* _camera;
    const std::vector<float> _weights;
};

}

#endif // OSGLIGHTNING_GAUSSIANBLUR_H
