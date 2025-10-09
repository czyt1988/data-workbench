#ifndef DANODEPALETTE_H
#define DANODEPALETTE_H
#include <QtCore/qglobal.h>
#include <QColor>
#include <QBrush>
#include "DAWorkFlowAPI.h"

#ifndef DANODEPALETTE_PROPERTY_COLOR
#define DANODEPALETTE_PROPERTY_COLOR(RoleName, Role)                                                                   \
    const QColor& get##RoleName() const                                                                                \
    {                                                                                                                  \
        return getColor(Role);                                                                                         \
    }                                                                                                                  \
    static const QColor& getGlobal##RoleName()                                                                         \
    {                                                                                                                  \
        return getGlobalPalette().get##RoleName();                                                                     \
    }
#endif
#ifndef DANODEPALETTE_PROPERTY_BRUSH
#define DANODEPALETTE_PROPERTY_BRUSH(RoleName, Role)                                                                   \
    const QBrush& get##RoleName() const                                                                                \
    {                                                                                                                  \
        return getBrush(Role);                                                                                         \
    }                                                                                                                  \
    static const QBrush& getGlobal##RoleName()                                                                         \
    {                                                                                                                  \
        return getGlobalPalette().get##RoleName();                                                                     \
    }
#endif
namespace DA
{
/**
 * @brief 节点相关的调色板，这里封装了节点相关的大部分颜色，
 *
 * 尽量不要在节点绘制使用其他颜色，统一通过此颜色可以实现主题颜色的更换
 *
 * 参考office调色板的样式，每个调色板一共有7种颜色
 */
class DAWORKFLOW_API DANodePalette
{
public:
    /**
     * @brief 颜色角色
     */
    enum ColorRole
    {
        RoleTextColor            = 0,  ///< 文本颜色
        RoleBorderColor          = 1,  ///< 边框颜色
        RoleLinkLineColor        = 2,  ///< 连接线的颜色
        RoleLinkPointBorderColor = 3   ///< 连接点的边框颜色
    };
    /**
     * @brief 颜色角色
     */
    enum BrushRole
    {
        RoleBackgroundBrush = 0,  ///< 背景颜色
        RoleInLinkPointBrush,     ///< 入口连接点的背景
        RoleOutLinkPointBrush     ///< 出口连接点的背景
    };
    DANodePalette();
    static DANodePalette& getGlobalPalette();
    // 颜色相关操作
    QColor& color(ColorRole r);
    const QColor& getColor(ColorRole r) const;
    static const QColor& getGlobalColor(ColorRole r);
    // 画刷相关操作
    QBrush& brush(BrushRole r);
    const QBrush& getBrush(BrushRole r) const;
    static const QBrush& getGlobalBrush(BrushRole r);
    DANODEPALETTE_PROPERTY_COLOR(TextColor, RoleTextColor)
    DANODEPALETTE_PROPERTY_COLOR(BorderColor, RoleBorderColor)
    DANODEPALETTE_PROPERTY_COLOR(LinkLineColor, RoleLinkLineColor)
    DANODEPALETTE_PROPERTY_COLOR(LinkPointBorderColor, RoleLinkPointBorderColor)

    // brush
    DANODEPALETTE_PROPERTY_BRUSH(BackgroundBrush, RoleBackgroundBrush)
    DANODEPALETTE_PROPERTY_BRUSH(InLinkPointBrush, RoleInLinkPointBrush)
    DANODEPALETTE_PROPERTY_BRUSH(OutLinkPointBrush, RoleOutLinkPointBrush)

private:
    QList< QColor > mColors;  ///< 存放所有颜色
    QList< QBrush > mBrushs;  ///< 存放所有填充颜色
};
}  // end of namespace DA

#endif  // FCNODEPALETTE_H
