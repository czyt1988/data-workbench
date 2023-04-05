#ifndef DAGRAPHICSRESIZEABLEPIXMAPITEM_H
#define DAGRAPHICSRESIZEABLEPIXMAPITEM_H
#include "DAGraphicsViewGlobal.h"
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include "DAGraphicsResizeableItem.h"
namespace DA
{
/**
 * @brief 支持缩放编辑的图片item
 */
class DAGRAPHICSVIEW_API DAGraphicsResizeablePixmapItem : public DAGraphicsResizeableItem
{
    Q_OBJECT
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_GraphicsResizeablePixmapItem
    };
    int type() const override
    {
        return (Type);
    }

public:
    DAGraphicsResizeablePixmapItem(QGraphicsItem* parent = nullptr);
    DAGraphicsResizeablePixmapItem(const QPixmap& pixmap, QGraphicsItem* parent = nullptr);
    //设置是否可选
    //    void setSelectable(bool on = true);
    void setMoveable(bool on = true);
    bool isMoveable() const;
    //
    void setSelectable(bool on = true);
    bool isSelectable() const;
    //
    void setPixmap(const QPixmap& pixmap);
    const QPixmap& getPixmap() const;
    const QPixmap& getOriginPixmap() const;
    //
    void setTransformationMode(Qt::TransformationMode t);
    Qt::TransformationMode getTransformationMode() const;
    //
    void setAspectRatioMode(Qt::AspectRatioMode t);
    Qt::AspectRatioMode getAspectRatioMode() const;

signals:
    void itemPosChange(const QPointF& oldPos, const QPointF& newPos);

protected:
    //    QRectF bodyBoundingRect() const override;
    virtual void setBodySize(const QSizeF& s) override;
    virtual void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;

private:
    QPixmap _pixmap;        ///< 设置尺寸后的图形
    QPixmap _pixmapOrigin;  ///< 保存原始的图形
    QPointF _oldPos;        ///< 保存移动前的位置
    Qt::TransformationMode _transformationMode;
    Qt::AspectRatioMode _aspectRatioMode;
};
}
#endif  // DAGRAPHICSRESIZEABLEPIXMAPITEM_H
