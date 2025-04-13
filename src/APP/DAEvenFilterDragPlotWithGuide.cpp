#include "DAEvenFilterDragPlotWithGuide.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include "DAFigureWidget.h"
#include "DAAppChartOperateWidget.h"
#include "MimeData/DAMimeDataForData.h"
#include "MimeData/DAMimeDataFormats.h"
#include "DAChartWidget.h"

namespace DA
{
DAEvenFilterDragPlotWithGuide::DAEvenFilterDragPlotWithGuide(QObject* par) : QObject(par)
{
}

/**
 * @brief 设置ChartOptWidget
 * @param c
 */
void DAEvenFilterDragPlotWithGuide::setChartOptWidget(DAAppChartOperateWidget* c)
{
    mChartOptWidget = c;
}

bool DAEvenFilterDragPlotWithGuide::eventFilter(QObject* obj, QEvent* event)
{
	switch (event->type()) {
	case QEvent::DragEnter: {
		QDragEnterEvent* e = static_cast< QDragEnterEvent* >(event);
		if (DAFigureWidget* fig = qobject_cast< DAFigureWidget* >(obj)) {
			return dragEnterEvent(e, fig);
		}
	} break;
	case QEvent::DragMove: {
		QDragMoveEvent* e = static_cast< QDragMoveEvent* >(event);
		if (DAFigureWidget* fig = qobject_cast< DAFigureWidget* >(obj)) {
			return dragMoveEvent(e, fig);
		}
	} break;
	case QEvent::DragLeave: {
		QDragLeaveEvent* e = static_cast< QDragLeaveEvent* >(event);
		if (DAFigureWidget* fig = qobject_cast< DAFigureWidget* >(obj)) {
			return dragLeaveEvent(e, fig);
		}
	} break;
	case QEvent::Drop: {
		QDropEvent* e = static_cast< QDropEvent* >(event);
		if (DAFigureWidget* fig = qobject_cast< DAFigureWidget* >(obj)) {
			return dropEvent(e, fig);
		}
	} break;
	default:
		break;
	}
	return QObject::eventFilter(obj, event);
}

bool DAEvenFilterDragPlotWithGuide::dragEnterEvent(QDragEnterEvent* e, DAFigureWidget* fig)
{
	if (!e) {
		return false;
	}
	if (e->source() == this || nullptr == e->source()) {
		return false;
	}
	const QMimeData* mimeData = e->mimeData();
	if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATA)) {
		e->acceptProposedAction();
	} else {
		qDebug() << "DAEvenFilterDragPlotWithGuide::dragEnterEvent get unknow format:" << mimeData->formats();
	}
	return true;
}

bool DAEvenFilterDragPlotWithGuide::dragMoveEvent(QDragMoveEvent* e, DAFigureWidget* fig)
{
	if (!e) {
		return false;
	}
	if (e->source() == this || nullptr == e->source()) {
		return false;
	}
	const QMimeData* mimeData = e->mimeData();
	if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATA)) {
		// 数据
		QWidget* w = fig->getWidgetUnderPos(Qt5Qt6Compat_QXXEvent_Pos(e));
		if (nullptr == w) {
			e->setDropAction(Qt::IgnoreAction);
			return true;
		}
		if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(w)) {
			Q_UNUSED(chart);
			e->setDropAction(Qt::CopyAction);
			e->accept();
			return true;
		}
	}
	e->setDropAction(Qt::IgnoreAction);
	return true;
}

bool DAEvenFilterDragPlotWithGuide::dragLeaveEvent(QDragLeaveEvent* e, DAFigureWidget* fig)
{
	if (!e) {
		return false;
	}
	e->accept();
	return true;
}

bool DAEvenFilterDragPlotWithGuide::dropEvent(QDropEvent* e, DAFigureWidget* fig)
{
	if (!e) {
		return false;
	}
	if (e->source() == this || nullptr == e->source()) {
		return false;
	}

	const QMimeData* mimeData = e->mimeData();
	if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATA)) {
		// 数据
		const DAMimeDataForData* datamime = qobject_cast< const DAMimeDataForData* >(mimeData);
		if (nullptr == datamime) {
			return false;
		}
		QWidget* w = fig->getWidgetUnderPos(Qt5Qt6Compat_QXXEvent_Pos(e));
		if (nullptr == w) {
			return false;
		}
		if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(w)) {
			if (fig->getCurrentChart() != chart) {
				// 如果当前绘图不是放下的绘图，则把当前绘图设置为放下数据的绘图
				fig->setCurrentChart(chart);
			}
			if (!mChartOptWidget) {
				return false;
			}
			QwtPlotItem* pi = mChartOptWidget->createPlotItemWithGuideDialog(datamime->getDAData());
			if (nullptr == pi) {
				e->ignore();
				return false;
			} else {
				// 加入
				fig->addItem_(pi);
				fig->gca()->zoomInCompatible();
			}
			e->acceptProposedAction();
		}
	}
	return true;
}

}  // end DA
