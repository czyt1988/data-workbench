#ifndef DAGRAPHICSLABELITEM_H
#define DAGRAPHICSLABELITEM_H
#include <QFont>
#include <QGraphicsSimpleTextItem>
#include <QVersionNumber>
#include "DAGraphicsViewGlobal.h"
#include "DAXMLFileInterface.h"
#include "DAShapeKeyPoint.h"
class QDomDocument;
class QDomElement;
namespace DA
{

/**
 * @brief The DAGraphicsLabelItem class
 */
class DAGRAPHICSVIEW_API DAGraphicsLabelItem : public QGraphicsSimpleTextItem, public DAXMLFileInterface
{
	DA_DECLARE_PRIVATE(DAGraphicsLabelItem)
public:
	/**
	 * @brief 适用qgraphicsitem_cast
	 */
	enum
	{
		Type = DA::ItemType_DAGraphicsLabelItem
	};
	int type() const override
	{
		return (Type);
	}

public:
	DAGraphicsLabelItem(QGraphicsItem* parent = nullptr);
	DAGraphicsLabelItem(const QString& str, QGraphicsItem* parent = nullptr);
	~DAGraphicsLabelItem();
	// 保存到xml中
	virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
	virtual bool loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver) override;
	// 获取item的id，id是这个id唯一的标识，id主要为了能单独的找到这个item，在分组加载时使用
	uint64_t getItemID() const;
	void setItemID(uint64_t id);
	// 设置相对父窗口的相对定位
	void setRelativePosition(qreal xp, qreal yp);
	QPointF getRelativePosition() const;
	bool isHaveRelativePosition() const;
	// 设置吸附点，如果设置了吸附点，那么RelativePosition无效
	void setAttachPoint(DAShapeKeyPoint parentAttachPoint);
	// 原点，原点主要用于对齐
	void setOriginPoint(DAShapeKeyPoint originPoint);
	DAShapeKeyPoint getOriginPoint() const;
	// 更新相对位置
	void updatePosition();
	// 设置是否被选中
	void setSelectable(bool on);

protected:
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
};
}
#endif  // DAGRAPHICSLABELITEM_H
