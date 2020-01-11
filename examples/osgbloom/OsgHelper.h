#ifndef UFO_OSGHELPER_H
#define UFO_OSGHELPER_H

#include <string>

#include <osgUtil/RenderStage>
#include <osgUtil/StateGraph>
#include <osgViewer/Renderer>
#include <osgUtil/PrintVisitor>
#include <osgGA/GUIEventHandler>
#include <osgDB/WriteFile>

namespace galaxy
{

std::string to_string( osg::StateSet::RenderBinMode mode );

std::string to_string( osgUtil::RenderBin::SortMode mode );

class RenderStagePrinter : public osg::Camera::DrawCallback
{
public:
    RenderStagePrinter( std::ostream& out, int indent = 0, int step = 2 );

    void operator()( osg::RenderInfo& renderInfo ) const override;

private:
    // osg::RenderStage doesn't provide functions to get pre and post render list.
    // void printRenderStage( const osgUtil::RenderStage* stage )

    void printRenderBin( const osgUtil::RenderBin* bin ) const;

    template <typename T>
    void printLeaves( const T& leaves ) const
    {
        for ( const auto leaf : leaves )
        {
            auto drawable = leaf->getDrawable();
            outputIndent() << "leaf : " << drawable << " \"" << drawable->getName()
                           << "\"\n";
        }
    }

    void enter() const { _indent += _step; }
    void leave() const { _indent -= _step; }

    std::ostream& outputIndent() const;

    std::ostream& _out;
    mutable unsigned int _indent;
    mutable unsigned int _step;
};

class VerbosePrintVisitor : public osgUtil::PrintVisitor
{
public:
    using PrintVisitor::PrintVisitor;

    void apply( osg::Node& node ) override;
};

class OsgDebugHandler : public osgGA::GUIEventHandler
{
public:
    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa );

    void setRoot( osg::Node* v ) { _root = v; }
    void setCamera( osg::Camera* v ) { _camera = v; }

private:
    osg::Node* _root;
    osg::Camera* _camera;
};

} // namespace galaxy
#endif // UFO_OSGHELPER_H
