#ifndef UFO_LIGHTNING_H
#define UFO_LIGHTNING_H

#include <string>
// #include <osg/Drawable>
#include <osg/Geometry>

namespace galaxy
{

class LightningSoftware : public osg::Geometry
{
public:
    LightningSoftware();

    void add(
        const std::string& pattern, const osg::Vec3& p0, const osg::Vec3& p1 );

    // void drawImplementation( osg::RenderInfo& renderInfo ) const override;

    void resetLightning();

    float getBillboardWidth() const { return _billboardWidth; }
    void setBillboardWidth( float v );

    float getMaxJitter() const { return _maxJitter; }
    void setMaxJitter( float v ) { _maxJitter = v; }

    float getMaxForkAngle() const { return _maxForkAngle; }
    void setMaxForkAngle( float v ) { _maxForkAngle = v; }

    float getForkRate() const { return _forkRate; }
    void setForkRate( float v ) { _forkRate = v; }

    const osg::Vec4& getCenterColor() const { return _centerColor; }
    void setCenterColor( const osg::Vec4& v ); 

    const osg::Vec4& getBorderColor() const { return _borderColor; }
    void setBorderColor( const osg::Vec4& v ); 

    bool getBillboard() const { return _billboard; }
    void setBillboard( bool v );

    float getExponent() const { return _exponent; }
    void setExponent(float v);

private:
    struct Bolt;
    std::vector<osg::Vec3> createLightning( const Bolt& bolt );

    void updateUniforms();

    bool _billboard = false;
    float _billboardWidth = 2;
    float _maxJitter = 0.5f; // normalized, in size of subdivide
    float _maxForkAngle = osg::PI_4f;
    float _forkRate = 0.5f;
    float _exponent = 0.35f;
    osg::Program* _billboardProgram = 0;

    osg::Vec4 _centerColor = osg::Vec4( 1, 1, 1, 1 );
    osg::Vec4 _borderColor = osg::Vec4( 0, 0, 0.5, 1 );

    struct Bolt
    {
        std::string pattern;
        osg::Vec3 start;
        osg::Vec3 end;

        int getNumVertices() const;
    };

    using BoltList = std::vector<Bolt>;

    BoltList _bolts;

    osg::Vec3Array* _vertices;
    // osg::ref_ptr<osg::Geometry> _geom;
};
} // namespace galaxy

#endif // UFO_LIGHTNING_H
