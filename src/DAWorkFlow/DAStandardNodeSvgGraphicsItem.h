#ifndef DASTANDARDNODESVGGRAPHICSITEM_H
#define DASTANDARDNODESVGGRAPHICSITEM_H
#include "DAWorkFlowGlobal.h"
#include "DAAbstractNodeGraphicsItem.h"
class QSvgRenderer;
class QGraphicsSimpleTextItem;
namespace DA
{
DA_IMPL_FORWARD_DECL(DAStandardNodeSvgGraphicsItem)
/**
 * @brief 标准的可resize的svg GraphicsItem,可以作为节点的图片显示
 *
 * @note 由于考虑性能问题DAAbstractNodeGraphicsItem没有继承QGraphicsObject，
 * 因此DAStandardNodeSvgGraphicsItem需要仿照
 */
class DAWORKFLOW_API DAStandardNodeSvgGraphicsItem : public DAAbstractNodeGraphicsItem
{
    Q_OBJECT
    DA_IMPL(DAStandardNodeSvgGraphicsItem)
    Q_PROPERTY(QString elementId READ elementId WRITE setElementId)
public:
    enum
    {
        Type = DA::ItemType_GraphicsStandardNodeSvgItem
    };
    int type() const
    {
        return (Type);
    }
    DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, QGraphicsItem* p = nullptr);
    DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, const QString& svgfile, QGraphicsItem* p = nullptr);
    DAStandardNodeSvgGraphicsItem(DAAbstractNode* n, QSvgRenderer* sharedrender, QGraphicsItem* p = nullptr);
    ~DAStandardNodeSvgGraphicsItem();

public:
    void setBodySize(const QSizeF& s) override;
    //绘制body
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;

public:
    //文本可以移动
    void setEnableMoveText(bool on = true);
    bool isEnableMoveText() const;
    //获取svg渲染器
    QSvgRenderer* renderer() const;
    void setSharedRenderer(QSvgRenderer* renderer);
    //设置ElementId
    void setElementId(const QString& id);
    QString elementId() const;
    //
    void setCachingEnabled(bool caching);
    bool isCachingEnabled() const;
    //获取字体的尺寸
    QSizeF getTextSize() const;
    void setText(const QString& t);
    //设置svg
    bool setSvg(const QString& svgfile);
    //把bodysize设置为svg的default size
    void resetBodySize();
    //设置图片变换时的比例
    void setAspectRatioMode(Qt::AspectRatioMode m);
    Qt::AspectRatioMode getAspectRatioMode() const;
    //获取文本图元
    QGraphicsSimpleTextItem* getTextItem() const;

protected:
    virtual void prepareNodeNameChanged(const QString& name);
public slots:
    void repaintItem();

protected:
    //添加事件处理
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value);
};
}  // end of namespace DA

#endif  // DASTANDARDNODESVGGRAPHICSITEM_H
