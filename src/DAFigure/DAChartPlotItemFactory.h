#ifndef DACHARTPLOTITEMFACTORY_H
#define DACHARTPLOTITEMFACTORY_H
#include <functional>
#include <unordered_map>
#include "qwt_plot_item.h"
namespace DA
{

/**
 * @brief  针对QwtPlotItem的工厂类
 */
class DAChartPlotItemFactory
{
public:
    using FpItemCreate = std::function< QwtPlotItem*() >;  ///< 函数指针：QwtPlotItem* itemCreate(int rtti);
public:
    DAChartPlotItemFactory();

    // 创建item
    static QwtPlotItem* createItem(int rtti);

    // 注册工厂函数
    static void registCreateItemFucntion(int rtti, FpItemCreate fp);

    // 判断是否存在此工厂函数
    static bool isHaveCreateItemFucntion(int rtti);

private:
    static QHash< int, DAChartPlotItemFactory::FpItemCreate >& factoryFunctionMap();
};
}

#endif  // DACHARTPLOTITEMFACTORY_H
