#include "DATableCellStyle.h"

namespace DA
{

DATableCellStyle::DATableCellStyle()
{
}

QBrush DATableCellStyle::background() const
{
    return mBackground;
}

void DATableCellStyle::setBackground(const QBrush& b)
{
    mBackground      = b;
    mBackgroundValid = true;
}

bool DATableCellStyle::backgroundValid() const
{
    return mBackgroundValid;
}

QColor DATableCellStyle::foreground() const
{
    return mForeground;
}

void DATableCellStyle::setForeground(const QColor& c)
{
    mForeground      = c;
    mForegroundValid = true;
}

bool DATableCellStyle::foregroundValid() const
{
    return mForegroundValid;
}

QFont DATableCellStyle::font() const
{
    return mFont;
}

void DATableCellStyle::setFont(const QFont& f)
{
    mFont      = f;
    mFontValid = true;
}

bool DATableCellStyle::fontValid() const
{
    return mFontValid;
}

bool DATableCellStyle::isNull() const
{
    return !mBackgroundValid && !mForegroundValid && !mFontValid;
}

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

bool DATableCellStyle::fromXml(const QDomElement& e)
{
    // e 本身是 <cell>/<column-style>/<row-style> 元素，遍历其子节点
    QDomNode n = e.firstChild();
    while (!n.isNull()) {
        QDomElement child = n.toElement();
        if (child.isNull()) {
            n = n.nextSibling();
            continue;
        }
        QString tag = child.tagName();
        if (tag == QLatin1String("background")) {
            Qt::BrushStyle bs = static_cast<Qt::BrushStyle>(child.attribute(QStringLiteral("brushstyle"), "1").toInt());
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
            setForeground(c);
        } else if (tag == QLatin1String("font")) {
            QFont f;
            f.setFamily(child.attribute(QStringLiteral("family")));
            f.setPointSize(child.attribute(QStringLiteral("size"), "9").toInt());
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
