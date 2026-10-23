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
    QVector< QPointF > xy = getSeries();
    if (xy.empty()) {
        return nullptr;
    }
    QwtPlotCurve* item = new QwtPlotCurve();
    item->setSamples(xy);
    item->setTitle(getNameHint());
    return item;
}

/**
 * @brief 多条曲线批量创建（1 个 x + 多个 y）
 *
 * 每条 y 与 x 组成一条 QwtPlotCurve，曲线标题用 y 序列名（y 自增时为 "y"）；
 * 数据缺失或 y 与 x 长度不一致时由 getMultiSeries 弹出警告并返回空列表
 * @return 曲线列表，无法创建时为空
 */
QList< QwtPlotItem* > DAChartAddCurveWidget::createPlotItems()
{
    QList< QVector< QPointF > > xyList;
    QStringList names;
    if (!getMultiSeries(xyList, names)) {
        return {};
    }
    QList< QwtPlotItem* > items;
    items.reserve(xyList.size());
    for (int i = 0; i < xyList.size(); ++i) {
        if (xyList[ i ].empty()) {
            continue;
        }
        QwtPlotCurve* item = new QwtPlotCurve();
        item->setSamples(xyList[ i ]);
        item->setTitle(names.value(i));
        items.append(item);
    }
    return items;
}

}  // end DA
