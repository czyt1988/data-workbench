#include "DAConditionalFormatPresets.h"
#include "DATableConditionalFormatRule.h"
#include <QPainter>
#include <QPixmap>
#include <QLinearGradient>

namespace DA
{

namespace
{
// 缩略图画布（菜单图标，代码绘制）
constexpr int c_thumbW = 48;
constexpr int c_thumbH = 36;
// 官方色板（DESIGN.md §2.2）
const QColor c_green(0x66, 0x9E, 0x8B);    ///< 绿（成功）
const QColor c_yellow(0xE6, 0xC2, 0x7C);   ///< 金黄（标记）
const QColor c_red(0xCE, 0x60, 0x43);      ///< 橙红（危险）
const QColor c_blue(0x52, 0x80, 0xC1);     ///< 主蓝
const QColor c_lblue(0xBD, 0xCD, 0xE8);    ///< 浅蓝
const QColor c_dblue(0x49, 0x7C, 0xAD);    ///< 深蓝
const QColor c_slate(0x5D, 0x6D, 0x7E);    ///< 深蓝灰（文字）
const QColor c_cyan(0x3C, 0xAE, 0xD5);     ///< 青蓝（系列）
const QColor c_purple(0x80, 0x55, 0x9F);   ///< 紫（系列）
const QColor c_grid(0x72, 0x72, 0x72);     ///< 中灰（网格/描边）
const QColor c_white(0xFF, 0xFF, 0xFF);    ///< 白

// 第 i 行（0~3）的内区矩形
QRect thumbRowRect(int i)
{
    const int innerH = c_thumbH - 2;
    const int y0     = 1 + i * innerH / 4;
    const int y1     = 1 + (i + 1) * innerH / 4;
    return QRect(1, y0, c_thumbW - 2, y1 - y0);
}

// 底色上的文本条颜色（深底用白、浅底用深蓝灰，保证缩略图可读）
QColor textColorOn(const QColor& bg)
{
    return qGray(bg.rgb()) < 128 ? c_white : c_slate;
}

// 画外框与行分隔线
void drawThumbGrid(QPainter& p)
{
    p.setPen(c_grid);
    p.drawRect(0, 0, c_thumbW - 1, c_thumbH - 1);
    for (int i = 1; i < 4; ++i) {
        const int y = thumbRowRect(i).top();
        p.drawLine(1, y, c_thumbW - 2, y);
    }
}

// 行内文本条（模拟单元格文字）
void drawTextDash(QPainter& p, const QRect& row, const QColor& color, double widthRatio = 0.6)
{
    const int h = qMin(3, row.height() - 2);
    if (h <= 0) {
        return;
    }
    p.fillRect(QRect(row.left() + 3, row.center().y() - h / 2, qRound((row.width() - 6) * widthRatio), h), color);
}
}  // namespace

bool DACFHighlightStyle::hasBackground() const
{
    return background.isValid();
}

bool DACFHighlightStyle::hasForeground() const
{
    return foreground.isValid();
}

/**
 * @brief 色阶预设 12 项（4 三色 + 8 双色，顺序即菜单顺序）
 * @return 预设列表
 */
QList< DACFColorScalePreset > DAConditionalFormatPresets::colorScalePresets()
{
    return {
        // 三色
        DACFColorScalePreset { { c_green, c_yellow, c_red } },    ///< 绿-金黄-橙红
        DACFColorScalePreset { { c_red, c_yellow, c_green } },    ///< 橙红-金黄-绿
        DACFColorScalePreset { { c_green, c_white, c_red } },     ///< 绿-白-橙红
        DACFColorScalePreset { { c_red, c_white, c_green } },     ///< 橙红-白-绿
        // 双色
        DACFColorScalePreset { { c_white, c_blue } },             ///< 白-主蓝
        DACFColorScalePreset { { c_blue, c_white } },             ///< 主蓝-白
        DACFColorScalePreset { { c_white, c_green } },            ///< 白-绿
        DACFColorScalePreset { { c_green, c_white } },            ///< 绿-白
        DACFColorScalePreset { { c_white, c_red } },              ///< 白-橙红
        DACFColorScalePreset { { c_red, c_white } },              ///< 橙红-白
        DACFColorScalePreset { { c_lblue, c_dblue } },            ///< 浅蓝-深蓝
        DACFColorScalePreset { { c_yellow, c_red } },             ///< 金黄-橙红
    };
}

/**
 * @brief 数据条预设 12 项（渐变 6 + 实心 6，顺序即菜单顺序）
 * @return 预设列表
 */
QList< DACFDataBarPreset > DAConditionalFormatPresets::dataBarPresets()
{
    const QList< QColor > colors { c_blue, c_green, c_red, c_yellow, c_cyan, c_purple };
    QList< DACFDataBarPreset > res;
    for (const QColor& c : colors) {
        res.append(DACFDataBarPreset { c, true });  // 渐变填充组
    }
    for (const QColor& c : colors) {
        res.append(DACFDataBarPreset { c, false });  // 实心填充组
    }
    return res;
}

/**
 * @brief 突出显示样式预设 5 项（"自定义..."由调用方另行提供）
 * @return 预设列表
 */
QList< DACFHighlightStyle > DAConditionalFormatPresets::highlightStyles()
{
    return {
        DACFHighlightStyle { c_red, c_white },      ///< 橙红底白字
        DACFHighlightStyle { c_yellow, c_slate },   ///< 金黄底深蓝灰字
        DACFHighlightStyle { c_green, c_white },    ///< 绿底白字
        DACFHighlightStyle { c_lblue, c_slate },    ///< 浅蓝底深蓝灰字
        DACFHighlightStyle { QColor(), c_red },     ///< 仅橙红文本
    };
}

/**
 * @brief 色阶缩略图（4 行色带按渐变取色 + 文本条）
 * @param colors 2 或 3 个颜色
 * @return 菜单图标
 */
QIcon DAConditionalFormatPresets::colorScaleThumb(const QList< QColor >& colors)
{
    QPixmap pm(c_thumbW, c_thumbH);
    pm.fill(c_white);
    QPainter p(&pm);
    for (int i = 0; i < 4; ++i) {
        const double t = i / 3.0;
        QColor bg;
        if (colors.size() >= 3) {
            bg = t <= 0.5 ? DATableConditionalFormatRule::lerpColor(colors.at(0), colors.at(1), t * 2)
                          : DATableConditionalFormatRule::lerpColor(colors.at(1), colors.at(2), (t - 0.5) * 2);
        } else if (colors.size() == 2) {
            bg = DATableConditionalFormatRule::lerpColor(colors.at(0), colors.at(1), t);
        } else {
            bg = c_white;
        }
        const QRect row = thumbRowRect(i);
        p.fillRect(row, bg);
        drawTextDash(p, row, textColorOn(bg));
    }
    drawThumbGrid(p);
    p.end();
    return QIcon(pm);
}

/**
 * @brief 数据条缩略图（4 行递减条长）
 * @param color 条填充色
 * @param gradient 渐变填充
 * @return 菜单图标
 */
QIcon DAConditionalFormatPresets::dataBarThumb(const QColor& color, bool gradient)
{
    constexpr double c_lens[ 4 ] = { 1.0, 0.72, 0.5, 0.28 };
    QPixmap pm(c_thumbW, c_thumbH);
    pm.fill(c_white);
    QPainter p(&pm);
    for (int i = 0; i < 4; ++i) {
        const QRect row = thumbRowRect(i);
        const int barH  = qMax(2, row.height() - 4);
        const int barW  = qRound((row.width() - 4) * c_lens[ i ]);
        const QRect bar(row.left() + 2, row.center().y() - barH / 2, barW, barH);
        if (gradient) {
            QColor c0 = color;
            c0.setAlpha(110);
            QLinearGradient g(bar.topLeft(), bar.topRight());
            g.setColorAt(0.0, c0);
            g.setColorAt(1.0, color);
            p.fillRect(bar, g);
        } else {
            p.fillRect(bar, color);
        }
    }
    drawThumbGrid(p);
    p.end();
    return QIcon(pm);
}

/**
 * @brief 突出显示样式缩略图（4 行单元格：背景 + 文本条）
 * @param style 样式预设
 * @return 菜单图标
 */
QIcon DAConditionalFormatPresets::highlightThumb(const DACFHighlightStyle& style)
{
    QPixmap pm(c_thumbW, c_thumbH);
    pm.fill(c_white);
    QPainter p(&pm);
    const QColor bg = style.hasBackground() ? style.background : c_white;
    const QColor fg = style.hasForeground() ? style.foreground : textColorOn(bg);
    for (int i = 0; i < 4; ++i) {
        const QRect row = thumbRowRect(i);
        p.fillRect(row, bg);
        drawTextDash(p, row, fg);
    }
    drawThumbGrid(p);
    p.end();
    return QIcon(pm);
}

}  // end of namespace DA
