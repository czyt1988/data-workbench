#include "DAPyDictConverter.h"
#include "DAPyWorkFlowEnumStringUtils.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyBindQt/DAPyJsonCast.h"
#include <QDebug>

namespace DA
{
namespace DictConverter
{

QColor colorFromPyObj(const pybind11::object& obj)
{
    if (obj.is_none()) {
        return QColor();  // invalid color → use default
    }
    // hex string: "#ff0000"
    if (pybind11::isinstance< pybind11::str >(obj)) {
        QString hexStr = pybind11::cast< QString >(obj);
        return QColor(hexStr);
    }
    // RGB tuple: (255, 200, 200) or (r, g, b, a)
    if (pybind11::isinstance< pybind11::tuple >(obj) || pybind11::isinstance< pybind11::list >(obj)) {
        auto seq = pybind11::cast< pybind11::sequence >(obj);
        int r    = pybind11::cast< int >(seq[ 0 ]);
        int g    = pybind11::cast< int >(seq[ 1 ]);
        int b    = pybind11::cast< int >(seq[ 2 ]);
        if (pybind11::len(obj) >= 4) {
            int a = pybind11::cast< int >(seq[ 3 ]);
            return QColor(r, g, b, a);
        }
        return QColor(r, g, b);
    }
    return QColor();
}

DAPyLinkPointStyle linkPointStyleFromDict(const pybind11::dict& d)
{
    DAPyLinkPointStyle s;
    if (d.contains("shape")) {
        try {
            QString shapeStr = pybind11::cast< QString >(d[ "shape" ]);
            s.shape          = stringToEnum(shapeStr, PortShape::Rect);
        } catch (const std::exception&) {
            s.shape = PortShape::Rect;
        }
    }
    if (d.contains("fill_color")) {
        s.fillColor = colorFromPyObj(d[ "fill_color" ]);
    }
    if (d.contains("border_color")) {
        s.borderColor = colorFromPyObj(d[ "border_color" ]);
    }
    if (d.contains("border_width")) {
        s.borderWidth = pybind11::cast< qreal >(d[ "border_width" ]);
    }
    return s;
}

DAPyNodeDisplayStyle nodeStyleFromDict(const pybind11::dict& d)
{
    DAPyNodeDisplayStyle s;  // setDefaults() already called by constructor

    // 主体样式
    if (d.contains("body_shape")) {
        try {
            QString str = pybind11::cast< QString >(d[ "body_shape" ]);
            s.bodyShape = stringToEnum(str, BodyShape::RoundedRect);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("name_position")) {
        try {
            QString str    = pybind11::cast< QString >(d[ "name_position" ]);
            s.namePosition = stringToEnum(str, NamePosition::Inside);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("icon_position")) {
        try {
            QString str    = pybind11::cast< QString >(d[ "icon_position" ]);
            s.iconPosition = stringToEnum(str, IconPosition::LeftOfText);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("background_color")) {
        s.backgroundColor = colorFromPyObj(d[ "background_color" ]);
    }
    if (d.contains("border_color")) {
        s.borderColor = colorFromPyObj(d[ "border_color" ]);
    }
    if (d.contains("border_width")) {
        try {
            s.borderWidth = pybind11::cast< qreal >(d[ "border_width" ]);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("corner_radius")) {
        try {
            s.cornerRadius = pybind11::cast< qreal >(d[ "corner_radius" ]);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("icon_size")) {
        try {
            s.iconSize = pybind11::cast< qreal >(d[ "icon_size" ]);
        } catch (const std::exception&) {
        }
    }

    // 端口配置
    if (d.contains("input_port_side")) {
        try {
            QString str     = pybind11::cast< QString >(d[ "input_port_side" ]);
            s.inputPortSide = stringToEnum(str, PortSide::West);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("output_port_side")) {
        try {
            QString str      = pybind11::cast< QString >(d[ "output_port_side" ]);
            s.outputPortSide = stringToEnum(str, PortSide::East);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("input_port_style")) {
        try {
            pybind11::object obj = d[ "input_port_style" ];
            if (pybind11::isinstance< pybind11::dict >(obj)) {
                s.inputPortStyle = linkPointStyleFromDict(pybind11::cast< pybind11::dict >(obj));
            }
        } catch (const std::exception&) {
        }
    }
    if (d.contains("output_port_style")) {
        try {
            pybind11::object obj = d[ "output_port_style" ];
            if (pybind11::isinstance< pybind11::dict >(obj)) {
                s.outputPortStyle = linkPointStyleFromDict(pybind11::cast< pybind11::dict >(obj));
            }
        } catch (const std::exception&) {
        }
    }
    if (d.contains("layout_strategy")) {
        try {
            QString str      = pybind11::cast< QString >(d[ "layout_strategy" ]);
            s.layoutStrategy = stringToEnum(str, LinkPointLayoutStrategy::Auto);
        } catch (const std::exception&) {
        }
    }

    // 节点体图标
    if (d.contains("body_icon_type")) {
        try {
            QString str    = pybind11::cast< QString >(d[ "body_icon_type" ]);
            s.bodyIconType = stringToEnum(str, BodyIconType::None);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("body_icon_source")) {
        try {
            s.bodyIconSource = pybind11::cast< QString >(d[ "body_icon_source" ]);
        } catch (const std::exception&) {
        }
    }
    if (d.contains("body_icon_scale")) {
        try {
            s.bodyIconScale = pybind11::cast< qreal >(d[ "body_icon_scale" ]);
        } catch (const std::exception&) {
        }
    }

    return s;
}

DAPyNodeRenderTemplate renderTemplateFromString(const QString& s)
{
    // 支持新旧格式：nodestyle/node_style/rect/svg → NodeStyleTemplate
    if (s == "nodestyle" || s == "node_style" || s == "rect" || s == "svg") {
        return DAPyNodeRenderTemplate::NodeStyleTemplate;
    }
    if (s == "widget") {
        return DAPyNodeRenderTemplate::WidgetTemplate;
    }
    // 尝试使用 stringToEnum
    return stringToEnum(s, DAPyNodeRenderTemplate::NodeStyleTemplate);
}

}  // namespace DictConverter
}  // namespace DA
