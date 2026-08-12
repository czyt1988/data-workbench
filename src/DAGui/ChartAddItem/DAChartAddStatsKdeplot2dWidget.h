#ifndef DACHARTADDSTATSKDEPLOT2DWIDGET_H
#define DACHARTADDSTATSKDEPLOT2DWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsKdeplot2dWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style 2-D KDE settings widget (plan06)
 *
 * Collects seaborn ``kdeplot`` 2-D parameters (bandwidth, grid size, contour
 * levels, threshold, fill mode) and delegates the computation to
 * DAStatsPlotCoordinator. Emits
 * plotRequested() on OK; the DAAppController slot invokes the coordinator
 * which renders via DAChartPlotRenderer with DAStatistics support, where
 * ``column`` is the X-axis column and ``params["y_column"]`` carries the Y-axis column.
 */
class DAGUI_API DAChartAddStatsKdeplot2dWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsKdeplot2dWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsKdeplot2dWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();
    /// Enable/disable bw_value spin box depending on bw_method selection
    void onBwMethodChanged(int index);

private:
    Ui::DAChartAddStatsKdeplot2dWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSKDEPLOT2DWIDGET_H
