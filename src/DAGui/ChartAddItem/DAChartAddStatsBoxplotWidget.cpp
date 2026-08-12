#include "DAChartAddStatsBoxplotWidget.h"
#include "ui_DAChartAddStatsBoxplotWidget.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include "DADataManager.h"
#include "DAChartSeriesSelectWidget.h"
#include "DAPySeriesListView.h"  // for AcceptMode
#include "DALogCategory.h"

namespace DA
{

DAChartAddStatsBoxplotWidget::DAChartAddStatsBoxplotWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsBoxplotWidget)
{
    ui->setupUi(this);

    // Data column: multi-series selection (one dataframe, multiple columns)
    ui->selectWidgetData->setRoleLabel(tr("Data"));  // cn: 数据
    ui->selectWidgetData->setAcceptMode(DAPySeriesListView::AcceptOneDataframeMultSeries);

    // Hue column: single-series selection
    ui->selectWidgetHue->setRoleLabel(tr("Hue"));  // cn: 分组
    ui->selectWidgetHue->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

}

DAChartAddStatsBoxplotWidget::~DAChartAddStatsBoxplotWidget()
{
    delete ui;
}

void DAChartAddStatsBoxplotWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetData->setDataManager(dmgr);
    ui->selectWidgetHue->setDataManager(dmgr);
}

QJsonObject DAChartAddStatsBoxplotWidget::buildPlotParams() const
{
    QJsonObject p;

    // Whisker multiplier
    p["whis"] = ui->doubleSpinBoxWhis->value();

    // Box width
    p["width"] = ui->doubleSpinBoxWidth->value();

    // Display options
    p["showfliers"] = ui->checkBoxShowFliers->isChecked();
    p["showmeans"] = ui->checkBoxShowMeans->isChecked();

    // Hue grouping
    p["use_hue"] = ui->groupBoxHue->isChecked();

    return p;
}

void DAChartAddStatsBoxplotWidget::onButtonBoxAccepted()
{
    // Validate: at least one data series must be selected
    QList< QPair< DAData, QStringList > > series = ui->selectWidgetData->getSeries();
    if (series.isEmpty()) {
        daWarning << tr("Please select one or more data columns before plotting");  // cn: 请先选择数据列再绘图
        return;
    }

    // Collect all column names from all (DAData, QStringList) pairs
    QStringList columnNames;
    DAData firstData;
    for (const auto& pair : series) {
        if (!firstData.isDataFrame()) {
            firstData = pair.first;
        }
        columnNames.append(pair.second);
    }

    // Remove empty entries
    columnNames.removeAll(QString());

    if (columnNames.isEmpty()) {
        daWarning << tr("Please select one or more data columns before plotting");  // cn: 请先选择数据列再绘图
        return;
    }

    if (!firstData.isDataFrame()) {
        daWarning << tr("Please select a data column before plotting");  // cn: 请先选择数据列再绘图
        return;
    }

    // Stash the DAData as a dynamic property so the controller's
    // onStatsPlotRequested slot can retrieve it via sender().
    setProperty("__da_data__", QVariant::fromValue(firstData));

    QJsonObject params = buildPlotParams();
    // Stash the first column name (controller reads "column" positionally)
    params["column"] = columnNames.first();
    // Stash the full column list as a JSON array
    QJsonArray colArray;
    for (const QString& col : columnNames) {
        colArray.append(col);
    }
    params["columns"] = colArray;
    // Identify which Python module the controller should import
    params["__plot_type__"] = QStringLiteral("boxplot");

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
