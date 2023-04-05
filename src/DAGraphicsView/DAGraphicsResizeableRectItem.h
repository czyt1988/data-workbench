#ifndef DAGRAPHICSRESIZEABLERECTITEM_H
#define DAGRAPHICSRESIZEABLERECTITEM_H
#include "DAGraphicsViewGlobal.h"
#include "DAGraphicsResizeableItem.h"
class QDomDocument;
class QDomElement;
namespace DA
{
DA_IMPL_FORWARD_DECL(DAGraphicsResizeableRectItem)
/**
 * @brief 矩形图元
 */
class DAGRAPHICSVIEW_API DAGraphicsResizeableRectItem : public DAGraphicsResizeableItem
{
    Q_OBJECT
    DA_IMPL(DAGraphicsResizeableRectItem)
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_GraphicsResizeableRectItem
    };
    int type() const override
    {
        return (Type);
    }

public:
    DAGraphicsResizeableRectItem(QGraphicsItem* parent = nullptr);
    ~DAGraphicsResizeableRectItem();
    //设置文本
    void setText(const QString& t);
    QString getText() const;
    //设置文本对齐方式
    void setTextAlignment(Qt::Alignment al);
    Qt::Alignment getTextAlignment() const;
    //文本画笔
    QPen getTextPen() const;
    void setTextPen(const QPen& p);
    //矩形填充
    QBrush getRectFillBrush() const;
    void setRectFillBrush(const QBrush& b);
    //保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* itemElement) override;

public:
    //绘制body
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;
};
}

#endif  // DAGRAPHICSRESIZEABLERECTITEM_H
