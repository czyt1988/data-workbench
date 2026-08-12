#include "DAChartFactory.h"
#include "DAChartWidget.h"
namespace DA
{
/**
 * @brief 默认构造函数
 */
DAChartFactory::DAChartFactory()
{
}

/**
 * @brief 析构函数
 */
DAChartFactory::~DAChartFactory()
{
}

/**
 * @brief 创建图表控件
 * @param par 父窗口
 * @return 新创建的DAChartWidget指针
 */
DAChartWidget* DAChartFactory::createChart(QWidget* par)
{
    return new DAChartWidget(par);
}
}
