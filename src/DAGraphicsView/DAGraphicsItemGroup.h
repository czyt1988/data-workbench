#ifndef DAGRAPHICSITEMGROUP_H
#define DAGRAPHICSITEMGROUP_H
#include <QGraphicsItemGroup>
#include "DAUtils/DAXMLFileInterface.h"
#include "DAGraphicsViewGlobal.h"
class QDomDocument;
class QDomElement;
class QGraphicsSceneHoverEvent;
namespace DA
{
/**
 * @brief QGraphicsItemGroup的继承
 */
class DAGRAPHICSVIEW_API DAGraphicsItemGroup : public QGraphicsItemGroup, public DAXMLFileInterface
{
    DA_DECLARE_PRIVATE(DAGraphicsItemGroup)
public:
    /**
     * @brief 适用qgraphicsitem_cast
     */
    enum
    {
        Type = DA::ItemType_DAGraphicsItem
    };
    virtual int type() const override
    {
        return (Type);
    }

public:
    DAGraphicsItemGroup(QGraphicsItem* parent = nullptr);
    ~DAGraphicsItemGroup();
    // 保存到xml中,注意，这里不会保存子item
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* parentElement) override;
    // 设置边框画笔，如果设置一个QPen,则不绘制边框
    void setBorderPen(const QPen& p);
    QPen getBorderPen() const;
    // 设置是否显示边框
    void setShowBorder(bool on);
    bool isShowBorder() const;
    // 背景
    void setBackgroundBrush(const QBrush& b);
    QBrush getBackgroundBrush() const;
    // 设置是否显示背景
    void setShowBackground(bool on);
    bool isShowBackground() const;
    // 获取item的id，id是这个id唯一的标识，id主要为了能单独的找到这个item，在分组加载时使用
    uint64_t getItemID() const;
    void setItemID(uint64_t id);
    // 获取分组下的分组
    QList< DAGraphicsItemGroup* > childGroups() const;
    // 获取不包含分组的子item
    QList< QGraphicsItem* > childItemsExcludingGrouping() const;

public:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
};

}

#endif  // DAGRAPHICSITEM_H
