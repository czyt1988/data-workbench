#ifndef DASTANDARDNODEPIXMAPGRAPHICSITEM_H
#define DASTANDARDNODEPIXMAPGRAPHICSITEM_H
#include "DAWorkFlowGlobal.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DAGraphicsResizeableItem.h"
class QDomDocument;
class QDomElement;
namespace DA
{

/**
 * @brief 标准的可resize的pixmap GraphicsItem,可以作为节点的图片显示
 */
class DAWORKFLOW_API DAStandardNodePixmapGraphicsItem : public DAAbstractNodeGraphicsItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAStandardNodePixmapGraphicsItem)
public:
    enum
    {
        Type = DA::ItemType_GraphicsStandardNodePixmapItem
    };
    int type() const
    {
        return (Type);
    }
    DAStandardNodePixmapGraphicsItem(DAAbstractNode* n, QGraphicsItem* p = nullptr);
    ~DAStandardNodePixmapGraphicsItem();
    void setBodySize(const QSizeF& s) override;
    //计算BoundingRect
    virtual QSizeF bodySizeHint() const;
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;
    //设置节点的图片，默认是node节点的icon图片
    void setPixmap(const QPixmap& p);
    const QPixmap& getPixmap() const;
    //设置图片的尺寸
    void setPixmapSize(const QSize& s);
    QSize getPixmapSize() const;
    //获取字体的尺寸
    QSizeF getTextSize() const;

    void setText(const QString& t);
    QString getText() const;
    //文本可以移动
    void setEnableMoveText(bool on = true);
    bool isEnableMoveText() const;
    //设置图片变换时的比例
    void setAspectRatioMode(Qt::AspectRatioMode m);
    Qt::AspectRatioMode getAspectRatioMode() const;
    //设置图片缩放时TransformationMode
    void setTransformationMode(Qt::TransformationMode m);
    Qt::TransformationMode getTransformationMode() const;
    //保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const;
    virtual bool loadFromXml(const QDomElement* itemElement);

protected:
    //添加事件处理
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
    //节点名字改变
    void prepareNodeNameChanged(const QString& name) override;
};

}  // end of namespace DA
#endif  // FCSTANDARDNODEGRAPHICSITEM_H
