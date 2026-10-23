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
#include "ChartAddItem/DAAbstractChartAddItemWidget.h"
#include "ChartAddItem/DAChartAddCurveWidget.h"
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
        DAChartAddCurveWidget* addCurveWidget = getChartAddCurveWidget();
        if (addCurveWidget) {
            if (series.size() == 1) {
                // 同一 dataframe 的多列：第一列作 x，其余全部作 y
                DAData data            = series.first().first;
                QStringList serieNames = series.first().second;
                if (serieNames.size() == 1) {
                    addCurveWidget->setY(data, serieNames.first());
                } else if (serieNames.size() > 1) {
                    addCurveWidget->setX(data, serieNames.first());
                    for (int i = 1; i < serieNames.size(); ++i) {
                        addCurveWidget->setY(data, serieNames[ i ]);
                    }
                }
            } else {
                // 多个数据源：第一个的第一列作 x，其余各数据源的列全部作 y
                DAData dataX = series[ 0 ].first;
                if (!series[ 0 ].second.isEmpty()) {
                    addCurveWidget->setX(dataX, series[ 0 ].second.first());
                }
                bool firstSkipped = false;
                for (const auto& p : series) {
                    for (const QString& name : p.second) {
                        if (!firstSkipped) {
                            // 跳过已作为 x 的第一列
                            firstSkipped = true;
                            continue;
                        }
                        addCurveWidget->setY(p.first, name);
                    }
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
