#include "DAAbstractGraphicsViewAction.h"
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include "DAGraphicsView.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param v 关联的DAGraphicsView视图
 */
DAAbstractGraphicsViewAction::DAAbstractGraphicsViewAction(DAGraphicsView* v) : mView(v)
{
}

/**
 * @brief 析构函数
 */
DAAbstractGraphicsViewAction::~DAAbstractGraphicsViewAction()
{
}

/**
 * @brief 获取关联的DAGraphicsView视图
 * @return 返回关联的视图指针
 */
DAGraphicsView* DAAbstractGraphicsViewAction::view() const
{
	return mView;
}

/**
 * @brief 销毁action，从视图中移除
 * @sa DAGraphicsView::clearViewAction
 */
void DAAbstractGraphicsViewAction::destroy()
{
	auto v = view();
	if (v) {
		v->clearViewAction();
	}
}

/**
 * @brief 开始激活时的回调
 */
void DAAbstractGraphicsViewAction::beginActive()
{
}

/**
 * @brief 结束激活时的回调
 */
void DAAbstractGraphicsViewAction::endAction()
{
}

/**
 * @brief 捕获键盘按下事件
 * @param event 键盘事件
 * @return 是否处理该事件，返回true表示已处理
 */
bool DAAbstractGraphicsViewAction::keyPressEvent(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

/**
 * @brief 捕获键盘释放事件
 * @param event 键盘事件
 * @return 是否处理该事件，返回true表示已处理
 */
bool DAAbstractGraphicsViewAction::keyReleaseEvent(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

/**
 * @brief 捕获鼠标双击事件
 * @param event 鼠标事件
 * @return 是否处理该事件，返回true表示已处理
 */
bool DAAbstractGraphicsViewAction::mouseDoubleClickEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

/**
 * @brief 捕获鼠标移动事件
 * @param event 鼠标事件
 * @return 是否处理该事件，返回true表示已处理
 */
bool DAAbstractGraphicsViewAction::mouseMoveEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

/**
 * @brief 捕获鼠标按下事件
 * @param event 鼠标事件
 * @return 是否处理该事件，返回true表示已处理
 */
bool DAAbstractGraphicsViewAction::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

/**
 * @brief 捕获鼠标释放事件
 * @param event 鼠标事件
 * @return 是否处理该事件，返回true表示已处理
 */
bool DAAbstractGraphicsViewAction::mouseReleaseEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	return false;
}

}
