#ifndef DASTANDARDNODEWIDGETGRAPHICSITEM_H
#define DASTANDARDNODEWIDGETGRAPHICSITEM_H
#include "DAAbstractNodeGraphicsItem.h"
#include "DAWorkFlowAPI.h"
#include <QGraphicsProxyWidget>
namespace DA
{
/**
 * @brief 此类类似QGraphicsProxyWidget，实现了窗口的封装
 */
class DAWORKFLOW_API DAStandardNodeWidgetGraphicsItem : public DAAbstractNodeGraphicsItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAStandardNodeWidgetGraphicsItem)
public:
    enum
    {
        Type = DA::ItemType_GraphicsStandardWidgetItem
    };
    int type() const
    {
        return (Type);
    }

public:
    DAStandardNodeWidgetGraphicsItem(DAAbstractNode* n, QGraphicsItem* p = nullptr);
    ~DAStandardNodeWidgetGraphicsItem();
    // 内部维护窗口，paintbody不做任何动作
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect);
    // 设置尺寸
    void setBodySize(const QSizeF& s);
    // 获取widget代理item
    QGraphicsProxyWidget* proxyWidgetItem();
    // 代理QGraphicsProxyWidget::setWidget
    void setWidget(QWidget* w);
    // 代理QGraphicsProxyWidget::widget
    QWidget* widget() const;
};
}  // end of namespace DA
#endif  // DASTANDARDNODEWIDGETGRAPHICSITEM_H
