#ifndef DACONDITIONALFORMATPRESETS_H
#define DACONDITIONALFORMATPRESETS_H
#include "DAGuiAPI.h"
#include <QColor>
#include <QIcon>
#include <QList>

namespace DA
{

/**
 * @brief 色阶预设（2/3 色，低→高）
 */
struct DAGUI_API DACFColorScalePreset
{
    QList< QColor > colors;  ///< 2 或 3 个颜色（低→高，三色中间为中点色）
};

/**
 * @brief 数据条预设（颜色 + 实心/渐变）
 */
struct DAGUI_API DACFDataBarPreset
{
    QColor color;          ///< 条填充色
    bool gradient { false };  ///< 渐变填充（false=实心）
};

/**
 * @brief 突出显示单元格样式预设（单元格规则输出样式）
 *
 * 颜色无效表示不设置该属性（如"仅红文本"预设 background 无效）。
 */
struct DAGUI_API DACFHighlightStyle
{
    QColor background;  ///< 背景色（无效=不设背景）
    QColor foreground;  ///< 前景色（无效=不设前景）
    bool hasBackground() const;
    bool hasForeground() const;
};

/**
 * @brief 条件格式预设库（WPS/Excel 式一键预设）
 *
 * 预设色全部取自官方色板（DESIGN.md §2.2）：绿 #669E8B / 金黄 #E6C27C /
 * 橙红 #CE6043 / 主蓝 #5280C1 / 浅蓝 #BDCDE8 / 深蓝 #497CAD / 深蓝灰 #5D6D7E / 白。
 * 缩略图为代码绘制的菜单图标（数据预览性质，不引入 SVG 图标资产）。
 */
class DAGUI_API DAConditionalFormatPresets
{
public:
    // 色阶预设 12 项（4 三色 + 8 双色，顺序即菜单顺序）
    static QList< DACFColorScalePreset > colorScalePresets();
    // 数据条预设 12 项（渐变 6 + 实心 6，顺序即菜单顺序）
    static QList< DACFDataBarPreset > dataBarPresets();
    // 突出显示样式预设 5 项（"自定义..."由调用方另行提供）
    static QList< DACFHighlightStyle > highlightStyles();

    // 色阶缩略图（4 行色带按渐变取色）
    static QIcon colorScaleThumb(const QList< QColor >& colors);
    // 数据条缩略图（4 行递减条长）
    static QIcon dataBarThumb(const QColor& color, bool gradient);
    // 突出显示样式缩略图（4 行单元格：背景 + 文本条）
    static QIcon highlightThumb(const DACFHighlightStyle& style);
};
}  // end of namespace DA
#endif  // DACONDITIONALFORMATPRESETS_H
