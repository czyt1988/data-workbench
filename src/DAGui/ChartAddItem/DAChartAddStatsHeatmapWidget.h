#ifndef DACHARTADDSTATSHEATMAPWIDGET_H
#define DACHARTADDSTATSHEATMAPWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsHeatmapWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style heatmap settings widget (plan08)
 *
 * Collects seaborn ``heatmap`` parameters (x/y category columns, optional
 * value column, aggregation function, colour map, vmin/vmax, centre,
 * annotations, standardisation) and delegates the computation to
 * DAStatsPlotCoordinator. Emits plotRequested() on OK;
 * the DAAppController slot invokes the coordinator which renders via
 * DAChartPlotRenderer with DAStatistics support.
 */
class DAGUI_API DAChartAddStatsHeatmapWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsHeatmapWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsHeatmapWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();
    /// Enable/disable aggfunc when value column groupbox is toggled
    void onValueColumnToggled(bool checked);
    /// Enable/disable fmt edit when annot is toggled
    void onAnnotToggled(bool checked);

private:
    Ui::DAChartAddStatsHeatmapWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSHEATMAPWIDGET_H
