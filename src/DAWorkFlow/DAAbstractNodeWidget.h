#ifndef DAABSTRACTNODEWIDGET_H
#define DAABSTRACTNODEWIDGET_H
#include <QWidget>
#include "DAWorkFlowGlobal.h"
#include "DAAbstractNode.h"
namespace DA
{
class DAAbstractNodeGraphicsItem;

/**
 * @brief FCNodeItem都可返回一个FCNodeWidget，用于设置node属性
 *
 * FCNodeWidget是一个空的窗口，可以通过@sa setWidget 函数设置窗口
 */
class DAWORKFLOW_API DAAbstractNodeWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAbstractNodeWidget)
public:
    explicit DAAbstractNodeWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit DAAbstractNodeWidget(const DAAbstractNode::SharedPointer& n,
                                  QWidget* parent   = nullptr,
                                  Qt::WindowFlags f = Qt::WindowFlags());
    ~DAAbstractNodeWidget();

    //设置节点，会触发nodeChanged信号
    void setNode(const DAAbstractNode::SharedPointer& n);

    //获取节点
    DAAbstractNode::SharedPointer getNode() const;
signals:
    /**
     * @brief 窗口管理的节点发生了改变触发的信号
     * @param n
     */
    void nodeChanged(const DAAbstractNode::SharedPointer& n);
};
}
#endif  // FCABSTRACTNODEWIDGET_H
