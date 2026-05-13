#include "DANodeDescriptor.h"
#include <QJsonObject>
#include <QJsonArray>
namespace DA
{

// ============================================================
// DANodeDescriptor — 默认构造函数
// ============================================================

/**
 * @brief 默认构造函数，初始化节点描述符
 *
 * 初始化字段为以下默认值：
 * - name: 空字符串
 * - qualifiedName: 空字符串
 * - category: 空字符串
 * - icon: 空字符串
 * - inputs: 空 QVector
 * - outputs: 空 QVector
 * - parameters: 空 QVector
 * - renderTemplate: RenderTemplate::NodeStyleTemplate
 * - style: 默认 DANodeStyle（调用 setDefaults）
 */
DANodeDescriptor::DANodeDescriptor() : renderTemplate(RenderTemplate::NodeStyleTemplate), style(DANodeStyle())
{
}

// ============================================================
// DANodeDescriptor::isValid — 有效性判断
// ============================================================

/**
 * @brief 判断节点描述符是否有效
 *
 * 仅当 qualifiedName 非空时返回 true，
 * 空字符串表示节点尚未绑定到具体 Python 类，视为无效。
 *
 * @return true 表示 qualifiedName 非空，false 表示无效
 */
bool DANodeDescriptor::isValid() const
{
    return !qualifiedName.isEmpty();
}

// ============================================================
// DANodeDescriptor::toMetaData — 转换为 DAPyNodeMetaData
// ============================================================

/**
 * @brief 从节点描述符提取 DAPyNodeMetaData
 *
 * 将描述符字段映射到工厂注册所需的元数据：
 * - name → metaData.name
 * - qualifiedName → metaData.qualifiedName
 * - category → metaData.group
 * - icon → metaData.iconPath
 * - tooltip = name + " (" + qualifiedName + ")"（qualifiedName 非空时）
 * - inputs 各项的 .name → metaData.inputKeys
 * - outputs 各项的 .name → metaData.outputKeys
 *
 * @return 包含注册信息的 DAPyNodeMetaData 实例
 * @see DAPyNodeFactory::convertDescriptorToMetaData（Python dict 版本）
 */
DAPyNodeMetaData DANodeDescriptor::toMetaData() const
{
    DAPyNodeMetaData metaData;

    metaData.name          = name;
    metaData.qualifiedName = qualifiedName;
    metaData.group         = category;
    metaData.iconPath      = icon;

    // tooltip: name + " (" + qualifiedName + ")"
    metaData.tooltip = name;
    if (!qualifiedName.isEmpty()) {
        metaData.tooltip += " (" + qualifiedName + ")";
    }

    // inputs → inputKeys（提取每项的 .name）
    for (const DAPortDescriptor& port : inputs) {
        metaData.inputKeys.append(port.name);
    }

    // outputs → outputKeys（提取每项的 .name）
    for (const DAPortDescriptor& port : outputs) {
        metaData.outputKeys.append(port.name);
    }

    return metaData;
}

/**
 * @brief 序列化为 QJsonObject
 *
 * JSON 键名使用 camelCase 以匹配 C++ 字段名。
 * 使用稀疏策略：icon 和 style 仅在非默认值时写入。
 * inputs/outputs/parameters 始终写入（可为空数组）。
 * style 通过内联字段序列化处理。
 */
QJsonObject DANodeDescriptor::toJson() const
{
    QJsonObject obj;
    obj[ QStringLiteral("name") ]          = name;
    obj[ QStringLiteral("qualifiedName") ] = qualifiedName;
    obj[ QStringLiteral("category") ]      = category;
    if (!icon.isEmpty()) {
        obj[ QStringLiteral("icon") ] = icon;
    }
    obj[ QStringLiteral("renderTemplate") ] = static_cast< int >(renderTemplate);

    // style（稀疏策略，仅非默认字段）
    {
        const DANodeStyle defaultStyle;
        QJsonObject styleJson;
        if (style.bodyShape != defaultStyle.bodyShape)
            styleJson[ QStringLiteral("bodyShape") ] = enumToString(style.bodyShape);
        if (style.namePosition != defaultStyle.namePosition)
            styleJson[ QStringLiteral("namePosition") ] = enumToString(style.namePosition);
        if (style.iconPosition != defaultStyle.iconPosition)
            styleJson[ QStringLiteral("iconPosition") ] = enumToString(style.iconPosition);
        if (style.backgroundColor != defaultStyle.backgroundColor)
            styleJson[ QStringLiteral("backgroundColor") ] = style.backgroundColor.name();
        if (style.borderColor != defaultStyle.borderColor)
            styleJson[ QStringLiteral("borderColor") ] = style.borderColor.name();
        if (qFuzzyCompare(style.borderWidth + 1.0, defaultStyle.borderWidth + 1.0) == false)
            styleJson[ QStringLiteral("borderWidth") ] = style.borderWidth;
        if (qFuzzyCompare(style.cornerRadius + 1.0, defaultStyle.cornerRadius + 1.0) == false)
            styleJson[ QStringLiteral("cornerRadius") ] = style.cornerRadius;
        if (qFuzzyCompare(style.iconSize + 1.0, defaultStyle.iconSize + 1.0) == false)
            styleJson[ QStringLiteral("iconSize") ] = style.iconSize;
        if (style.inputPortSide != defaultStyle.inputPortSide)
            styleJson[ QStringLiteral("inputPortSide") ] = enumToString(style.inputPortSide);
        if (style.outputPortSide != defaultStyle.outputPortSide)
            styleJson[ QStringLiteral("outputPortSide") ] = enumToString(style.outputPortSide);
        // 输入端口样式
        {
            const DAPyLinkPointStyle portDefaults;
            QJsonObject portJson;
            if (style.inputPortStyle.shape != portDefaults.shape)
                portJson[ QStringLiteral("shape") ] = enumToString(style.inputPortStyle.shape);
            if (style.inputPortStyle.isFillColorValid() && style.inputPortStyle.fillColor != portDefaults.fillColor)
                portJson[ QStringLiteral("fillColor") ] = style.inputPortStyle.fillColor.name();
            if (style.inputPortStyle.isBorderColorValid() && style.inputPortStyle.borderColor != portDefaults.borderColor)
                portJson[ QStringLiteral("borderColor") ] = style.inputPortStyle.borderColor.name();
            if (qFuzzyCompare(style.inputPortStyle.borderWidth + 1.0, portDefaults.borderWidth + 1.0) == false)
                portJson[ QStringLiteral("borderWidth") ] = style.inputPortStyle.borderWidth;
            if (!portJson.isEmpty())
                styleJson[ QStringLiteral("inputPortStyle") ] = portJson;
        }
        // 输出端口样式
        {
            const DAPyLinkPointStyle portDefaults;
            QJsonObject portJson;
            if (style.outputPortStyle.shape != portDefaults.shape)
                portJson[ QStringLiteral("shape") ] = enumToString(style.outputPortStyle.shape);
            if (style.outputPortStyle.isFillColorValid() && style.outputPortStyle.fillColor != portDefaults.fillColor)
                portJson[ QStringLiteral("fillColor") ] = style.outputPortStyle.fillColor.name();
            if (style.outputPortStyle.isBorderColorValid() && style.outputPortStyle.borderColor != portDefaults.borderColor)
                portJson[ QStringLiteral("borderColor") ] = style.outputPortStyle.borderColor.name();
            if (qFuzzyCompare(style.outputPortStyle.borderWidth + 1.0, portDefaults.borderWidth + 1.0) == false)
                portJson[ QStringLiteral("borderWidth") ] = style.outputPortStyle.borderWidth;
            if (!portJson.isEmpty())
                styleJson[ QStringLiteral("outputPortStyle") ] = portJson;
        }
        if (style.layoutStrategy != defaultStyle.layoutStrategy)
            styleJson[ QStringLiteral("layoutStrategy") ] = enumToString(style.layoutStrategy);
        if (style.bodyIconType != defaultStyle.bodyIconType)
            styleJson[ QStringLiteral("bodyIconType") ] = enumToString(style.bodyIconType);
        if (style.bodyIconSource != defaultStyle.bodyIconSource)
            styleJson[ QStringLiteral("bodyIconSource") ] = style.bodyIconSource;
        if (qFuzzyCompare(style.bodyIconScale + 1.0, defaultStyle.bodyIconScale + 1.0) == false)
            styleJson[ QStringLiteral("bodyIconScale") ] = style.bodyIconScale;
        if (!styleJson.isEmpty()) {
            obj[ QStringLiteral("style") ] = QJsonValue(styleJson);
        }
    }

    // inputs
    QJsonArray inputsArray;
    for (const auto& port : inputs) {
        QJsonObject portObj;
        portObj[ QStringLiteral("name") ]      = port.name;
        portObj[ QStringLiteral("data_type") ] = port.dataType;
        if (!port.required)
            portObj[ QStringLiteral("required") ] = port.required;
        if (!port.description.isEmpty())
            portObj[ QStringLiteral("description") ] = port.description;
        inputsArray.append(portObj);
    }
    obj[ QStringLiteral("inputs") ] = QJsonValue(inputsArray);

    // outputs
    QJsonArray outputsArray;
    for (const auto& port : outputs) {
        QJsonObject portObj;
        portObj[ QStringLiteral("name") ]      = port.name;
        portObj[ QStringLiteral("data_type") ] = port.dataType;
        if (!port.required)
            portObj[ QStringLiteral("required") ] = port.required;
        if (!port.description.isEmpty())
            portObj[ QStringLiteral("description") ] = port.description;
        outputsArray.append(portObj);
    }
    obj[ QStringLiteral("outputs") ] = QJsonValue(outputsArray);

    // parameters（跳过 propertys/propertyId，由后续任务处理）
    QJsonArray paramsArray;
    for (const auto& param : parameters) {
        QJsonObject paramObj;
        paramObj[ QStringLiteral("name") ]        = param.name;
        paramObj[ QStringLiteral("type") ]        = param.type;
        paramObj[ QStringLiteral("description") ] = param.description;
        if (param.hasDefaultValue()) {
            paramObj[ QStringLiteral("defaultValue") ] = QJsonValue::fromVariant(param.defaultValue);
        }
        paramsArray.append(QJsonValue(paramObj));
    }
    obj[ QStringLiteral("parameters") ] = QJsonValue(paramsArray);

    return obj;
}

/**
 * @brief 从 QJsonObject 反序列化
 *
 * 缺失字段使用默认值。
 * 嵌套 style 通过内联字段反序列化处理。
 * 嵌套 port 通过直接字段赋值反序列化。
 * 嵌套参数跳过 propertys/propertyId。
 */
DANodeDescriptor DANodeDescriptor::fromJson(const QJsonObject& obj)
{
    DANodeDescriptor desc;

    desc.name          = obj.value(QStringLiteral("name")).toString();
    desc.qualifiedName = obj.value(QStringLiteral("qualifiedName")).toString();
    desc.category      = obj.value(QStringLiteral("category")).toString();
    desc.icon          = obj.value(QStringLiteral("icon")).toString();

    if (obj.contains(QStringLiteral("renderTemplate"))) {
        desc.renderTemplate = static_cast< RenderTemplate >(obj.value(QStringLiteral("renderTemplate")).toInt());
    }

    // style
    if (obj.contains(QStringLiteral("style"))) {
        const QJsonObject styleObj = obj.value(QStringLiteral("style")).toObject();
        DANodeStyle s;
        if (styleObj.contains(QStringLiteral("bodyShape")))
            s.bodyShape = stringToEnum< BodyShape >(styleObj.value(QStringLiteral("bodyShape")).toString(),
                                                    BodyShape::RoundedRect);
        if (styleObj.contains(QStringLiteral("namePosition")))
            s.namePosition = stringToEnum< NamePosition >(styleObj.value(QStringLiteral("namePosition")).toString(),
                                                          NamePosition::Inside);
        if (styleObj.contains(QStringLiteral("iconPosition")))
            s.iconPosition = stringToEnum< IconPosition >(styleObj.value(QStringLiteral("iconPosition")).toString(),
                                                          IconPosition::LeftOfText);
        if (styleObj.contains(QStringLiteral("backgroundColor"))) {
            const QString colorStr = styleObj.value(QStringLiteral("backgroundColor")).toString();
            if (!colorStr.isEmpty())
                s.backgroundColor = QColor(colorStr);
        }
        if (styleObj.contains(QStringLiteral("borderColor"))) {
            const QString colorStr = styleObj.value(QStringLiteral("borderColor")).toString();
            if (!colorStr.isEmpty())
                s.borderColor = QColor(colorStr);
        }
        if (styleObj.contains(QStringLiteral("borderWidth")))
            s.borderWidth = styleObj.value(QStringLiteral("borderWidth")).toDouble(1.0);
        if (styleObj.contains(QStringLiteral("cornerRadius")))
            s.cornerRadius = styleObj.value(QStringLiteral("cornerRadius")).toDouble(4.0);
        if (styleObj.contains(QStringLiteral("iconSize")))
            s.iconSize = styleObj.value(QStringLiteral("iconSize")).toDouble(24.0);
        if (styleObj.contains(QStringLiteral("inputPortSide")))
            s.inputPortSide =
                stringToEnum< PortSide >(styleObj.value(QStringLiteral("inputPortSide")).toString(), PortSide::West);
        if (styleObj.contains(QStringLiteral("outputPortSide")))
            s.outputPortSide =
                stringToEnum< PortSide >(styleObj.value(QStringLiteral("outputPortSide")).toString(), PortSide::East);
        if (styleObj.contains(QStringLiteral("inputPortStyle"))) {
            const QJsonObject portObj = styleObj.value(QStringLiteral("inputPortStyle")).toObject();
            DAPyLinkPointStyle ps;
            if (portObj.contains(QStringLiteral("shape")))
                ps.shape = stringToEnum< PortShape >(portObj.value(QStringLiteral("shape")).toString(), PortShape::Rect);
            if (portObj.contains(QStringLiteral("fillColor"))) {
                const QString cs = portObj.value(QStringLiteral("fillColor")).toString();
                if (!cs.isEmpty())
                    ps.fillColor = QColor(cs);
            }
            if (portObj.contains(QStringLiteral("borderColor"))) {
                const QString cs = portObj.value(QStringLiteral("borderColor")).toString();
                if (!cs.isEmpty())
                    ps.borderColor = QColor(cs);
            }
            if (portObj.contains(QStringLiteral("borderWidth")))
                ps.borderWidth = portObj.value(QStringLiteral("borderWidth")).toDouble(1.0);
            s.inputPortStyle = ps;
        }
        if (styleObj.contains(QStringLiteral("outputPortStyle"))) {
            const QJsonObject portObj = styleObj.value(QStringLiteral("outputPortStyle")).toObject();
            DAPyLinkPointStyle ps;
            if (portObj.contains(QStringLiteral("shape")))
                ps.shape = stringToEnum< PortShape >(portObj.value(QStringLiteral("shape")).toString(), PortShape::Rect);
            if (portObj.contains(QStringLiteral("fillColor"))) {
                const QString cs = portObj.value(QStringLiteral("fillColor")).toString();
                if (!cs.isEmpty())
                    ps.fillColor = QColor(cs);
            }
            if (portObj.contains(QStringLiteral("borderColor"))) {
                const QString cs = portObj.value(QStringLiteral("borderColor")).toString();
                if (!cs.isEmpty())
                    ps.borderColor = QColor(cs);
            }
            if (portObj.contains(QStringLiteral("borderWidth")))
                ps.borderWidth = portObj.value(QStringLiteral("borderWidth")).toDouble(1.0);
            s.outputPortStyle = ps;
        }
        if (styleObj.contains(QStringLiteral("layoutStrategy")))
            s.layoutStrategy = stringToEnum< LinkPointLayoutStrategy >(
                styleObj.value(QStringLiteral("layoutStrategy")).toString(), LinkPointLayoutStrategy::Auto);
        if (styleObj.contains(QStringLiteral("bodyIconType")))
            s.bodyIconType = stringToEnum< BodyIconType >(styleObj.value(QStringLiteral("bodyIconType")).toString(),
                                                          BodyIconType::None);
        if (styleObj.contains(QStringLiteral("bodyIconSource")))
            s.bodyIconSource = styleObj.value(QStringLiteral("bodyIconSource")).toString();
        if (styleObj.contains(QStringLiteral("bodyIconScale")))
            s.bodyIconScale = styleObj.value(QStringLiteral("bodyIconScale")).toDouble(0.8);
        desc.style = s;
    }

    // inputs
    if (obj.contains(QStringLiteral("inputs"))) {
        const QJsonArray inputsArray = obj.value(QStringLiteral("inputs")).toArray();
        for (const auto& val : inputsArray) {
            const QJsonObject portObj = val.toObject();
            DAPortDescriptor port;
            port.name     = portObj.value(QStringLiteral("name")).toString();
            port.dataType = portObj.value(QStringLiteral("data_type")).toString();
            if (portObj.contains(QStringLiteral("required")))
                port.required = portObj.value(QStringLiteral("required")).toBool();
            port.description = portObj.value(QStringLiteral("description")).toString();
            desc.inputs.append(port);
        }
    }

    // outputs
    if (obj.contains(QStringLiteral("outputs"))) {
        const QJsonArray outputsArray = obj.value(QStringLiteral("outputs")).toArray();
        for (const auto& val : outputsArray) {
            const QJsonObject portObj = val.toObject();
            DAPortDescriptor port;
            port.name     = portObj.value(QStringLiteral("name")).toString();
            port.dataType = portObj.value(QStringLiteral("data_type")).toString();
            if (portObj.contains(QStringLiteral("required")))
                port.required = portObj.value(QStringLiteral("required")).toBool();
            port.description = portObj.value(QStringLiteral("description")).toString();
            desc.outputs.append(port);
        }
    }

    // parameters（跳过 propertys/propertyId，由后续任务处理）
    if (obj.contains(QStringLiteral("parameters"))) {
        const QJsonArray paramsArray = obj.value(QStringLiteral("parameters")).toArray();
        for (const auto& val : paramsArray) {
            const QJsonObject paramObj = val.toObject();
            DAParameterDescriptor param;
            param.name        = paramObj.value(QStringLiteral("name")).toString();
            param.type        = paramObj.value(QStringLiteral("type")).toString();
            param.description = paramObj.value(QStringLiteral("description")).toString();
            if (paramObj.contains(QStringLiteral("defaultValue"))) {
                param.defaultValue = paramObj.value(QStringLiteral("defaultValue")).toVariant();
            }
            desc.parameters.append(param);
        }
    }

    return desc;
}

}  // namespace DA
