#ifndef DAGRAPHICSRECTITEM_H
#define DAGRAPHICSRECTITEM_H
#include "DAGraphicsViewGlobal.h"
#include "DAGraphicsResizeableItem.h"
class QDomDocument;
class QDomElement;
namespace DA
{
/**
 * @brief 矩形图元
 */
class DAGRAPHICSVIEW_API DAGraphicsRectItem : public DAGraphicsResizeableItem
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsRectItem)
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_DAGraphicsRectItem
    };
    int type() const override
    {
        return (Type);
    }

public:
    DAGraphicsRectItem(QGraphicsItem* parent = nullptr);
    ~DAGraphicsRectItem();
    // 设置文本
    void setText(const QString& t);
    QString getText() const;
    // 设置文本对齐方式
    void setTextAlignment(Qt::Alignment al);
    Qt::Alignment getTextAlignment() const;
    // 文本画笔
    QPen getTextPen() const;
    void setTextPen(const QPen& p);
    // 矩形填充
    QBrush getRectFillBrush() const;
    void setRectFillBrush(const QBrush& b);
    // 保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement,const QVersionNumber& ver) const override;
    virtual bool loadFromXml(const QDomElement* itemElement,const QVersionNumber& ver) override;

public:
    // 绘制body
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;
};
}

#endif  // DAGRAPHICSRECTITEM_H
