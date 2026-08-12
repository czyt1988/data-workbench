#ifndef DACHART3DPLOTITEMFACTORY_H
#define DACHART3DPLOTITEMFACTORY_H
#include <QHash>
#include <functional>
#include "DAFigureAPI.h"
// qwt3d
#include "qwt3d_plotitem.h"
namespace DA
{

/**
 * @brief 针对Qwt3DPlotItem的工厂类
 *
 * 参考DAChartPlotItemFactory的设计，用于反序列化时根据rtti创建对应的3D plot item。
 * rtti值定义在qwt3d_types.h中（由plan 01添加）：
 * - Rtti_Plot3DSurface = 1001
 * - Rtti_Plot3DBar = 1002
 * - Rtti_Plot3DLine = 1003
 *
 * 注意：此工厂由plan 03独占创建，plan 08不重复定义，
 * 改为#include "DAChart3DPlotItemFactory.h"直接使用。
 */
class DAFIGURE_API DAChart3DPlotItemFactory
{
public:
    using Fp3DItemCreate = std::function< Qwt3DPlotItem*() >;  ///< 函数指针：Qwt3DPlotItem* itemCreate();

public:
    DAChart3DPlotItemFactory();

    // 创建3D item，如果未知的rtti，返回nullptr
    static Qwt3DPlotItem* createItem(int rtti);

    // 注册工厂函数
    static void registCreateItemFucntion(int rtti, Fp3DItemCreate fp);

    // 判断是否存在此工厂函数
    static bool isHaveCreateItemFucntion(int rtti);

private:
    static QHash< int, DAChart3DPlotItemFactory::Fp3DItemCreate >& factoryFunctionMap();
};
}  // namespace DA

#endif  // DACHART3DPLOTITEMFACTORY_H
