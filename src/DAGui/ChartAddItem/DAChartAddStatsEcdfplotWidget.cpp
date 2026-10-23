#include "DAChartAddStatsEcdfplotWidget.h"
#include "ui_DAChartAddStatsEcdfplotWidget.h"

#include <QJsonObject>
#include <QVariant>
#include "DADataManager.h"
#include "DAChartSeriesSelectWidget.h"
#include "DAPySeriesListView.h"  // for AcceptMode
#include "DALogCategory.h"

namespace DA
{

DAChartAddStatsEcdfplotWidget::DAChartAddStatsEcdfplotWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsEcdfplotWidget)
{
    ui->setupUi(this);

    // Data column: single-series selection
    ui->selectWidgetData->setRoleLabel(tr("Data"));  // cn: 数据
    ui->selectWidgetData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Hue column: single-series selection
    ui->selectWidgetHue->setRoleLabel(tr("Hue"));  // cn: 分组
    ui->selectWidgetHue->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Weights column (optional): single-series selection
    ui->selectWidgetWeights->setRoleLabel(tr("Weights (optional)"));  // cn: 权重（可选）
    ui->selectWidgetWeights->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

}

DAChartAddStatsEcdfplotWidget::~DAChartAddStatsEcdfplotWidget()
{
    delete ui;
}

void DAChartAddStatsEcdfplotWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetData->setDataManager(dmgr);
    ui->selectWidgetHue->setDataManager(dmgr);
    ui->selectWidgetWeights->setDataManager(dmgr);
}

QJsonObject DAChartAddStatsEcdfplotWidget::buildPlotParams() const
{
    QJsonObject p;

    // Stat
    p["stat"] = ui->comboBoxStat->currentText();

    // Complementary
    p["complementary"] = ui->checkBoxComplementary->isChecked();

    // Line width
    p["line_width"] = ui->doubleSpinBoxLineWidth->value();

    // Legend
    p["legend"] = ui->checkBoxLegend->isChecked();

    // Hue grouping
    p["use_hue"] = ui->groupBoxHue->isChecked();

    // Weights (optional)
    if (ui->groupBoxWeights->isChecked()) {
        QPair< DAData, QString > wSel = ui->selectWidgetWeights->getCurrentSeries();
        if (wSel.first.isDataFrame() && !wSel.second.isEmpty()) {
            p["weights"] = wSel.second;
        }
    }

    return p;
}

void DAChartAddStatsEcdfplotWidget::onButtonBoxAccepted()
{
    // Validate: a data series must be selected
    QPair< DAData, QString > sel = ui->selectWidgetData->getCurrentSeries();
    if (!sel.first.isDataFrame() || sel.second.isEmpty()) {
        daWarning << tr("Please select a data column before plotting");  // cn: 请先选择数据列再绘图
        return;
    }

    QJsonObject params = buildPlotParams();
    // Stash the selected column name into params so the controller can read it
    params["column"] = sel.second;
    // Identify which Python module the controller should import
    params["__plot_type__"] = QStringLiteral("ecdfplot");

    // Hue column name (if enabled)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            params["hue"] = hueSel.second;
        }
    }

    Q_EMIT plotRequested(params, sel.first, getFigureWidget(), getChartWidget());
}

}  // namespace DA
