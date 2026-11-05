#include "Table/Metadata/DATableCellStyle.h"
#include "da_qt5qt6_compat.hpp"

namespace DA
{

/**
 * @brief 构造函数，三个 valid 标志位默认为 false
 */
DATableCellStyle::DATableCellStyle()
{
}

/**
 * @brief 未显式设色时使用的默认边框色
 *
 * 取官方色板「深色边框与深色线条」深蓝灰 #5D6D7E（DESIGN.md §2）。不能用
 * 「普通边框 / 分隔线」的中灰 #727272：单元格边框就画在表格网格线旁边，
 * 同色系 1px 线条在视觉上与网格线无法区分，用户会误以为「边框没生效」。
 * 绘制（delegate）与导出（HTML）共用此函数，避免两处回落色漂移。
 * @return 默认边框颜色
 */
QColor DATableCellStyle::defaultBorderColor()
{
    return QColor(0x5D, 0x6D, 0x7E);
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
 *
 * 颜色菜单的「无」传入的是 QBrush(无效 QColor())，这种画刷在 Qt 里 style 仍是
 * SolidPattern，绘制时按黑色实心填充；这里统一归一为 Qt::NoBrush，表示「显式不填充」，
 * 绘制/导出层据此保留控件默认底色（亮色主题即白色）。
 * @param b 底色画刷
 */
void DATableCellStyle::setBackground(const QBrush& b)
{
    mBackground      = b;
    if (mBackground.style() == Qt::SolidPattern && !mBackground.color().isValid()) {
        mBackground = QBrush(Qt::NoBrush);
    }
    mBackgroundValid = true;
}

/**
 * @brief 底色是否真的会画出颜色
 *
 * 已设置且不是 NoBrush、颜色有效才参与绘制；「无」（setBackground(QBrush())）
 * 返回 false，绘制层据此跳过填充、露出控件默认底色。
 * @return 需要填充返回 true
 */
bool DATableCellStyle::hasFill() const
{
    return mBackgroundValid && mBackground.style() != Qt::NoBrush && mBackground.color().isValid();
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
 * @brief 获取对齐方式（水平位 + 垂直位）
 * @return 对齐方式，未设置时返回空对齐
 */
Qt::Alignment DATableCellStyle::alignment() const
{
    return mAlignment;
}

/**
 * @brief 设置对齐方式，同时置 alignmentValid=true
 *
 * 只保留水平位（AlignLeft/AlignHCenter/AlignRight）与垂直位（AlignTop/AlignVCenter/AlignBottom），
 * 其余位（如 AlignJustify、AlignAbsolute）一律丢弃——绘制与导出只认这两组。
 * @param a 对齐方式（Qt::AlignCenter 等价于 AlignHCenter|AlignVCenter）
 */
void DATableCellStyle::setAlignment(Qt::Alignment a)
{
    mAlignment      = (a & (Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight))
                 | (a & (Qt::AlignTop | Qt::AlignVCenter | Qt::AlignBottom));
    mAlignmentValid = true;
}

/**
 * @brief 对齐方式是否已设置
 * @return 已设置返回 true
 */
bool DATableCellStyle::alignmentValid() const
{
    return mAlignmentValid;
}

/**
 * @brief 获取边框边位掩码
 * @return 位掩码（BorderSide 组合）；未设置时为 0
 */
quint8 DATableCellStyle::borderSides() const
{
    return mBorderSides;
}

/**
 * @brief 设置边框边位掩码，同时置 borderValid=true
 *
 * sides=0 表示显式无框线（覆盖下层继承的边框），与"未设置"不同。
 * @param sides BorderSide 位组合
 */
void DATableCellStyle::setBorderSides(quint8 sides)
{
    mBorderSides = sides;
    mBorderValid = true;
}

/**
 * @brief 获取边框颜色
 * @return 边框颜色；未设置时返回无效 QColor（绘制时回落到 defaultBorderColor）
 */
QColor DATableCellStyle::borderColor() const
{
    return mBorderColor;
}

/**
 * @brief 设置边框颜色（不单独改变 valid，边框是否生效由 setBorderSides/setBorder 决定）
 * @param c 边框颜色
 */
void DATableCellStyle::setBorderColor(const QColor& c)
{
    mBorderColor = c;
}

/**
 * @brief 获取边框宽度（像素）
 * @return 宽度，默认 1
 */
int DATableCellStyle::borderWidth() const
{
    return mBorderWidth;
}

/**
 * @brief 设置边框宽度（像素，1..3，越界钳制）
 * @param w 宽度
 */
void DATableCellStyle::setBorderWidth(int w)
{
    mBorderWidth = qBound(1, w, 3);
}

/**
 * @brief 边框是否已设置（sides=0 + valid=true 表示显式无框线）
 * @return 已设置返回 true
 */
bool DATableCellStyle::borderValid() const
{
    return mBorderValid;
}

/**
 * @brief 一次性设置边框（预设入口）
 * @param sides BorderSide 位组合（0 = 无框线）
 * @param color 边框颜色（无效表示绘制时用 defaultBorderColor）
 * @param width 宽度（<=0 按 1）
 */
void DATableCellStyle::setBorder(quint8 sides, const QColor& color, int width)
{
    setBorderSides(sides);
    mBorderColor = color;
    mBorderWidth = qBound(1, width <= 0 ? 1 : width, 3);
}

/**
 * @brief 是否自动换行
 * @return 换行返回 true
 */
bool DATableCellStyle::wrapWord() const
{
    return mWrapWord;
}

/**
 * @brief 设置自动换行，同时置 wrapWordValid=true
 *
 * 轻量方案（A4）：只改绘制（Qt::TextWordWrap），行高不随内容增长；
 * 需要更高行高时用「单元格大小 → 设置行高」手动调整。
 * @param on 是否换行
 */
void DATableCellStyle::setWrapWord(bool on)
{
    mWrapWord      = on;
    mWrapWordValid = true;
}

/**
 * @brief 自动换行是否已设置
 * @return 已设置返回 true
 */
bool DATableCellStyle::wrapWordValid() const
{
    return mWrapWordValid;
}

/**
 * @brief 所有属性是否全部未设置
 * @return 全部 valid 为 false 时返回 true
 */
bool DATableCellStyle::isNull() const
{
    return !mBackgroundValid && !mForegroundValid && !mFontValid && !mAlignmentValid && !mBorderValid
        && !mWrapWordValid;
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
    if (other.mAlignmentValid) {
        mAlignment      = other.mAlignment;
        mAlignmentValid = true;
    }
    if (other.mBorderValid) {
        mBorderSides = other.mBorderSides;
        mBorderColor = other.mBorderColor;
        mBorderWidth = other.mBorderWidth;
        mBorderValid = true;
    }
    if (other.mWrapWordValid) {
        mWrapWord      = other.mWrapWord;
        mWrapWordValid = true;
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
    if (mAlignmentValid) {
        // 缺属性 = 该轴未设置（读取方各自回落默认）
        QDomElement al = doc.createElement(QStringLiteral("alignment"));
        if (mAlignment & Qt::AlignLeft) {
            al.setAttribute(QStringLiteral("horizontal"), QStringLiteral("left"));
        } else if (mAlignment & Qt::AlignHCenter) {
            al.setAttribute(QStringLiteral("horizontal"), QStringLiteral("center"));
        } else if (mAlignment & Qt::AlignRight) {
            al.setAttribute(QStringLiteral("horizontal"), QStringLiteral("right"));
        }
        if (mAlignment & Qt::AlignTop) {
            al.setAttribute(QStringLiteral("vertical"), QStringLiteral("top"));
        } else if (mAlignment & Qt::AlignVCenter) {
            al.setAttribute(QStringLiteral("vertical"), QStringLiteral("vcenter"));
        } else if (mAlignment & Qt::AlignBottom) {
            al.setAttribute(QStringLiteral("vertical"), QStringLiteral("bottom"));
        }
        e.appendChild(al);
    }
    if (mBorderValid) {
        // sides 用字母串 l/r/t/b 子集；空串表示显式无框线
        QString sides;
        if (mBorderSides & BorderLeft) {
            sides += QLatin1Char('l');
        }
        if (mBorderSides & BorderRight) {
            sides += QLatin1Char('r');
        }
        if (mBorderSides & BorderTop) {
            sides += QLatin1Char('t');
        }
        if (mBorderSides & BorderBottom) {
            sides += QLatin1Char('b');
        }
        QDomElement bd = doc.createElement(QStringLiteral("border"));
        bd.setAttribute(QStringLiteral("sides"), sides);
        if (mBorderColor.isValid()) {
            bd.setAttribute(QStringLiteral("color"), mBorderColor.name(QColor::HexArgb));
        }
        bd.setAttribute(QStringLiteral("width"), mBorderWidth);
        e.appendChild(bd);
    }
    if (mWrapWordValid) {
        QDomElement wr = doc.createElement(QStringLiteral("wrap"));
        wr.setAttribute(QStringLiteral("on"), mWrapWord ? 1 : 0);
        e.appendChild(wr);
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
            compat::setNamedColor(c, child.attribute(QStringLiteral("color")));
            QBrush b;
            b.setStyle(bs);
            if (c.isValid()) {
                b.setColor(c);
            }
            setBackground(b);
        } else if (tag == QLatin1String("foreground")) {
            QColor c;
            compat::setNamedColor(c, child.attribute(QStringLiteral("color")));
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
        } else if (tag == QLatin1String("alignment")) {
            const QString hz = child.attribute(QStringLiteral("horizontal"));
            const QString vt = child.attribute(QStringLiteral("vertical"));
            Qt::Alignment al;
            if (hz == QLatin1String("left")) {
                al |= Qt::AlignLeft;
            } else if (hz == QLatin1String("center")) {
                al |= Qt::AlignHCenter;
            } else if (hz == QLatin1String("right")) {
                al |= Qt::AlignRight;
            }
            if (vt == QLatin1String("top")) {
                al |= Qt::AlignTop;
            } else if (vt == QLatin1String("vcenter")) {
                al |= Qt::AlignVCenter;
            } else if (vt == QLatin1String("bottom")) {
                al |= Qt::AlignBottom;
            }
            // 两个属性都缺视为未设置（旧工程/异常 XML 不产生空 valid 样式）
            if (!hz.isEmpty() || !vt.isEmpty()) {
                setAlignment(al);
            }
        } else if (tag == QLatin1String("border")) {
            const QString sidesStr = child.attribute(QStringLiteral("sides"));
            quint8 sides          = BorderNone;
            if (sidesStr.contains(QLatin1Char('l'))) {
                sides |= BorderLeft;
            }
            if (sidesStr.contains(QLatin1Char('r'))) {
                sides |= BorderRight;
            }
            if (sidesStr.contains(QLatin1Char('t'))) {
                sides |= BorderTop;
            }
            if (sidesStr.contains(QLatin1Char('b'))) {
                sides |= BorderBottom;
            }
            QColor c;
            compat::setNamedColor(c, child.attribute(QStringLiteral("color")));
            setBorder(sides, c, child.attribute(QStringLiteral("width"), "1").toInt());
        } else if (tag == QLatin1String("wrap")) {
            setWrapWord(child.attribute(QStringLiteral("on"), "1").toInt() != 0);
        }
        // 未知标签忽略（前向兼容）
        n = n.nextSibling();
    }
    return true;
}

}  // end of namespace DA
