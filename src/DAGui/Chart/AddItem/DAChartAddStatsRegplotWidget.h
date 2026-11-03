#ifndef DACHARTADDSTATSREGPLOTWIDGET_H
#define DACHARTADDSTATSREGPLOTWIDGET_H
#include "DAGuiAPI.h"
#include "Chart/AddItem/DAAbstractStatsChartAddWidget.h"

namespace Ui
{
class DAChartAddStatsRegplotWidget;
}

namespace DA
{
class DADataManager;
class DAChartSeriesSelectWidget;

/**
 * @brief Statistics-style regression plot settings widget (plan11)
 *
 * Collects seaborn ``regplot`` parameters (X / Y columns, polynomial
 * order, CI, bootstrap iterations, scatter / fit toggles, scatter_kws,
 * reg_kws) and delegates the computation to
 * DAStatsPlotCoordinator. Emits plotRequested() on
 * OK; the DAAppController slot invokes the coordinator which renders via
 * DAChartPlotRenderer with DAStatistics support.
 */
class DAGUI_API DAChartAddStatsRegplotWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsRegplotWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsRegplotWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a typed DAStatsRegplotRequest.
    virtual DAStatsPlotRequest* createPlotRequest() const override;

private Q_SLOTS:
    /// Enable/disable n_boot when CI changes
    void onCiChanged(int index);
    /// Enable/disable scatter_kws panel
    void onScatterToggled(bool checked);
    /// Enable/disable reg_kws panel
    void onFitRegToggled(bool checked);

private:
    Ui::DAChartAddStatsRegplotWidget* ui;
};

}  // namespace DA

#endif  // DACHARTADDSTATSREGPLOTWIDGET_H
