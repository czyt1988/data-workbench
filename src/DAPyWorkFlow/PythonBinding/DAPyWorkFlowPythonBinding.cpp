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
#include "DAPyWorkFlow/DAPyNodeStyle.h"
#include "DAPyWorkFlow/DAPyWorkFlowEnumStringUtils.h"
#include "DAPyWorkFlow/DAWorkflowState.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
#include "DAPyBindQt/DAPyJsonCast.h"
#include "DAPyBindQt/DAPybind11InQt.h"  // slots workaround，必须第一个pybind11相关头文件

namespace DA
{

/**
 * @brief 通过 qualified_name 获取 DAPyNodeProxy 实例
 * @param[in] qualified_name Python 节点的完整限定名（模块.类名）
 * @return 代理节点智能指针，如果节点未注册返回空指针
 */
std::shared_ptr< DA::DAPyNodeProxy > getNodeProxy(const std::string& qualified_name)
{
    // TODO: 实现通过 DAPyModuleWorkflow 获取 Python 节点类并创建代理
    return std::shared_ptr< DA::DAPyNodeProxy >();
}

}  // namespace DA

PYBIND11_EMBEDDED_MODULE(da_py_workflow, m)
{

    // =================================================================================
    //                      样式枚举绑定
    // =================================================================================
    // 导出 DAPyNodeState 枚举
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

    // 导出 AspectDirection 枚举（PortSide 是 AspectDirection 的类型别名）
    pybind11::enum_< DA::AspectDirection >(m, "AspectDirection")
        .value("East", DA::AspectDirection::East)
        .value("South", DA::AspectDirection::South)
        .value("West", DA::AspectDirection::West)
        .value("North", DA::AspectDirection::North)
        .export_values();
    m.attr("PortSide") = m.attr("AspectDirection");

    // 导出 BodyShape 枚举
    pybind11::enum_< DA::BodyShape >(m, "BodyShape")
        .value("RoundedRect", DA::BodyShape::RoundedRect)
        .value("Ellipse", DA::BodyShape::Ellipse)
        .export_values();

    // 导出 PortShape 枚举
    pybind11::enum_< DA::PortShape >(m, "PortShape")
        .value("Rect", DA::PortShape::Rect)
        .value("Circle", DA::PortShape::Circle)
        .value("Diamond", DA::PortShape::Diamond)
        .export_values();

    // 导出 NamePosition 枚举
    pybind11::enum_< DA::NamePosition >(m, "NamePosition")
        .value("Inside", DA::NamePosition::Inside)
        .value("Below", DA::NamePosition::Below)
        .export_values();

    // 导出 IconPosition 枚举
    pybind11::enum_< DA::IconPosition >(m, "IconPosition")
        .value("LeftOfText", DA::IconPosition::LeftOfText)
        .value("AboveText", DA::IconPosition::AboveText)
        .export_values();

    // 导出 BodyIconType 枚举
    pybind11::enum_< DA::BodyIconType >(m, "BodyIconType")
        .value("None", DA::BodyIconType::None)
        .value("Pixmap", DA::BodyIconType::Pixmap)
        .value("Svg", DA::BodyIconType::Svg)
        .export_values();

    // 导出 LinkPointLayoutStrategy 枚举
    pybind11::enum_< DA::LinkPointLayoutStrategy >(m, "LinkPointLayoutStrategy")
        .value("Auto", DA::LinkPointLayoutStrategy::Auto)
        .value("Manual", DA::LinkPointLayoutStrategy::Manual)
        .export_values();

    // 导出 RenderTemplate 枚举
    pybind11::enum_< DA::RenderTemplate >(m, "RenderTemplate")
        .value("NodeStyleTemplate", DA::RenderTemplate::NodeStyleTemplate)
        .value("WidgetTemplate", DA::RenderTemplate::WidgetTemplate)
        .export_values();

    // 绑定 DAPyLinkPoint 类
    pybind11::class_< DA::DAPyLinkPoint >(m, "DAPyLinkPoint")
        .def(pybind11::init<>())
        .def(pybind11::init< const QPointF&, const QString&, DA::DAPyLinkPoint::Way, DA::AspectDirection >(),
             pybind11::arg("position"),
             pybind11::arg("name"),
             pybind11::arg("way")       = DA::DAPyLinkPoint::Way::Output,
             pybind11::arg("direction") = DA::AspectDirection::East)
        .def_readwrite("position", &DA::DAPyLinkPoint::position, "Connection point position relative to graphics item")
        .def_readwrite("name", &DA::DAPyLinkPoint::name, "Connection point name")
        .def_readwrite("way", &DA::DAPyLinkPoint::way, "Input or Output attribute")
        .def_readwrite("direction", &DA::DAPyLinkPoint::direction, "Link line extending direction")
        .def("isValid", &DA::DAPyLinkPoint::isValid, "Check if connection point is valid (name not empty)")
        .def("isInput", &DA::DAPyLinkPoint::isInput, "Check if this is an input connection point")
        .def("isOutput", &DA::DAPyLinkPoint::isOutput, "Check if this is an output connection point")
        .def(
            "__eq__",
            [](const DA::DAPyLinkPoint& a, const DA::DAPyLinkPoint& b) { return a == b; },
            pybind11::arg("other"),
            "Equality comparison with another DAPyLinkPoint")
        .def(
            "__eq__",
            [](const DA::DAPyLinkPoint& a, const std::string& b) { return a == QString::fromStdString(b); },
            pybind11::arg("other"),
            "Equality comparison with a string (by name)")
        .def("__repr__", [](const DA::DAPyLinkPoint& a) {
            return QString("DAPyLinkPoint(name=%1, way=%2)")
                .arg(a.name)
                .arg(a.way == DA::DAPyLinkPoint::Way::Input ? "Input" : "Output")
                .toStdString();
        });

    // 绑定 DAPyNodeMetaData 结构体
    pybind11::class_< DA::DAPyNodeMetaData >(m, "DAPyNodeMetaData")
        .def(pybind11::init<>())
        .def_readwrite("name", &DA::DAPyNodeMetaData::name, "Node display name")
        .def_readwrite("qualifiedName", &DA::DAPyNodeMetaData::qualifiedName, "Node unique qualified name (qualified_name)")
        .def_readwrite("group", &DA::DAPyNodeMetaData::group, "Node group/category")
        .def_readwrite("iconPath", &DA::DAPyNodeMetaData::iconPath, "Node icon path")
        .def_readwrite("tooltip", &DA::DAPyNodeMetaData::tooltip, "Node tooltip text")
        .def_readwrite("inputKeys", &DA::DAPyNodeMetaData::inputKeys, "Input key list")
        .def_readwrite("outputKeys", &DA::DAPyNodeMetaData::outputKeys, "Output key list")
        .def("isValid", &DA::DAPyNodeMetaData::isValid, "Check if metadata is valid (qualifiedName not empty)")
        .def(
            "__eq__",
            [](const DA::DAPyNodeMetaData& a, const DA::DAPyNodeMetaData& b) { return a == b; },
            pybind11::arg("other"),
            "Equality comparison by qualifiedName")
        .def("__repr__", [](const DA::DAPyNodeMetaData& m) {
            return QString("DAPyNodeMetaData(name=%1, qualifiedName=%2, group=%3, inputs=%4, outputs=%5)")
                .arg(m.name)
                .arg(m.qualifiedName)
                .arg(m.group)
                .arg(m.inputKeys.size())
                .arg(m.outputKeys.size())
                .toStdString();
        });

    // =================================================================================
    //                      DAWorkflowNodeState 绑定
    // =================================================================================

    pybind11::class_< DA::DAWorkflowNodeState >(m, "DAWorkflowNodeState")
        .def(pybind11::init<>())
        .def_readwrite("nodeId", &DA::DAWorkflowNodeState::nodeId, "节点唯一标识（Python侧node_id）")
        .def_readwrite("qualifiedName", &DA::DAWorkflowNodeState::qualifiedName, "节点限定名（Python qualified_name）")
        .def_readwrite("metaData", &DA::DAWorkflowNodeState::metaData, "节点元数据描述（DAPyNodeMetaData）")
        .def(
            "setPosition",
            [](DA::DAWorkflowNodeState& ns, QPointF pos) { ns.position = pos; },
            pybind11::arg("pos"),
            "设置节点位置 (QPointF or tuple)")
        .def(
            "setPosition",
            [](DA::DAWorkflowNodeState& ns, double x, double y) { ns.position = QPointF(x, y); },
            pybind11::arg("x"),
            pybind11::arg("y"),
            "设置节点位置 (x, y)")
        .def("getPosition", [](const DA::DAWorkflowNodeState& ns) -> QPointF { return ns.position; }, "获取节点位置");

    // =================================================================================
    //                      DAWorkflowConnectionState 绑定
    // =================================================================================

    pybind11::class_< DA::DAWorkflowConnectionState >(m, "DAWorkflowConnectionState")
        .def(pybind11::init<>())
        .def_readwrite("connectionId", &DA::DAWorkflowConnectionState::connectionId, "连接唯一标识")
        .def_readwrite("fromNodeId", &DA::DAWorkflowConnectionState::fromNodeId, "源节点ID")
        .def_readwrite("fromChannel", &DA::DAWorkflowConnectionState::fromChannel, "源节点输出通道编号")
        .def_readwrite("toNodeId", &DA::DAWorkflowConnectionState::toNodeId, "目标节点ID")
        .def_readwrite("toChannel", &DA::DAWorkflowConnectionState::toChannel, "目标节点输入通道编号");

    // =================================================================================
    //                      DAWorkflowState 绑定
    // =================================================================================

    pybind11::class_< DA::DAWorkflowState >(m, "DAWorkflowState")
        .def(pybind11::init<>())
        .def_readwrite("name", &DA::DAWorkflowState::name, "工作流名称")
        // nodes: QVector<DAWorkflowNodeState> ↔ Python list
        .def(
            "setNodes",
            [](DA::DAWorkflowState& ws, const pybind11::list& pyList) {
                ws.nodes.clear();
                for (auto item : pyList) {
                    ws.nodes.append(item.cast< DA::DAWorkflowNodeState >());
                }
            },
            pybind11::arg("nodes"),
            "设置节点状态列表")
        .def(
            "getNodes",
            [](const DA::DAWorkflowState& ws) {
                pybind11::list pyList;
                for (const DA::DAWorkflowNodeState& ns : ws.nodes) {
                    pyList.append(ns);
                }
                return pyList;
            },
            "获取节点状态列表")
        // connections: QVector<DAWorkflowConnectionState> ↔ Python list
        .def(
            "setConnections",
            [](DA::DAWorkflowState& ws, const pybind11::list& pyList) {
                ws.connections.clear();
                for (auto item : pyList) {
                    ws.connections.append(item.cast< DA::DAWorkflowConnectionState >());
                }
            },
            pybind11::arg("connections"),
            "设置连接线状态列表")
        .def(
            "getConnections",
            [](const DA::DAWorkflowState& ws) {
                pybind11::list pyList;
                for (const DA::DAWorkflowConnectionState& cs : ws.connections) {
                    pyList.append(cs);
                }
                return pyList;
            },
            "获取连接线状态列表")
        // toXml: 返回XML字符串
        .def(
            "toXml",
            [](const DA::DAWorkflowState& ws) {
                QDomDocument doc;
                const_cast< DA::DAWorkflowState& >(ws).toXml(doc);
                return doc.toString().toStdString();
            },
            "序列化为XML字符串")
        // fromXml: 从XML字符串反序列化
        .def_static(
            "fromXml",
            [](const std::string& xmlStr) {
                QDomDocument doc;
                doc.setContent(QString::fromStdString(xmlStr));
                return DA::DAWorkflowState::fromXml(doc);
            },
            pybind11::arg("xml_string"),
            "从XML字符串反序列化")
        .def("__repr__", [](const DA::DAWorkflowState& ws) {
            return QString("DAWorkflowState(name=%1, nodes=%2, connections=%3)")
                .arg(ws.name)
                .arg(ws.nodes.size())
                .arg(ws.connections.size())
                .toStdString();
        });

    // 绑定 DAPyWorkFlowScene 类
    pybind11::class_< DA::DAPyWorkFlowScene >(m, "DAPyWorkFlowScene")
        .def(pybind11::init<>())
        // 节点管理 — 通过 DAPyNodeMetaData 创建节点
        .def(
            "createPyNode",
            [](DA::DAPyWorkFlowScene& self, const DA::DAPyNodeMetaData& metaData, QPointF pos) {
                return self.createPyNode(metaData, pos);
            },
            pybind11::arg("metaData"),
            pybind11::arg("pos"),
            "Create a Python node by DAPyNodeMetaData at specified position")
        .def(
            "createPyNode",
            [](DA::DAPyWorkFlowScene& self, const DA::DAPyNodeMetaData& metaData, double x, double y) {
                return self.createPyNode(metaData, QPointF(x, y));
            },
            pybind11::arg("metaData"),
            pybind11::arg("x"),
            pybind11::arg("y"),
            "Create a Python node by DAPyNodeMetaData at (x, y) position")
        .def(
            "removePyNodeItem",
            [](DA::DAPyWorkFlowScene& self, DA::DAPyNodeGraphicsItem* item) { return self.removePyNodeItem(item); },
            pybind11::arg("item"),
            "Remove a node item from scene")
        .def(
            "nodeItemAt",
            [](DA::DAPyWorkFlowScene& self, QPointF pos) { return self.nodeItemAt(pos); },
            pybind11::arg("pos"),
            "Get node item at specified position")
        .def(
            "nodeItemAt",
            [](DA::DAPyWorkFlowScene& self, double x, double y) { return self.nodeItemAt(QPointF(x, y)); },
            pybind11::arg("x"),
            pybind11::arg("y"),
            "Get node item at (x, y) position")
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
            "Get all node items in scene as a list")
        // 连接管理
        .def(
            "addPyNodeLink",
            [](DA::DAPyWorkFlowScene& self,
               DA::DAPyNodeGraphicsItem* fromItem,
               const std::string& fromOutput,
               DA::DAPyNodeGraphicsItem* toItem,
               const std::string& toInput) {
                return self.addPyNodeLink(
                    fromItem, QString::fromStdString(fromOutput), toItem, QString::fromStdString(toInput));
            },
            pybind11::arg("fromItem"),
            pybind11::arg("fromOutput"),
            pybind11::arg("toItem"),
            pybind11::arg("toInput"),
            "Add link between node items")
        .def(
            "removePyNodeLink",
            [](DA::DAPyWorkFlowScene& self, DA::DAPyLinkGraphicsItem* linkItem) {
                return self.removePyNodeLink(linkItem);
            },
            pybind11::arg("linkItem"),
            "Remove link from scene")
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
            "Get all link items in scene as a list")
        // 清空场景
        .def("clearPyScene", &DA::DAPyWorkFlowScene::clearPyScene, "Clear scene, remove all nodes and links");

    // 绑定 DAPyNodeFactory 类
    pybind11::class_< DA::DAPyNodeFactory >(m, "DAPyNodeFactory")
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
            pybind11::arg("scan_paths")       = std::vector< std::string >(),
            pybind11::arg("use_entry_points") = false,
            "Discover Python nodes in specified paths")
        // 节点创建
        .def(
            "createNodeProxy",
            [](DA::DAPyNodeFactory& self, const std::string& qualifiedName) {
                return self.createNodeProxy(QString::fromStdString(qualifiedName));
            },
            pybind11::arg("qualified_name"),
            "Create DAPyNodeProxy by qualified name")
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
            "Get all discovered node metadata as list")
        .def(
            "getNodePrototypes",
            [](DA::DAPyNodeFactory& self) {
                pybind11::list pyList;
                for (const QString& proto : self.getNodePrototypes()) {
                    pyList.append(proto.toStdString());
                }
                return pyList;
            },
            "Get all discovered node prototype identifiers")
        // 工厂信息
        .def(
            "factoryName", [](DA::DAPyNodeFactory& self) { return self.factoryName().toStdString(); }, "Get factory name")
        .def(
            "factoryDescribe",
            [](DA::DAPyNodeFactory& self) { return self.factoryDescribe().toStdString(); },
            "Get factory description");

    // 绑定 DAPyPainterProxy 类
    pybind11::class_< DA::DAPyPainterProxy >(m, "DAPyPainterProxy")
        .def("drawRect",
             &DA::DAPyPainterProxy::drawRect,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("w"),
             pybind11::arg("h"),
             "Draw rectangle outline")
        .def("drawText",
             &DA::DAPyPainterProxy::drawText,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("text"),
             "Draw text at specified position")
        .def("drawLine",
             &DA::DAPyPainterProxy::drawLine,
             pybind11::arg("x1"),
             pybind11::arg("y1"),
             pybind11::arg("x2"),
             pybind11::arg("y2"),
             "Draw line from (x1,y1) to (x2,y2)")
        .def("drawEllipse",
             &DA::DAPyPainterProxy::drawEllipse,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("w"),
             pybind11::arg("h"),
             "Draw ellipse in specified rectangle")
        .def("fillRect",
             &DA::DAPyPainterProxy::fillRect,
             pybind11::arg("x"),
             pybind11::arg("y"),
             pybind11::arg("w"),
             pybind11::arg("h"),
             pybind11::arg("r"),
             pybind11::arg("g"),
             pybind11::arg("b"),
             pybind11::arg("a") = 255,
             "Fill rectangle with RGBA color")
        .def("setPenColor",
             &DA::DAPyPainterProxy::setPenColor,
             pybind11::arg("r"),
             pybind11::arg("g"),
             pybind11::arg("b"),
             pybind11::arg("a") = 255,
             "Set pen color (RGBA)")
        .def("setPenWidth", &DA::DAPyPainterProxy::setPenWidth, pybind11::arg("width"), "Set pen width")
        .def("setBrushColor",
             &DA::DAPyPainterProxy::setBrushColor,
             pybind11::arg("r"),
             pybind11::arg("g"),
             pybind11::arg("b"),
             pybind11::arg("a") = 255,
             "Set brush color (RGBA)")
        .def("setFont", &DA::DAPyPainterProxy::setFont, pybind11::arg("family"), pybind11::arg("size"), "Set font family and size")
        .def("setNoPen", &DA::DAPyPainterProxy::setNoPen, "Set no pen (disable outline drawing)")
        .def("setNoBrush", &DA::DAPyPainterProxy::setNoBrush, "Set no brush (disable fill)")
        .def("isValid", &DA::DAPyPainterProxy::isValid, "Check if painter proxy is valid");

    // 自由函数
    m.def("getNodeProxy",
          &DA::getNodeProxy,
          pybind11::arg("qualified_name"),
          "Get DAPyNodeProxy instance by Python node's qualified name");

    m.def(
        "_note_signal_handler",
        []() {
            return "Python→C++ state updates use DAPythonSignalHandler::callInMainThread from da_interface module "
                   "(NOT a custom bridge class). Example: core.getPythonSignalHandler().callInMainThread(callback)";
        },
        "Note about state update mechanism");
}
