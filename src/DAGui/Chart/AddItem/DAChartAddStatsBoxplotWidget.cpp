#include "Chart/AddItem/DAChartAddStatsBoxplotWidget.h"
#include "ui_DAChartAddStatsBoxplotWidget.h"

#include <QVariant>
#include "DADataManager.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Table/Views/DAPySeriesListView.h"  // for AcceptMode

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

DAStatsPlotRequest* DAChartAddStatsBoxplotWidget::createPlotRequest() const
{
    auto* req = new DAStatsBoxplotRequest;

    req->figure = getFigureWidget();
    req->chart  = getChartWidget();

    // Multi-series selection (one dataframe, multiple columns)
    const QList< QPair< DAData, QStringList > > series = ui->selectWidgetData->getSeries();
    for (const auto& pair : series) {
        if (!req->data.isDataFrame()) {
            req->data = pair.first;
        }
        req->columns.append(pair.second);
    }
    // Remove empty entries
    req->columns.removeAll(QString());

    // Hue column (only if the groupbox is checked and a column is selected)
    if (ui->groupBoxHue->isChecked()) {
        QPair< DAData, QString > hueSel = ui->selectWidgetHue->getCurrentSeries();
        if (hueSel.first.isDataFrame() && !hueSel.second.isEmpty()) {
            req->hue = hueSel.second;
        }
    }

    // Whisker multiplier
    req->whis = ui->doubleSpinBoxWhis->value();

    // Box width
    req->width = ui->doubleSpinBoxWidth->value();

    // Display options
    req->showfliers = ui->checkBoxShowFliers->isChecked();
    req->showmeans  = ui->checkBoxShowMeans->isChecked();

    return req;
}

}  // namespace DA
