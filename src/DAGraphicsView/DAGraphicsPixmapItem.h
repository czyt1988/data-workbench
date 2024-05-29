#ifndef DAGRAPHICSPIXMAPITEM_H
#define DAGRAPHICSPIXMAPITEM_H
#include "DAGraphicsViewGlobal.h"
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include "DAGraphicsResizeableItem.h"
namespace DA
{
/**
 * @brief 支持缩放编辑的图片item
 */
class DAGRAPHICSVIEW_API DAGraphicsPixmapItem : public DAGraphicsResizeableItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsPixmapItem)
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_DAGraphicsPixmapItem
    };
    int type() const override
    {
        return (Type);
    }

public:
    DAGraphicsPixmapItem(QGraphicsItem* parent = nullptr);
    DAGraphicsPixmapItem(const QPixmap& pixmap, QGraphicsItem* parent = nullptr);
    ~DAGraphicsPixmapItem();
    // 移动操作
    void setMoveable(bool on = true);
    bool isMoveable() const;
    // 选择操作
    void setSelectable(bool on = true);
    bool isSelectable() const;
    // 图片操作
    void setPixmap(const QPixmap& pixmap);
    const QPixmap& getPixmap() const;
    const QPixmap& getOriginPixmap() const;
    // 图片属性设置
    void setTransformationMode(Qt::TransformationMode t);
    Qt::TransformationMode getTransformationMode() const;
    // 图片缩放属性设置
    void setAspectRatioMode(Qt::AspectRatioMode t);
    Qt::AspectRatioMode getAspectRatioMode() const;
    // 判断是否存在有效图片
    bool isHaveValidPixmap() const;
    // 设置透明度
    void setAlpha(int a);
    int getAlpha() const;
    // 接口函数
    virtual void setBodySize(const QSizeF& s) override;
    // 保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement,const QVersionNumber& ver) const override;
    virtual bool loadFromXml(const QDomElement* itemElement,const QVersionNumber& ver) override;
signals:
    void itemPosChange(const QPointF& oldPos, const QPointF& newPos);

protected:
    virtual void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;

private:
};
}
#endif  // DAGRAPHICSPIXMAPITEM_H
