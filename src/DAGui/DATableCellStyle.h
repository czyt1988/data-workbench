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
 * 三个属性各带 valid 标志位，区分"未设置（继承默认/上层）"与"显式设为透明/黑色"。
 * 用于 DATableStyleManager 的三层存储（列/行/单元格）。
 */
class DAGUI_API DATableCellStyle
{
public:
    DATableCellStyle();

    // 底色
    QBrush background() const;
    void setBackground(const QBrush& b);
    bool backgroundValid() const;

    // 字体颜色
    QColor foreground() const;
    void setForeground(const QColor& c);
    bool foregroundValid() const;

    // 字体
    QFont font() const;
    void setFont(const QFont& f);
    bool fontValid() const;

    // 三个 valid 全 false 时返回 true
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
};
}  // end of namespace DA
#endif  // DATABLECELLSTYLE_H
