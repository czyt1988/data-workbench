#ifndef DAABSTRACTGRAPHICSVIEWACTION_H
#define DAABSTRACTGRAPHICSVIEWACTION_H
#include "DAGraphicsViewGlobal.h"
class QPaintEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
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

	// 结束action
	void destroy();

protected:
	// 开始激活时的回调
	virtual void beginActive();

	// 结束激活时的回调
	virtual void endAction();

	// 捕获的按钮点击事件
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
