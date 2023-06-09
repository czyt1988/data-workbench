#ifndef DASTANDARDGRAPHICSTEXTITEM_H
#define DASTANDARDGRAPHICSTEXTITEM_H

#include <QFont>
#include <QGraphicsTextItem>
#include "DAGraphicsViewGlobal.h"
#include "DAUtils/DAXMLFileInterface.h"
namespace DA
{
class DAGRAPHICSVIEW_API DAStandardGraphicsTextItem : public QGraphicsTextItem, public DAXMLFileInterface
{
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_GraphicsStandardTextItem
    };
    int type() const override
    {
        return (Type);
    }

public:
    DAStandardGraphicsTextItem(QGraphicsItem* parent = nullptr);
    DAStandardGraphicsTextItem(const QString& str, const QFont& f, QGraphicsItem* parent = nullptr);

    DAStandardGraphicsTextItem(const QFont& f, QGraphicsItem* parent = nullptr);
    //设置编辑模式
    void setEditMode(bool on = true);
    //保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* itemElement) override;

protected:
    //焦点移出事件
    virtual void focusOutEvent(QFocusEvent* focusEvent) override;
    //鼠标双击事件进入编辑
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void initItem();
};
}  // end of namespace DA
#endif  // DASTANDARDGRAPHICSTEXTITEM_H
