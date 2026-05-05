#include "DAPyNodeStyle.h"
#include "DAPyWorkFlowEnumStringUtils.h"
#include <QJsonObject>
#include <QJsonArray>

namespace DA
{

// =================================================================================
//                      DAPyLinkPointStyle JSON 序列化
// =================================================================================

/**
 * @brief 从 JSON 对象解析连接点样式
 * @param[in] json JSON 对象
 * @return 解析后的 DAPyLinkPointStyle
 * @note 缺失字段使用默认值，无效字符串使用默认值
 */
static DAPyLinkPointStyle linkPointStyleFromJson(const QJsonObject& json)
{
    DAPyLinkPointStyle style;

    // shape
    if (json.contains(QStringLiteral("shape"))) {
        style.shape = stringToEnum<PortShape>(json.value(QStringLiteral("shape")).toString(), PortShape::Rect);
    }

    // fillColor
    if (json.contains(QStringLiteral("fillColor"))) {
        const QString colorStr = json.value(QStringLiteral("fillColor")).toString();
        if (!colorStr.isEmpty()) {
            style.fillColor = QColor(colorStr);
        }
    }

    // borderColor
    if (json.contains(QStringLiteral("borderColor"))) {
        const QString colorStr = json.value(QStringLiteral("borderColor")).toString();
        if (!colorStr.isEmpty()) {
            style.borderColor = QColor(colorStr);
        }
    }

    // borderWidth
    if (json.contains(QStringLiteral("borderWidth"))) {
        style.borderWidth = json.value(QStringLiteral("borderWidth")).toDouble(1.0);
    }

    return style;
}

/**
 * @brief 将连接点样式序列化为 JSON 对象（稀疏策略）
 * @param[in] style 样式对象
 * @return JSON 对象
 */
static QJsonObject linkPointStyleToJson(const DAPyLinkPointStyle& style)
{
    QJsonObject json;
    static const DAPyLinkPointStyle defaults;

    // shape
    if (style.shape != defaults.shape) {
        json.insert(QStringLiteral("shape"), enumToString(style.shape));
    }

    // fillColor
    if (style.isFillColorValid() && style.fillColor != defaults.fillColor) {
        json.insert(QStringLiteral("fillColor"), style.fillColor.name());
    }

    // borderColor
    if (style.isBorderColorValid() && style.borderColor != defaults.borderColor) {
        json.insert(QStringLiteral("borderColor"), style.borderColor.name());
    }

    // borderWidth
    if (qFuzzyCompare(style.borderWidth + 1.0, defaults.borderWidth + 1.0) == false) {
        json.insert(QStringLiteral("borderWidth"), style.borderWidth);
    }

    return json;
}

// =================================================================================
//                      DANodeStyle JSON 序列化
// =================================================================================

/**
 * @brief 从 JSON 对象解析节点样式
 * @param[in] json JSON 对象
 * @return 解析后的 DANodeStyle
 * @note 缺失字段使用默认值，无效字符串使用默认枚举值
 */
DANodeStyle DANodeStyleFromJson(const QJsonObject& json)
{
    DANodeStyle style;  // 构造函数已调用 setDefaults()

    // 主体样式
    if (json.contains(QStringLiteral("bodyShape"))) {
        style.bodyShape = stringToEnum<BodyShape>(json.value(QStringLiteral("bodyShape")).toString(),
                                                   BodyShape::RoundedRect);
    }

    if (json.contains(QStringLiteral("namePosition"))) {
        style.namePosition = stringToEnum<NamePosition>(json.value(QStringLiteral("namePosition")).toString(),
                                                         NamePosition::Inside);
    }

    if (json.contains(QStringLiteral("iconPosition"))) {
        style.iconPosition = stringToEnum<IconPosition>(json.value(QStringLiteral("iconPosition")).toString(),
                                                         IconPosition::LeftOfText);
    }

    if (json.contains(QStringLiteral("backgroundColor"))) {
        const QString colorStr = json.value(QStringLiteral("backgroundColor")).toString();
        if (!colorStr.isEmpty()) {
            style.backgroundColor = QColor(colorStr);
        }
    }

    if (json.contains(QStringLiteral("borderColor"))) {
        const QString colorStr = json.value(QStringLiteral("borderColor")).toString();
        if (!colorStr.isEmpty()) {
            style.borderColor = QColor(colorStr);
        }
    }

    if (json.contains(QStringLiteral("borderWidth"))) {
        style.borderWidth = json.value(QStringLiteral("borderWidth")).toDouble(1.0);
    }

    if (json.contains(QStringLiteral("cornerRadius"))) {
        style.cornerRadius = json.value(QStringLiteral("cornerRadius")).toDouble(4.0);
    }

    if (json.contains(QStringLiteral("iconSize"))) {
        style.iconSize = json.value(QStringLiteral("iconSize")).toDouble(24.0);
    }

    // 端口配置
    if (json.contains(QStringLiteral("inputPortSide"))) {
        style.inputPortSide = stringToEnum<PortSide>(json.value(QStringLiteral("inputPortSide")).toString(),
                                                      PortSide::West);
    }

    if (json.contains(QStringLiteral("outputPortSide"))) {
        style.outputPortSide = stringToEnum<PortSide>(json.value(QStringLiteral("outputPortSide")).toString(),
                                                       PortSide::East);
    }

    if (json.contains(QStringLiteral("inputPortStyle"))) {
        style.inputPortStyle = linkPointStyleFromJson(json.value(QStringLiteral("inputPortStyle")).toObject());
    }

    if (json.contains(QStringLiteral("outputPortStyle"))) {
        style.outputPortStyle = linkPointStyleFromJson(json.value(QStringLiteral("outputPortStyle")).toObject());
    }

    if (json.contains(QStringLiteral("layoutStrategy"))) {
        style.layoutStrategy = stringToEnum<LinkPointLayoutStrategy>(
            json.value(QStringLiteral("layoutStrategy")).toString(), LinkPointLayoutStrategy::Auto);
    }

    // 节点体图标
    if (json.contains(QStringLiteral("bodyIconType"))) {
        style.bodyIconType = stringToEnum<BodyIconType>(json.value(QStringLiteral("bodyIconType")).toString(),
                                                         BodyIconType::None);
    }

    if (json.contains(QStringLiteral("bodyIconSource"))) {
        style.bodyIconSource = json.value(QStringLiteral("bodyIconSource")).toString();
    }

    if (json.contains(QStringLiteral("bodyIconScale"))) {
        style.bodyIconScale = json.value(QStringLiteral("bodyIconScale")).toDouble(0.8);
    }

    return style;
}

/**
 * @brief 将节点样式序列化为 JSON 对象（稀疏策略）
 * @param[in] style 样式对象
 * @return JSON 对象，仅包含与默认值不同的字段
 */
QJsonObject DANodeStyleToJson(const DANodeStyle& style)
{
    QJsonObject json;
    const DANodeStyle defaults;  // 默认实例用于比较

    // 主体样式
    if (style.bodyShape != defaults.bodyShape) {
        json.insert(QStringLiteral("bodyShape"), enumToString(style.bodyShape));
    }

    if (style.namePosition != defaults.namePosition) {
        json.insert(QStringLiteral("namePosition"), enumToString(style.namePosition));
    }

    if (style.iconPosition != defaults.iconPosition) {
        json.insert(QStringLiteral("iconPosition"), enumToString(style.iconPosition));
    }

    if (style.backgroundColor != defaults.backgroundColor) {
        json.insert(QStringLiteral("backgroundColor"), style.backgroundColor.name());
    }

    if (style.borderColor != defaults.borderColor) {
        json.insert(QStringLiteral("borderColor"), style.borderColor.name());
    }

    if (qFuzzyCompare(style.borderWidth + 1.0, defaults.borderWidth + 1.0) == false) {
        json.insert(QStringLiteral("borderWidth"), style.borderWidth);
    }

    if (qFuzzyCompare(style.cornerRadius + 1.0, defaults.cornerRadius + 1.0) == false) {
        json.insert(QStringLiteral("cornerRadius"), style.cornerRadius);
    }

    if (qFuzzyCompare(style.iconSize + 1.0, defaults.iconSize + 1.0) == false) {
        json.insert(QStringLiteral("iconSize"), style.iconSize);
    }

    // 端口配置
    if (style.inputPortSide != defaults.inputPortSide) {
        json.insert(QStringLiteral("inputPortSide"), enumToString(style.inputPortSide));
    }

    if (style.outputPortSide != defaults.outputPortSide) {
        json.insert(QStringLiteral("outputPortSide"), enumToString(style.outputPortSide));
    }

    // 输入端口样式（仅当有非默认值时写入）
    {
        const QJsonObject inputPortJson = linkPointStyleToJson(style.inputPortStyle);
        if (!inputPortJson.isEmpty()) {
            json.insert(QStringLiteral("inputPortStyle"), inputPortJson);
        }
    }

    // 输出端口样式
    {
        const QJsonObject outputPortJson = linkPointStyleToJson(style.outputPortStyle);
        if (!outputPortJson.isEmpty()) {
            json.insert(QStringLiteral("outputPortStyle"), outputPortJson);
        }
    }

    if (style.layoutStrategy != defaults.layoutStrategy) {
        json.insert(QStringLiteral("layoutStrategy"), enumToString(style.layoutStrategy));
    }

    // 节点体图标
    if (style.bodyIconType != defaults.bodyIconType) {
        json.insert(QStringLiteral("bodyIconType"), enumToString(style.bodyIconType));
    }

    if (style.bodyIconSource != defaults.bodyIconSource) {
        json.insert(QStringLiteral("bodyIconSource"), style.bodyIconSource);
    }

    if (qFuzzyCompare(style.bodyIconScale + 1.0, defaults.bodyIconScale + 1.0) == false) {
        json.insert(QStringLiteral("bodyIconScale"), style.bodyIconScale);
    }

    return json;
}

}  // namespace DA
