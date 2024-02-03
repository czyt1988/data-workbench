#include "DAEvenFilterDragPlotWithGuide.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include "MimeData/DAMimeDataForData.h"
#include "MimeData/DAMimeDataFormats.h"
#include "Dialog/DADialogChartGuide.h"
#include "DAWaitCursorScoped.h"
namespace DA
{
DAEvenFilterDragPlotWithGuide::DAEvenFilterDragPlotWithGuide(QObject* par) : QObject(par)
{
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
        qDebug() << "DAAppFigureWidget::dragEnterEvent get unknow format:" << mimeData->formats();
    }
}

bool DAEvenFilterDragPlotWithGuide::dragMoveEvent(QDragMoveEvent* e, DAFigureWidget* fig)
{
}

bool DAEvenFilterDragPlotWithGuide::dragLeaveEvent(QDragLeaveEvent* e, DAFigureWidget* fig)
{
}

bool DAEvenFilterDragPlotWithGuide::dropEvent(QDropEvent* e, DAFigureWidget* fig)
{
}

}  // end DA
