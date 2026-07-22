#include "DAChartAddStatsEcdfplotWidget.h"
#include "ui_DAChartAddStatsEcdfplotWidget.h"

#include <QJsonObject>
#include <QVariant>
#include <QColor>
#include "DADataManager.h"
#include "DAChartSeriesSelectWidget.h"
#include "DAPySeriesListView.h"  // for AcceptMode
#include "DAColorPickerButton.h"
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

    // Default colour
    ui->colorButton->setColor(QColor("#4C72B0"));

    // Palette / colour enable/disable sync
    auto syncHueControls = [this]() {
        bool hueOn = ui->groupBoxHue->isChecked();
        ui->comboBoxPalette->setEnabled(hueOn);
        ui->labelPalette->setEnabled(hueOn);
        ui->colorButton->setEnabled(!hueOn);
        ui->labelColor->setEnabled(!hueOn);
    };
    syncHueControls();
    connect(ui->groupBoxHue, &QGroupBox::toggled, this, &DAChartAddStatsEcdfplotWidget::onHueToggled);

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
    if (ui->groupBoxHue->isChecked()) {
        p["use_hue"] = true;
        p["palette"] = ui->comboBoxPalette->currentText();
    } else {
        p["use_hue"] = false;
        QColor c = ui->colorButton->color();
        if (c.isValid()) {
            p["color"] = c.name();
        }
    }

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

    // Stash the DAData as a dynamic property so the controller's
    // onStatsPlotRequested slot can retrieve it via sender().
    setProperty("__da_data__", QVariant::fromValue(sel.first));

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

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
}

void DAChartAddStatsEcdfplotWidget::onHueToggled(bool checked)
{
    Q_UNUSED(checked);
    bool hueOn = ui->groupBoxHue->isChecked();
    ui->comboBoxPalette->setEnabled(hueOn);
    ui->labelPalette->setEnabled(hueOn);
    ui->colorButton->setEnabled(!hueOn);
    ui->labelColor->setEnabled(!hueOn);
}

}  // namespace DA
