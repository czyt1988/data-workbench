#include "Chart/AddItem/DAChartAddStatsEcdfplotWidget.h"
#include "ui_DAChartAddStatsEcdfplotWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

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

DAStatsPlotRequest* DAChartAddStatsEcdfplotWidget::createPlotRequest() const
{
    auto* req = new DAStatsEcdfplotRequest;

    // Data column (single-series selection)
    QPair< DAData, QString > sel = ui->selectWidgetData->getCurrentSeries();
    req->data    = sel.first;
    req->figure  = getFigureWidget();
    req->chart   = getChartWidget();
    req->column  = sel.second;

    // Hue column (only if the groupbox is checked and a column is selected)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            req->hue = hueSel.second;
        }
    }

    // Stat
    req->stat = ui->comboBoxStat->currentText();

    // Complementary
    req->complementary = ui->checkBoxComplementary->isChecked();

    // Line width
    req->lineWidth = ui->doubleSpinBoxLineWidth->value();

    // Legend
    req->legend = ui->checkBoxLegend->isChecked();

    // Weights (optional)
    if (ui->groupBoxWeights->isChecked()) {
        QPair< DAData, QString > wSel = ui->selectWidgetWeights->getCurrentSeries();
        if (wSel.first.isDataFrame() && !wSel.second.isEmpty()) {
            req->weights = wSel.second;
        }
    }

    return req;
}

}  // namespace DA
