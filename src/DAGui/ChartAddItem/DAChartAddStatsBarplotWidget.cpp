#include "DAChartAddStatsBarplotWidget.h"
#include "ui_DAChartAddStatsBarplotWidget.h"

#include <QJsonObject>
#include <QVariant>
#include "DADataManager.h"
#include "DAChartSeriesSelectWidget.h"
#include "DAPySeriesListView.h"  // for AcceptMode
#include "DALogCategory.h"

namespace DA
{

DAChartAddStatsBarplotWidget::DAChartAddStatsBarplotWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsBarplotWidget)
{
    ui->setupUi(this);

    // X column: single-series selection (categorical)
    ui->selectWidgetXData->setRoleLabel(tr("X axis (categorical)"));  // cn: X 轴（分类）
    ui->selectWidgetXData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Y column: single-series selection (numeric, optional)
    ui->selectWidgetYData->setRoleLabel(tr("Y axis (numeric, optional)"));  // cn: Y 轴（数值，可选）
    ui->selectWidgetYData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Hue column: single-series selection
    ui->selectWidgetHue->setRoleLabel(tr("Hue"));  // cn: 分组
    ui->selectWidgetHue->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // CI-related controls sync
    auto syncCiControls = [this]() {
        int ciIdx = ui->comboBoxCi->currentIndex();
        bool ciEnabled = (ciIdx > 0);  // index 0 = "None"
        ui->spinBoxNBoot->setEnabled(ciEnabled);
        ui->labelNBoot->setEnabled(ciEnabled);
    };
    syncCiControls();
    connect(ui->comboBoxCi, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAddStatsBarplotWidget::onCiChanged);

    // Estimator / CI controls are only active in barplot mode (Y column selected).
    // We check the Y selection widget on OK; the mode toggle is advisory here.
}

DAChartAddStatsBarplotWidget::~DAChartAddStatsBarplotWidget()
{
    delete ui;
}

void DAChartAddStatsBarplotWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetXData->setDataManager(dmgr);
    ui->selectWidgetYData->setDataManager(dmgr);
    ui->selectWidgetHue->setDataManager(dmgr);
}

QJsonObject DAChartAddStatsBarplotWidget::buildPlotParams() const
{
    QJsonObject p;

    // Estimator
    p["estimator"] = ui->comboBoxEstimator->currentText();

    // CI
    int ciIdx = ui->comboBoxCi->currentIndex();
    if (ciIdx == 0) {
        p["ci"] = QJsonValue::Null;  // None
    } else {
        // Items: None, 68, 95, 99 — index maps to the percentage value
        static const int ciValues[] = { 0, 68, 95, 99 };
        p["ci"] = ciValues[ciIdx];
    }
    p["n_boot"] = ui->spinBoxNBoot->value();

    // Orientation
    p["orient"] = ui->comboBoxOrient->currentText();

    // Hue grouping
    p["use_hue"] = ui->groupBoxHue->isChecked();

    // Bar width
    p["width"]   = ui->doubleSpinBoxWidth->value();
    p["legend"]  = ui->checkBoxLegend->isChecked();

    return p;
}

void DAChartAddStatsBarplotWidget::onButtonBoxAccepted()
{
    // Validate: X column must be selected
    QPair< DAData, QString > selX = ui->selectWidgetXData->getCurrentSeries();
    if (!selX.first.isDataFrame() || selX.second.isEmpty()) {
        daWarning << tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
        return;
    }

    // Y column is optional (empty = countplot mode)
    QPair< DAData, QString > selY = ui->selectWidgetYData->getCurrentSeries();
    bool hasY = selY.first.isDataFrame() && !selY.second.isEmpty();

    // Stash the DAData as a dynamic property so the controller's
    // onStatsPlotRequested slot can retrieve it via sender().
    setProperty("__da_data__", QVariant::fromValue(selX.first));

    QJsonObject params = buildPlotParams();
    // Stash the selected X column name (controller reads "column" positionally)
    params["column"] = selX.second;
    // Stash the Y column name for the Python side (empty string = countplot)
    params["y_column"] = hasY ? selY.second : QString();
    // Identify which Python module the controller should import
    params["__plot_type__"] = QStringLiteral("barplot");

    // Hue column name (if enabled)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            params["hue"] = hueSel.second;
        }
    }

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
}

void DAChartAddStatsBarplotWidget::onYColumnChanged()
{
    // Toggle barplot/countplot mode based on whether Y is selected.
    // In countplot mode, estimator and CI are greyed out.
    QPair< DAData, QString > selY = ui->selectWidgetYData->getCurrentSeries();
    bool barplotMode = selY.first.isDataFrame() && !selY.second.isEmpty();
    ui->comboBoxEstimator->setEnabled(barplotMode);
    ui->labelEstimator->setEnabled(barplotMode);
    // CI controls: only in barplot mode and when CI != None
    bool ciActive = barplotMode && (ui->comboBoxCi->currentIndex() > 0);
    ui->comboBoxCi->setEnabled(barplotMode);
    ui->labelCi->setEnabled(barplotMode);
    ui->spinBoxNBoot->setEnabled(ciActive);
    ui->labelNBoot->setEnabled(ciActive);

    // Update window title
    if (barplotMode) {
        setWindowTitle(tr("Barplot Settings"));  // cn: 柱状图设置
    } else {
        setWindowTitle(tr("Countplot Settings"));  // cn: 计数图设置
    }
}

void DAChartAddStatsBarplotWidget::onCiChanged(int index)
{
    Q_UNUSED(index);
    bool ciEnabled = (ui->comboBoxCi->currentIndex() > 0);
    bool barplotMode = ui->comboBoxCi->isEnabled();
    ui->spinBoxNBoot->setEnabled(ciEnabled && barplotMode);
    ui->labelNBoot->setEnabled(ciEnabled && barplotMode);
}

}  // namespace DA
