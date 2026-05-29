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

namespace PY
{

namespace
{
/// 安全读取 Python 属性并转为枚举，属性不存在或为 None 时返回 defaultValue
template< typename EnumType >
EnumType readEnumAttr(const pybind11::object& obj, const char* name, EnumType defaultValue)
{
    if (!pybind11::hasattr(obj, name))
        return defaultValue;
    auto a = obj.attr(name);
    if (a.is_none())
        return defaultValue;
    return stringToEnum(a.cast< QString >(), defaultValue);
}

/// 安全读取 Python 属性并转为 QColor，属性不存在或为 None 时返回 defaultValue
QColor readColorAttr(const pybind11::object& obj, const char* name, QColor defaultValue = QColor())
{
    if (!pybind11::hasattr(obj, name))
        return defaultValue;
    auto a = obj.attr(name);
    if (a.is_none())
        return defaultValue;
    return qcolorFromPyObject(a);
}

/// 安全读取 Python 属性并直接 cast，属性不存在或为 None 时返回 defaultValue
template< typename T >
T readCastAttr(const pybind11::object& obj, const char* name, T defaultValue)
{
    if (!pybind11::hasattr(obj, name))
        return defaultValue;
    auto a = obj.attr(name);
    if (a.is_none())
        return defaultValue;
    return a.cast< T >();
}
}  // namespace

DAPyNodeMetaData toNodeMetaData(const pybind11::object& obj)
{
    DAPyNodeMetaData metaData;

    // qualified_name 为必填字段
    metaData.qualifiedName = readCastAttr< QString >(obj, "qualified_name", metaData.qualifiedName);
    if (metaData.qualifiedName.isEmpty()) {
        return metaData;
    }
    metaData.name     = readCastAttr< QString >(obj, "name", metaData.name);
    metaData.category = readCastAttr< QString >(obj, "category", metaData.category);
    metaData.iconPath = readCastAttr< QString >(obj, "icon", metaData.iconPath);
    return metaData;
}

DAPyNodeState getNodeState(const pybind11::object& obj)
{
    if (obj.is_none()) {
        return DAPyNodeState::Idle;
    }
    try {
        if (!pybind11::hasattr(obj, "_node_state"))
            return DAPyNodeState::Idle;
        auto stateObj = obj.attr("_node_state");
        if (stateObj.is_none())
            return DAPyNodeState::Idle;
        // Python 端 _node_state 可能是字符串或 da_py_workflow.DAPyNodeState
        if (pybind11::isinstance< pybind11::str >(stateObj)) {
            return stringToEnum(stateObj.cast< QString >(), DAPyNodeState::Idle);
        }
        // 如果是整数枚举值
        if (pybind11::isinstance< pybind11::int_ >(stateObj)) {
            return static_cast< DAPyNodeState >(stateObj.cast< int >());
        }
    } catch (const std::exception& e) {
        return DAPyNodeState::Idle;
    }
    return DAPyNodeState::Idle;
}

/**
 * @brief 从 Python NodeDisplay 对象属性读取样式，转换为 DAPyNodeStyle
 *
 * NodeDisplay 是 Python 端的 dataclass，所有样式字段都是显式属性（body_shape、background_color 等），
 * 此函数直接从对象属性读取，无需经过 dict 中转。
 * 字段为 None 时表示使用 C++ 默认值，不修改 DAPyNodeStyle 中对应的字段。
 *
 * @param obj Python NodeDisplay 对象（_node_display 属性）
 * @return DAPyNodeStyle 结构体，仅包含 Python 端非 None 的字段值
 */
DAPyNodeStyle toNodeStyle(const pybind11::object& obj)
{
    DAPyNodeStyle style;  // 默认构造已调用 setDefaults()

    if (obj.is_none()) {
        return style;
    }

    try {
        // 渲染模板
        style.renderTemplate = readEnumAttr(obj, "render_template", style.renderTemplate);
        // === 主体样式 ===
        style.bodyShape       = readEnumAttr(obj, "body_shape", style.bodyShape);
        style.namePosition    = readEnumAttr(obj, "name_position", style.namePosition);
        style.iconPosition    = readEnumAttr(obj, "icon_position", style.iconPosition);
        style.backgroundColor = readColorAttr(obj, "background_color", style.backgroundColor);
        style.borderColor     = readColorAttr(obj, "border_color", style.borderColor);
        style.borderWidth     = readCastAttr< double >(obj, "border_width", style.borderWidth);
        style.cornerRadius    = readCastAttr< double >(obj, "corner_radius", style.cornerRadius);
        style.iconSize        = readCastAttr< double >(obj, "icon_size", style.iconSize);

        // === 端口配置 ===
        style.inputPortSide  = readEnumAttr(obj, "input_port_side", style.inputPortSide);
        style.outputPortSide = readEnumAttr(obj, "output_port_side", style.outputPortSide);
        style.layoutStrategy = readEnumAttr(obj, "layout_strategy", style.layoutStrategy);

        // === 端口样式（LinkPointStyle 子对象） ===
        if (pybind11::hasattr(obj, "input_port_style") && !obj.attr("input_port_style").is_none()) {
            auto ipsObj                      = obj.attr("input_port_style");
            style.inputPortStyle.shape       = readEnumAttr(ipsObj, "shape", style.inputPortStyle.shape);
            style.inputPortStyle.fillColor   = readColorAttr(ipsObj, "fill_color", style.inputPortStyle.fillColor);
            style.inputPortStyle.borderColor = readColorAttr(ipsObj, "border_color", style.inputPortStyle.borderColor);
            style.inputPortStyle.borderWidth =
                readCastAttr< double >(ipsObj, "border_width", style.inputPortStyle.borderWidth);
        }
        if (pybind11::hasattr(obj, "output_port_style") && !obj.attr("output_port_style").is_none()) {
            auto opsObj                     = obj.attr("output_port_style");
            style.outputPortStyle.shape     = readEnumAttr(opsObj, "shape", style.outputPortStyle.shape);
            style.outputPortStyle.fillColor = readColorAttr(opsObj, "fill_color", style.outputPortStyle.fillColor);
            style.outputPortStyle.borderColor = readColorAttr(opsObj, "border_color", style.outputPortStyle.borderColor);
            style.outputPortStyle.borderWidth =
                readCastAttr< double >(opsObj, "border_width", style.outputPortStyle.borderWidth);
        }

        // === 节点体图标 ===
        style.bodyIconType   = readEnumAttr(obj, "body_icon_type", style.bodyIconType);
        style.bodyIconSource = readCastAttr< QString >(obj, "body_icon_source", style.bodyIconSource);
        style.bodyIconScale  = readCastAttr< double >(obj, "body_icon_scale", style.bodyIconScale);
    } catch (const std::exception& e) {
        // 读取失败时返回默认样式
    }

    return style;
}

}  // namespace PY

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

    m.def(
        "_note_signal_handler",
        []() {
            return "Python→C++ state updates use DAPythonSignalHandler::callInMainThread from da_interface module "
                   "(NOT a custom bridge class). Example: core.getPythonSignalHandler().callInMainThread(callback)";
        },
        "Note about state update mechanism");
}
