#include "DAChartAddStatsHistplotWidget.h"
#include "ui_DAChartAddStatsHistplotWidget.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include "DADataManager.h"
#include "DAChartSeriesSelectWidget.h"
#include "DAPySeriesListView.h"  // for AcceptMode
#include "DALogCategory.h"

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

QJsonObject DAChartAddStatsHistplotWidget::buildPlotParams() const
{
    QJsonObject p;

    // Bins
    p["bins"] = ui->spinBoxBins->value();
    p["binwidth"] = ui->doubleSpinBoxBinwidth->value();

    // Bin range — we can't easily detect "blank" with QDoubleSpinBox, so
    double binMin = ui->doubleSpinBoxBinRangeMin->value();
    double binMax = ui->doubleSpinBoxBinRangeMax->value();
    QJsonArray binrange;
    if (binMin == 0.0 && binMax == 0.0) {
        binrange.append(QJsonValue::Null);
        binrange.append(QJsonValue::Null);
    } else {
        binrange.append(binMin);
        binrange.append(binMax);
    }
    p["binrange"] = binrange;

    // Statistics
    p["stat"] = ui->comboBoxStat->currentText();
    p["kde"] = ui->checkBoxKde->isChecked();
    p["kde_bw"] = ui->comboBoxKdeBw->currentText();
    p["cumulative"] = ui->checkBoxCumulative->isChecked();
    p["fill"] = ui->checkBoxFill->isChecked();

    // Hue (only if the groupbox is checked and a column is selected)
    if (ui->groupBoxHue->isChecked()) {
        // The hue column name is extracted by the controller from the
        // series-select widget, because we need to pair it with the DAData.
        // We just mark the flag here; the controller reads it back.
        p["use_hue"] = true;
    } else {
        p["use_hue"] = false;
    }

    // Colour / element / multiple have sensible Python-side defaults
    p["element"] = QStringLiteral("bars");
    p["multiple"] = QStringLiteral("layer");

    return p;
}

void DAChartAddStatsHistplotWidget::onButtonBoxAccepted()
{
    // Validate: a data series must be selected
    QPair< DAData, QString > sel = ui->selectWidgetData->getCurrentSeries();
    if (!sel.first.isDataFrame()) {
        // The controller will re-check, but we can warn early
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
    params["__plot_type__"] = QStringLiteral("histplot");

    // Hue column name (if enabled)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            params["hue"] = hueSel.second;
        }
    }

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
}

}  // namespace DA
