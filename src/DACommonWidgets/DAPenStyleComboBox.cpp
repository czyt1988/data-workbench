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
#if !DAPENSTYLECOMBOBOX_USE_DELEGATE
class DAPenStyleComboBoxPrivate
{
    DA_IMPL_PUBLIC(DAPenStyleComboBox)
public:
    DAPenStyleComboBoxPrivate(DAPenStyleComboBox* p);
    QPen _drawPen;
};

DAPenStyleComboBoxPrivate::DAPenStyleComboBoxPrivate(DAPenStyleComboBox* p) : q_ptr(p), _drawPen(Qt::black)
{
    _drawPen.setWidth(1);
}

#endif

//===================================================
// DAPenStyleComboBox
//===================================================
#if DAPENSTYLECOMBOBOX_USE_DELEGATE
DAPenStyleComboBox::DAPenStyleComboBox(QWidget* parent) : QComboBox(parent)
{
    setEditable(false);
    setIconSize(QSize(40, 15));
    setItemDelegate(new DAPenStyleComboBoxItemDelegate(this));
    addItem(Qt::NoPen);
    addItem(Qt::SolidLine);
    addItem(Qt::DashLine);
    addItem(Qt::DotLine);
    addItem(Qt::DashDotLine);
    addItem(Qt::DashDotDotLine);
    connect(this, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAPenStyleComboBox::onCurrentIndexChanged);
}
#else

DAPenStyleComboBox::DAPenStyleComboBox(QWidget* parent) : QComboBox(parent), d_ptr(new DAPenStyleComboBoxPrivate(this))
{
    setEditable(false);
    setIconSize(QSize(40, 15));
    addItem(Qt::NoPen);
    addItem(Qt::SolidLine);
    addItem(Qt::DashLine);
    addItem(Qt::DotLine);
    addItem(Qt::DashDotLine);
    addItem(Qt::DashDotDotLine);
    connect(this, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DAPenStyleComboBox::onCurrentIndexChanged);
}

DAPenStyleComboBox::~DAPenStyleComboBox()
{
}

#endif

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
#if DAPENSTYLECOMBOBOX_USE_DELEGATE
    DAPenStyleComboBoxItemDelegate* d = qobject_cast< DAPenStyleComboBoxItemDelegate* >(itemDelegate());
    if (d) {
        d->setPenColor(c);
    }
#else
    d_ptr->_drawPen.setColor(c);
    updateItems();
#endif
}

void DAPenStyleComboBox::setPenLineWidth(int w)
{
#if DAPENSTYLECOMBOBOX_USE_DELEGATE
    DAPenStyleComboBoxItemDelegate* d = qobject_cast< DAPenStyleComboBoxItemDelegate* >(itemDelegate());
    if (d) {
        d->setPenLineWidth(w);
    }
#else
    d_ptr->_drawPen.setWidth(w);
    updateItems();
#endif
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
    //获取颜色
    QColor c = (d == nullptr) ? Qt::black : d->getPenColor();
    int w    = (d == nullptr) ? 2 : d->getPenLineWidth();
    QPen p(c);
    p.setWidth(w);
    p.setStyle(s);
    drawPenStyle(&painter, QRect(QPoint(0, 0), iss), p);
    return QIcon(pixmap);
#else
    if (Qt::NoPen == s) {
        static QIcon s_noPenIcon(":/icon/icon/no-style.svg");
        return s_noPenIcon;
    }
    QSize iss = iconSize();
    QPixmap pixmap(iss);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    d_ptr->_drawPen.setStyle(s);
    drawPenStyle(&painter, QRect(QPoint(0, 0), iss), d_ptr->_drawPen);
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
 * @brief 添加item
 * @param s
 */
void DAPenStyleComboBox::addItem(Qt::PenStyle s)
{
    QComboBox::addItem(generatePenIcon(s), penStyleToString(s), static_cast< int >(s));
}

void DAPenStyleComboBox::onCurrentIndexChanged(int index)
{
    emit currentPenStyleChanged(static_cast< Qt::PenStyle >(itemData(index).toInt()));
}

//==============================================================
// DAPenStyleComboBoxItemDelegate
//==============================================================
#if DAPENSTYLECOMBOBOX_USE_DELEGATE
DAPenStyleComboBoxItemDelegate::DAPenStyleComboBoxItemDelegate(QObject* parent)
    : QAbstractItemDelegate(parent), _penColor(Qt::black), _penWidth(4)
{
}

void DAPenStyleComboBoxItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    drawBackground(painter, option, index);
    drawFocus(painter, option, index);
    Qt::PenStyle s = (Qt::PenStyle)index.data(Qt::UserRole).toInt();
    QPen p(_penColor);
    p.setWidth(_penWidth);
    p.setStyle(s);
    DAPenStyleComboBox::drawPenStyle(painter, option.rect, p);
}

QSize DAPenStyleComboBoxItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    int h = option.fontMetrics.height() + 4;
    if (h < _penWidth) {
        h = _penWidth + 2;
    }
    return QSize(option.rect.width(), h);
}
/**
 * @brief 设置画笔颜色
 * @param c
 */
void DAPenStyleComboBoxItemDelegate::setPenColor(const QColor& c)
{
    _penColor = c;
}
/**
 * @brief 获取画笔颜色
 * @param c
 */
QColor DAPenStyleComboBoxItemDelegate::getPenColor() const
{
    return _penColor;
}
/**
 * @brief 设置画笔宽度
 * @param c
 */
void DAPenStyleComboBoxItemDelegate::setPenLineWidth(int w)
{
    _penWidth = w;
}
/**
 * @brief 获取画笔宽度
 * @param c
 */
int DAPenStyleComboBoxItemDelegate::getPenLineWidth() const
{
    return _penWidth;
}

void DAPenStyleComboBoxItemDelegate::drawBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;
        painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
    } else {
        QVariant value = index.data(Qt::BackgroundRole);
        if (value.canConvert< QBrush >()) {
            QPointF oldBO = painter->brushOrigin();
            painter->setBrushOrigin(option.rect.topLeft());
            painter->fillRect(option.rect, qvariant_cast< QBrush >(value));
            painter->setBrushOrigin(oldBO);
        }
    }
}

void DAPenStyleComboBoxItemDelegate::drawFocus(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    if ((option.state & QStyle::State_HasFocus) == 0)
        return;
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(option);
    o.state |= QStyle::State_KeyboardFocusChange;
    o.state |= QStyle::State_Item;
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);
    const QWidget* widget = qobject_cast< const QWidget* >(parent());
    QStyle* style         = widget ? widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, widget);
}
#endif
}  // DA
