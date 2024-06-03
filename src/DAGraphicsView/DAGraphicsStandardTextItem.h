#ifndef DAGRAPHICSSTANDARDTEXTITEM_H
#define DAGRAPHICSSTANDARDTEXTITEM_H

#include <QFont>
#include <QGraphicsTextItem>
#include "DAGraphicsViewGlobal.h"
#include "DAUtils/DAXMLFileInterface.h"
namespace DA
{
/**
 * @brief QGraphicsTextItem是一个非常复杂的item，里面包含了QWidgetTextControl多达3000行，暂时不尝试写一个类似的控件
 */
class DAGRAPHICSVIEW_API DAGraphicsStandardTextItem : public QGraphicsTextItem, public DAXMLFileInterface
{
public:
	/**
	 * @brief 适用qgraphicsitem_cast
	 */
	enum
	{
		Type = DA::ItemType_DAGraphicsStandardTextItem
	};
	int type() const override
	{
		return (Type);
	}

public:
	DAGraphicsStandardTextItem(QGraphicsItem* parent = nullptr);
	DAGraphicsStandardTextItem(const QString& str, const QFont& f, QGraphicsItem* parent = nullptr);

	DAGraphicsStandardTextItem(const QFont& f, QGraphicsItem* parent = nullptr);
	// 设置编辑模式
	void setEditable(bool on = true);
	bool isEditable() const;
	// 保存到xml中
	virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
	virtual bool loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver) override;
	// 设置在场景的位置，如果没有分组，和setPos一样，如果分组了，最终也能保证位置在pos位置
	void setScenePos(const QPointF& p);
	void setScenePos(qreal x, qreal y);
	// 获取item的id，id是这个id唯一的标识，id主要为了能单独的找到这个item，在分组加载时使用
	uint64_t getItemID() const;
	void setItemID(uint64_t id);

protected:
	// 焦点移出事件
	virtual void focusOutEvent(QFocusEvent* focusEvent) override;
	// 鼠标双击事件进入编辑
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
	void initItem();
	uint64_t mID { 0 };
};
}  // end of namespace DA
#endif  // DAGRAPHICSSTANDARDTEXTITEM_H
