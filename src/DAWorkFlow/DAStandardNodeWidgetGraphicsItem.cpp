#include "DAStandardNodeWidgetGraphicsItem.h"
#include <QWidget>

namespace DA
{
class DAStandardNodeWidgetGraphicsItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAStandardNodeWidgetGraphicsItem)
public:
    PrivateData(DAStandardNodeWidgetGraphicsItem* p);

public:
    QGraphicsProxyWidget* mProxyWidget;
};

//===================================================
// DAStandardNodeWidgetGraphicsItemPrivate
//===================================================
DAStandardNodeWidgetGraphicsItem::PrivateData::PrivateData(DAStandardNodeWidgetGraphicsItem* p) : q_ptr(p)
{
    mProxyWidget = new QGraphicsProxyWidget(p);
}

//==============================================================
// DAStandardNodeWidgetGraphicsItem
//==============================================================

DAStandardNodeWidgetGraphicsItem::DAStandardNodeWidgetGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), DA_PIMPL_CONSTRUCT
{
}

DAStandardNodeWidgetGraphicsItem::~DAStandardNodeWidgetGraphicsItem()
{
}

void DAStandardNodeWidgetGraphicsItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
    Q_UNUSED(bodyRect);
}

void DAStandardNodeWidgetGraphicsItem::setBodySize(const QSizeF& s)
{
    d_ptr->mProxyWidget->resize(s);
    DAAbstractNodeGraphicsItem::setBodySize(s);
}
/**
 * @brief 获取widget代理item
 * @return
 */
QGraphicsProxyWidget* DAStandardNodeWidgetGraphicsItem::proxyWidgetItem()
{
    return d_ptr->mProxyWidget;
}

/**
 * @brief 代理QGraphicsProxyWidget::setWidget
 * @param w
 */
void DAStandardNodeWidgetGraphicsItem::setWidget(QWidget* w)
{
    QSize s = w->size();
    d_ptr->mProxyWidget->setWidget(w);
    changeBodySize(s);
}

/**
 * @brief 代理QGraphicsProxyWidget::widget
 * @return
 */
QWidget* DAStandardNodeWidgetGraphicsItem::widget() const
{
    return d_ptr->mProxyWidget->widget();
}
}  // end of namespace DA
