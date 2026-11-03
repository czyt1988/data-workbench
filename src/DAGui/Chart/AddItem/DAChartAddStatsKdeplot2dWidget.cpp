#include "Chart/AddItem/DAChartAddStatsKdeplot2dWidget.h"
#include "ui_DAChartAddStatsKdeplot2dWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

namespace DA
{

DAChartAddStatsKdeplot2dWidget::DAChartAddStatsKdeplot2dWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsKdeplot2dWidget)
{
    ui->setupUi(this);

    // X column: single-series selection
    ui->selectWidgetXData->setRoleLabel(tr("X axis"));  // cn: X 轴
    ui->selectWidgetXData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Y column: single-series selection
    ui->selectWidgetYData->setRoleLabel(tr("Y axis"));  // cn: Y 轴
    ui->selectWidgetYData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // bw_value is only enabled when bw_method = custom (index 2)
    ui->doubleSpinBoxBwValue->setEnabled(false);
    connect(ui->comboBoxBwMethod, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAddStatsKdeplot2dWidget::onBwMethodChanged);

}

DAChartAddStatsKdeplot2dWidget::~DAChartAddStatsKdeplot2dWidget()
{
    delete ui;
}

void DAChartAddStatsKdeplot2dWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetXData->setDataManager(dmgr);
    ui->selectWidgetYData->setDataManager(dmgr);
}

DAStatsPlotRequest* DAChartAddStatsKdeplot2dWidget::createPlotRequest() const
{
    auto* req = new DAStatsKdeplot2dRequest;

    // X / Y columns (single-series selection)
    QPair< DAData, QString > selX = ui->selectWidgetXData->getCurrentSeries();
    req->data     = selX.first;
    req->figure   = getFigureWidget();
    req->chart    = getChartWidget();
    req->xColumn  = selX.second;
    req->yColumn  = ui->selectWidgetYData->getCurrentSeries().second;

    // Bandwidth
    req->bwMethod = ui->comboBoxBwMethod->currentText();
    if (req->bwMethod == QStringLiteral("custom")) {
        req->bwValue = ui->doubleSpinBoxBwValue->value();
    }

    // Grid size
    req->gridSize = ui->spinBoxGridSize->value();

    // Contour levels
    req->levels = ui->spinBoxLevels->value();

    // Threshold
    req->thresh = ui->doubleSpinBoxThresh->value();

    // Display options
    req->fillContours = ui->checkBoxFillContours->isChecked();
    req->showHeatmap  = ui->checkBoxShowHeatmap->isChecked();
    req->commonNorm   = ui->checkBoxCommonNorm->isChecked();

    return req;
}

void DAChartAddStatsKdeplot2dWidget::onBwMethodChanged(int index)
{
    // "custom" is at index 2 in the combo box
    bool isCustom = (ui->comboBoxBwMethod->itemText(index) == QStringLiteral("custom"));
    ui->doubleSpinBoxBwValue->setEnabled(isCustom);
    ui->labelBwValue->setEnabled(isCustom);
}

}  // namespace DA
