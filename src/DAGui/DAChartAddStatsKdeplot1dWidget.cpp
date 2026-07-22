#include "DAChartAddStatsKdeplot1dWidget.h"
#include "ui_DAChartAddStatsKdeplot1dWidget.h"

#include <QJsonArray>
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

    // Palette is only enabled when hue is checked; color button when unchecked
    // Set a default curve colour (seaborn deep[0])
    ui->colorButton->setColor(QColor("#4C72B0"));
    auto syncHueControls = [this]() {
        bool hueOn = ui->groupBoxHue->isChecked();
        ui->comboBoxPalette->setEnabled(hueOn);
        ui->labelPalette->setEnabled(hueOn);
        ui->colorButton->setEnabled(!hueOn);
        ui->labelColor->setEnabled(!hueOn);
    };
    syncHueControls();
    connect(ui->groupBoxHue, &QGroupBox::toggled, this, syncHueControls);

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

QJsonObject DAChartAddStatsKdeplot1dWidget::buildPlotParams() const
{
    QJsonObject p;

    // Bandwidth
    p["bw_method"] = ui->comboBoxBwMethod->currentText();
    if (ui->comboBoxBwMethod->currentText() == QStringLiteral("custom")) {
        p["bw_value"] = ui->doubleSpinBoxBwValue->value();
    }

    // Grid size
    p["grid_size"] = ui->spinBoxGridSize->value();

    // Fill / shade / cumulative / common_norm
    p["fill"]      = ui->checkBoxFill->isChecked();
    p["shade"]     = ui->checkBoxShade->isChecked();
    p["cumulative"] = ui->checkBoxCumulative->isChecked();
    p["common_norm"] = ui->checkBoxCommonNorm->isChecked();

    // Threshold clipping (only when fill is enabled and the threshold is active)
    if (ui->checkBoxFill->isChecked()) {
        if (ui->checkBoxBelow->isChecked()) {
            p["below"] = ui->doubleSpinBoxBelow->value();
        }
        if (ui->checkBoxAbove->isChecked()) {
            p["above"] = ui->doubleSpinBoxAbove->value();
        }
    }

    // Hue grouping
    if (ui->groupBoxHue->isChecked()) {
        p["use_hue"] = true;
        p["palette"] = ui->comboBoxPalette->currentText();
    } else {
        p["use_hue"] = false;
        // Single-color mode: pass the chosen colour as a hex string
        QColor c = ui->colorButton->color();
        if (c.isValid()) {
            p["color"] = c.name();  // "#RRGGBB"
        }
    }

    return p;
}

void DAChartAddStatsKdeplot1dWidget::onButtonBoxAccepted()
{
    // Validate: a data series must be selected
    QPair< DAData, QString > sel = ui->selectWidgetData->getCurrentSeries();
    if (!sel.first.isDataFrame()) {
        daWarning << tr("Please select a data column before plotting");  // cn: 请先选择数据列再绘图
        return;
    }
    if (sel.second.isEmpty()) {
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
    params["__plot_type__"] = QStringLiteral("kdeplot_1d");

    // Hue column name (if enabled)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            params["hue"] = hueSel.second;
        }
    }

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
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
