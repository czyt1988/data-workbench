#include "DATableCellStyle.h"

namespace DA
{

/**
 * @brief 构造函数，三个 valid 标志位默认为 false
 */
DATableCellStyle::DATableCellStyle()
{
}

/**
 * @brief 获取底色画刷
 * @return 底色画刷，未设置时返回默认 QBrush
 */
QBrush DATableCellStyle::background() const
{
    return mBackground;
}

/**
 * @brief 设置底色画刷，同时置 backgroundValid=true
 * @param b 底色画刷
 */
void DATableCellStyle::setBackground(const QBrush& b)
{
    mBackground      = b;
    mBackgroundValid = true;
}

/**
 * @brief 底色是否已设置
 * @return 已设置返回 true
 */
bool DATableCellStyle::backgroundValid() const
{
    return mBackgroundValid;
}

/**
 * @brief 获取字体颜色
 * @return 字体颜色，未设置时返回默认 QColor
 */
QColor DATableCellStyle::foreground() const
{
    return mForeground;
}

/**
 * @brief 设置字体颜色，同时置 foregroundValid=true
 * @param c 字体颜色
 */
void DATableCellStyle::setForeground(const QColor& c)
{
    mForeground      = c;
    mForegroundValid = true;
}

/**
 * @brief 字体颜色是否已设置
 * @return 已设置返回 true
 */
bool DATableCellStyle::foregroundValid() const
{
    return mForegroundValid;
}

/**
 * @brief 获取字体
 * @return 字体，未设置时返回默认 QFont
 */
QFont DATableCellStyle::font() const
{
    return mFont;
}

/**
 * @brief 设置字体，同时置 fontValid=true
 * @param f 字体
 */
void DATableCellStyle::setFont(const QFont& f)
{
    mFont      = f;
    mFontValid = true;
}

/**
 * @brief 字体是否已设置
 * @return 已设置返回 true
 */
bool DATableCellStyle::fontValid() const
{
    return mFontValid;
}

/**
 * @brief 三个属性是否全部未设置
 * @return 全部 valid 为 false 时返回 true
 */
bool DATableCellStyle::isNull() const
{
    return !mBackgroundValid && !mForegroundValid && !mFontValid;
}

/**
 * @brief 局部合并：用 other 中"已设置(valid)"的属性覆盖 this，未设置的保留
 * @param other 提供覆盖属性的源样式
 * @return this 引用，支持链式调用
 */
DATableCellStyle& DATableCellStyle::mergeFrom(const DATableCellStyle& other)
{
    if (other.mBackgroundValid) {
        mBackground      = other.mBackground;
        mBackgroundValid = true;
    }
    if (other.mForegroundValid) {
        mForeground      = other.mForeground;
        mForegroundValid = true;
    }
    if (other.mFontValid) {
        mFont      = other.mFont;
        mFontValid = true;
    }
    return *this;
}

/**
 * @brief 序列化到 XML 元素
 *
 * 仅写入已设置(valid)的属性，未设置的属性不写入。
 * background 写入 brushstyle 和 color（仅纯色模式写 color）。
 * foreground 写入 color。font 写入 family/size/bold/italic/underline。
 * @param doc QDomDocument 用于创建子元素
 * @param e 目标父元素（cell/column-style/row-style）
 */
void DATableCellStyle::toXml(QDomDocument& doc, QDomElement& e) const
{
    if (mBackgroundValid) {
        QDomElement bg = doc.createElement(QStringLiteral("background"));
        bg.setAttribute(QStringLiteral("brushstyle"), static_cast<int>(mBackground.style()));
        if (mBackground.style() == Qt::SolidPattern && mBackground.color().isValid()) {
            bg.setAttribute(QStringLiteral("color"), mBackground.color().name(QColor::HexArgb));
        }
        e.appendChild(bg);
    }
    if (mForegroundValid) {
        QDomElement fg = doc.createElement(QStringLiteral("foreground"));
        if (mForeground.isValid()) {
            fg.setAttribute(QStringLiteral("color"), mForeground.name(QColor::HexArgb));
        }
        e.appendChild(fg);
    }
    if (mFontValid) {
        QDomElement f = doc.createElement(QStringLiteral("font"));
        f.setAttribute(QStringLiteral("family"), mFont.family());
        f.setAttribute(QStringLiteral("size"), mFont.pointSize());
        f.setAttribute(QStringLiteral("bold"), mFont.bold() ? 1 : 0);
        f.setAttribute(QStringLiteral("italic"), mFont.italic() ? 1 : 0);
        f.setAttribute(QStringLiteral("underline"), mFont.underline() ? 1 : 0);
        e.appendChild(f);
    }
}

/**
 * @brief 从 XML 元素反序列化
 *
 * 遍历子节点，解析 background/foreground/font 标签并设置对应属性。
 * 未知标签忽略（前向兼容）。缺失属性使用合理默认值。
 * @param e 源元素（cell/column-style/row-style）
 * @return 始终返回 true（与项目 DAXMLFileInterface 惯例一致）
 */
bool DATableCellStyle::fromXml(const QDomElement& e)
{
    QDomNode n = e.firstChild();
    while (!n.isNull()) {
        QDomElement child = n.toElement();
        if (child.isNull()) {
            n = n.nextSibling();
            continue;
        }
        QString tag = child.tagName();
        if (tag == QLatin1String("background")) {
            int bsInt    = child.attribute(QStringLiteral("brushstyle"), "1").toInt();
            // 范围校验，避免外部 XML 传入越界值触发 Q_ASSERT
            Qt::BrushStyle bs = (bsInt >= Qt::NoBrush && bsInt <= Qt::TexturePattern)
                                    ? static_cast<Qt::BrushStyle>(bsInt)
                                    : Qt::SolidPattern;
            QColor c;
            c.setNamedColor(child.attribute(QStringLiteral("color")));
            QBrush b;
            b.setStyle(bs);
            if (c.isValid()) {
                b.setColor(c);
            }
            setBackground(b);
        } else if (tag == QLatin1String("foreground")) {
            QColor c;
            c.setNamedColor(child.attribute(QStringLiteral("color")));
            if (c.isValid()) {
                setForeground(c);
            }
        } else if (tag == QLatin1String("font")) {
            QFont f;
            f.setFamily(child.attribute(QStringLiteral("family")));
            int pointSize = child.attribute(QStringLiteral("size"), "9").toInt();
            if (pointSize <= 0) {
                pointSize = 9;
            }
            f.setPointSize(pointSize);
            f.setBold(child.attribute(QStringLiteral("bold"), "0").toInt() != 0);
            f.setItalic(child.attribute(QStringLiteral("italic"), "0").toInt() != 0);
            f.setUnderline(child.attribute(QStringLiteral("underline"), "0").toInt() != 0);
            setFont(f);
        }
        // 未知标签忽略（前向兼容）
        n = n.nextSibling();
    }
    return true;
}

}  // end of namespace DA
