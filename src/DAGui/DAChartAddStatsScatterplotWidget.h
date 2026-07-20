#ifndef DACHARTADDSTATSSCATTERPLOTWIDGET_H
#define DACHARTADDSTATSSCATTERPLOTWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsScatterplotWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style scatter plot settings widget (plan09)
 *
 * Collects seaborn ``scatterplot`` parameters (X/Y columns, hue grouping,
 * marker size / style / colour, palette, alpha, legend toggle) and
 * delegates the computation to the Python
 * ``DAWorkbench.DAPlotting.scatterplot`` module. Emits plotRequested() on
 * OK; the DAAppController slot acquires the GIL, imports the module and
 * calls ``plot(df, column, chart, **params)``.
 */
class DAGUI_API DAChartAddStatsScatterplotWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsScatterplotWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsScatterplotWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();
    /// Enable/disable palette / colour controls depending on hue checkbox
    void onHueToggled(bool checked);

private:
    Ui::DAChartAddStatsScatterplotWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSSCATTERPLOTWIDGET_H
