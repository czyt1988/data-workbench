#ifndef DASTANDARDNODECONSTVALUEGRAPHICSITEM_H
#define DASTANDARDNODECONSTVALUEGRAPHICSITEM_H
#include "DAStandardNodeTextGraphicsItem.h"
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include "DAStandardNodeConstValue.h"
class QFocusEvent;
class QGraphicsSceneMouseEvent;
namespace DA
{
/**
 * @brief 常数节点
 */
class DAWORKFLOW_API DAStandardNodeConstValueGraphicsItem : public DAStandardNodeTextGraphicsItem
{
public:
	DAStandardNodeConstValueGraphicsItem(DAStandardNodeConstValue* n, QGraphicsItem* p = nullptr);
	~DAStandardNodeConstValueGraphicsItem();
	virtual void nodeDisplayNameChanged(const QString& name);
	// 获取当前的值
	QVariant::Type getCurrentValueType() const;
	void setCurrentValueType(QVariant::Type v);
	// 内部维护窗口，paintbody不做任何动作
	void paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect) override;

	// 轮廓圆角
	qreal getRectRadius() const;
	void setRectRadius(qreal newRectRadius);

private:
	qreal mRectRadius { 5 };  ///< 外轮廓的圆角
	QVariant::Type mValueType;
};
}
#endif  // DASTANDARDNODECONSTVALUEGRAPHICSITEM_H
