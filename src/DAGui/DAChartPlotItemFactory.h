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

    /**
     * @brief 创建item
     * @param rtti
     * @return 如果未知的rtti，返回nullptr
     */
    static QwtPlotItem* createItem(int rtti);

    /**
     * @brief 注册工厂函数
     * @param rtti
     * @param fp
     */
    static void registCreateItemFucntion(int rtti, FpItemCreate fp);

    /**
     * @brief 判断是否存在此工厂函数
     * @param rtti
     * @return
     */
    static bool isHaveCreateItemFucntion(int rtti);

private:
    static QHash< int, DAChartPlotItemFactory::FpItemCreate >& factoryFunctionMap();
};
}

#endif  // DACHARTPLOTITEMFACTORY_H
