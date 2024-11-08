#ifndef DAABSTRACTGRAPHICSVIEWACTION_H
#define DAABSTRACTGRAPHICSVIEWACTION_H
#include "DAGraphicsViewGlobal.h"
class QPaintEvent;
class QKeyEvent;
class QMouseEvent;

namespace DA
{
class DAGraphicsView;

/**
 * @brief 针对DAGraphicsView的action.
 *
 */
class DAGRAPHICSVIEW_API DAAbstractGraphicsViewAction
{
	friend class DAGraphicsView;

public:
	DAAbstractGraphicsViewAction(DAGraphicsView* v);
	virtual ~DAAbstractGraphicsViewAction();
	// 视图
	DAGraphicsView* view() const;
	// 结束action,此行数会删除action，end后不能有任何成员函数的操作，否则会出现异常
	void destroy();

protected:
	// 开始激活，这是使用setAction后调用的函数
	virtual void beginActive();
	// 结束激活，这是使用删除action前调用的函数
	virtual void endAction();
	// 捕获绘图事件,返回true，代表action劫持了此事件，不会在scene中继续传递事件,默认返回false
	virtual bool paintEvent(QPaintEvent* event);
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	virtual bool mouseDoubleClickEvent(QMouseEvent* event);
	virtual bool mouseMoveEvent(QMouseEvent* event);
	virtual bool mousePressEvent(QMouseEvent* event);
	virtual bool mouseReleaseEvent(QMouseEvent* event);

protected:
	DAGraphicsView* mView { nullptr };
};
}  // end ns DA
#endif
