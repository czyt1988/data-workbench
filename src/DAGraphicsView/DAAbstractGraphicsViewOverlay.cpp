#include "DAAbstractGraphicsViewOverlay.h"
#include <QMouseEvent>
#include <QHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QGraphicsView>
#include <QGraphicsScene>
namespace DA
{
/**
 * @brief 构造函数，初始化图形视图覆盖层
 * @param parent 父图形视图控件
 */
DAAbstractGraphicsViewOverlay::DAAbstractGraphicsViewOverlay(QGraphicsView* parent) : DAAbstractWidgetOverlay(parent)
{
	parent->setMouseTracking(true);
	mIsInstalled = tryInstall();
	setActive(true);
}

/**
 * @brief 析构函数
 */
DAAbstractGraphicsViewOverlay::~DAAbstractGraphicsViewOverlay()
{
}

/**
 * @brief 获取覆盖层矩形区域
 * @return 返回父控件的contentsRect，若父控件为空则返回空矩形
 */
QRect DAAbstractGraphicsViewOverlay::overlayRect() const
{
	const QWidget* widget = parentWidget();
	if (widget) {
		return widget->contentsRect();
	}
	return QRect();
}

/**
 * @brief 获取当前鼠标位置
 * @return 返回鼠标在视图坐标系中的位置
 */
QPoint DAAbstractGraphicsViewOverlay::getMousePos() const
{
	return mMousePos;
}

/**
 * @brief 获取覆盖层是否处于激活状态
 * @return 返回true表示激活，false表示未激活
 * @sa setActive
 */
bool DAAbstractGraphicsViewOverlay::isActive() const
{
	return mIsActive;
}

/**
 * @brief 设置覆盖层的激活状态
 * @param v 是否激活
 * @sa isActive
 */
void DAAbstractGraphicsViewOverlay::setActive(bool v)
{
	mIsActive = v;
	setMouseTracking(v);
}

/**
 * @brief 事件过滤器，拦截并处理视口的鼠标事件
 * @param obj 监听的对象
 * @param event 事件对象
 * @return 返回父类eventFilter的处理结果
 */
bool DAAbstractGraphicsViewOverlay::eventFilter(QObject* obj, QEvent* event)
{
	if (!mIsInstalled) {
		mIsInstalled = tryInstall();
	}
	if (obj && (obj == mViewPort)) {
		switch (event->type()) {
		case QEvent::MouseMove: {
			QMouseEvent* me = static_cast< QMouseEvent* >(event);
			viewMouseMove(view()->mapFromGlobal(mViewPort->mapToGlobal(me->pos())));
			break;
		}
		case QEvent::MouseButtonPress: {
			QMouseEvent* me = static_cast< QMouseEvent* >(event);
			viewMousePress(view()->mapFromGlobal(mViewPort->mapToGlobal(me->pos())));
			break;
		}
		case QEvent::MouseButtonRelease: {
			QMouseEvent* me = static_cast< QMouseEvent* >(event);
			viewMouseRelease(view()->mapFromGlobal(mViewPort->mapToGlobal(me->pos())));
			break;
		}
		default:
			break;
		}
	}
	return DAAbstractWidgetOverlay::eventFilter(obj, event);
}

/**
 * @brief 获取关联的QGraphicsView控件
 * @return 返回关联的QGraphicsView指针，若类型不匹配则返回nullptr
 */
QGraphicsView* DAAbstractGraphicsViewOverlay::view() const
{
	return qobject_cast< QGraphicsView* >(parentWidget());
}

/**
 * @brief 判断覆盖层是否有效
 * @return 返回true表示视口已安装且有效
 */
bool DAAbstractGraphicsViewOverlay::isValid() const
{
	return mViewPort != nullptr;
}

/**
 * @brief 处理视口鼠标移动事件
 * @param viewPos 视图坐标系中的鼠标位置
 */
void DAAbstractGraphicsViewOverlay::viewMouseMove(const QPoint& viewPos)
{
	mMousePos = viewPos;
	if (isActive()) {
		updateOverlay();
	}
}

/**
 * @brief 处理视口鼠标按下事件
 * @param viewPos 视图坐标系中的鼠标位置
 */
void DAAbstractGraphicsViewOverlay::viewMousePress(const QPoint& viewPos)
{
	mMousePos = viewPos;
	if (isActive()) {
		updateOverlay();
	}
}

/**
 * @brief 处理视口鼠标释放事件
 * @param viewPos 视图坐标系中的鼠标位置
 */
void DAAbstractGraphicsViewOverlay::viewMouseRelease(const QPoint& viewPos)
{
	mMousePos = viewPos;
	if (isActive()) {
		updateOverlay();
	}
}

/**
 * @brief 尝试向图形视图的视口安装事件过滤器
 * @return 返回true表示安装成功或已安装，false表示视口为空
 */
bool DAAbstractGraphicsViewOverlay::tryInstall()
{
	QGraphicsView* v = view();
	QWidget* vp      = v->viewport();
	if (!vp) {
		return false;
	}
	if (vp == mViewPort) {
		// vp不为空且和原来的一样，返回true，表示安装完成
		return true;
	}
	mViewPort = vp;
	mViewPort->installEventFilter(this);
	return true;
}
}  // end ns da
