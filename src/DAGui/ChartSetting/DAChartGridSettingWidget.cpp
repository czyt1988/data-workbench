#include "DAChartGridSettingWidget.h"
#include "ui_DAChartGridSettingWidget.h"
#include "qwt_plot_grid.h"
namespace DA
{

DAChartGridSettingWidget::DAChartGridSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), ui(new Ui::DAChartGridSettingWidget)
{
    ui->setupUi(this);
    connect(ui->widgetMajorPen, &DAPenEditWidget::penChanged, this, &DAChartGridSettingWidget::onMajorLinePenChanged);
    connect(ui->widgetMinorPen, &DAPenEditWidget::penChanged, this, &DAChartGridSettingWidget::onMinorLinePenChanged);
}

DAChartGridSettingWidget::~DAChartGridSettingWidget()
{
    delete ui;
}

void DAChartGridSettingWidget::onMajorLinePenChanged(const QPen& p)
{
    QwtPlotGrid* grid = dynamic_cast< QwtPlotGrid* >(getPlotItem());
    if (!grid) {
        return;
    }
    grid->setMajorPen(p);
    replot();
}

void DAChartGridSettingWidget::onMinorLinePenChanged(const QPen& p)
{
    QwtPlotGrid* grid = dynamic_cast< QwtPlotGrid* >(getPlotItem());
    if (!grid) {
        return;
    }
    grid->setMinorPen(p);
    replot();
}

void DAChartGridSettingWidget::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        clearUI();
        return;
    }
    QSignalBlocker b1(ui->widgetMajorPen), b2(ui->widgetMinorPen);
    if (QwtPlotItem::Rtti_PlotGrid == item->rtti()) {
        QwtPlotGrid* grid = static_cast< QwtPlotGrid* >(item);
        ui->widgetMajorPen->setCurrentPen(grid->majorPen());
        ui->widgetMinorPen->setCurrentPen(grid->minorPen());
    } else {
        clearUI();
    }
}

void DAChartGridSettingWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DAChartGridSettingWidget::clearUI()
{
    ui->widgetMajorPen->setCurrentPen(QPen());
    ui->widgetMinorPen->setCurrentPen(QPen());
}

}  // end DA
