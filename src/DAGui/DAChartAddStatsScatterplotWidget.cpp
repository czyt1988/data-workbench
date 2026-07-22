#include "DAChartAddStatsScatterplotWidget.h"
#include "ui_DAChartAddStatsScatterplotWidget.h"

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

    // Set a default marker colour (seaborn deep[0])
    ui->colorButton->setColor(QColor("#4C72B0"));

    // Palette is only enabled when hue is checked; color button when unchecked
    auto syncHueControls = [this]() {
        bool hueOn = ui->groupBoxHue->isChecked();
        ui->comboBoxPalette->setEnabled(hueOn);
        ui->labelPalette->setEnabled(hueOn);
        ui->colorButton->setEnabled(!hueOn);
        ui->labelColor->setEnabled(!hueOn);
    };
    syncHueControls();
    connect(ui->groupBoxHue, &QGroupBox::toggled, this, &DAChartAddStatsScatterplotWidget::onHueToggled);

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

QJsonObject DAChartAddStatsScatterplotWidget::buildPlotParams() const
{
    QJsonObject p;

    // Marker options
    p["size"]   = ui->spinBoxSize->value();
    p["style"]  = ui->comboBoxStyle->currentText();
    p["alpha"]  = ui->doubleSpinBoxAlpha->value();
    p["legend"] = ui->checkBoxLegend->isChecked();

    // Hue grouping
    if (ui->groupBoxHue->isChecked()) {
        p["use_hue"] = true;
        p["palette"] = ui->comboBoxPalette->currentText();
    } else {
        p["use_hue"] = false;
        // Single-color mode: pass the chosen colour as a hex string
        QColor c = ui->colorButton->color();
        if (c.isValid()) {
            p["marker_color"] = c.name();  // "#RRGGBB"
        }
    }

    return p;
}

void DAChartAddStatsScatterplotWidget::onButtonBoxAccepted()
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
    params["__plot_type__"] = QStringLiteral("scatterplot");

    // Hue column name (if enabled)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            params["hue"] = hueSel.second;
        }
    }

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
}

void DAChartAddStatsScatterplotWidget::onHueToggled(bool checked)
{
    Q_UNUSED(checked);
    // Enable palette when hue is on; enable color button when hue is off
    bool hueOn = ui->groupBoxHue->isChecked();
    ui->comboBoxPalette->setEnabled(hueOn);
    ui->labelPalette->setEnabled(hueOn);
    ui->colorButton->setEnabled(!hueOn);
    ui->labelColor->setEnabled(!hueOn);
}

}  // namespace DA
