#include "DAAbstractNodeWidget.h"
#include "DAAbstractNodeGraphicsItem.h"
#include <QVBoxLayout>

namespace DA
{
class DAAbstractNodeWidgetPrivate
{
    DA_IMPL_PUBLIC(DAAbstractNodeWidget)
public:
    DAAbstractNodeWidgetPrivate(DAAbstractNodeWidget* p);

    DAAbstractNode::WeakPointer _node;
};
}

//////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////
using namespace DA;

//////////////////////////////////////////////////////////////
/// DAAbstractNodeWidgetPrivate
//////////////////////////////////////////////////////////////
DAAbstractNodeWidgetPrivate::DAAbstractNodeWidgetPrivate(DAAbstractNodeWidget* p) : q_ptr(p)
{
}

//////////////////////////////////////////////////////////////
/// DAAbstractNodeWidget
//////////////////////////////////////////////////////////////
DAAbstractNodeWidget::DAAbstractNodeWidget(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f), d_ptr(new DAAbstractNodeWidgetPrivate(this))
{
}

DAAbstractNodeWidget::DAAbstractNodeWidget(const DAAbstractNode::SharedPointer& n, QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f), d_ptr(new DAAbstractNodeWidgetPrivate(this))
{
    setNode(n);
}

DAAbstractNodeWidget::~DAAbstractNodeWidget()
{
}
/**
 * @brief 设置节点
 *
 * 此函数会发生信号@sa nodeChanged
 * @param n
 */
void DAAbstractNodeWidget::setNode(const DAAbstractNode::SharedPointer& n)
{
    d_ptr->_node = n;
    emit nodeChanged(n);
}

DAAbstractNode::SharedPointer DAAbstractNodeWidget::getNode() const
{
    return (d_ptr->_node.lock());
}
