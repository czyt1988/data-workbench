#ifndef DACHARTFACTORY_H
#define DACHARTFACTORY_H
#include "DAFigureAPI.h"
class QWidget;
namespace DA
{
class DAChartWidget;
/**
 * @brief DAFigureWidget持有一个ChartFactory，在添加Chart的时候，将调用DAChartFactory::createChart
 */
class DAFIGURE_API DAChartFactory
{
public:
    DAChartFactory();
    virtual ~DAChartFactory();
    //创建绘图窗口，默认为DAChartWidget
    virtual DAChartWidget* createChart(QWidget* par = nullptr);
};
}

#endif  // DACHARTFACTORY_H
