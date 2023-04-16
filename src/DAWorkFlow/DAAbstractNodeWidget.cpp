#include "DAAbstractNodeWidget.h"
#include "DAAbstractNodeGraphicsItem.h"
#include <QVBoxLayout>

namespace DA
{
class DAAbstractNodeWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractNodeWidget)
public:
    PrivateData(DAAbstractNodeWidget* p);

    DAAbstractNode::WeakPointer mNode;
};

//////////////////////////////////////////////////////////////
/// DAAbstractNodeWidgetPrivate
//////////////////////////////////////////////////////////////
DAAbstractNodeWidget::PrivateData::PrivateData(DAAbstractNodeWidget* p) : q_ptr(p)
{
}

//////////////////////////////////////////////////////////////
/// DAAbstractNodeWidget
//////////////////////////////////////////////////////////////
DAAbstractNodeWidget::DAAbstractNodeWidget(QWidget* parent, Qt::WindowFlags f) : QWidget(parent, f), DA_PIMPL_CONSTRUCT
{
}

DAAbstractNodeWidget::DAAbstractNodeWidget(const DAAbstractNode::SharedPointer& n, QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f), DA_PIMPL_CONSTRUCT
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
    d_ptr->mNode = n;
    emit nodeChanged(n);
}

DAAbstractNode::SharedPointer DAAbstractNodeWidget::getNode() const
{
    return (d_ptr->mNode.lock());
}
}  // end of DA
