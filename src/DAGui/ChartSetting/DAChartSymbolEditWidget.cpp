#include "DAChartSymbolEditWidget.h"
#include "ui_DAChartSymbolEditWidget.h"
namespace DA
{
DAChartSymbolEditWidget::DAChartSymbolEditWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DAChartSymbolEditWidget)
{
    ui->setupUi(this);
}

DAChartSymbolEditWidget::~DAChartSymbolEditWidget()
{
    delete ui;
}
}
