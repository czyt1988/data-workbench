#include "DAPyWorkFlowPythonBinding.h"
#include <QPointF>
#include "DAPyWorkFlow/DAPyWorkFlowScene.h"
#include "DAPyWorkFlow/DAPyNode.h"
#include "DAPyWorkFlow/DAPyNodeState.h"
#include "DAPyWorkFlow/DAPyNodeGraphicsItem.h"
#include "DAPyWorkFlow/DAPyLinkGraphicsItem.h"
#include "DAPyWorkFlow/DAPyPainterProxy.h"
#include "DAPyWorkFlow/DAPyLinkPoint.h"
#include "DAPyWorkFlow/DAPyNodeFactory.h"
#include "DAPyWorkFlow/DAPyNodeStyle.h"
#include "DAPyWorkFlow/DAPyWorkFlowEnumStringUtils.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
#include "DAPyBindQt/DAPyJsonCast.h"
#include "DAPyBindQt/DAPybind11InQt.h"  // slots workaround，必须第一个pybind11相关头文件

namespace DA
{

/**
 * @brief 通过 qualified_name 获取 DAPyNode 实例
 * @param[in] qualified_name Python 节点的完整限定名（模块.类名）
 * @return 代理节点智能指针，如果节点未注册返回空指针
 */
std::shared_ptr< DA::DAPyNode > getNodeProxy(const std::string& qualified_name)
{
    // TODO: 实现通过 DAPyModuleWorkflow 获取 Python 节点类并创建代理
    return std::shared_ptr< DA::DAPyNode >();
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
    pybind11::enum_< DA::DAAspectDirection >(m, "AspectDirection")
        .value("East", DA::DAAspectDirection::East)
        .value("South", DA::DAAspectDirection::South)
        .value("West", DA::DAAspectDirection::West)
        .value("North", DA::DAAspectDirection::North)
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
        .def(pybind11::init< const QPointF&, const QString&, DA::DAPyLinkPoint::Way, DA::DAAspectDirection >(),
             pybind11::arg("position"),
             pybind11::arg("name"),
             pybind11::arg("way")       = DA::DAPyLinkPoint::Way::Output,
             pybind11::arg("direction") = DA::DAAspectDirection::East)
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
          "Get DAPyNode instance by Python node's qualified name");

    m.def(
        "_note_signal_handler",
        []() {
            return "Python→C++ state updates use DAPythonSignalHandler::callInMainThread from da_interface module "
                   "(NOT a custom bridge class). Example: core.getPythonSignalHandler().callInMainThread(callback)";
        },
        "Note about state update mechanism");
}
