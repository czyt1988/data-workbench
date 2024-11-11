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
	/**
	 * @brief 视图
	 * @return
	 */
	DAGraphicsView* view() const;

	/**
	 * @brief 结束action
	 *
	 * 此行数会删除action，destroy后不能有任何成员函数的操作，否则会出现异常
	 */
	void destroy();

protected:
	/**
	 * @brief 开始激活时的回调
	 */
	virtual void beginActive();

	/**
	 * @brief 结束激活时的回调
	 */
	virtual void endAction();

	/**
	 * @brief 捕获绘图事件
	 *
	 * 此paintEvent会在view的paintevent之后介入，无法捕获
	 * @param event
	 */
	virtual void paintEvent(QPaintEvent* event);

	/**
	 * @brief 捕获的按钮点击事件
	 * @param event
	 * @return
	 */
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
