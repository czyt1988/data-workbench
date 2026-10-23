#include "DAChartAddCurveWidget.h"
#include <QMessageBox>
#include "qwt_plot_curve.h"
namespace DA
{

//===================================================
// DAChartAddCurveWidget
//===================================================

DAChartAddCurveWidget::DAChartAddCurveWidget(QWidget* parent) : DAChartAddXYSeriesWidget(parent)
{
}

DAChartAddCurveWidget::~DAChartAddCurveWidget()
{
}

/**
 * @brief 此函数创建QwtPlotCurve
 * @return
 */
QwtPlotItem* DAChartAddCurveWidget::createPlotItem()
{
    DAChartSeriesResult res = extractSeries();
    if (!res.isOk()) {
        warnSeriesError(res.error);
        return nullptr;
    }
    if (res.points.empty()) {
        return nullptr;
    }
    QwtPlotCurve* item = new QwtPlotCurve();
    item->setSamples(res.points);
    item->setTitle(getNameHint());
    return item;
}

/**
 * @brief 多条曲线批量创建（1 个 x + 多个 y）
 *
 * 每条 y 与 x 组成一条 QwtPlotCurve，曲线标题用 y 序列名（y 自增时为 "y"）；
 * 取数失败时弹出警告并返回空列表
 * @return 曲线列表，无法创建时为空
 */
QList< QwtPlotItem* > DAChartAddCurveWidget::createPlotItems()
{
    DAChartMultiSeriesResult res = extractMultiSeries();
    if (!res.isOk()) {
        warnSeriesError(res.error);
        return {};
    }
    QList< QwtPlotItem* > items;
    items.reserve(res.series.size());
    for (int i = 0; i < res.series.size(); ++i) {
        if (res.series[ i ].empty()) {
            continue;
        }
        QwtPlotCurve* item = new QwtPlotCurve();
        item->setSamples(res.series[ i ]);
        item->setTitle(res.names.value(i));
        items.append(item);
    }
    return items;
}

}  // end DA
