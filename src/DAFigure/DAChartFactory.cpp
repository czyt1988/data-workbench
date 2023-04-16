#include "DAChartFactory.h"
#include "DAChartWidget.h"
namespace DA
{
DAChartFactory::DAChartFactory()
{
}

DAChartFactory::~DAChartFactory()
{
}

DAChartWidget* DAChartFactory::createChart(QWidget* par)
{
    return new DAChartWidget(par);
}
}
