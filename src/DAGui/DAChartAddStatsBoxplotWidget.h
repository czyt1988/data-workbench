#ifndef DACHARTADDSTATSBOXPLOTWIDGET_H
#define DACHARTADDSTATSBOXPLOTWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsBoxplotWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style box plot settings widget (plan07)
 *
 * Collects seaborn ``boxplot`` parameters (multiple data columns, hue
 * grouping, whisker multiplier, showfliers / showmeans toggles,
 * box width) and delegates the computation to
 * DAStatsPlotCoordinator. Emits plotRequested() on OK;
 * the DAAppController slot invokes the coordinator which renders via
 * DAChartPlotRenderer with DAStatistics support.
 */
class DAGUI_API DAChartAddStatsBoxplotWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsBoxplotWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsBoxplotWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();

private:
    Ui::DAChartAddStatsBoxplotWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSBOXPLOTWIDGET_H
