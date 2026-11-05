#ifndef DATABLECELLSTYLE_H
#define DATABLECELLSTYLE_H
#include "DAGuiAPI.h"
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 表格单元格样式值对象（非 QObject，结构体形态，便于作为 QHash value 频繁拷贝）
 *
 * 每个属性各带 valid 标志位，区分"未设置（继承默认/上层）"与"显式设为透明/黑色/无框线"。
 * 用于 DATableStyleManager 的三层存储（列/行/单元格）。
 */
class DAGUI_API DATableCellStyle
{
public:
    /** @brief 边框边位掩码（可组合；0 = 无框线） */
    enum BorderSide
    {
        BorderNone   = 0x00,
        BorderLeft   = 0x01,
        BorderRight  = 0x02,
        BorderTop    = 0x04,
        BorderBottom = 0x08,
        BorderAll    = BorderLeft | BorderRight | BorderTop | BorderBottom
    };

public:
    DATableCellStyle();

    // 底色
    QBrush background() const;
    void setBackground(const QBrush& b);
    bool backgroundValid() const;
    // 底色是否真的会画出颜色（已设置 + 非 NoBrush + 颜色有效）
    bool hasFill() const;

    // 字体颜色
    QColor foreground() const;
    void setForeground(const QColor& c);
    bool foregroundValid() const;

    // 字体
    QFont font() const;
    void setFont(const QFont& f);
    bool fontValid() const;

    // 对齐方式（只保留水平位 AlignLeft/AlignHCenter/AlignRight 与垂直位 AlignTop/AlignVCenter/AlignBottom）
    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment a);
    bool alignmentValid() const;

    // 自动换行（A4 轻量：只改绘制，行高不随内容增长——需要行高配合时用「单元格大小→设置行高」）
    bool wrapWord() const;
    void setWrapWord(bool on);
    bool wrapWordValid() const;

    // 边框（单色，四边用位掩码组合；sides=0 且 borderValid=true 表示显式无框线）
    quint8 borderSides() const;
    void setBorderSides(quint8 sides);
    QColor borderColor() const;
    void setBorderColor(const QColor& c);
    int borderWidth() const;
    void setBorderWidth(int w);
    bool borderValid() const;
    // 一次性设置边框（预设用）：sides 置 valid；color 无效表示绘制用默认色；width <= 0 按 1
    void setBorder(quint8 sides, const QColor& color = QColor(), int width = 1);
    // 未显式设色时的默认边框色（绘制与导出共用）
    static QColor defaultBorderColor();

    // 所有 valid 全 false 时返回 true
    bool isNull() const;

    // 局部合并：用 other 中"已设置(valid)"的属性覆盖 this，未设置的保留
    DATableCellStyle& mergeFrom(const DATableCellStyle& other);

    // 序列化
    void toXml(QDomDocument& doc, QDomElement& e) const;
    bool fromXml(const QDomElement& e);

private:
    QBrush mBackground;
    bool mBackgroundValid { false };
    QColor mForeground;
    bool mForegroundValid { false };
    QFont mFont;
    bool mFontValid { false };
    Qt::Alignment mAlignment { Qt::Alignment() };
    bool mAlignmentValid { false };
    bool mWrapWord { false };
    bool mWrapWordValid { false };
    quint8 mBorderSides { 0 };
    QColor mBorderColor;
    int mBorderWidth { 1 };
    bool mBorderValid { false };
};
}  // end of namespace DA
#endif  // DATABLECELLSTYLE_H
