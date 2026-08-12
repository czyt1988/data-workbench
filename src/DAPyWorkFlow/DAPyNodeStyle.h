#ifndef DAPYNODESTYLE_H
#define DAPYNODESTYLE_H

#include "DAPyWorkFlowAPI.h"
#include "DAPyLinkPointStyle.h"
#include <QColor>
#include <QString>
#include "DAEnumStringUtils.hpp"
#include "DAGraphicsViewGlobal.h"
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
//                      DAPyNodeDisplayStyle — 节点整体样式配置
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
class DAPYWORKFLOW_API DAPyNodeStyle
{
public:
    /**
     * @brief 连接点布局策略枚举
     */
    enum LinkPointLayoutStrategy
    {
        AutoLayoutLinkPoint   = 0,  ///< 自动布局（系统计算位置）
        ManualLayoutLinkPoint = 1   ///< 手动布局（用户指定位置）
    };

    enum NodeRenderTemplate
    {
        RenderDefaultTemplate = 0,  ///< 节点样式模板（使用 BodyShape/PortShape 等配置绘制）
        RenderWidgetTemplate  = 1   ///< 嵌入Widget模板
    };

    enum BodyShape
    {
        RoundedRectShape = 0,  ///< 圆角矩形
        EllipseShape     = 1,  ///< 椭圆形
        DiamondShape     = 2   ///< 菱形
    };

    /**
     * @brief 节点体图标类型枚举
     */
    enum BodyIconType
    {
        NoneBodyIcon   = 0,  ///< 无图标
        PixmapBodyIcon = 1,  ///< 位图图标（QIcon/QPixmap）
        SvgBodyIcon    = 2   ///< SVG矢量图标
    };

    /**
     * @brief 节点名称位置枚举
     */
    enum NamePosition
    {
        NameInsideBody = 0,  ///< 名称在节点内部
        NameBelowBody  = 1   ///< 名称在节点下方
    };
    /**
     * @brief 图标位置枚举
     */
    enum IconPosition
    {
        IconLeftOfText = 0,  ///< 图标在文本左侧
        IconAboveText  = 1   ///< 图标在文本上方
    };

    using PortSide = DAAspectDirection;

public:
    // 默认构造函数，调用 setDefaults() 初始化所有字段
    DAPyNodeStyle()
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

    // 渲染模板
    NodeRenderTemplate renderTemplate;  ///< 渲染模板
    // 重置所有字段为默认值（与 paintRectTemplate() 硬编码值一致）
    void setDefaults()
    {
        // 主体样式
        bodyShape       = BodyShape::RoundedRectShape;
        namePosition    = NamePosition::NameInsideBody;
        iconPosition    = IconPosition::IconLeftOfText;
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
        layoutStrategy  = LinkPointLayoutStrategy::AutoLayoutLinkPoint;

        // 节点体图标
        bodyIconType = BodyIconType::NoneBodyIcon;
        bodyIconSource.clear();
        bodyIconScale = 0.8;

        // 渲染模板
        renderTemplate = RenderDefaultTemplate;
    }
    // 辅助函数
    inline bool isNameInside() const
    {
        return namePosition == NamePosition::NameInsideBody;
    }
    inline bool isIconLeftOfText() const
    {
        return iconPosition == IconPosition::IconLeftOfText;
    }
};
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyNodeStyle::LinkPointLayoutStrategy)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyNodeStyle::NodeRenderTemplate)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyNodeStyle::BodyShape)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyNodeStyle::NamePosition)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyNodeStyle::IconPosition)
DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyNodeStyle::BodyIconType)
}  // namespace DA

#endif  // DAPYNODESTYLE_H
