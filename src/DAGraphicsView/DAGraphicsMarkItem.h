#ifndef DAGRAPHICSMARKITEM_H
#define DAGRAPHICSMARKITEM_H
#include "DAGraphicsItem.h"
#include "DAGraphicsViewGlobal.h"
class QDomDocument;
class QDomElement;

namespace DA
{
class DAGraphicsScene;
/**
 * @brief DAGraphicsView的基本元件
 *
 * DAGraphicsItem提供了统一的saveToXml接口
 * 加载的过程通过DAGraphicsItemFactory进行加载
 */
class DAGRAPHICSVIEW_API DAGraphicsMarkItem : public DAGraphicsItem
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAGraphicsMarkItem)
public:
	enum
	{
		Type = DA::ItemType_DAGraphicsMarkItem
	};
	virtual int type() const override
	{
		return (Type);
	}

	/**
	 * @标记的形状
	 */
	enum MarkShape
	{
		ShapeRect,  ///< 矩形
        ShapeCross,  ///< 十字
		ShapeUserDefine = 1000
	};

public:
	DAGraphicsMarkItem(QGraphicsItem* parent = nullptr);
	~DAGraphicsMarkItem();
	// 保存到xml中
	virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
	virtual bool loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver) override;
	//设置形状
	void setMarkShape(int shapeStyle);
	int getMarkShape() const;
	//
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr);
	virtual QRectF boundingRect() const;
};

}

#endif  // DAGRAPHICSMARKITEM_H
