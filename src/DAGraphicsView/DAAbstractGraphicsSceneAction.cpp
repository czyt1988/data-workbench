#include "DAAbstractGraphicsSceneAction.h"
#include "DAGraphicsScene.h"
#include <QGraphicsView>
#include <QSvgRenderer>
#include <QSvgGenerator>
#include <QPainter>
#include <QCursor>
namespace DA
{
DAAbstractGraphicsSceneAction::DAAbstractGraphicsSceneAction(DAGraphicsScene* sc) : mScene(sc)
{
}

DAAbstractGraphicsSceneAction::~DAAbstractGraphicsSceneAction()
{
}

DAGraphicsScene* DAAbstractGraphicsSceneAction::scene() const
{
	return mScene;
}

void DAAbstractGraphicsSceneAction::beginActive()
{
}

bool DAAbstractGraphicsSceneAction::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	Q_UNUSED(mouseEvent);
	return false;
}

bool DAAbstractGraphicsSceneAction::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	Q_UNUSED(mouseEvent);
	return false;
}

bool DAAbstractGraphicsSceneAction::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	Q_UNUSED(mouseEvent);
	return false;
}

void DAAbstractGraphicsSceneAction::endAction()
{
}

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

void DAAbstractGraphicsSceneAction::setupCursor()
{
	QList< QGraphicsView* > views = scene()->views();
	for (QGraphicsView* v : views) {
		v->setCursor(QCursor(mCursorPixmap));
	}
}

void DAAbstractGraphicsSceneAction::restoreCursor()
{
	QList< QGraphicsView* > views = scene()->views();
	for (QGraphicsView* v : views) {
		v->unsetCursor();
	}
}

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
