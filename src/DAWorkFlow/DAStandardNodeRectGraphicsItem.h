#ifndef DASTANDARDNODERECTGRAPHICSITEM_H
#define DASTANDARDNODERECTGRAPHICSITEM_H
#include "DAWorkFlowAPI.h"
#include "DAAbstractNodeGraphicsItem.h"
namespace DA
{
/**
 * @brief 标准的可resize的rect GraphicsItem,可以作为节点的内容显示
 */
class DAWORKFLOW_API DAStandardNodeRectGraphicsItem : public DAAbstractNodeGraphicsItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAStandardNodeRectGraphicsItem)
public:
    enum
    {
        Type = DA::ItemType_GraphicsStandardRectItem
    };
    int type() const
    {
        return (Type);
    }

public:
    DAStandardNodeRectGraphicsItem(DAAbstractNode* n, QGraphicsItem* p = nullptr);
    ~DAStandardNodeRectGraphicsItem();

public:
    // 绘制body
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;
    // 设置文本
    void setText(const QString& t);
    QString getText() const;
    // 设置文本对齐方式
    void setTextAlignment(Qt::Alignment al);
    Qt::Alignment getTextAlignment() const;
};
}
#endif  // DASTANDARDNODERECTPGRAPHICSITEM_H
