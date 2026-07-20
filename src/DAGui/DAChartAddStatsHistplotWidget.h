#ifndef DACHARTADDSTATSHISTPLOTWIDGET_H
#define DACHARTADDSTATSHISTPLOTWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsHistplotWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style histogram settings widget (plan04)
 *
 * Unlike the existing DAChartAddHistogramWidget which computes binning in C++,
 * this widget collects seaborn-style parameters and delegates the computation
 * to the Python ``DAWorkbench.DAPlotting.histplot`` module. It emits
 * plotRequested() on OK; the DAAppController slot acquires the GIL, imports
 * the module and calls ``plot(df, column, chart, **params)``.
 */
class DAGUI_API DAChartAddStatsHistplotWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsHistplotWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsHistplotWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();

private:
    Ui::DAChartAddStatsHistplotWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSHISTPLOTWIDGET_H
