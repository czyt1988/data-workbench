#include "DAEvenFilterDragPlotWithGuide.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QTimer>
#include "DAFigureWidget.h"
#include "DAAppChartOperateWidget.h"
#include "MimeData/DAMimeDataForData.h"
#include "MimeData/DAMimeDataFormats.h"
#include "Dialog/DADialogChartGuide.h"
#include "DAAbstractChartAddItemWidget.h"
#include "DAChartAddCurveWidget.h"
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
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
    if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATAS)) {
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
    if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATAS)) {
        // 数据
        QWidget* w = fig->plotUnderPos(DA::compat::eventPos(e));
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
    QwtPlot* w = fig->plotUnderPos(DA::compat::eventPos(e));
    if (w) {
        // 先把current设置为当前光标的绘图
        fig->setCurrentChart(w);
    }
    const QMimeData* mimeData = e->mimeData();
    if (!mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATAS)) {
        return false;
    }
    // 数据
    const DAMimeDataForData* datamime = qobject_cast< const DAMimeDataForData* >(mimeData);
    if (nullptr == datamime) {
        return false;
    }

    if (datamime->isHaveDataframe()) {
        auto datas = datamime->getDataframes();
        if (datas.empty()) {
            return false;
        }
        DAData data = datas.first();
        e->acceptProposedAction();
        mChartOptWidget->showPlotGuideDialog();
    } else if (datamime->isHaveDataSeries()) {
        // 存在series
        const QList< QPair< DAData, QStringList > >& series = datamime->getDataSeries();
        if (series.isEmpty()) {
            return false;
        }
        // 情况1：只有1个series
        if (series.size() == 1) {
            DAData data            = series.first().first;
            QStringList serieNames = series.first().second;
            if (serieNames.size() > 0) {
                DAChartAddCurveWidget* addCurveWidget = getChartAddCurveWidget();
                if (addCurveWidget) {
                    if (serieNames.size() == 1) {
                        addCurveWidget->setY(data, serieNames.first());
                    } else {
                        addCurveWidget->setX(data, serieNames.first());
                        addCurveWidget->setY(data, serieNames[ 1 ]);
                    }
                }
            }
        } else {
            // 多个数据，各取
            DAData dataX = series[ 0 ].first;
            DAData dataY = series[ 1 ].first;
            if (!series[ 0 ].second.isEmpty() && !series[ 1 ].second.isEmpty()) {
                DAChartAddCurveWidget* addCurveWidget = getChartAddCurveWidget();
                if (addCurveWidget) {
                    addCurveWidget->setX(dataX, series[ 0 ].second.first());
                    addCurveWidget->setY(dataY, series[ 1 ].second.first());
                }
            }
        }
        mChartOptWidget->showPlotGuideDialog();
    }

    return true;
}

DAChartAddCurveWidget* DAEvenFilterDragPlotWithGuide::getChartAddCurveWidget()
{
    if (!mChartOptWidget) {
        return nullptr;
    }
    DADialogChartGuide* dlg = mChartOptWidget->getChartGuideDlg();
    if (!dlg) {
        return nullptr;
    }
    dlg->setCurrentChartType(DAChartTypes::Curve);
    // 获取curve对话框
    DAAbstractChartAddItemWidget* chartWidget = dlg->getChartAddItemWidget(DAChartTypes::Curve);
    return qobject_cast< DAChartAddCurveWidget* >(chartWidget);
}

}  // end DA
