#ifndef DAGRAPHICSRESIZEABLETEXTITEM_H
#define DAGRAPHICSRESIZEABLETEXTITEM_H

#include <QGraphicsTextItem>
#include "DAGraphicsResizeableItem.h"
namespace DA
{
class DAStandardGraphicsTextItem;
/**
 * @brief 支持缩放编辑的文本框Item
 */
class DAGRAPHICSVIEW_API DAGraphicsResizeableTextItem : public DAGraphicsResizeableItem
{
public:
    DAGraphicsResizeableTextItem(QGraphicsItem* parent = nullptr);
    DAGraphicsResizeableTextItem(const QFont& f, QGraphicsItem* parent = nullptr);
    DAGraphicsResizeableTextItem(const QString& str, const QFont& f, QGraphicsItem* parent = nullptr);
    //设置编辑模式
    void setEditMode(bool on = true);
    //保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* itemElement) override;

    //获取内部的文本item
    DAStandardGraphicsTextItem* textItem() const;

protected:
    void setBodySize(const QSizeF& s) override;
    void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;

private:
    DAStandardGraphicsTextItem* m_textItem;
};
}
#endif  // DAGRAPHICSRESIZEABLETEXTITEM_H
