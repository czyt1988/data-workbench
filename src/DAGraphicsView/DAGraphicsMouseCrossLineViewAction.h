#ifndef DAGRAPHICSMOUSECROSSLINEVIEWACTION_H
#define DAGRAPHICSMOUSECROSSLINEVIEWACTION_H
#pragma once
#include <QPen>
#include <QPoint>
#include <QPointF>
#include "DAAbstractGraphicsViewAction.h"
#include "DAGraphicsViewGlobal.h"
class QPaintEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
namespace DA
{
class DAGraphicsView;

/**
 * @brief 绘制屏幕十字线的动作.
 *
 */
class DAGRAPHICSVIEW_API DAGraphicsMouseCrossLineViewAction : public DAAbstractGraphicsViewAction
{
public:
	enum ActionTypes
	{
		OneTimeMarking,  ///< 一次性标记，鼠标点击或任意键盘动作后消失
		FollowMouse      ///< 跟随鼠标移动
	};

public:
	DAGraphicsMouseCrossLineViewAction(DAGraphicsView* v, ActionTypes type = OneTimeMarking);
	~DAGraphicsMouseCrossLineViewAction();
	/**
	 * @brief 设置动作类型
	 *
	 * @param 动作类型 @ref ActionTypes
	 */
	void setActionTypes(ActionTypes t);

	/**
	 * @brief 获取动作类型
	 *
	 * @return 动作类型 @ref ActionTypes
	 */
	ActionTypes getActionTypes() const;

	/**
	 * @brief 设置绘图画笔.
	 *
	 * @param $PARAMS
	 * @return $RETURN
	 */
	void setDrawPen(const QPen& c);

	/**
	 * @brief 获取绘图画笔.
	 *
	 * @return 绘图画笔
	 */
	QPen getDrawPen() const;

	/**
	 * @brief 设置绘图颜色.
	 */
	void setDrawColor(const QColor& c);

	/**
	 * @brief 获取当前设置的绘图颜色.
	 */
	QColor getDrawColor() const;

	/**
	 * @brief 设置十字中心位置.
	 *
	 * @param 十字中心位置
	 */
	void setCrossViewPos(const QPoint& p);
	QPoint getCrossViewPos() const;

	/**
	 * @brief 基于场景坐标设置十字线坐标.
	 *
	 * 如果当前场景坐标不在view区域内，将不会显示，因此此函数会先调用ensureview
	 *
	 * @param 十字中心位置
	 */
	void setCrossScenePos(const QPointF& p);

	/**
	 * @brief 设置是否有效，无效会不显示
	 */
	void setValid(bool on);
	bool isValid() const;

protected:
	// 开始激活，这是使用view中setAction后调用的回调函数
	virtual void beginActive();
	// 结束激活，这是使用删除action前调用的的回调函数
	virtual void endAction();
	// 捕获绘图事件,返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
	virtual void paintEvent(QPaintEvent* event);
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool mousePressEvent(QMouseEvent* event);
	virtual bool mouseReleaseEvent(QMouseEvent* event);
	virtual bool mouseMoveEvent(QMouseEvent* event);
	virtual void paintCross(QPainter* painter, const QPoint& point, const QRect& viewRect);

private:
	ActionTypes mType { OneTimeMarking };  ///< 动作类型
	QPen mDrawPen { Qt::black };           ///< 绘制画笔
	QPoint mPos;                           ///< 十字位置
	bool mValid { true };
};
}  // end ns da

#endif
