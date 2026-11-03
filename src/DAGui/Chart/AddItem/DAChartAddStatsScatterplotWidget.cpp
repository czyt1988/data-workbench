#include "Chart/AddItem/DAChartAddStatsScatterplotWidget.h"
#include "ui_DAChartAddStatsScatterplotWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

namespace DA
{

DAChartAddStatsScatterplotWidget::DAChartAddStatsScatterplotWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsScatterplotWidget)
{
    ui->setupUi(this);

    // X column: single-series selection
    ui->selectWidgetXData->setRoleLabel(tr("X axis"));  // cn: X 轴
    ui->selectWidgetXData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Y column: single-series selection
    ui->selectWidgetYData->setRoleLabel(tr("Y axis"));  // cn: Y 轴
    ui->selectWidgetYData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Hue column: single-series selection
    ui->selectWidgetHue->setRoleLabel(tr("Hue"));  // cn: 分组
    ui->selectWidgetHue->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

}

DAChartAddStatsScatterplotWidget::~DAChartAddStatsScatterplotWidget()
{
    delete ui;
}

void DAChartAddStatsScatterplotWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetXData->setDataManager(dmgr);
    ui->selectWidgetYData->setDataManager(dmgr);
    ui->selectWidgetHue->setDataManager(dmgr);
}

DAStatsPlotRequest* DAChartAddStatsScatterplotWidget::createPlotRequest() const
{
    auto* req = new DAStatsScatterplotRequest;

    // X / Y columns (single-series selection)
    QPair< DAData, QString > selX = ui->selectWidgetXData->getCurrentSeries();
    req->data    = selX.first;
    req->figure  = getFigureWidget();
    req->chart   = getChartWidget();
    req->xColumn = selX.second;
    req->yColumn = ui->selectWidgetYData->getCurrentSeries().second;

    // Hue column (only if the groupbox is checked and a column is selected)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            req->hue = hueSel.second;
        }
    }

    // Marker options
    req->size   = ui->spinBoxSize->value();
    req->style  = ui->comboBoxStyle->currentText();
    req->alpha  = ui->doubleSpinBoxAlpha->value();
    req->legend = ui->checkBoxLegend->isChecked();

    return req;
}

}  // namespace DA
