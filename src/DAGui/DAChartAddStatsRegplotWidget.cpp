#include "DAChartAddStatsRegplotWidget.h"
#include "ui_DAChartAddStatsRegplotWidget.h"

#include <QJsonObject>
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

DAChartAddStatsRegplotWidget::DAChartAddStatsRegplotWidget(QWidget* parent)
    : DAAbstractStatsChartAddWidget(parent), ui(new Ui::DAChartAddStatsRegplotWidget)
{
    ui->setupUi(this);

    // X column: single-series selection (independent variable)
    ui->selectWidgetXData->setRoleLabel(tr("X axis"));  // cn: X 轴
    ui->selectWidgetXData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Y column: single-series selection (dependent variable)
    ui->selectWidgetYData->setRoleLabel(tr("Y axis"));  // cn: Y 轴
    ui->selectWidgetYData->setAcceptMode(DAPySeriesListView::AcceptOneSeries);

    // Default colours
    ui->colorButton->setColor(QColor("#4C72B0"));
    ui->scatterColorButton->setColor(QColor("#4C72B0"));
    ui->regColorButton->setColor(QColor("#4C72B0"));

    // CI-related sync
    auto syncCi = [this]() {
        int ciIdx = ui->comboBoxCi->currentIndex();
        bool ciEnabled = (ciIdx > 0);  // index 0 = "None"
        ui->spinBoxNBoot->setEnabled(ciEnabled);
        ui->labelNBoot->setEnabled(ciEnabled);
    };
    syncCi();
    connect(ui->comboBoxCi, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAddStatsRegplotWidget::onCiChanged);

    // scatter_kws / reg_kws panel sync
    ui->groupBoxScatterKws->setEnabled(ui->checkBoxScatter->isChecked());
    ui->groupBoxRegKws->setEnabled(ui->checkBoxFitReg->isChecked());
    connect(ui->checkBoxScatter, &QCheckBox::toggled,
            this, &DAChartAddStatsRegplotWidget::onScatterToggled);
    connect(ui->checkBoxFitReg, &QCheckBox::toggled,
            this, &DAChartAddStatsRegplotWidget::onFitRegToggled);

}

DAChartAddStatsRegplotWidget::~DAChartAddStatsRegplotWidget()
{
    delete ui;
}

void DAChartAddStatsRegplotWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractStatsChartAddWidget::setDataManager(dmgr);
    ui->selectWidgetXData->setDataManager(dmgr);
    ui->selectWidgetYData->setDataManager(dmgr);
}

QJsonObject DAChartAddStatsRegplotWidget::buildPlotParams() const
{
    QJsonObject p;

    // Polynomial order
    p["order"] = ui->spinBoxOrder->value();

    // CI
    int ciIdx = ui->comboBoxCi->currentIndex();
    if (ciIdx == 0) {
        p["ci"] = QJsonValue::Null;
    } else {
        static const int ciValues[] = { 0, 68, 95, 99 };
        p["ci"] = ciValues[ciIdx];
    }
    p["n_boot"] = ui->spinBoxNBoot->value();

    // Toggles
    p["scatter"]  = ui->checkBoxScatter->isChecked();
    p["fit_reg"] = ui->checkBoxFitReg->isChecked();

    // Colour (unified default)
    QColor c = ui->colorButton->color();
    if (c.isValid()) {
        p["color"] = c.name();
    }

    // scatter_kws
    QJsonObject scatterKws;
    QColor sc = ui->scatterColorButton->color();
    if (sc.isValid()) {
        scatterKws["marker_color"] = sc.name();
    }
    scatterKws["marker_size"] = ui->spinBoxMarkerSize->value();
    scatterKws["alpha"]       = ui->doubleSpinBoxAlpha->value();
    p["scatter_kws"] = scatterKws;

    // reg_kws
    QJsonObject regKws;
    QColor rc = ui->regColorButton->color();
    if (rc.isValid()) {
        regKws["line_color"] = rc.name();
    }
    regKws["line_width"] = ui->doubleSpinBoxLineWidth->value();
    p["reg_kws"] = regKws;

    return p;
}

void DAChartAddStatsRegplotWidget::onButtonBoxAccepted()
{
    // Validate: both X and Y series must be selected
    QPair< DAData, QString > selX = ui->selectWidgetXData->getCurrentSeries();
    if (!selX.first.isDataFrame() || selX.second.isEmpty()) {
        daWarning << tr("Please select an X-axis data column before plotting");  // cn: 请先选择 X 轴数据列再绘图
        return;
    }

    QPair< DAData, QString > selY = ui->selectWidgetYData->getCurrentSeries();
    if (!selY.first.isDataFrame() || selY.second.isEmpty()) {
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
    params["__plot_type__"] = QStringLiteral("regplot");

    Q_EMIT plotRequested(params, getFigureWidget(), getChartWidget());
}

void DAChartAddStatsRegplotWidget::onCiChanged(int index)
{
    Q_UNUSED(index);
    bool ciEnabled = (ui->comboBoxCi->currentIndex() > 0);
    ui->spinBoxNBoot->setEnabled(ciEnabled);
    ui->labelNBoot->setEnabled(ciEnabled);
}

void DAChartAddStatsRegplotWidget::onScatterToggled(bool checked)
{
    ui->groupBoxScatterKws->setEnabled(checked);
}

void DAChartAddStatsRegplotWidget::onFitRegToggled(bool checked)
{
    ui->groupBoxRegKws->setEnabled(checked);
}

}  // namespace DA
