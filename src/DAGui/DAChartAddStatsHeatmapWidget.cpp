#include "DAChartAddStatsHeatmapWidget.h"
#include "ui_DAChartAddStatsHeatmapWidget.h"

#include <QJsonObject>
#include <QVariant>
#include "DADataManager.h"
#include "DAChartSeriesSelectWidget.h"
#include "DAPySeriesListView.h"  // for AcceptMode
#include "DALogCategory.h"

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

QJsonObject DAChartAddStatsHeatmapWidget::buildPlotParams() const
{
    QJsonObject p;

    // Aggregation function (only meaningful when value column is selected)
    if (ui->groupBoxValueColumn->isChecked()) {
        p["aggfunc"] = ui->comboBoxAggfunc->currentText();
    }

    // Colour map
    p["cmap"] = ui->comboBoxCmap->currentText();

    // Standardisation
    QString scale = ui->comboBoxStandardScale->currentText();
    if (scale != QStringLiteral("None")) {
        p["standard_scale"] = scale;
    }

    // Value range — 0 means auto (NaN on the Python side)
    double vmin = ui->doubleSpinBoxVmin->value();
    double vmax = ui->doubleSpinBoxVmax->value();
    if (vmin != 0.0) {
        p["vmin"] = vmin;
    }
    if (vmax != 0.0) {
        p["vmax"] = vmax;
    }

    // Center — 0 means no centering (NaN on the Python side)
    double center = ui->doubleSpinBoxCenter->value();
    if (center != 0.0) {
        p["center"] = center;
    }

    // Annotations
    p["annot"] = ui->checkBoxAnnot->isChecked();
    if (ui->checkBoxAnnot->isChecked()) {
        p["fmt"] = ui->lineEditFmt->text();
    }

    return p;
}

void DAChartAddStatsHeatmapWidget::onButtonBoxAccepted()
{
    // Validate: both X and Y series must be selected
    QPair< DAData, QString > selX = ui->selectWidgetXData->getCurrentSeries();
    if (!selX.first.isDataFrame()) {
        daWarning << tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
        return;
    }
    if (selX.second.isEmpty()) {
        daWarning << tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
        return;
    }

    QPair< DAData, QString > selY = ui->selectWidgetYData->getCurrentSeries();
    if (!selY.first.isDataFrame()) {
        daWarning << tr("Please select a Y-axis data column before plotting");  // cn: 请先选择 Y 轴数据列再绘图
        return;
    }
    if (selY.second.isEmpty()) {
        daWarning << tr("Please select a Y-axis data column before plotting");  // cn: 请先选择 Y 轴数据列再绘图
        return;
    }

    // Stash the DAData as a dynamic property so the controller's
    // onStatsPlotRequested slot can retrieve it via sender().
    setProperty("__da_data__", QVariant::fromValue(selX.first));

    QJsonObject params = buildPlotParams();
    // Stash the selected X column name (controller reads "column" positionally)
    params["column"] = selX.second;
    // Stash the Y column name for the Python side
    params["y_column"] = selY.second;
    // Identify which Python module the controller should import
    params["__plot_module__"] = QStringLiteral("heatmap");

    // Value column name (if enabled)
    if (ui->groupBoxValueColumn->isChecked()) {
        QPair< DAData, QString > valSel = ui->selectWidgetValue->getCurrentSeries();
        if (valSel.first.isDataFrame() && !valSel.second.isEmpty()) {
            params["value_column"] = valSel.second;
        }
    }

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
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
