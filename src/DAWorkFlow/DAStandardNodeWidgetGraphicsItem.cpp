#include "DAStandardNodeWidgetGraphicsItem.h"
#include <QWidget>

namespace DA
{
class DAStandardNodeWidgetGraphicsItemPrivate
{
    DA_IMPL_PUBLIC(DAStandardNodeWidgetGraphicsItem)
public:
    DAStandardNodeWidgetGraphicsItemPrivate(DAStandardNodeWidgetGraphicsItem* p);

public:
    QGraphicsProxyWidget* _proxyWidget;
};
}  // end of namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAStandardNodeWidgetGraphicsItemPrivate
//===================================================
DAStandardNodeWidgetGraphicsItemPrivate::DAStandardNodeWidgetGraphicsItemPrivate(DAStandardNodeWidgetGraphicsItem* p)
    : q_ptr(p)
{
    _proxyWidget = new QGraphicsProxyWidget(p);
}

//

DAStandardNodeWidgetGraphicsItem::DAStandardNodeWidgetGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAAbstractNodeGraphicsItem(n, p), d_ptr(new DAStandardNodeWidgetGraphicsItemPrivate(this))
{
}

DAStandardNodeWidgetGraphicsItem::~DAStandardNodeWidgetGraphicsItem()
{
}

//==============================================================
// DAStandardNodeWidgetGraphicsItem
//==============================================================

void DAStandardNodeWidgetGraphicsItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
    Q_UNUSED(bodyRect);
}

void DAStandardNodeWidgetGraphicsItem::setBodySize(const QSizeF& s)
{
    d_ptr->_proxyWidget->resize(s);
    DAAbstractNodeGraphicsItem::setBodySize(s);
}
/**
 * @brief 获取widget代理item
 * @return
 */
QGraphicsProxyWidget* DAStandardNodeWidgetGraphicsItem::proxyWidgetItem()
{
    return d_ptr->_proxyWidget;
}

/**
 * @brief 代理QGraphicsProxyWidget::setWidget
 * @param w
 */
void DAStandardNodeWidgetGraphicsItem::setWidget(QWidget* w)
{
    QSize s = w->size();
    d_ptr->_proxyWidget->setWidget(w);
    changeBodySize(s);
}

/**
 * @brief 代理QGraphicsProxyWidget::widget
 * @return
 */
QWidget* DAStandardNodeWidgetGraphicsItem::widget() const
{
    return d_ptr->_proxyWidget->widget();
}
