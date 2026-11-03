#include "Chart/AddItem/DAChartAddStatsRegplotWidget.h"
#include "ui_DAChartAddStatsRegplotWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

namespace DA
{

DAChartAddStatsRegplotWidget::DAChartAddStatsRegplotWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsRegplotWidget)
{
    ui->setupUi(this);

    // X column: single-series selection (independent variable)
    ui->selectWidgetXData->setRoleLabel(tr("X axis"));  // cn: X 轴
    ui->selectWidgetXData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Y column: single-series selection (dependent variable)
    ui->selectWidgetYData->setRoleLabel(tr("Y axis"));  // cn: Y 轴
    ui->selectWidgetYData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // CI-related sync
    auto syncCi = [this]() {
        int ciIdx = ui->comboBoxCi->currentIndex();
        bool ciEnabled = (ciIdx > 0);  // index 0 = "None"
        ui->spinBoxNBoot->setEnabled(ciEnabled);
        ui->labelNBoot->setEnabled(ciEnabled);
    };
    syncCi();
    connect(ui->comboBoxCi, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAddStatsRegplotWidget::onCiChanged);

    // scatter_kws / reg_kws panel sync
    ui->groupBoxScatterKws->setEnabled(ui->checkBoxScatter->isChecked());
    ui->groupBoxRegKws->setEnabled(ui->checkBoxFitReg->isChecked());
    connect(ui->checkBoxScatter, &QCheckBox::toggled,
            this, &DAChartAddStatsRegplotWidget::onScatterToggled);
    connect(ui->checkBoxFitReg, &QCheckBox::toggled,
            this, &DAChartAddStatsRegplotWidget::onFitRegToggled);

}

DAChartAddStatsRegplotWidget::~DAChartAddStatsRegplotWidget()
{
    delete ui;
}

void DAChartAddStatsRegplotWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetXData->setDataManager(dmgr);
    ui->selectWidgetYData->setDataManager(dmgr);
}

DAStatsPlotRequest* DAChartAddStatsRegplotWidget::createPlotRequest() const
{
    auto* req = new DAStatsRegplotRequest;

    // X / Y columns (single-series selection)
    QPair< DAData, QString > selX = ui->selectWidgetXData->getCurrentSeries();
    req->data    = selX.first;
    req->figure  = getFigureWidget();
    req->chart   = getChartWidget();
    req->xColumn = selX.second;
    req->yColumn = ui->selectWidgetYData->getCurrentSeries().second;

    // Polynomial order
    req->order = ui->spinBoxOrder->value();

    // CI — index 0 = "None" (ci stays 0)
    const int ciIdx = ui->comboBoxCi->currentIndex();
    if (ciIdx > 0) {
        static const int ciValues[] = { 0, 68, 95, 99 };
        req->ci = ciValues[ ciIdx ];
    }
    req->nBoot = ui->spinBoxNBoot->value();

    // Toggles
    req->scatter = ui->checkBoxScatter->isChecked();
    req->fitReg   = ui->checkBoxFitReg->isChecked();

    // scatter_kws
    QVariantMap scatterKws;
    scatterKws[ "marker_size" ] = ui->spinBoxMarkerSize->value();
    scatterKws[ "alpha" ]        = ui->doubleSpinBoxAlpha->value();
    req->scatterKws = scatterKws;

    // reg_kws
    QVariantMap regKws;
    regKws[ "line_width" ] = ui->doubleSpinBoxLineWidth->value();
    req->regKws = regKws;

    return req;
}

void DAChartAddStatsRegplotWidget::onCiChanged(int index)
{
    Q_UNUSED(index);
    bool ciEnabled = (ui->comboBoxCi->currentIndex() > 0);
    ui->spinBoxNBoot->setEnabled(ciEnabled);
    ui->labelNBoot->setEnabled(ciEnabled);
}

void DAChartAddStatsRegplotWidget::onScatterToggled(bool checked)
{
    ui->groupBoxScatterKws->setEnabled(checked);
}

void DAChartAddStatsRegplotWidget::onFitRegToggled(bool checked)
{
    ui->groupBoxRegKws->setEnabled(checked);
}

}  // namespace DA
