#include "DAWordWrapItemDelegate.h"
#include <QApplication>
#include <QPainter>
namespace DA
{
DAWordWrapItemDelegate::DAWordWrapItemDelegate(QObject* parent) : QStyledItemDelegate(parent), mWrapMode(Wrap)
{
}

/**
 * @brief 设置换行模式
 * @param m
 */
void DAWordWrapItemDelegate::setWrapMode(DAWordWrapItemDelegate::WrapMode m)
{
    mWrapMode = m;
}

/**
 * @brief 获取换行模式
 * @return
 */
DAWordWrapItemDelegate::WrapMode DAWordWrapItemDelegate::getWrapMode() const
{
    return mWrapMode;
}

void DAWordWrapItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (NoWrap == mWrapMode) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    QString value              = index.model()->data(index, Qt::DisplayRole).toString();
    QStyleOptionViewItemV4 opt = option;
    this->initStyleOption(&opt, index);

    const QWidget* widget = option.widget;
    opt.text              = "";
    // option
    QStyle* style = widget ? widget->style() : QApplication::style();

    //下边是设置选中时颜色
    if (option.state & QStyle::State_Selected) {
        // Whitee pen while selection
        painter->setPen(Qt::white);
        painter->setBrush(option.palette.highlightedText());
        // This call will take care to draw, dashed line while selecting
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    } else {
        painter->setPen(QPen(option.palette.foreground(), 0));
        painter->setBrush(qvariant_cast< QBrush >(index.data(Qt::ForegroundRole)));
    }

    //设置字体颜色
    painter->setPen(qvariant_cast< QColor >(index.data(Qt::ForegroundRole)));

    //有\n时自动换行
    bool ok = false;
    int al  = index.model()->data(index, Qt::TextAlignmentRole).toInt(&ok);
    if (!ok) {
        al = (Qt::AlignLeft | Qt::AlignVCenter);
    }
    painter->drawText(option.rect, Qt::TextWordWrap | al, value);
}
}
