#include "DANodePalette.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================
namespace DA
{
DANodePalette::DANodePalette()
{
    mColors                          //颜色初始化
            << QColor(0, 0, 0)        // RoleText
            << QColor(128, 128, 147)  // RoleBorderColor
            << QColor(128, 128, 147)  // RoleLinkLine
            << QColor(128, 128, 147)  // RoleLinkPointBorderColor
            ;
    mBrushs << QBrush(QColor(255, 255, 255))  // RoleBackgroundBrush
             << QBrush(QColor(207, 69, 74))    // RoleInLinkPointBrush
             << QBrush(QColor(23, 115, 51))    // RoleOutLinkPointBrush
            ;
}

/**
 * @brief 全局色板
 * @return
 */
DANodePalette& DANodePalette::getGlobalPalette()
{
    static DANodePalette globalNodePalette;
    return (globalNodePalette);
}

QColor& DANodePalette::color(ColorRole r)
{
    return (mColors[ int(r) ]);
}

const QColor& DANodePalette::getColor(DANodePalette::ColorRole r) const
{
    return (mColors[ int(r) ]);
}

const QColor& DANodePalette::getGlobalColor(DANodePalette::ColorRole r)
{
    return (getGlobalPalette().getColor(r));
}

QBrush& DANodePalette::brush(BrushRole r)
{
    return (mBrushs[ int(r) ]);
}

const QBrush& DANodePalette::getBrush(BrushRole r) const
{
    return (mBrushs[ int(r) ]);
}
/**
 * @brief DANodePalette::getGlobalBrush
 * @param r
 * @return
 */
const QBrush& DANodePalette::getGlobalBrush(BrushRole r)
{
    return (getGlobalPalette().getBrush(r));
}

}  // end DA
