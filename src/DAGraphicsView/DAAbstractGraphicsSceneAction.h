#ifndef DAABSTRACTGRAPHICSSCENEACTION_H
#define DAABSTRACTGRAPHICSSCENEACTION_H
#include "DAGraphicsViewGlobal.h"

namespace DA
{
class DAGraphicsScene;

/**
 * @brief 这是一个scene的动作，DAGraphicsScene支持持有一个动作，这个动作可以捕获scene的所有界面事件
 *
 * 一般要使用动作的情景是：
 * 绘制一个箭头，需要在屏幕上点击两个点
 * 绘制一个矩形，需要在屏幕上圈出区域
 * 进行一个链接进入一个动作，直到动作的结束
 *
 * scene仅能存在一个动作，新动作设置，会销毁旧的动作
 *
 * 动作可以自身取消
 */
class DAGRAPHICSVIEW_API DAAbstractGraphicsSceneAction
{
    friend class DAGraphicsScene;

public:
	DAAbstractGraphicsSceneAction(DAGraphicsScene* sc);
	virtual ~DAAbstractGraphicsSceneAction();
	// 场景
	DAGraphicsScene* scene() const;
	// 结束action,此行数会删除action，end后不能有任何成员函数的操作，否则会出现异常
	void end();
	// 设置光标的图像
	void setCursorPixmap(const QPixmap& p);
	// 设置光标
	void setupCursor();
	// 恢复光标
	void restoreCursor();

public:
	// svg转换为pixmap
	static QPixmap svgToPixmap(const QString& svgPath, const QSize& size);
	// 通过任意两个点，获取topleft点
	static QPointF topLeftPoint(const QPointF& p1, const QPointF& p2);
	// 通过任意两个点，获取bottomRight点
	static QPointF bottomRightPoint(const QPointF& p1, const QPointF& p2);
	// 通过任意两点，获取构成矩形的尺寸
	static QSizeF pointRectSize(const QPointF& p1, const QPointF& p2);

protected:
	// 开始激活，这是使用setAction后调用的函数
	virtual void beginActive();
	// 鼠标点击事件,返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
	virtual bool mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent);
	// 鼠标移动事件,返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
	virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent);
	// 鼠标释放,返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
	virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent);
	// 结束激活，这是使用删除action前调用的函数
	virtual void endAction();

protected:
	DAGraphicsScene* mScene { nullptr };
	QPixmap mCursorPixmap;
};
}

#endif  // DAABSTRACTGRAPHICSSCENEACTION_H
