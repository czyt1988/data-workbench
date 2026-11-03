#include "Chart/AddItem/DAChartAddStatsHistplotWidget.h"
#include "ui_DAChartAddStatsHistplotWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

namespace DA
{

DAChartAddStatsHistplotWidget::DAChartAddStatsHistplotWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsHistplotWidget)
{
    ui->setupUi(this);

    // Data column: single-series selection
    ui->selectWidgetData->setRoleLabel(tr("Data"));  // cn: 数据
    ui->selectWidgetData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Hue column: also single-series selection
    ui->selectWidgetHue->setRoleLabel(tr("Hue"));  // cn: 分组
    ui->selectWidgetHue->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

}

DAChartAddStatsHistplotWidget::~DAChartAddStatsHistplotWidget()
{
    delete ui;
}

void DAChartAddStatsHistplotWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetData->setDataManager(dmgr);
    ui->selectWidgetHue->setDataManager(dmgr);
}

DAStatsPlotRequest* DAChartAddStatsHistplotWidget::createPlotRequest() const
{
    auto* req = new DAStatsHistplotRequest;

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

    // Bins
    req->bins     = ui->spinBoxBins->value();
    req->binwidth = ui->doubleSpinBoxBinwidth->value();
    double binMin = ui->doubleSpinBoxBinRangeMin->value();
    double binMax = ui->doubleSpinBoxBinRangeMax->value();
    if (binMin != 0.0 || binMax != 0.0) {
        req->binrange = qMakePair(binMin, binMax);
    }

    // Statistics
    req->stat       = ui->comboBoxStat->currentText();
    req->kde        = ui->checkBoxKde->isChecked();
    req->kdeBw      = ui->comboBoxKdeBw->currentText();
    req->cumulative = ui->checkBoxCumulative->isChecked();
    req->fill       = ui->checkBoxFill->isChecked();

    // Constants kept for parity with the old JSON contract (unconsumed)
    req->element  = QStringLiteral("bars");
    req->multiple = QStringLiteral("layer");

    return req;
}

}  // namespace DA
