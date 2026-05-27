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
#include "DAPyWorkFlow/DAPyDictConverter.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
#include "DAPyBindQt/DAPyJsonCast.h"
#include "DAPyBindQt/DAPybind11InQt.h"  // slots workaround，必须第一个pybind11相关头文件

namespace DA
{

namespace PY
{

DAPyNodeMetaData toNodeMetaData(const pybind11::object& obj)
{
    DAPyNodeMetaData metaData;

    // 从 Python 类属性直接读取
    if (pybind11::hasattr(obj, "qualified_name")) {
        metaData.qualifiedName = obj.attr("qualified_name").cast< QString >();
    } else {
        // 必须要有qualified_name
        return metaData;
    }
    if (pybind11::hasattr(obj, "name")) {
        metaData.name = obj.attr("name").cast< QString >();
    }
    if (pybind11::hasattr(obj, "category")) {
        metaData.category = obj.attr("category").cast< QString >();
    }
    if (pybind11::hasattr(obj, "icon")) {
        metaData.iconPath = obj.attr("icon").cast< QString >();
    }
    return metaData;
}

DAPyNodeState getNodeState(const pybind11::object& obj)
{
    if (obj.is_none()) {
        return DAPyNodeState::Idle;
    }
    try {
        // 尝试从 Python 对象读取 _node_state 属性
        if (pybind11::hasattr(obj, "_node_state")) {
            pybind11::object stateObj = obj.attr("_node_state");
            // Python 端 _node_state 可能是字符串或 da_py_workflow.DAPyNodeState
            if (pybind11::isinstance< pybind11::str >(stateObj)) {
                QString stateStr = pybind11::cast< QString >(stateObj);
                return stringToEnum(stateStr, DAPyNodeState::Idle);
            }
            // 如果是整数枚举值
            if (pybind11::isinstance< pybind11::int_ >(stateObj)) {
                int val = pybind11::cast< int >(stateObj);
                return static_cast< DAPyNodeState >(val);
            }
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
        // === 主体样式 ===
        if (pybind11::hasattr(obj, "body_shape") && !obj.attr("body_shape").is_none()) {
            QString val     = obj.attr("body_shape").cast< QString >();
            style.bodyShape = stringToEnum(val, BodyShape::RoundedRect);
        }
        if (pybind11::hasattr(obj, "name_position") && !obj.attr("name_position").is_none()) {
            QString val        = obj.attr("name_position").cast< QString >();
            style.namePosition = stringToEnum(val, NamePosition::Inside);
        }
        if (pybind11::hasattr(obj, "icon_position") && !obj.attr("icon_position").is_none()) {
            QString val        = obj.attr("icon_position").cast< QString >();
            style.iconPosition = stringToEnum(val, IconPosition::LeftOfText);
        }
        if (pybind11::hasattr(obj, "background_color") && !obj.attr("background_color").is_none()) {
            style.backgroundColor = qcolorFromPyObject(obj.attr("background_color"));
        }
        if (pybind11::hasattr(obj, "border_color") && !obj.attr("border_color").is_none()) {
            style.borderColor = qcolorFromPyObject(obj.attr("border_color"));
        }
        if (pybind11::hasattr(obj, "border_width") && !obj.attr("border_width").is_none()) {
            style.borderWidth = obj.attr("border_width").cast< double >();
        }
        if (pybind11::hasattr(obj, "corner_radius") && !obj.attr("corner_radius").is_none()) {
            style.cornerRadius = obj.attr("corner_radius").cast< double >();
        }
        if (pybind11::hasattr(obj, "icon_size") && !obj.attr("icon_size").is_none()) {
            style.iconSize = obj.attr("icon_size").cast< double >();
        }

        // === 端口配置 ===
        if (pybind11::hasattr(obj, "input_port_side") && !obj.attr("input_port_side").is_none()) {
            QString val         = obj.attr("input_port_side").cast< QString >();
            style.inputPortSide = stringToEnum(val, DAAspectDirection::West);
        }
        if (pybind11::hasattr(obj, "output_port_side") && !obj.attr("output_port_side").is_none()) {
            QString val          = obj.attr("output_port_side").cast< QString >();
            style.outputPortSide = stringToEnum(val, DAAspectDirection::East);
        }
        if (pybind11::hasattr(obj, "layout_strategy") && !obj.attr("layout_strategy").is_none()) {
            QString val          = obj.attr("layout_strategy").cast< QString >();
            style.layoutStrategy = stringToEnum(val, LinkPointLayoutStrategy::Auto);
        }

        // === 端口样式（LinkPointStyle 子对象） ===
        if (pybind11::hasattr(obj, "input_port_style") && !obj.attr("input_port_style").is_none()) {
            pybind11::object ipsObj = obj.attr("input_port_style");
            if (pybind11::hasattr(ipsObj, "shape") && !ipsObj.attr("shape").is_none()) {
                style.inputPortStyle.shape = stringToEnum(ipsObj.attr("shape").cast< QString >(), PortShape::Rect);
            }
            if (pybind11::hasattr(ipsObj, "fill_color") && !ipsObj.attr("fill_color").is_none()) {
                style.inputPortStyle.fillColor = qcolorFromPyObject(ipsObj.attr("fill_color"));
            }
            if (pybind11::hasattr(ipsObj, "border_color") && !ipsObj.attr("border_color").is_none()) {
                style.inputPortStyle.borderColor = qcolorFromPyObject(ipsObj.attr("border_color"));
            }
            if (pybind11::hasattr(ipsObj, "border_width") && !ipsObj.attr("border_width").is_none()) {
                style.inputPortStyle.borderWidth = ipsObj.attr("border_width").cast< double >();
            }
        }
        if (pybind11::hasattr(obj, "output_port_style") && !obj.attr("output_port_style").is_none()) {
            pybind11::object opsObj = obj.attr("output_port_style");
            if (pybind11::hasattr(opsObj, "shape") && !opsObj.attr("shape").is_none()) {
                style.outputPortStyle.shape = stringToEnum(opsObj.attr("shape").cast< QString >(), PortShape::Rect);
            }
            if (pybind11::hasattr(opsObj, "fill_color") && !opsObj.attr("fill_color").is_none()) {
                style.outputPortStyle.fillColor = qcolorFromPyObject(opsObj.attr("fill_color"));
            }
            if (pybind11::hasattr(opsObj, "border_color") && !opsObj.attr("border_color").is_none()) {
                style.outputPortStyle.borderColor = qcolorFromPyObject(opsObj.attr("border_color"));
            }
            if (pybind11::hasattr(opsObj, "border_width") && !opsObj.attr("border_width").is_none()) {
                style.outputPortStyle.borderWidth = opsObj.attr("border_width").cast< double >();
            }
        }

        // === 节点体图标 ===
        if (pybind11::hasattr(obj, "body_icon_type") && !obj.attr("body_icon_type").is_none()) {
            QString val        = obj.attr("body_icon_type").cast< QString >();
            style.bodyIconType = stringToEnum(val, BodyIconType::None);
        }
        if (pybind11::hasattr(obj, "body_icon_source") && !obj.attr("body_icon_source").is_none()) {
            style.bodyIconSource = obj.attr("body_icon_source").cast< QString >();
        }
        if (pybind11::hasattr(obj, "body_icon_scale") && !obj.attr("body_icon_scale").is_none()) {
            style.bodyIconScale = obj.attr("body_icon_scale").cast< double >();
        }
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
    pybind11::enum_< DA::DAPyNodeRenderTemplate >(m, "RenderTemplate")
        .value("NodeStyleTemplate", DA::DAPyNodeRenderTemplate::NodeStyleTemplate)
        .value("WidgetTemplate", DA::DAPyNodeRenderTemplate::WidgetTemplate)
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
