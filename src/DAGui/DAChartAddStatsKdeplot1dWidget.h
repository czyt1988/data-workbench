#ifndef DACHARTADDSTATSKDEPLOT1DWIDGET_H
#define DACHARTADDSTATSKDEPLOT1DWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsKdeplot1dWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style 1-D KDE settings widget (plan05)
 *
 * Collects seaborn ``kdeplot`` parameters (bandwidth, grid size, fill,
 * cumulative, hue grouping, palette, threshold clipping) and delegates the
 * computation to the Python ``DAWorkbench.DAPlotting.kdeplot_1d`` module.
 * Emits plotRequested() on OK; the DAAppController slot acquires the GIL,
 * imports the module and calls ``plot(df, column, chart, **params)``.
 */
class DAGUI_API DAChartAddStatsKdeplot1dWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsKdeplot1dWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsKdeplot1dWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();
    /// Enable/disable bw_value spin box depending on bw_method selection
    void onBwMethodChanged(int index);
    /// Enable/disable threshold and palette controls depending on fill / hue
    void onFillToggled(bool checked);

private:
    Ui::DAChartAddStatsKdeplot1dWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSKDEPLOT1DWIDGET_H
