#include "TextureFilter.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osgDB/ReadFile>

namespace galaxy
{

TextureFilter::TextureFilter( TextureInitList inputs, TextureInitList outputs )
{
    _leaf = new osg::Geode;

    _quad = osg::createTexturedQuadGeometry(
        osg::Vec3( -1, -1, 0 ), osg::Vec3( 2, 0, 0 ), osg::Vec3( 0, 2, 0 ) );
    _quad->setName( "quad" );
    _quad->setUseDisplayList( false );
    _quad->setUseVertexBufferObjects( true );
    _leaf->addDrawable( _quad );

    _camera = new osg::Camera;
    _camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    _camera->setRenderOrder( osg::Camera::PRE_RENDER );
    _camera->setClearMask( 0 );
    _camera->addChild( _leaf );
    addChild( _camera );

    setup( inputs, outputs );
}

void TextureFilter::setup( TextureInitList inputs, TextureInitList outputs )
{
    if ( inputs.begin() == inputs.end() || outputs.begin() == outputs.end() )
    {
        OSG_WARN << __func__ << " : blank inputs or outputs" << std::endl;
        return;
    }

    auto invalidOutput =
        std::find_if( outputs.begin(), outputs.end(), []( auto v ) {
            return !v || v->getInternalFormat() == 0 ||
                   v->getTextureWidth() == 0 || v->getTextureHeight() == 0;
        } );

    if ( invalidOutput != outputs.end() )
    {
        OSG_WARN << __func__ << " : The "
                 << std::distance( outputs.begin(), invalidOutput )
                 << "th outputs is invalid. " << std::endl;
        return;
    }

    auto iter = std::find_first_of(
        inputs.begin(), inputs.end(), outputs.begin(), outputs.end() );

    if ( iter != inputs.end() )
    {
        OSG_WARN
            << __func__
            << " : you can't use the same Texture2D for both inputs and outputs"
            << std::endl;
        return;
    }

    _inputs.assign( inputs.begin(), inputs.end() );
    _outputs.assign( outputs.begin(), outputs.end() );

    _camera->setViewport( 0, 0, _outputs.front()->getTextureWidth(),
        _outputs.front()->getTextureHeight() );
    for ( auto i = 0; i < _outputs.size(); ++i )
    {
        _camera->attach( static_cast<osg::Camera::BufferComponent>(
                             osg::Camera::COLOR_BUFFER0 + i ),
            _outputs[i] );
    }

    auto ss = getOrCreateStateSet();
    for ( auto i = 0; i < _inputs.size(); ++i )
    {
        ss->setTextureAttributeAndModes( i, _inputs[i] );
    }
}

void TextureFilter::setProgram( osg::Program* v )
{
    _program = v;
    getOrCreateStateSet()->setAttributeAndModes( _program );
}

BrightnessFilter::BrightnessFilter(
    osg::Texture2D* input, osg::Texture2D* output )
    : TextureFilter( {input}, {output} )
{
    auto program = new osg::Program;

    auto vertShader =
        osgDB::readShaderFile( osg::Shader::VERTEX, "shader/quad.vert" );
    auto fragShader = osgDB::readShaderFile(
        osg::Shader::FRAGMENT, "shader/brightness.frag" );
    program->addShader( vertShader );
    program->addShader( fragShader );
    setProgram( program );
    setThreshold( _threshold );
}

void BrightnessFilter::setThreshold( float v )
{
    _threshold = v;
    auto ss = getOrCreateStateSet();
    ss->addUniform( new osg::Uniform( "threshold", _threshold ) );
}

BloomFilter::BloomFilter( osg::Texture2D* colorInput,
    osg::Texture2D* brightnessInput, osg::Texture2D* output )
    : TextureFilter( {colorInput, brightnessInput}, {output} )
{
    auto program = new osg::Program;

    auto vertShader =
        osgDB::readShaderFile( osg::Shader::VERTEX, "shader/quad.vert" );
    auto fragShader =
        osgDB::readShaderFile( osg::Shader::FRAGMENT, "shader/bloom.frag" );
    program->addShader( vertShader );
    program->addShader( fragShader );
    setProgram( program );
    setExposure( 1.0f );

    auto ss = getOrCreateStateSet();
    ss->addUniform(new osg::Uniform("hdr_map", 0));
    ss->addUniform(new osg::Uniform("brightness_map", 1));
}

void BloomFilter::setExposure( float v )
{
    _exposure = v;
    getOrCreateStateSet()->addUniform(
        new osg::Uniform( "exposure", _exposure ) );
}

} // namespace galaxy
