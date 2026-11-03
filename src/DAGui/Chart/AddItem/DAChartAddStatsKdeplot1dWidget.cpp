#include "Chart/AddItem/DAChartAddStatsKdeplot1dWidget.h"
#include "ui_DAChartAddStatsKdeplot1dWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

namespace DA
{

DAChartAddStatsKdeplot1dWidget::DAChartAddStatsKdeplot1dWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsKdeplot1dWidget)
{
    ui->setupUi(this);

    // Data column: single-series selection
    ui->selectWidgetData->setRoleLabel(tr("Data"));  // cn: 数据
    ui->selectWidgetData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Hue column: also single-series selection
    ui->selectWidgetHue->setRoleLabel(tr("Hue"));  // cn: 分组
    ui->selectWidgetHue->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // bw_value is only enabled when bw_method = custom (index 2)
    ui->doubleSpinBoxBwValue->setEnabled(false);
    connect(ui->comboBoxBwMethod, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAddStatsKdeplot1dWidget::onBwMethodChanged);

    // Threshold controls are only enabled when fill is checked
    onFillToggled(ui->checkBoxFill->isChecked());
    connect(ui->checkBoxFill, &QCheckBox::toggled,
            this, &DAChartAddStatsKdeplot1dWidget::onFillToggled);

    // Threshold spin boxes follow their own checkboxes (and fill must be on)
    connect(ui->checkBoxBelow, &QCheckBox::toggled, this, [this]() {
        ui->doubleSpinBoxBelow->setEnabled(
            ui->checkBoxFill->isChecked() && ui->checkBoxBelow->isChecked());
    });
    connect(ui->checkBoxAbove, &QCheckBox::toggled, this, [this]() {
        ui->doubleSpinBoxAbove->setEnabled(
            ui->checkBoxFill->isChecked() && ui->checkBoxAbove->isChecked());
    });

}

DAChartAddStatsKdeplot1dWidget::~DAChartAddStatsKdeplot1dWidget()
{
    delete ui;
}

void DAChartAddStatsKdeplot1dWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetData->setDataManager(dmgr);
    ui->selectWidgetHue->setDataManager(dmgr);
}

DAStatsPlotRequest* DAChartAddStatsKdeplot1dWidget::createPlotRequest() const
{
    auto* req = new DAStatsKdeplot1dRequest;

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

    // Bandwidth
    req->bwMethod = ui->comboBoxBwMethod->currentText();
    if (req->bwMethod == QStringLiteral("custom")) {
        req->bwValue = ui->doubleSpinBoxBwValue->value();
    }

    // Grid size
    req->gridSize = ui->spinBoxGridSize->value();

    // Fill / shade / cumulative / common_norm
    req->fill       = ui->checkBoxFill->isChecked();
    req->shade      = ui->checkBoxShade->isChecked();
    req->cumulative = ui->checkBoxCumulative->isChecked();
    req->commonNorm = ui->checkBoxCommonNorm->isChecked();

    // Threshold clipping (only when fill is enabled and the threshold is active)
    if (ui->checkBoxFill->isChecked()) {
        if (ui->checkBoxBelow->isChecked()) {
            req->below = ui->doubleSpinBoxBelow->value();
        }
        if (ui->checkBoxAbove->isChecked()) {
            req->above = ui->doubleSpinBoxAbove->value();
        }
    }

    return req;
}

void DAChartAddStatsKdeplot1dWidget::onBwMethodChanged(int index)
{
    // "custom" is at index 2 in the combo box
    bool isCustom = (ui->comboBoxBwMethod->itemText(index) == QStringLiteral("custom"));
    ui->doubleSpinBoxBwValue->setEnabled(isCustom);
    ui->labelBwValue->setEnabled(isCustom);
}

void DAChartAddStatsKdeplot1dWidget::onFillToggled(bool checked)
{
    // Threshold controls are only meaningful when fill is enabled
    ui->checkBoxBelow->setEnabled(checked);
    ui->checkBoxAbove->setEnabled(checked);
    ui->doubleSpinBoxBelow->setEnabled(checked && ui->checkBoxBelow->isChecked());
    ui->doubleSpinBoxAbove->setEnabled(checked && ui->checkBoxAbove->isChecked());
    ui->labelBelow->setEnabled(checked);
    ui->labelAbove->setEnabled(checked);

    // shade is a sub-option of fill
    if (!checked) {
        ui->checkBoxShade->setChecked(false);
    }
    ui->checkBoxShade->setEnabled(checked);
}

}  // namespace DA
