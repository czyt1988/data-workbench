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

DAPortDescriptor portFromDict(const pybind11::dict& d)
{
    DAPortDescriptor pd;
    if (d.contains("name")) {
        pd.name = pybind11::cast< QString >(d[ "name" ]);
    }
    if (d.contains("data_type")) {
        pd.dataType = pybind11::cast< QString >(d[ "data_type" ]);
    }
    if (d.contains("required")) {
        pd.required = pybind11::cast< bool >(d[ "required" ]);
    }
    if (d.contains("description")) {
        pd.description = pybind11::cast< QString >(d[ "description" ]);
    }
    return pd;
}

DAParameterDescriptor paramFromDict(const pybind11::dict& d)
{
    DAParameterDescriptor pd;
    if (d.contains("name")) {
        pd.name = pybind11::cast< QString >(d[ "name" ]);
    }
    if (d.contains("type")) {
        pd.type = pybind11::cast< QString >(d[ "type" ]);
    }
    if (d.contains("description")) {
        pd.description = pybind11::cast< QString >(d[ "description" ]);
    }
    if (d.contains("default")) {
        try {
            pybind11::object defaultObj = d[ "default" ];
            if (!defaultObj.is_none()) {
                // 尝试转换为 QVariant（通过 JSON 中间格式）
                QJsonObject tmpJson = DA::PY::pyDictToQJsonObject(pybind11::dict());
                // 直接通过 pybind11 cast 常见类型
                if (pybind11::isinstance< pybind11::str >(defaultObj)) {
                    pd.defaultValue = pybind11::cast< QString >(defaultObj);
                } else if (pybind11::isinstance< pybind11::int_ >(defaultObj)) {
                    pd.defaultValue = pybind11::cast< int >(defaultObj);
                } else if (pybind11::isinstance< pybind11::float_ >(defaultObj)) {
                    pd.defaultValue = pybind11::cast< double >(defaultObj);
                } else if (pybind11::isinstance< pybind11::bool_ >(defaultObj)) {
                    pd.defaultValue = pybind11::cast< bool >(defaultObj);
                }
            }
        } catch (const std::exception& e) {
            qWarning() << "DAPyDictConverter::paramFromDict default value cast failed:" << e.what();
        }
    }
    // 处理 propertys：Python 的 extra kwargs
    if (d.contains("properties")) {
        try {
            pybind11::object propObj = d[ "properties" ];
            if (pybind11::isinstance< pybind11::dict >(propObj)) {
                pybind11::dict propDict = pybind11::cast< pybind11::dict >(propObj);
                // 逐键转换
                QVariantHash props;
                for (auto item : propDict) {
                    std::string key      = pybind11::cast< std::string >(item.first);
                    pybind11::object val = pybind11::reinterpret_borrow< pybind11::object >(item.second);
                    QString qKey         = QString::fromStdString(key);
                    // 基本类型转换
                    if (pybind11::isinstance< pybind11::str >(val)) {
                        props[ qKey ] = pybind11::cast< QString >(val);
                    } else if (pybind11::isinstance< pybind11::int_ >(val)) {
                        props[ qKey ] = pybind11::cast< int >(val);
                    } else if (pybind11::isinstance< pybind11::float_ >(val)) {
                        props[ qKey ] = pybind11::cast< double >(val);
                    } else if (pybind11::isinstance< pybind11::bool_ >(val)) {
                        props[ qKey ] = pybind11::cast< bool >(val);
                    } else if (pybind11::isinstance< pybind11::list >(val)) {
                        // list → QStringList（用于 enum_options 等）
                        QStringList strList;
                        pybind11::list pyList = pybind11::cast< pybind11::list >(val);
                        for (auto listItem : pyList) {
                            strList.append(pybind11::cast< QString >(listItem));
                        }
                        props[ qKey ] = strList;
                    }
                }
                pd.propertys = props;
            }
        } catch (const std::exception& e) {
            qWarning() << "DAPyDictConverter::paramFromDict properties cast failed:" << e.what();
        }
    }
    return pd;
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

DANodeStyle nodeStyleFromDict(const pybind11::dict& d)
{
    DANodeStyle s;  // setDefaults() already called by constructor

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

RenderTemplate renderTemplateFromString(const QString& s)
{
    // 支持新旧格式：nodestyle/node_style/rect/svg → NodeStyleTemplate
    if (s == "nodestyle" || s == "node_style" || s == "rect" || s == "svg") {
        return RenderTemplate::NodeStyleTemplate;
    }
    if (s == "widget") {
        return RenderTemplate::WidgetTemplate;
    }
    // 尝试使用 stringToEnum
    return stringToEnum(s, RenderTemplate::NodeStyleTemplate);
}

QVector< DAPortDescriptor > portListFromPyList(const pybind11::list& lst)
{
    QVector< DAPortDescriptor > result;
    for (auto item : lst) {
        pybind11::dict d = pybind11::cast< pybind11::dict >(item);
        result.append(portFromDict(d));
    }
    return result;
}

QVector< DAParameterDescriptor > paramListFromPyList(const pybind11::list& lst)
{
    QVector< DAParameterDescriptor > result;
    for (auto item : lst) {
        pybind11::dict d = pybind11::cast< pybind11::dict >(item);
        result.append(paramFromDict(d));
    }
    return result;
}

}  // namespace DictConverter
}  // namespace DA
