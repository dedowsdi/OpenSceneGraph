#include "OsgHelper.h"

namespace galaxy
{

std::string to_string( osg::StateSet::RenderBinMode mode )
{
    switch ( mode )
    {
        case osg::StateSet::INHERIT_RENDERBIN_DETAILS:
            return "INHERIT_RENDERBIN_DETAILS";
        case osg::StateSet::OVERRIDE_PROTECTED_RENDERBIN_DETAILS:
            return "OVERRIDE_PROTECTED_RENDERBIN_DETAILS";
        case osg::StateSet::OVERRIDE_RENDERBIN_DETAILS:
            return "OVERRIDE_RENDERBIN_DETAILS";
        case osg::StateSet::PROTECTED_RENDERBIN_DETAILS:
            return "PROTECTED_RENDERBIN_DETAILS";
        case osg::StateSet::USE_RENDERBIN_DETAILS:
            return "USE_RENDERBIN_DETAILS";
        default:
            throw std::runtime_error( "unknown mode " + std::to_string( mode ) );
    }
}

std::string to_string( osgUtil::RenderBin::SortMode mode )
{
    switch ( mode )
    {
        case osgUtil::RenderBin::SORT_BACK_TO_FRONT:
            return "SORT_BACK_TO_FRONT";
        case osgUtil::RenderBin::SORT_BY_STATE:
            return "SORT_BY_STATE";
        case osgUtil::RenderBin::SORT_BY_STATE_THEN_FRONT_TO_BACK:
            return "SORT_BY_STATE_THEN_FRONT_TO_BACK";
        case osgUtil::RenderBin::SORT_FRONT_TO_BACK:
            return "SORT_FRONT_TO_BACK";
        case osgUtil::RenderBin::TRAVERSAL_ORDER:
            return "TRAVERSAL_ORDER";
        default:
            throw std::runtime_error( "unknown mode " + std::to_string( mode ) );
    }
}

RenderStagePrinter::RenderStagePrinter( std::ostream& out, int indent, int step )
    : _out( out ), _indent( indent ), _step( step )
{
}

void RenderStagePrinter::operator()( osg::RenderInfo& renderInfo ) const
{
    auto renderer =
        dynamic_cast<osgViewer::Renderer*>( renderInfo.getCurrentCamera()->getRenderer() );
    auto stage = renderer->getSceneView( 0 )->getRenderStage();

    _out << std::string( 60, '-' ) << "\n";
    printRenderBin( stage );
    renderInfo.getCurrentCamera()->setInitialDrawCallback( 0 );
}

void RenderStagePrinter::printRenderBin( const osgUtil::RenderBin* bin ) const
{
    if ( !bin ) return;

    // print child bins with binNum <0
    auto& bins = bin->getRenderBinList();
    for ( auto it = bins.begin(); it->first < 0 && it != bins.end(); ++it )
    {
        enter();
        printRenderBin( it->second );
        leave();
    }

    // print self
    outputIndent() << bin->getBinNum() << " : \"" << to_string( bin->getSortMode() )
                   << "\"\n";

    outputIndent() << "fine grained : \n";
    // print fine grained leaves
    printLeaves( bin->getRenderLeafList() );

    // print coarse grained graphes
    outputIndent() << "coarse grained : \n";
    for ( const auto graph : bin->getStateGraphList() )
    {
        printLeaves( graph->_leaves );
    }

    // print child bins with binNum >0
    for ( auto it = bins.begin(); it->first > 0 && it != bins.end(); ++it )
    {
        enter();
        printRenderBin( it->second );
        leave();
    }
}

std::ostream& RenderStagePrinter::outputIndent() const
{
    for ( unsigned int i = 0; i < _indent; ++i ) _out << " ";
    return _out;
}

void VerbosePrintVisitor::apply( osg::Node& node )
{
    output() << node.libraryName() << "::" << node.className() << " ( " << &node << " ) "
             << "\"" << node.getName() << "\"";
    auto ss = node.getStateSet();
    if ( ss )
    {
        _out << " StateSet( " << ss;
        if ( ss->getRenderBinMode() != osg::StateSet::INHERIT_RENDERBIN_DETAILS )
        {
            _out << " " << to_string( ss->getRenderBinMode() ) << " " << ss->getBinNumber()
                 << " \"" << ss->getBinName() << "\"";
        }

        _out << " )";
    }
    _out << "\n";
    enter();
    traverse( node );
    leave();
}

bool OsgDebugHandler::handle(
    const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
{
    switch ( ea.getEventType() )
    {
        case osgGA::GUIEventAdapter::KEYDOWN:
            switch ( ea.getKey() )
            {
                case osgGA::GUIEventAdapter::KEY_F2:
                {
                    OSG_NOTICE << std::string( 60, '-' ) << "\n";
                    auto visitor = new VerbosePrintVisitor( std::cout );
                    _root->accept( *visitor );
                    break;
                }

                case osgGA::GUIEventAdapter::KEY_F4:
                    osgDB::writeNodeFile( *_root, "ufo.osgt",
                        new osgDB::Options( "WriteImageHint=UseExternal" ) );
                    break;

                case osgGA::GUIEventAdapter::KEY_F3:
                    _camera->setInitialDrawCallback( new RenderStagePrinter( std::cout ) );
                    break;

                default:
                    break;
            }
            break;
        default:
            break;
    }
    return false; // return true will stop event
}

} // namespace galaxy
