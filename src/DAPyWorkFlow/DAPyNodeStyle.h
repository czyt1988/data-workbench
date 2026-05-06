#ifndef DAPYNODESTYLE_H
#define DAPYNODESTYLE_H

#include "DAPyWorkFlowAPI.h"
#include "DAPyNodeStyleDefine.h"
#include <QColor>
#include <QJsonObject>
#include <QString>
#include "DAEnumStringUtils.hpp"

/**
 * @file DAPyNodeStyle.h
 * @brief Python工作流节点样式配置结构体
 *
 * 本文件定义连接点样式配置结构体及布局策略枚举，
 * 用于控制端口的视觉表现和布局行为。
 */

namespace DA
{

// =================================================================================
//                      LinkPointLayoutStrategy — 连接点布局策略
// =================================================================================
/**
 * @brief 连接点布局策略枚举
 */
enum class LinkPointLayoutStrategy
{
    Auto   = 0,  ///< 自动布局（系统计算位置）
    Manual = 1   ///< 手动布局（用户指定位置）
};

DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DA::LinkPointLayoutStrategy)

// =================================================================================
//                      DAPyLinkPointStyle — 连接点样式配置
// =================================================================================
/**
 * @brief 连接点样式配置结构体
 *
 * 用于自定义端口（连接点）的视觉样式。
 * 当 fillColor 或 borderColor 为无效颜色（default constructed QColor）时，
 * 将使用默认值：输入端口为白色(Qt::white)，输出端口为深灰色(Qt::darkGray)。
 *
 * 使用示例：
 * @code
 * DAPyLinkPointStyle style;
 * style.shape = PortShape::Circle;
 * style.fillColor = QColor(Qt::red);
 * style.borderWidth = 2.0;
 * @endcode
 *
 * @see DAPyNodeStyleDefine.h 中的 PortShape 枚举
 */
struct DAPYWORKFLOW_API DAPyLinkPointStyle
{
    // 构造函数
    /**
     * @brief 默认构造函数
     *
     * 初始化为默认值：
     * - shape: PortShape::Rect
     * - fillColor: 无效颜色（使用默认值）
     * - borderColor: 无效颜色（使用默认值）
     * - borderWidth: 1.0
     */
    DAPyLinkPointStyle() : shape(PortShape::Rect), borderWidth(1.0)
    {
    }

    // 公共字段
    PortShape shape;     ///< 端口形状
    QColor fillColor;    ///< 填充颜色（无效时使用默认值：输入=白色，输出=深灰色）
    QColor borderColor;  ///< 边框颜色（无效时使用默认值：黑色）
    qreal borderWidth;   ///< 边框宽度（默认 1.0）

    // 辅助方法
    /**
     * @brief 判断填充颜色是否有效
     * @return true 表示 fillColor 有效，false 表示应使用默认值
     */
    bool isFillColorValid() const
    {
        return fillColor.isValid();
    }

    /**
     * @brief 判断边框颜色是否有效
     * @return true 表示 borderColor 有效，false 表示应使用默认值
     */
    bool isBorderColorValid() const
    {
        return borderColor.isValid();
    }
};

// =================================================================================
//                      DANodeStyle — 节点整体样式配置
// =================================================================================
/**
 * @brief 节点整体样式配置结构体
 *
 * 用于自定义节点的视觉表现，包括主体形状、颜色、边框、圆角、图标、端口配置及布局策略。
 * 所有默认值与 DAPyNodeGraphicsItem::paintRectTemplate() 中的硬编码值保持一致。
 *
 * 使用示例：
 * @code
 * DANodeStyle style;
 * style.bodyShape = BodyShape::Ellipse;
 * style.backgroundColor = QColor(Qt::lightGray);
 * style.cornerRadius = 8.0;
 * style.setDefaults();  // 重置为默认值
 * @endcode
 *
 * @see DAPyNodeStyleDefine.h 中的 BodyShape、NamePosition、IconPosition 等枚举
 * @see DAPyLinkPointStyle 端口样式配置
 */
struct DAPYWORKFLOW_API DANodeStyle
{
    // 构造函数
    /**
     * @brief 默认构造函数
     *
     * 调用 setDefaults() 将所有字段初始化为默认值。
     */
    DANodeStyle()
    {
        setDefaults();
    }

    // 公共字段 — 主体样式
    BodyShape bodyShape;        ///< 节点体形状（默认 RoundedRect）
    NamePosition namePosition;  ///< 节点名称位置（默认 Inside）
    IconPosition iconPosition;  ///< 图标位置（默认 LeftOfText）
    QColor backgroundColor;     ///< 背景颜色（默认 QColor(240, 240, 240)）
    QColor borderColor;         ///< 边框颜色（默认 QColor(180, 180, 180)）
    qreal borderWidth;          ///< 边框宽度（默认 1.0）
    qreal cornerRadius;         ///< 圆角半径（默认 4.0）
    qreal iconSize;             ///< 图标尺寸（默认 24.0）

    // 公共字段 — 端口配置
    PortSide inputPortSide;                  ///< 输入端口方位（默认 West）
    PortSide outputPortSide;                 ///< 输出端口方位（默认 East）
    DAPyLinkPointStyle inputPortStyle;       ///< 输入端口样式（默认构造）
    DAPyLinkPointStyle outputPortStyle;      ///< 输出端口样式（默认构造）
    LinkPointLayoutStrategy layoutStrategy;  ///< 连接点布局策略（默认 Auto）

    // 公共字段 — 节点体图标（SVG支持）
    BodyIconType bodyIconType;  ///< 节点体图标类型（默认 None）
    QString bodyIconSource;     ///< 图标源路径（SVG文件路径或资源路径，默认空）
    qreal bodyIconScale;        ///< 图标缩放比例（默认 0.8，相对于bodyRect）

    // 辅助方法
    /**
     * @brief 重置所有字段为默认值
     *
     * 默认值与 DAPyNodeGraphicsItem::paintRectTemplate() 中的硬编码值一致：
     * - 主体: RoundedRect, 名称Inside, 图标LeftOfText
     * - 颜色: 背景(240,240,240), 边框(180,180,180), 边框宽1.0, 圆角4.0
     * - 图标: 尺寸24.0, 类型None, 缩放0.8
     * - 端口: 输入West, 输出East, 样式默认构造, 布局Auto
     */
    void setDefaults()
    {
        // 主体样式
        bodyShape       = BodyShape::RoundedRect;
        namePosition    = NamePosition::Inside;
        iconPosition    = IconPosition::LeftOfText;
        backgroundColor = QColor(240, 240, 240);
        borderColor     = QColor(180, 180, 180);
        borderWidth     = 1.0;
        cornerRadius    = 4.0;
        iconSize        = 24.0;

        // 端口配置
        inputPortSide   = PortSide::West;
        outputPortSide  = PortSide::East;
        inputPortStyle  = DAPyLinkPointStyle();
        outputPortStyle = DAPyLinkPointStyle();
        layoutStrategy  = LinkPointLayoutStrategy::Auto;

        // 节点体图标
        bodyIconType = BodyIconType::None;
        bodyIconSource.clear();
        bodyIconScale = 0.8;
    }
    // 辅助函数
    inline bool isNameInside() const
    {
        return namePosition == NamePosition::Inside;
    }
    inline bool isIconLeftOfText() const
    {
        return iconPosition == IconPosition::LeftOfText;
    }
};

// =================================================================================
//                      JSON 序列化函数
// =================================================================================
/**
 * @brief 从 JSON 对象解析节点样式
 * @param[in] json JSON 对象
 * @return 解析后的 DANodeStyle
 * @note 缺失字段使用默认值，无效字符串使用默认枚举值
 */
DAPYWORKFLOW_API DANodeStyle DANodeStyleFromJson(const QJsonObject& json);

/**
 * @brief 将节点样式序列化为 JSON 对象（稀疏策略）
 * @param[in] style 样式对象
 * @return JSON 对象，仅包含与默认值不同的字段
 */
DAPYWORKFLOW_API QJsonObject DANodeStyleToJson(const DANodeStyle& style);

}  // namespace DA

#endif  // DAPYNODESTYLE_H
