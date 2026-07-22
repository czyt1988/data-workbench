#ifndef DACHARTADDSTATSECDFPLOTWIDGET_H
#define DACHARTADDSTATSECDFPLOTWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsEcdfplotWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style empirical CDF plot settings widget (plan12)
 *
 * Collects seaborn ``ecdfplot`` parameters (data column, hue grouping,
 * stat, complementary, weights, palette, colour, line width, legend) and
 * delegates the computation to
 * DAStatsPlotCoordinator. Emits plotRequested() on
 * OK; the DAAppController slot invokes the coordinator which renders via
 * DAChartPlotRenderer with DAStatistics support.
 */
class DAGUI_API DAChartAddStatsEcdfplotWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsEcdfplotWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsEcdfplotWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();
    /// Enable/disable palette / colour controls depending on hue checkbox
    void onHueToggled(bool checked);

private:
    Ui::DAChartAddStatsEcdfplotWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSECDFPLOTWIDGET_H
