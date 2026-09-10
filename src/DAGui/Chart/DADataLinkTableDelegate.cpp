#include "DADataLinkTableDelegate.h"
#include "DADataLinkTableModel.h"
// Qt
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyle>

namespace DA
{
DADataLinkTableDelegate::DADataLinkTableDelegate(QObject* parent) : QStyledItemDelegate(parent)
{
}

DADataLinkTableDelegate::~DADataLinkTableDelegate()
{
}

void DADataLinkTableDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    if (index.column() != 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    bool isPlotNode = index.data(DADataLinkTableModel::RoleIsPlotNode).toBool();
    if (isPlotNode) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    QColor color = index.data(DADataLinkTableModel::RoleCurveColor).value< QColor >();
    if (!color.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, opt.palette.highlight());
    }

    QRect rect  = opt.rect;
    int blockY  = rect.y() + (rect.height() - s_colorBlockSize) / 2;
    QRect colorRect(rect.x() + s_colorBlockMargin, blockY, s_colorBlockSize, s_colorBlockSize);
    painter->setBrush(QBrush(color));
    painter->setPen(Qt::NoPen);
    painter->drawRect(colorRect);

    QRect textRect = rect.adjusted(s_colorBlockSize + s_colorBlockMargin * 2, 0, 0, 0);
    QString text   = index.data(Qt::DisplayRole).toString();
    QColor textColor = (opt.state & QStyle::State_Selected) ? opt.palette.highlightedText().color()
                                                            : index.data(Qt::ForegroundRole).value< QColor >();
    if (!textColor.isValid()) {
        textColor = opt.palette.text().color();
    }
    painter->setPen(textColor);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

QSize DADataLinkTableDelegate::sizeHint(const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    if (index.column() == 0) {
        bool isPlotNode = index.data(DADataLinkTableModel::RoleIsPlotNode).toBool();
        if (!isPlotNode) {
            size.setWidth(size.width() + s_colorBlockSize + s_colorBlockMargin * 2);
        }
    }
    return size;
}
}  // namespace DA
