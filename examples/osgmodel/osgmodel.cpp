#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgFX/Scribe>
#include <osgDB/ReadFile>
#include <osg/Switch>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>

osg::ref_ptr<osg::Group> root;

namespace
{

const char* normVsSource = "#version 120                                      \n"
                       "#extension GL_EXT_geometry_shader4 : enable       \n"
                       "                                                  \n"
                       "varying vec3 normal;                              \n"
                       "                                                  \n"
                       "void main(void)                                   \n"
                       "{                                                 \n"
                       "    normal = gl_NormalMatrix * gl_Normal;         \n"
                       "    gl_Position = gl_ModelViewMatrix * gl_Vertex; \n"
                       "}                                                 \n";

const char* normGsSource = "#version 120                                 \n"
                       "#extension GL_EXT_geometry_shader4 : enable  \n"
                       "                                             \n"
                       "varying in vec3 normal[];                    \n"
                       "                                             \n"
                       "uniform float lineSize;                      \n"
                       "                                             \n"
                       "void main(void)                              \n"
                       "{                                            \n"
                       "    // gl_in require 150                     \n"
                       "    vec4 pos = gl_PositionIn[0];             \n"
                       "    gl_Position = gl_ProjectionMatrix * pos; \n"
                       "    EmitVertex();                            \n"
                       "                                             \n"
                       "    pos.xyz += normal[0] * lineSize;         \n"
                       "    gl_Position = gl_ProjectionMatrix * pos; \n"
                       "    EmitVertex();                            \n"
                       "                                             \n"
                       "    EndPrimitive();                          \n"
                       "}                                            \n";

const char* normFsSource = "#version 120                                \n"
                       "#extension GL_EXT_geometry_shader4 : enable \n"
                       "                                            \n"
                       "uniform vec4 color;                         \n"
                       "                                            \n"
                       "void main(void)                             \n"
                       "{                                           \n"
                       "    gl_FragColor = color;                   \n"
                       "}                                           \n";
}

class ModelEventHandler : public osgGA::GUIEventHandler
{

public:

    ModelEventHandler(osg::Switch* modelSwitch):_modelSwitch(modelSwitch)
    {
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter&)
    {
        switch (ea.getEventType())
        {
            case osgGA::GUIEventAdapter::KEYDOWN:
                switch (ea.getKey())
                {
                    case osgGA::GUIEventAdapter::KEY_Q:
                        _modelSwitch->setValue(0, !_modelSwitch->getValue(0));
                        _modelSwitch->setValue(1, !_modelSwitch->getValue(1));
                        break;

                    case osgGA::GUIEventAdapter::KEY_E:
                        _modelSwitch->setValue(2, !_modelSwitch->getValue(2));
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        return false; //return true will stop event
    }

private:
    osg::Switch* _modelSwitch;
};

class CollectPoints : public osg::NodeVisitor
{
public:
    CollectPoints():
        NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
    }

    void apply(osg::Drawable& drawable) override
    {
        auto geom = drawable.asGeometry();
        if (geom)
        {
            auto vertices = geom->getVertexArray();
            auto normals = geom->getNormalArray();
            if (vertices && normals)
            {
                auto pointGeom = new osg::Geometry;
                pointGeom->setVertexArray(vertices);
                pointGeom->setNormalArray(normals);
                pointGeom->addPrimitiveSet(new osg::DrawArrays(
                  GL_POINTS, 0, vertices->getNumElements()));
                _points.push_back(pointGeom);
            }
        }
    }

    const auto& getPointGeoms() const
    {
        return _points;
    }

private:
    std::vector< osg::ref_ptr<osg::Geometry> > _points;

};

int main(int argc, char* argv[])
{
    osg::ArgumentParser args(&argc, argv);
    args.getApplicationUsage()->setDescription(args.getApplicationName() + " is a small util to check model contents");
    args.getApplicationUsage()->addKeyboardMouseBinding("q", "toggle scribe");
    args.getApplicationUsage()->addKeyboardMouseBinding("e", "toggle normal");

                                 // load model

    auto model = osgDB::readNodeFiles(args);
    if (!model)
    {
        model = osgDB::readNodeFile("cow.osgt");
    }

    // child 0 : usual
    // child 1 : scribe
    // child 2 : normal
    auto modelSwitch = new osg::Switch;
    modelSwitch->addChild(model, false);

                             // add scribed model

    auto scribe = new osgFX::Scribe;
    scribe->addChild(model);
    modelSwitch->addChild(scribe);

                                 // add normal

    // collect point geometry
    auto normalLeaf = new osg::Geode;
    CollectPoints cp;
    model->accept(cp);
    auto& pointGeoms = cp.getPointGeoms();
    for (const auto& geom : pointGeoms)
    {
        normalLeaf->addChild(geom);
    }

    modelSwitch->addChild(normalLeaf);

    // setup program
    auto ss = normalLeaf->getOrCreateStateSet();
    auto normProgram = new osg::Program;
    ss->setAttribute(normProgram);
    normProgram->addShader( new osg::Shader(osg::Shader::VERTEX, normVsSource) );
    normProgram->addShader( new osg::Shader(osg::Shader::GEOMETRY, normGsSource) );
    normProgram->addShader( new osg::Shader(osg::Shader::FRAGMENT, normFsSource) );
    normProgram->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 2);
    normProgram->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
    normProgram->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_LINE_STRIP);
    ss->addUniform( new osg::Uniform( "lineSize", model->getBound().radius() * 0.08f ) );
    ss->addUniform( new osg::Uniform( "color", osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f) ) );

    // add keyboard control to toggle draw element
    root = new osg::Group;
    root->addChild(modelSwitch);
    root->addEventCallback( new ModelEventHandler(modelSwitch) );


    osgViewer::Viewer viewer;
    viewer.setSceneData(root);
    auto rootSS = root->getOrCreateStateSet();
    viewer.addEventHandler( new osgGA::StateSetManipulator(rootSS) );
    viewer.addEventHandler( new osgViewer::HelpHandler( args.getApplicationUsage() ) );
    viewer.addEventHandler(new osgViewer::StatsHandler);

    return viewer.run();
}
