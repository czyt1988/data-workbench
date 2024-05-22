#include "DAFigureWidgetOverlayChartEditor.h"
#include "DAChartWidget.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHash>
#include <QCoreApplication>
#include <QDebug>
#define DAFigureWidgetOverlayChartEditor_DebugPrint 0
namespace DA
{

class DAFigureWidgetOverlayChartEditor::PrivateData
{
	DA_DECLARE_PUBLIC(DAFigureWidgetOverlayChartEditor)
public:
	QPoint mLastMousePressPos { 0, 0 };      ///< 记录最后一次窗口移动的坐标
	QBrush mContorlPointBrush { Qt::blue };  ///< 绘制chart2d在编辑模式下控制点的画刷
	QPen mBorderPen { Qt::blue };            ///< 绘制chart2d在编辑模式下的画笔
	bool mIsStartResize { false };           ///< 标定开始进行缩放
	QWidget* mActiveWidget { nullptr };      /// 标定当前激活的窗口，如果没有就为nullptr
	QRect mOldGeometry;                      ///< 保存旧的窗口位置，用于redo/undo
	QSize mControlPointSize { 8, 8 };        ///< 控制点大小
	DAFigureWidgetOverlayChartEditor::ControlType mControlType { DAFigureWidgetOverlayChartEditor::OutSide };  ///< 记录当前缩放窗口的位置情况

	bool mFigOldMouseTracking { false };  /// 记录原来fig是否设置了mousetrack
	bool mFigOldHasHoverAttr { false };   ///< 记录原来的figure是否含有Hover属性
	bool mShowPrecentText { true };       ///< 显示占比文字
	PrivateData(DAFigureWidgetOverlayChartEditor* p) : q_ptr(p)
	{
	}
};

//===================================================
// DAFigureWidgetChartRubberbandEditOverlay
//===================================================

DAFigureWidgetOverlayChartEditor::DAFigureWidgetOverlayChartEditor(DAFigureWidget* fig)
    : DAFigureWidgetOverlay(fig), DA_PIMPL_CONSTRUCT
{
	// setAttribute( Qt::WA_TransparentForMouseEvents,false);
	setFocusPolicy(Qt::ClickFocus);
	// 由于QwtWidgetOverlay是对鼠标隐藏的，因此不能直接使用mouseEvent，直接捕获DAFigureWidget的event
	//  QwtWidgetOverlay已经install了，因此无需再fig->installEventFilter(this);
	d_ptr->mFigOldMouseTracking = fig->hasMouseTracking();
	if (!d_ptr->mFigOldMouseTracking) {
		fig->setMouseTracking(true);
	}
	d_ptr->mFigOldHasHoverAttr = fig->testAttribute(Qt::WA_Hover);
	if (!d_ptr->mFigOldHasHoverAttr) {
		fig->setAttribute(Qt::WA_Hover, true);
	}
	DAChartWidget* curChart = figure()->getCurrentChart();
	if (curChart) {
		setActiveWidget(curChart);
	} else {
		selectNextChart();
		if (!getCurrentActiveWidget()) {
			selectNextWidget();
		}
	}
}

DAFigureWidgetOverlayChartEditor::~DAFigureWidgetOverlayChartEditor()
{
	// 如果fig原来没有mousetrack，设置回去
	if (!d_ptr->mFigOldMouseTracking) {
		figure()->setMouseTracking(false);
	}
	if (!d_ptr->mFigOldHasHoverAttr) {
		figure()->setAttribute(Qt::WA_Hover, false);
	}
}

void DAFigureWidgetOverlayChartEditor::drawChartEditMode(QPainter* painter, const QRect& chartRect) const
{
	painter->setBrush(Qt::NoBrush);
	painter->setPen(d_ptr->mBorderPen);
	QRect edgetRect = chartRect.adjusted(-1, -1, 1, 1);

	// 绘制矩形边框
	painter->drawRect(edgetRect);
	// 绘制边框到figure四周
	QPen linePen(d_ptr->mBorderPen);

	linePen.setStyle(Qt::DotLine);
	painter->setPen(linePen);
	QPoint center = chartRect.center();

	painter->drawLine(center.x(), 0, center.x(), chartRect.top());            // top
	painter->drawLine(center.x(), chartRect.bottom(), center.x(), height());  // bottom
	painter->drawLine(0, center.y(), chartRect.left(), center.y());           // left
	painter->drawLine(chartRect.right(), center.y(), width(), center.y());    // right
	// 绘制顶部数据
	QRectF percent  = DAFigureContainer::calcRectPosPercentByOtherRect(rect(), chartRect);
	QFontMetrics fm = painter->fontMetrics();
	// top text
	QString percentText = QString::number(percent.y() * 100, 'g', 2) + "%";
	QRectF textRect     = fm.boundingRect(percentText);
	textRect.moveTopLeft(QPoint(center.x(), 0));
	painter->drawText(textRect, Qt::AlignCenter, percentText);
	// left
	percentText = QString::number(percent.x() * 100, 'g', 2) + "%";
	textRect    = fm.boundingRect(percentText);
	textRect.moveBottomLeft(QPoint(0, center.y()));
	painter->drawText(textRect, Qt::AlignCenter, percentText);

	//    painter->drawText(QPointF(0, chartRect.y()), QString::number(percent.x(), 'g', 2));
	// 绘制四个角落
	painter->setPen(Qt::NoPen);
	painter->setBrush(d_ptr->mContorlPointBrush);
	QRect connerRect(0, 0, d_ptr->mControlPointSize.width(), d_ptr->mControlPointSize.height());
	QPoint offset = QPoint(d_ptr->mControlPointSize.width() / 2, d_ptr->mControlPointSize.height() / 2);
	connerRect.moveTo(edgetRect.topLeft() - offset);
	painter->drawRect(connerRect);
	connerRect.moveTo(edgetRect.topRight() - offset);
	painter->drawRect(connerRect);
	connerRect.moveTo(edgetRect.bottomLeft() - offset);
	painter->drawRect(connerRect);
	connerRect.moveTo(edgetRect.bottomRight() - offset);
	painter->drawRect(connerRect);
}

///
/// \brief 根据范围获取图标
/// \param rr 范围
/// \return 图标
///
Qt::CursorShape DAFigureWidgetOverlayChartEditor::controlTypeToCursor(DAFigureWidgetOverlayChartEditor::ControlType rr)
{
	switch (rr) {
	case ControlLineTop:
	case ControlLineBottom:
		return (Qt::SizeVerCursor);

	case ControlLineLeft:
	case ControlLineRight:
		return (Qt::SizeHorCursor);

	case ControlPointTopLeft:
	case ControlPointBottomRight:
		return (Qt::SizeFDiagCursor);

	case ControlPointTopRight:
	case ControlPointBottomLeft:
		return (Qt::SizeBDiagCursor);

	case Inner:
		return (Qt::SizeAllCursor);

	default:
		break;
	}
	return (Qt::ArrowCursor);
}

///
/// \brief 根据点和矩形的关系，返回图标的样式
/// \param pos 点
/// \param region 矩形区域
/// \param err 允许误差
/// \return
///
DAFigureWidgetOverlayChartEditor::ControlType DAFigureWidgetOverlayChartEditor::getPositionControlType(const QPoint& pos,
                                                                                                       const QRect& region,
                                                                                                       int err)
{
	if (!region.adjusted(-err, -err, err, err).contains(pos)) {
		return (OutSide);
	}
	if (pos.x() < (region.left() + err)) {
		if (pos.y() < region.top() + err) {
			return (ControlPointTopLeft);
		} else if (pos.y() > region.bottom() - err) {
			return (ControlPointBottomLeft);
		}
		return (ControlLineLeft);
	} else if (pos.x() > (region.right() - err)) {
		if (pos.y() < region.top() + err) {
			return (ControlPointTopRight);
		} else if (pos.y() > region.bottom() - err) {
			return (ControlPointBottomRight);
		}
		return (ControlLineRight);
	} else if (pos.y() < (region.top() + err)) {
		if (pos.x() < region.left() + err) {
			return (ControlPointTopLeft);
		} else if (pos.x() > region.right() - err) {
			return (ControlPointTopRight);
		}
		return (ControlLineTop);
	} else if (pos.y() > region.bottom() - err) {
		if (pos.x() < region.left() + err) {
			return (ControlPointBottomLeft);
		} else if (pos.x() > region.right() - err) {
			return (ControlPointBottomRight);
		}
		return (ControlLineBottom);
	}
	return (Inner);
}

///
/// \brief 判断点是否在矩形区域的边缘
/// \param pos 点
/// \param region 矩形区域
/// \param err 允许误差
/// \return 如果符合边缘条件，返回true
///
bool DAFigureWidgetOverlayChartEditor::isPointInRectEdget(const QPoint& pos, const QRect& region, int err)
{
	if (!region.adjusted(-err, -err, err, err).contains(pos)) {
		return (false);
	}
	if ((pos.x() < (region.left() - err)) && (pos.x() < (region.left() + err))) {
		return (true);
	} else if ((pos.x() > (region.right() - err)) && (pos.x() < (region.right() + err))) {
		return (true);
	} else if ((pos.y() > (region.top() - err)) && (pos.y() < (region.top() + err))) {
		return (true);
	} else if ((pos.y() > region.bottom() - err) && (pos.y() < region.bottom() + err)) {
		return (true);
	}
	return (false);
}

/**
 * @brief 选择下一个窗口作为激活窗体
 * @param forward
 */
void DAFigureWidgetOverlayChartEditor::selectNextWidget(bool forward)
{
	QList< QWidget* > ws = figure()->getWidgetList();
	if (ws.isEmpty()) {
		setActiveWidget(nullptr);
		return;
	}
	if (d_ptr->mActiveWidget) {
		// 说明有窗口，找到这个窗口的下一个
		auto ite = std::find(ws.begin(), ws.end(), d_ptr->mActiveWidget);
		if (ite != ws.end()) {
			// 说明找到了
			if (forward) {
				// 向前寻找
				++ite;
				if (ite == ws.end()) {
					ite = ws.begin();  // 找到最后就把最前的给它
				}
			} else {
				// 向后寻找
				if (ite != ws.begin()) {
					--ite;
				} else {
					// 如果ite是最前，就把最后一个给予
					ite = ws.end() - 1;
				}
			}
		}
		setActiveWidget(*ite);
	} else {
		// 原来就没有激活窗口，给第一个
		setActiveWidget(ws.first());
	}
}

/**
 * @brief 选择下一个绘图作为激活窗体
 * @param forward
 */
void DAFigureWidgetOverlayChartEditor::selectNextChart(bool forward)
{
	QList< DAChartWidget* > ws = figure()->getCharts();
	if (ws.isEmpty()) {
		setActiveWidget(nullptr);
		return;
	}
	if (d_ptr->mActiveWidget) {
		// 说明有窗口，找到这个窗口的下一个
		auto ite = std::find(ws.begin(), ws.end(), d_ptr->mActiveWidget);
		if (ite != ws.end()) {
			// 说明找到了
			if (forward) {
				// 向前寻找
				++ite;
				if (ite == ws.end()) {
					ite = ws.begin();  // 找到最后就把最前的给它
				}
			} else {
				// 向后寻找
				if (ite != ws.begin()) {
					--ite;
				} else {
					// 如果ite是最前，就把最后一个给予
					ite = ws.end() - 1;
				}
			}
		}
		setActiveWidget(*ite);
	} else {
		// 原来就没有激活窗口，给第一个
		setActiveWidget(ws.first());
	}
}

/**
 * @brief 获取当前激活的窗体
 * @return
 */
QWidget* DAFigureWidgetOverlayChartEditor::getCurrentActiveWidget() const
{
    return d_ptr->mActiveWidget;
}

/**
 * @brief 显示占比数值
 * @param on
 */
void DAFigureWidgetOverlayChartEditor::showPercentText(bool on)
{
	d_ptr->mShowPrecentText = on;
	updateOverlay();
}

/**
 * @brief 设置边框的画笔
 * @param p
 */
void DAFigureWidgetOverlayChartEditor::setBorderPen(const QPen& p)
{
    d_ptr->mBorderPen = p;
}

/**
 * @brief 边框的画笔
 * @param p
 */
QPen DAFigureWidgetOverlayChartEditor::getBorderPen() const
{
    return d_ptr->mBorderPen;
}

/**
 * @brief 设置控制点的填充
 * @param b
 */
void DAFigureWidgetOverlayChartEditor::setControlPointBrush(const QBrush& b)
{
    d_ptr->mContorlPointBrush = b;
}

/**
 * @brief 控制点的填充
 * @param b
 */
QBrush DAFigureWidgetOverlayChartEditor::getControlPointBrush() const
{
    return d_ptr->mContorlPointBrush;
}

void DAFigureWidgetOverlayChartEditor::drawOverlay(QPainter* p) const
{
	if (d_ptr->mActiveWidget) {
		// 对于激活的窗口，绘制到四周的距离提示线
		p->save();
		drawChartEditMode(p, d_ptr->mActiveWidget->frameGeometry());
		p->restore();
	}
}

QRegion DAFigureWidgetOverlayChartEditor::maskHint() const
{
    return (figure()->rect());
}

/**
 * @brief DAFigureWidgetChartRubberbandEditOverlay::eventFilter
 *
 * 注意，鼠标移动事件在setMouseTracking(true)后，button永远是NoButton,需要配合press事件才能判断
 * @param obj
 * @param event
 * @return
 */
bool DAFigureWidgetOverlayChartEditor::eventFilter(QObject* obj, QEvent* event)
{
	if (d_ptr->mActiveWidget) {
		if (obj == figure()) {
#if DAFigureWidgetOverlayChartEditor_DebugPrint
			qDebug() << "Overlay eventFilter=" << event->type();
#endif
			// 捕获DAChartWidget和DAFigure都是无法捕获正常鼠标移动的事件
			switch (event->type()) {
			case QEvent::MouseButtonPress: {
				QMouseEvent* me = static_cast< QMouseEvent* >(event);
				return (onMousePressedEvent(me));
			}

			case QEvent::MouseButtonRelease: {
				QMouseEvent* me = static_cast< QMouseEvent* >(event);
				return (onMouseReleaseEvent(me));
			}

			case QEvent::KeyPress: {
				QKeyEvent* ke = static_cast< QKeyEvent* >(event);
				return (onKeyPressedEvent(ke));
			}

			case QEvent::MouseMove: {
				QMouseEvent* e = static_cast< QMouseEvent* >(event);
				return (onMouseMoveEvent(e));
			}

			case QEvent::HoverMove: {
				QHoverEvent* e = static_cast< QHoverEvent* >(event);
				return (onHoverMoveEvent(e));
			}
			default:
				break;
			}
		}
	}

	// QwtWidgetOverlay也继承了eventFilter
	return (QwtWidgetOverlay::eventFilter(obj, event));
}

bool DAFigureWidgetOverlayChartEditor::onMouseMoveEvent(QMouseEvent* me)
{
	if (d_ptr->mActiveWidget) {
		if (d_ptr->mIsStartResize) { }
	}
	return (true);  // 托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onMouseReleaseEvent(QMouseEvent* me)
{
	if (Qt::LeftButton == me->button()) {
		if (d_ptr->mIsStartResize) {
			d_ptr->mIsStartResize = false;
			if (d_ptr->mActiveWidget) {
				QRect newGeometry = d_ptr->mActiveWidget->geometry();
				emit widgetGeometryChanged(d_ptr->mActiveWidget, d_ptr->mOldGeometry, newGeometry);
				return (true);  // 这里把消息截取不传递下去
			}
		}
	}
	return (true);  // 托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onMousePressedEvent(QMouseEvent* me)
{
	if (Qt::LeftButton == me->button()) {
		// 左键点击
		QList< QWidget* > orderws = figure()->getOrderedWidgetList();
		for (auto ite = orderws.begin(); ite != orderws.end(); ++ite) {
			// 点击到了其他窗体
			if ((*ite)->geometry().contains(me->pos())) {
				setActiveWidget(*ite);
			}
		}
		ControlType ct = getPositionControlType(me->pos(), d_ptr->mActiveWidget->frameGeometry(), 4);
		if (OutSide == ct) {
			// 如果点击了外部，那么久尝试变更激活窗口
			QList< QWidget* > ws = figure()->getWidgetList();
			for (QWidget* w : qAsConst(ws)) {
				if (w->frameGeometry().contains(me->pos(), true)) {
					if (d_ptr->mActiveWidget != w) {
						setActiveWidget(w);
						updateOverlay();
						return (true);  // 这里把消息截取不传递下去
					}
				}
			}
		} else {
			// 点击了其他区域，就执行变换
			d_ptr->mOldGeometry       = d_ptr->mActiveWidget->geometry();
			d_ptr->mLastMousePressPos = me->pos();
			d_ptr->mIsStartResize     = true;
			d_ptr->mControlType       = ct;
			return (true);  // 这里把消息截取不传递下去
		}

		// 没有点击到任何的地方就
	}
	return (true);  // 托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onHoverMoveEvent(QHoverEvent* me)
{
	if (d_ptr->mActiveWidget) {
		if (d_ptr->mIsStartResize) {
			// 开始resize（鼠标按下左键后触发为true）
			//! 注意，不要在onMouseMoveEvent进行处理，因为鼠标移动到子窗体后，
			//! onMouseMoveEvent不会触发，但onHoverMoveEvent还会继续触发
#if DAFigureWidgetOverlayChartEditor_DebugPrint
			qDebug() << "DAFigureWidgetChartRubberbandEditOverlay::onHoverMoveEvent(" << me->pos() << ")"
					 << "\n   lastMousePressPos=" << d_ptr->_lastMousePressPos
					 << "\n   oldGeometry=" << d_ptr->_oldGeometry << "\n   controlType=" << d_ptr->_controlType;
#endif
			QRect geoRect = d_ptr->mOldGeometry;
			switch (d_ptr->mControlType) {
			case ControlLineTop: {
				QPoint offset = Qt5Qt6Compat_QXXEvent_Pos(me) - d_ptr->mLastMousePressPos;
				geoRect.adjust(0, offset.y(), 0, 0);
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case ControlLineBottom: {
				int resultY = Qt5Qt6Compat_QXXEvent_y(me);
				geoRect.adjust(0, 0, 0, resultY - geoRect.bottom());
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case ControlLineLeft: {
				int resultX = Qt5Qt6Compat_QXXEvent_x(me);
				geoRect.adjust(resultX - geoRect.left(), 0, 0, 0);
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case ControlLineRight: {
				int resultX = Qt5Qt6Compat_QXXEvent_x(me);
				geoRect.adjust(0, 0, resultX - geoRect.right(), 0);
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case ControlPointTopLeft: {
				geoRect.adjust(Qt5Qt6Compat_QXXEvent_x(me) - geoRect.left(), Qt5Qt6Compat_QXXEvent_y(me) - geoRect.top(), 0, 0);
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case ControlPointTopRight: {
				geoRect.adjust(0,
							   Qt5Qt6Compat_QXXEvent_y(me) - geoRect.top(),
							   Qt5Qt6Compat_QXXEvent_x(me) - geoRect.right(),
							   0);
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case ControlPointBottomLeft: {
				geoRect.adjust(Qt5Qt6Compat_QXXEvent_x(me) - geoRect.left(),
							   0,
							   0,
							   Qt5Qt6Compat_QXXEvent_y(me) - geoRect.bottom());
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case ControlPointBottomRight: {
				geoRect.adjust(0,
							   0,
							   Qt5Qt6Compat_QXXEvent_x(me) - geoRect.right(),
							   Qt5Qt6Compat_QXXEvent_y(me) - geoRect.bottom());
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			case Inner: {
				QPoint offset = Qt5Qt6Compat_QXXEvent_Pos(me) - d_ptr->mLastMousePressPos;
				geoRect.moveTo(d_ptr->mOldGeometry.topLeft() + offset);
				d_ptr->mActiveWidget->setGeometry(geoRect);
				break;
			}

			default:
				return (false);
			}
			updateOverlay();
			return (true);  // 这里把消息截取不传递下去
		} else {
			ControlType ct = getPositionControlType(Qt5Qt6Compat_QXXEvent_Pos(me), d_ptr->mActiveWidget->frameGeometry(), 4);
			if (d_ptr->mControlType != ct) {
				// 说明控制点变更
				Qt::CursorShape cur = controlTypeToCursor(ct);
				figure()->setCursor(cur);
				d_ptr->mControlType = ct;
			}
		}
	}
	return (true);  // 托管所有的鼠标事件
}

bool DAFigureWidgetOverlayChartEditor::onKeyPressedEvent(QKeyEvent* ke)
{
#if DAFigureWidgetOverlayChartEditor_DebugPrint
	qDebug() << "DAFigureWidgetChartRubberbandEditOverlay::onKeyPressedEvent:key=" << Qt::Key(ke->key());
#endif
	switch (ke->key()) {
	case Qt::Key_Return: {
		selectNextWidget(true);
	} break;

	case Qt::Key_Up:
	case Qt::Key_Left:
		selectNextWidget(true);
		break;

	case Qt::Key_Right:
	case Qt::Key_Down:
		selectNextWidget(false);
		break;

	default:
		return (false);
	}
	return (true);  // 这里把消息截取不传递下去
}

/**
 * @brief 控制点尺寸
 * @default 8*8
 * @return
 */
QSize DAFigureWidgetOverlayChartEditor::getControlPointSize() const
{
    return d_ptr->mControlPointSize;
}

/**
 * @brief 设置控制点尺寸
 * @param c
 */
void DAFigureWidgetOverlayChartEditor::setControlPointSize(const QSize& c)
{
    d_ptr->mControlPointSize = c;
}

/**
 * @brief 改变激活窗口
 * @param w 如果w和当前的activeWidget一样，不做任何动作
 * @note 此函数会发射信号activeWidgetChanged
 * @sa activeWidgetChanged
 */
void DAFigureWidgetOverlayChartEditor::setActiveWidget(QWidget* w)
{
	if (w == d_ptr->mActiveWidget) {
		// 避免嵌套
		return;
	}
	QWidget* oldact      = d_ptr->mActiveWidget;
	d_ptr->mActiveWidget = w;
	updateOverlay();
	emit activeWidgetChanged(oldact, w);
}

}
