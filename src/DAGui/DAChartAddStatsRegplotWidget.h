#ifndef DACHARTADDSTATSREGPLOTWIDGET_H
#define DACHARTADDSTATSREGPLOTWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractStatsChartAddWidget.h"

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
 * reg_kws, colour) and delegates the computation to the Python
 * ``DAWorkbench.DAPlotting.regplot`` module. Emits plotRequested() on
 * OK; the DAAppController slot acquires the GIL, imports the module and
 * calls ``plot(df, column, chart, **params)``.
 */
class DAGUI_API DAChartAddStatsRegplotWidget : public DAAbstractStatsChartAddWidget
{
    Q_OBJECT
public:
    explicit DAChartAddStatsRegplotWidget(QWidget* parent = nullptr);
    ~DAChartAddStatsRegplotWidget();

    /// Re-implemented to forward the data manager to the series-select widgets.
    virtual void setDataManager(DADataManager* dmgr) override;

    /// Collect every UI field into a JSON object suitable for Python.
    virtual QJsonObject buildPlotParams() const override;

private Q_SLOTS:
    void onButtonBoxAccepted();
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
