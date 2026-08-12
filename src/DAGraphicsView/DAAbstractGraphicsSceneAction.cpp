#include "DAAbstractGraphicsSceneAction.h"
#include "DAGraphicsScene.h"
#include <QGraphicsView>
#include <QSvgRenderer>
#include <QSvgGenerator>
#include <QPainter>
#include <QCursor>
namespace DA
{

/**
 * @brief 构造函数
 * @param sc 关联的图形场景
 */
DAAbstractGraphicsSceneAction::DAAbstractGraphicsSceneAction(DAGraphicsScene* sc) : mScene(sc)
{
}

/**
 * @brief 析构函数
 */
DAAbstractGraphicsSceneAction::~DAAbstractGraphicsSceneAction()
{
}

/**
 * @brief 获取关联的图形场景
 * @return 关联的DAGraphicsScene指针
 */
DAGraphicsScene* DAAbstractGraphicsSceneAction::scene() const
{
	return mScene;
}

/**
 * @brief 开始激活动作，子类可重写此方法进行激活时的初始化
 */
void DAAbstractGraphicsSceneAction::beginActive()
{
}

/**
 * @brief 鼠标按下事件处理
 * @param mouseEvent 鼠标事件
 * @return 是否处理了该事件
 */
bool DAAbstractGraphicsSceneAction::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	Q_UNUSED(mouseEvent);
	return false;
}

/**
 * @brief 鼠标移动事件处理
 * @param mouseEvent 鼠标事件
 * @return 是否处理了该事件
 */
bool DAAbstractGraphicsSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	Q_UNUSED(mouseEvent);
	return false;
}

/**
 * @brief 鼠标释放事件处理
 * @param mouseEvent 鼠标事件
 * @return 是否处理了该事件
 */
bool DAAbstractGraphicsSceneAction::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	Q_UNUSED(mouseEvent);
	return false;
}

/**
 * @brief 结束动作，子类可重写此方法进行清理工作
 */
void DAAbstractGraphicsSceneAction::endAction()
{
}

/**
 * @brief 销毁动作，从关联场景中清除当前动作
 */
void DAAbstractGraphicsSceneAction::destroy()
{
	auto sc = scene();
	if (sc) {
		sc->clearSceneAction();
	}
}

/**
 * @brief 设置光标的图像
 * @param p
 */
void DAAbstractGraphicsSceneAction::setCursorPixmap(const QPixmap& p)
{
    mCursorPixmap = p;
}

/**
 * @brief 为场景关联的所有视图设置光标
 */
void DAAbstractGraphicsSceneAction::setupCursor()
{
	const QList< QGraphicsView* > views = scene()->views();
	for (QGraphicsView* v : views) {
		v->setCursor(QCursor(mCursorPixmap));
	}
}

/**
 * @brief 恢复场景关联的所有视图的默认光标
 */
void DAAbstractGraphicsSceneAction::restoreCursor()
{
	const QList< QGraphicsView* > views = scene()->views();
	for (QGraphicsView* v : views) {
		v->unsetCursor();
	}
}

/**
 * @brief 将SVG文件转换为指定尺寸的QPixmap
 * @param svgPath SVG文件路径
 * @param size 目标尺寸
 * @return 转换后的QPixmap，若加载失败返回透明图片
 */
QPixmap DAAbstractGraphicsSceneAction::svgToPixmap(const QString& svgPath, const QSize& size)
{
	QPixmap pixmap(size);
	pixmap.fill(Qt::transparent);
	QPainter pixmapPainter(&pixmap);
	QSvgRenderer svgRender;
	if (!svgRender.load(svgPath)) {
		return pixmap;
	}
	svgRender.render(&pixmapPainter, QRectF(0, 0, size.width(), size.height()));
	return pixmap;
}

/**
 * @brief 通过任意两个点，获取topleft点
 * @param p1
 * @param p2
 * @return topleft
 */
QPointF DAAbstractGraphicsSceneAction::topLeftPoint(const QPointF& p1, const QPointF& p2)
{
	QPointF tl;
	tl.setX(qMin(p1.x(), p2.x()));
	tl.setY(qMin(p1.y(), p2.y()));
	return tl;
}
/**
 * @brief 通过任意两个点，获取bottomRight点
 * @param p1
 * @param p2
 * @return bottomRight
 */
QPointF DAAbstractGraphicsSceneAction::bottomRightPoint(const QPointF& p1, const QPointF& p2)
{
	QPointF br;
	br.setX(qMax(p1.x(), p2.x()));
	br.setY(qMax(p1.y(), p2.y()));
	return br;
}

/**
 * @brief 通过任意两点，获取构成矩形的尺寸
 * @param p1
 * @param p2
 * @return
 */
QSizeF DAAbstractGraphicsSceneAction::pointRectSize(const QPointF& p1, const QPointF& p2)
{
	QSizeF s;
	s.setWidth(qAbs(p1.x() - p2.x()));
	s.setHeight(qAbs(p1.y() - p2.y()));
	return s;
}
}  // end namespace DA
