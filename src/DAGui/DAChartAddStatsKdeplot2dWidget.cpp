#include "DAChartAddStatsKdeplot2dWidget.h"
#include "ui_DAChartAddStatsKdeplot2dWidget.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include <QColor>
#include "DADataManager.h"
#include "DAChartSeriesSelectWidget.h"
#include "DAPySeriesListView.h"  // for AcceptMode
#include "DALogCategory.h"

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

QJsonObject DAChartAddStatsKdeplot2dWidget::buildPlotParams() const
{
    QJsonObject p;

    // Bandwidth
    p["bw_method"] = ui->comboBoxBwMethod->currentText();
    if (ui->comboBoxBwMethod->currentText() == QStringLiteral("custom")) {
        p["bw_value"] = ui->doubleSpinBoxBwValue->value();
    }

    // Grid size
    p["grid_size"] = ui->spinBoxGridSize->value();

    // Contour levels
    p["levels"] = ui->spinBoxLevels->value();

    // Threshold
    p["thresh"] = ui->doubleSpinBoxThresh->value();

    // Colour map
    p["cmap"] = ui->comboBoxCmap->currentText();

    // Display options
    p["fill_contours"] = ui->checkBoxFillContours->isChecked();
    p["show_heatmap"] = ui->checkBoxShowHeatmap->isChecked();
    p["common_norm"] = ui->checkBoxCommonNorm->isChecked();

    return p;
}

void DAChartAddStatsKdeplot2dWidget::onButtonBoxAccepted()
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
    // Stash the selected X column name into params so the controller can read it
    params["column"] = selX.second;
    // Stash the Y column name for the Python side
    params["y_column"] = selY.second;
    // Identify which Python module the controller should import
    params["__plot_type__"] = QStringLiteral("kdeplot_2d");

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
}

void DAChartAddStatsKdeplot2dWidget::onBwMethodChanged(int index)
{
    // "custom" is at index 2 in the combo box
    bool isCustom = (ui->comboBoxBwMethod->itemText(index) == QStringLiteral("custom"));
    ui->doubleSpinBoxBwValue->setEnabled(isCustom);
    ui->labelBwValue->setEnabled(isCustom);
}

}  // namespace DA
