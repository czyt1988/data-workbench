#ifndef DACHARTADDSTATSBARPLOTWIDGET_H
#define DACHARTADDSTATSBARPLOTWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsBarplotWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style bar plot / count plot settings widget (plan10)
 *
 * Collects seaborn ``barplot`` / ``countplot`` parameters (X column,
 * optional Y column, hue grouping, estimator, CI, orientation, palette,
 * error bar colour, bar colour, bar width, legend) and delegates the
 * computation to the Python ``DAWorkbench.DAPlotting.barplot`` module.
 * Emits plotRequested() on OK; the DAAppController slot acquires the GIL,
 * imports the module and calls ``plot(df, column, chart, **params)``.
 *
 * When ``y_column`` is empty the widget operates in **countplot** mode
 * (value counts); otherwise it is a **barplot** (grouped aggregation).
 */
class DAGUI_API DAChartAddStatsBarplotWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsBarplotWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsBarplotWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();
    /// Toggle barplot/countplot mode based on Y-column selection
    void onYColumnChanged();
    /// Enable/disable palette / colour controls depending on hue checkbox
    void onHueToggled(bool checked);
    /// Enable/disable CI-related controls
    void onCiChanged(int index);

private:
    Ui::DAChartAddStatsBarplotWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSBARPLOTWIDGET_H
