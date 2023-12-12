#include "DAPenStyleComboBox.h"
#include <QPainter>
#include <QPixmap>
#include <QDebug>
#include <QStyleOptionFocusRect>
#include <QStyleOption>
#include <QStyle>
#include <QApplication>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================
namespace DA
{

class DAPenStyleComboBox::PrivateData
{
    DA_DECLARE_PUBLIC(DAPenStyleComboBox)
public:
    PrivateData(DAPenStyleComboBox* p);
    QPen mDrawPen { Qt::black };
    bool mStyleTextVisible { false };
};

DAPenStyleComboBox::PrivateData::PrivateData(DAPenStyleComboBox* p) : q_ptr(p)
{
    mDrawPen.setWidth(1);
}

//===================================================
// DAPenStyleComboBox
//===================================================

DAPenStyleComboBox::DAPenStyleComboBox(QWidget* parent) : QComboBox(parent), DA_PIMPL_CONSTRUCT
{
    setEditable(false);
    setIconSize(QSize(40, 15));
    rebuildItems();
    connect(this, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAPenStyleComboBox::onCurrentIndexChanged);
}

DAPenStyleComboBox::~DAPenStyleComboBox()
{
}

/**
 * @brief PenStyle转换为文本
 * @param s
 * @return 如果没有对应的penstyle，返回QString()
 */
QString DAPenStyleComboBox::penStyleToString(Qt::PenStyle s)
{
    switch (s) {
    case Qt::NoPen:
        return QObject::tr("No Pen");
    case Qt::SolidLine:
        return QObject::tr("Solid Line");
    case Qt::DashLine:
        return QObject::tr("Dash Line");
    case Qt::DotLine:
        return QObject::tr("Dot Line");
    case Qt::DashDotLine:
        return QObject::tr("Dash Dot Line");
    case Qt::DashDotDotLine:
        return QObject::tr("Dash Dot Dot Line");
    case Qt::CustomDashLine:
        return QObject::tr("Custom Dash Line");
    default:
        break;
    }
    return QString();
}

/**
 * @brief 设置颜色
 * @param c
 */
void DAPenStyleComboBox::setPenColor(const QColor& c)
{
    d_ptr->mDrawPen.setColor(c);
    updateItems();
}

void DAPenStyleComboBox::setPenLineWidth(int w)
{
    d_ptr->mDrawPen.setWidth(w);
    updateItems();
}

/**
 * @brief 设置当前的画笔样式
 * @param s
 */
void DAPenStyleComboBox::setCurrentPenStyle(Qt::PenStyle s)
{
    int i = findData(static_cast< int >(s));
    if (i >= 0 && i < count()) {
        setCurrentIndex(i);
    }
}

/**
 * @brief 生成icon
 * @param s
 * @return
 */
QIcon DAPenStyleComboBox::generatePenIcon(Qt::PenStyle s) const
{
#if DAPENSTYLECOMBOBOX_USE_DELEGATE
    QSize iss = iconSize();
    if (!iss.isValid()) {
        return QIcon();
    }
    QPixmap pixmap(iss);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    DAPenStyleComboBoxItemDelegate* d = qobject_cast< DAPenStyleComboBoxItemDelegate* >(itemDelegate());
    // 获取颜色
    QColor c = (d == nullptr) ? Qt::black : d->getPenColor();
    int w    = (d == nullptr) ? 2 : d->getPenLineWidth();
    QPen p(c);
    p.setWidth(w);
    p.setStyle(s);
    drawPenStyle(&painter, QRect(QPoint(0, 0), iss), p);
    return QIcon(pixmap);
#else
    if (Qt::NoPen == s) {
        static QIcon s_noPenIcon(":/commonWidget/icon/icon/no-style.svg");
        return s_noPenIcon;
    }
    QSize iss = iconSize();
    QPixmap pixmap(iss);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    d_ptr->mDrawPen.setStyle(s);
    drawPenStyle(&painter, QRect(QPoint(0, 0), iss), d_ptr->mDrawPen);
    return QIcon(pixmap);
#endif
}

void DAPenStyleComboBox::drawPenStyle(QPainter* painter, const QRect& rect, const QPen& pen)
{
    painter->save();
    int y = rect.top() + rect.height() / 2;
    painter->setPen(pen);
    painter->drawLine(rect.left() + 3, y, rect.right() - 3, y);
    painter->restore();
}

void DAPenStyleComboBox::updateItems()
{
    for (int i = 0; i < count(); ++i) {
        setItemIcon(i, generatePenIcon(Qt::PenStyle(itemData(i).toInt())));
    }
}

/**
   @brief 是否在样式上显示文字

   @default 默认为否
   @param on
 */
void DAPenStyleComboBox::setStyleTextVisible(bool on)
{
    if (d_ptr->mStyleTextVisible != on) {
        // 要重新生成
        // 先获取已经设置的
        QList< Qt::PenStyle > ps;
        int c = count();
        for (int i = 0; i < c; ++i) {
            ps.append(static_cast< Qt::PenStyle >(itemData(i).toInt()));
        }
        clear();
        for (Qt::PenStyle s : std::as_const(ps)) {
            addItem(s);
        }
    }
    d_ptr->mStyleTextVisible = on;
}

bool DAPenStyleComboBox::isStyleTextVisible() const
{
    return d_ptr->mStyleTextVisible;
}

/**
   @brief 重建所有items
 */
void DAPenStyleComboBox::rebuildItems()
{
    clear();
    addItem(Qt::NoPen);
    addItem(Qt::SolidLine);
    addItem(Qt::DashLine);
    addItem(Qt::DotLine);
    addItem(Qt::DashDotLine);
    addItem(Qt::DashDotDotLine);
}

/**
 * @brief 设置画笔
 * @param p
 */
void DAPenStyleComboBox::setPen(const QPen& p)
{
    d_ptr->mDrawPen = p;
    updateItems();
    setCurrentPenStyle(p.style());
}

/**
 * @brief 添加item
 * @param s
 */
void DAPenStyleComboBox::addItem(Qt::PenStyle s)
{
    QComboBox::addItem(generatePenIcon(s), isStyleTextVisible() ? penStyleToString(s) : "", static_cast< int >(s));
}

void DAPenStyleComboBox::onCurrentIndexChanged(int index)
{
    emit currentPenStyleChanged(static_cast< Qt::PenStyle >(itemData(index).toInt()));
}

}  // DA
