#include "DAPyWorkFlowPythonBinding.h"
#include <QPointF>
#include "DAPyWorkFlow/DAPyWorkFlowScene.h"
#include "DAPyWorkFlow/DAPyNodeProxy.h"
#include "DAPyWorkFlow/DAPyNodeState.h"
#include "DAPyWorkFlow/DAPyNodeGraphicsItem.h"
#include "DAPyWorkFlow/DAPyLinkGraphicsItem.h"
#include "DAPyWorkFlow/DAPyPainterProxy.h"
#include "DAPyWorkFlow/DAPyLinkPoint.h"
#include "DAPyWorkFlow/DAPyNodeFactory.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
#include "DAPyBindQt/DAPyJsonCast.h"
#include "DAPyBindQt/DAPybind11InQt.h"  // slots workaround，必须第一个pybind11相关头文件

namespace DA {

/**
 * @brief 通过 qualified_name 获取 DAPyNodeProxy 实例
 * @param[in] qualified_name Python 节点的完整限定名（模块.类名）
 * @return 代理节点智能指针，如果节点未注册返回空指针
 */
std::shared_ptr< DA::DAPyNodeProxy > getNodeProxy(const std::string& qualified_name)
{
    // TODO: 实现通过 DAPyModuleWorkflow 获取 Python 节点类并创建代理
    // 目前返回空指针，待 DAPyModuleWorkflow 和节点注册系统实现后完善
    return std::shared_ptr< DA::DAPyNodeProxy >();
}

}  // namespace DA

PYBIND11_EMBEDDED_MODULE(da_py_workflow, m)
{
    // 导出 DAPyNodeState 枚举（Scenario C 模式）
    pybind11::enum_< DA::DAPyNodeState >(m, "DAPyNodeState")
        .value("Idle", DA::DAPyNodeState::Idle)
        .value("Waiting", DA::DAPyNodeState::Waiting)
        .value("Running", DA::DAPyNodeState::Running)
        .value("Success", DA::DAPyNodeState::Success)
        .value("Error", DA::DAPyNodeState::Error)
        .value("Skipped", DA::DAPyNodeState::Skipped)
        .export_values();

    // 导出 DAPyLinkPoint::Way 枚举
    pybind11::enum_< DA::DAPyLinkPoint::Way >(m, "DAPyLinkPointWay")
        .value("Input", DA::DAPyLinkPoint::Way::Input)
        .value("Output", DA::DAPyLinkPoint::Way::Output)
        .export_values();

    // 绑定 DAPyLinkPoint 类
    pybind11::class_< DA::DAPyLinkPoint >(m, "DAPyLinkPoint")
        .def(pybind11::init<>())
        .def(pybind11::init< const QPointF&, const QString&, DA::DAPyLinkPoint::Way, DA::AspectDirection >(),
             pybind11::arg("position"),
             pybind11::arg("name"),
             pybind11::arg("way")         = DA::DAPyLinkPoint::Way::Output,
             pybind11::arg("direction")   = DA::AspectDirection::East)
        .def_readwrite("position", &DA::DAPyLinkPoint::position,
                       "Connection point position relative to graphics item")
        .def_readwrite("name", &DA::DAPyLinkPoint::name,
                       "Connection point name")
        .def_readwrite("way", &DA::DAPyLinkPoint::way,
                       "Input or Output attribute")
        .def_readwrite("direction", &DA::DAPyLinkPoint::direction,
                       "Link line extending direction")
        .def("isValid", &DA::DAPyLinkPoint::isValid,
             "Check if connection point is valid (name not empty)")
        .def("isInput", &DA::DAPyLinkPoint::isInput,
             "Check if this is an input connection point")
        .def("isOutput", &DA::DAPyLinkPoint::isOutput,
             "Check if this is an output connection point")
        .def("__eq__",
             [](const DA::DAPyLinkPoint& a, const DA::DAPyLinkPoint& b) { return a == b; },
             pybind11::arg("other"),
             "Equality comparison with another DAPyLinkPoint")
        .def("__eq__",
             [](const DA::DAPyLinkPoint& a, const std::string& b) { return a == QString::fromStdString(b); },
             pybind11::arg("other"),
             "Equality comparison with a string (by name)")
        .def("__repr__",
             [](const DA::DAPyLinkPoint& a) {
                 return QString("DAPyLinkPoint(name=%1, way=%2)")
                     .arg(a.name)
                     .arg(a.way == DA::DAPyLinkPoint::Way::Input ? "Input" : "Output")
                     .toStdString();
             });

    // 绑定 DAPyNodeMetaData 结构体
    pybind11::class_< DA::DAPyNodeMetaData >(m, "DAPyNodeMetaData")
        .def(pybind11::init<>())
        .def_readwrite("name", &DA::DAPyNodeMetaData::name,
                       "Node display name")
        .def_readwrite("prototype", &DA::DAPyNodeMetaData::prototype,
                       "Node unique prototype identifier (qualified_name)")
        .def_readwrite("group", &DA::DAPyNodeMetaData::group,
                       "Node group/category")
        .def_readwrite("iconPath", &DA::DAPyNodeMetaData::iconPath,
                       "Node icon path")
        .def_readwrite("tooltip", &DA::DAPyNodeMetaData::tooltip,
                       "Node tooltip text")
        .def_readwrite("inputKeys", &DA::DAPyNodeMetaData::inputKeys,
                       "Input key list")
        .def_readwrite("outputKeys", &DA::DAPyNodeMetaData::outputKeys,
                       "Output key list")
        .def("isValid", &DA::DAPyNodeMetaData::isValid,
             "Check if metadata is valid (prototype not empty)")
        .def("__eq__",
             [](const DA::DAPyNodeMetaData& a, const DA::DAPyNodeMetaData& b) { return a == b; },
             pybind11::arg("other"),
             "Equality comparison by prototype")
        .def("__repr__",
             [](const DA::DAPyNodeMetaData& m) {
                 return QString("DAPyNodeMetaData(name=%1, prototype=%2, group=%3, inputs=%4, outputs=%5)")
                     .arg(m.name)
                     .arg(m.prototype)
                     .arg(m.group)
                     .arg(m.inputKeys.size())
                     .arg(m.outputKeys.size())
                     .toStdString();
             });

    // 绑定 DAPyWorkFlowScene 类（Scenario B 模式：Qt 接口类 + lambda 包装）
    pybind11::class_< DA::DAPyWorkFlowScene >(m, "DAPyWorkFlowScene")
        .def(pybind11::init<>())
        .def(pybind11::init< QObject* >(), pybind11::arg("parent") = nullptr)
        // 节点管理
        .def(
            "createPyNode",
            [](DA::DAPyWorkFlowScene& self, const std::string& qualified_name, const std::tuple< double, double >& pos) {
                QPointF qpos(std::get< 0 >(pos), std::get< 1 >(pos));
                QJsonObject descriptor;
                descriptor["qualified_name"] = QString::fromStdString(qualified_name);
                return self.createPyNode(descriptor, qpos);
            },
            pybind11::arg("qualified_name"),
            pybind11::arg("pos"),
            "Create a Python node at specified position"
        )
        .def(
            "removePyNodeItem",
            [](DA::DAPyWorkFlowScene& self, DA::DAPyNodeGraphicsItem* item) { return self.removePyNodeItem(item); },
            pybind11::arg("item"),
            "Remove a node item from scene"
        )
        .def(
            "nodeItemAt",
            [](DA::DAPyWorkFlowScene& self, const std::tuple< double, double >& pos) {
                QPointF qpos(std::get< 0 >(pos), std::get< 1 >(pos));
                return self.nodeItemAt(qpos);
            },
            pybind11::arg("pos"),
            "Get node item at specified position, returns None if no node"
        )
        .def(
            "getPyNodeItems",
            [](DA::DAPyWorkFlowScene& self) {
                QList< DA::DAPyNodeGraphicsItem* > items = self.getPyNodeItems();
                pybind11::list pyList;
                for (DA::DAPyNodeGraphicsItem* item : std::as_const(items)) {
                    pyList.append(item);
                }
                return pyList;
            },
            "Get all node items in scene as a list"
        )
        // 连接管理
        .def(
            "addPyNodeLink",
            [](DA::DAPyWorkFlowScene& self,
               DA::DAPyNodeGraphicsItem* fromItem,
               const std::string& fromOutput,
               DA::DAPyNodeGraphicsItem* toItem,
               const std::string& toInput) {
                return self.addPyNodeLink(fromItem,
                                          QString::fromStdString(fromOutput),
                                          toItem,
                                          QString::fromStdString(toInput));
            },
            pybind11::arg("fromItem"),
            pybind11::arg("fromOutput"),
            pybind11::arg("toItem"),
            pybind11::arg("toInput"),
            "Add link between node items"
        )
        .def(
            "removePyNodeLink",
            [](DA::DAPyWorkFlowScene& self, DA::DAPyLinkGraphicsItem* linkItem) {
                return self.removePyNodeLink(linkItem);
            },
            pybind11::arg("linkItem"),
            "Remove link from scene"
        )
        .def(
            "getPyNodeLinkItems",
            [](DA::DAPyWorkFlowScene& self) {
                QList< DA::DAPyLinkGraphicsItem* > links = self.getPyNodeLinkItems();
                pybind11::list pyList;
                for (DA::DAPyLinkGraphicsItem* link : std::as_const(links)) {
                    pyList.append(link);
                }
                return pyList;
            },
            "Get all link items in scene as a list"
        )
        // 清空场景
        .def("clearPyScene", &DA::DAPyWorkFlowScene::clearPyScene, "Clear scene, remove all nodes and links");

    // 绑定 DAPyNodeProxy 类（独立类，不再继承DAAbstractNode）
    pybind11::class_< DA::DAPyNodeProxy >(m, "DAPyNodeProxy")
        // 状态管理
        .def("getNodeState", &DA::DAPyNodeProxy::getNodeState, "Get current node state")
        .def(
            "setNodeState",
            [](DA::DAPyNodeProxy& self, DA::DAPyNodeState state) { self.setNodeState(state); },
            pybind11::arg("state"),
            "Set node state"
        )
        .def(
            "getLastErrorString",
            [](const DA::DAPyNodeProxy& self) { return self.getLastErrorString().toStdString(); },
            "Get error message string (when state is Error)"
        )
        // 执行
        .def("exec", &DA::DAPyNodeProxy::exec, "Execute node logic, returns success flag")
        // 数据传递
        .def(
            "setPyInputData",
            [](DA::DAPyNodeProxy& self, const std::string& key, pybind11::object data) {
                self.setPyInputData(QString::fromStdString(key), data);
            },
            pybind11::arg("key"),
            pybind11::arg("data"),
            "Set input data (Python object reference)"
        )
        .def(
            "getPyOutputData",
            [](DA::DAPyNodeProxy& self, const std::string& key) { return self.getPyOutputData(QString::fromStdString(key)); },
            pybind11::arg("key"),
            "Get output data (Python object reference)"
        )
        // 信息查询（新增方法）
        .def(
            "getNodeName",
            [](const DA::DAPyNodeProxy& self) { return self.getNodeName().toStdString(); },
            "Get node display name"
        )
        .def(
            "setNodeName",
            [](DA::DAPyNodeProxy& self, const std::string& name) { self.setNodeName(QString::fromStdString(name)); },
            pybind11::arg("name"),
            "Set node display name"
        )
        .def(
            "getInputKeys",
            [](const DA::DAPyNodeProxy& self) {
                pybind11::list pyList;
                for (const QString& key : self.getInputKeys()) {
                    pyList.append(key.toStdString());
                }
                return pyList;
            },
            "Get input key list"
        )
        .def(
            "getOutputKeys",
            [](const DA::DAPyNodeProxy& self) {
                pybind11::list pyList;
                for (const QString& key : self.getOutputKeys()) {
                    pyList.append(key.toStdString());
                }
                return pyList;
            },
            "Get output key list"
        )
        .def(
            "getNodePrototype",
            [](const DA::DAPyNodeProxy& self) { return self.getNodePrototype().toStdString(); },
            "Get node prototype (qualified_name)"
        )
        .def(
            "getNodeGroup",
            [](const DA::DAPyNodeProxy& self) { return self.getNodeGroup().toStdString(); },
            "Get node group/category"
        )
        .def(
            "getID",
            [](const DA::DAPyNodeProxy& self) { return self.getID(); },
            "Get node ID"
        )
        .def(
            "setID",
            [](DA::DAPyNodeProxy& self, unsigned int id) { self.setID(id); },
            pybind11::arg("id"),
            "Set node ID"
        )
        // 原有信息查询
        .def(
            "getQualifiedName",
            [](const DA::DAPyNodeProxy& self) { return self.getQualifiedName().toStdString(); },
            "Get Python node's qualified name (module.class)"
        )
        .def(
            "getDescriptor",
            [](const DA::DAPyNodeProxy& self) {
                QJsonObject json = self.getDescriptor();
                return DA::PY::qjsonObjectToPyDict(json);
            },
            "Get node descriptor dictionary (inputs/outputs/parameters metadata)"
        );

    // 绑定 DAPyNodeFactory 类（独立QObject，不再继承DAAbstractNodeFactory）
    pybind11::class_< DA::DAPyNodeFactory, QObject >(m, "DAPyNodeFactory")
        .def(pybind11::init< QObject* >(), pybind11::arg("parent") = nullptr,
             "Create Python node factory with optional parent")
        // 节点发现
        .def(
            "discoverNodes",
            [](DA::DAPyNodeFactory& self, const std::vector< std::string >& scanPaths, bool useEntryPoints) {
                QStringList paths;
                for (const auto& p : scanPaths) {
                    paths.append(QString::fromStdString(p));
                }
                return self.discoverNodes(paths, useEntryPoints);
            },
            pybind11::arg("scan_paths")    = std::vector< std::string >(),
            pybind11::arg("use_entry_points") = false,
            "Discover Python nodes in specified paths"
        )
        // 节点创建
        .def(
            "createNodeProxy",
            [](DA::DAPyNodeFactory& self, const std::string& qualifiedName) {
                return self.createNodeProxy(QString::fromStdString(qualifiedName));
            },
            pybind11::arg("qualified_name"),
            "Create DAPyNodeProxy by qualified name"
        )
        // 元数据查询
        .def(
            "getNodeMetadataList",
            [](DA::DAPyNodeFactory& self) {
                pybind11::list pyList;
                for (const DA::DAPyNodeMetaData& meta : self.getNodeMetadataList()) {
                    pyList.append(meta);
                }
                return pyList;
            },
            "Get all discovered node metadata as list"
        )
        .def(
            "getNodePrototypes",
            [](DA::DAPyNodeFactory& self) {
                pybind11::list pyList;
                for (const QString& proto : self.getNodePrototypes()) {
                    pyList.append(proto.toStdString());
                }
                return pyList;
            },
            "Get all discovered node prototype identifiers"
        )
        // 工厂信息
        .def(
            "factoryName",
            [](DA::DAPyNodeFactory& self) { return self.factoryName().toStdString(); },
            "Get factory name"
        )
        .def(
            "factoryDescribe",
            [](DA::DAPyNodeFactory& self) { return self.factoryDescribe().toStdString(); },
            "Get factory description"
        );

    // 绑定 DAPyPainterProxy 类（Scenario B 模式：非QObject代理类）
    // 注意：DAPyPainterProxy不是QObject，是QPainter的轻量代理
    pybind11::class_< DA::DAPyPainterProxy >(m, "DAPyPainterProxy")
        // 绘制操作
        .def("drawRect", &DA::DAPyPainterProxy::drawRect,
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"),
             "Draw rectangle outline")
        .def("drawText", &DA::DAPyPainterProxy::drawText,
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("text"),
             "Draw text at specified position")
        .def("drawLine", &DA::DAPyPainterProxy::drawLine,
             pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"),
             "Draw line from (x1,y1) to (x2,y2)")
        .def("drawEllipse", &DA::DAPyPainterProxy::drawEllipse,
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"),
             "Draw ellipse in specified rectangle")
        .def("fillRect", &DA::DAPyPainterProxy::fillRect,
             pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"),
             pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a") = 255,
             "Fill rectangle with RGBA color")
        // 样式设置
        .def("setPenColor", &DA::DAPyPainterProxy::setPenColor,
             pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a") = 255,
             "Set pen color (RGBA)")
        .def("setPenWidth", &DA::DAPyPainterProxy::setPenWidth,
             pybind11::arg("width"),
             "Set pen width")
        .def("setBrushColor", &DA::DAPyPainterProxy::setBrushColor,
             pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a") = 255,
             "Set brush color (RGBA)")
        .def("setFont", &DA::DAPyPainterProxy::setFont,
             pybind11::arg("family"), pybind11::arg("size"),
             "Set font family and size")
        .def("setNoPen", &DA::DAPyPainterProxy::setNoPen,
             "Set no pen (disable outline drawing)")
        .def("setNoBrush", &DA::DAPyPainterProxy::setNoBrush,
             "Set no brush (disable fill)")
        .def("isValid", &DA::DAPyPainterProxy::isValid,
             "Check if painter proxy is valid");

    // 自由函数（Scenario A 模式）
    m.def("getNodeProxy",
          &DA::getNodeProxy,
          pybind11::arg("qualified_name"),
          "Get DAPyNodeProxy instance by Python node's qualified name");

    // 节点状态变化通知说明
    m.def("_note_signal_handler",
          []() {
              return "Python→C++ state updates use DAPythonSignalHandler::callInMainThread from da_interface module "
                     "(NOT a custom bridge class). Example: core.getPythonSignalHandler().callInMainThread(callback)";
          },
          "Note about state update mechanism");
}
