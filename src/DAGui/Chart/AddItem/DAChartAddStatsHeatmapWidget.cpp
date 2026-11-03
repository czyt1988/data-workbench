#include "Chart/AddItem/DAChartAddStatsHeatmapWidget.h"
#include "ui_DAChartAddStatsHeatmapWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

namespace DA
{

DAChartAddStatsHeatmapWidget::DAChartAddStatsHeatmapWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsHeatmapWidget)
{
    ui->setupUi(this);

    // X column: single-series selection
    ui->selectWidgetXData->setRoleLabel(tr("X axis"));  // cn: X 轴
    ui->selectWidgetXData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Y column: single-series selection
    ui->selectWidgetYData->setRoleLabel(tr("Y axis"));  // cn: Y 轴
    ui->selectWidgetYData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Value column: single-series selection
    ui->selectWidgetValue->setRoleLabel(tr("Value"));  // cn: 值
    ui->selectWidgetValue->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Aggregation function is only enabled when value column is checked
    onValueColumnToggled(ui->groupBoxValueColumn->isChecked());
    connect(ui->groupBoxValueColumn, &QGroupBox::toggled,
            this, &DAChartAddStatsHeatmapWidget::onValueColumnToggled);

    // Format string is only enabled when annot is checked
    onAnnotToggled(ui->checkBoxAnnot->isChecked());
    connect(ui->checkBoxAnnot, &QCheckBox::toggled,
            this, &DAChartAddStatsHeatmapWidget::onAnnotToggled);

}

DAChartAddStatsHeatmapWidget::~DAChartAddStatsHeatmapWidget()
{
    delete ui;
}

void DAChartAddStatsHeatmapWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetXData->setDataManager(dmgr);
    ui->selectWidgetYData->setDataManager(dmgr);
    ui->selectWidgetValue->setDataManager(dmgr);
}

DAStatsPlotRequest* DAChartAddStatsHeatmapWidget::createPlotRequest() const
{
    auto* req = new DAStatsHeatmapRequest;

    // X / Y columns (single-series selection)
    // 字段命名对齐 compute_pivot_matrix 的位置参数语义（index_col/columns_col/values_col），
    // 修复原实现「widget 写 column/y_column/value_column、coordinator 读 index_col/columns_col/values_col」的键名断裂
    QPair< DAData, QString > selX = ui->selectWidgetXData->getCurrentSeries();
    req->data          = selX.first;
    req->figure        = getFigureWidget();
    req->chart         = getChartWidget();
    req->indexColumn   = selX.second;
    req->columnsColumn = ui->selectWidgetYData->getCurrentSeries().second;

    // Value column + aggregation function (only meaningful when the groupbox is checked)
    if (ui->groupBoxValueColumn->isChecked()) {
        QPair< DAData, QString > valSel = ui->selectWidgetValue->getCurrentSeries();
        if (valSel.first.isDataFrame() && !valSel.second.isEmpty()) {
            req->valueColumn = valSel.second;
        }
        req->aggfunc = ui->comboBoxAggfunc->currentText();
    }

    // Colour map
    req->cmap = ui->comboBoxCmap->currentText();

    // Standardisation
    const QString scale = ui->comboBoxStandardScale->currentText();
    if (scale != QStringLiteral("None")) {
        req->standardScale = scale;
    }

    // Value range — 0 means auto
    if (ui->doubleSpinBoxVmin->value() != 0.0) {
        req->vmin = ui->doubleSpinBoxVmin->value();
    }
    if (ui->doubleSpinBoxVmax->value() != 0.0) {
        req->vmax = ui->doubleSpinBoxVmax->value();
    }

    // Center — 0 means no centering
    if (ui->doubleSpinBoxCenter->value() != 0.0) {
        req->center = ui->doubleSpinBoxCenter->value();
    }

    // Annotations
    req->annot = ui->checkBoxAnnot->isChecked();
    if (ui->checkBoxAnnot->isChecked()) {
        req->fmt = ui->lineEditFmt->text();
    }

    return req;
}

void DAChartAddStatsHeatmapWidget::onValueColumnToggled(bool checked)
{
    // Aggregation function is only meaningful when a value column is selected
    ui->comboBoxAggfunc->setEnabled(checked);
    ui->labelAggfunc->setEnabled(checked);
}

void DAChartAddStatsHeatmapWidget::onAnnotToggled(bool checked)
{
    // Format string is only meaningful when annotations are enabled
    ui->lineEditFmt->setEnabled(checked);
    ui->labelFmt->setEnabled(checked);
}

}  // namespace DA
