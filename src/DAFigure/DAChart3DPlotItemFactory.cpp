#include "DAChart3DPlotItemFactory.h"
#include <QHash>
// qwt3d
#include "qwt3d_surface.h"
#include "qwt3d_bar.h"
#include "qwt3d_line3d.h"
#include "qwt3d_types.h"

namespace DA
{

/**
 * @brief qwt3d的默认plot3d item注册到工厂
 * @return
 */
static QHash< int, DAChart3DPlotItemFactory::Fp3DItemCreate > initDAChart3DPlotItemFactory()
{
    QHash< int, DAChart3DPlotItemFactory::Fp3DItemCreate > res;
    // RTTI 值定义在 qwt3d_types.h 中（由 plan 01 添加）
    res[ Rtti_Plot3DSurface ] = []() -> Qwt3DPlotItem* { return new Qwt3DSurface(); };
    res[ Rtti_Plot3DBar ]     = []() -> Qwt3DPlotItem* { return new Qwt3DBar(); };
    res[ Rtti_Plot3DLine ]    = []() -> Qwt3DPlotItem* { return new Qwt3DLine(); };
    return res;
}

DAChart3DPlotItemFactory::DAChart3DPlotItemFactory()
{
}

Qwt3DPlotItem* DAChart3DPlotItemFactory::createItem(int rtti)
{
    Fp3DItemCreate fp = factoryFunctionMap().value(rtti, nullptr);
    if (!fp) {
        return nullptr;
    }
    return fp();
}

void DAChart3DPlotItemFactory::registCreateItemFucntion(int rtti, DAChart3DPlotItemFactory::Fp3DItemCreate fp)
{
    factoryFunctionMap()[ rtti ] = fp;
}

bool DAChart3DPlotItemFactory::isHaveCreateItemFucntion(int rtti)
{
    return factoryFunctionMap().contains(rtti);
}

/**
 * @brief 工程函数map
 * @return
 */
QHash< int, DAChart3DPlotItemFactory::Fp3DItemCreate >& DAChart3DPlotItemFactory::factoryFunctionMap()
{
    static QHash< int, DAChart3DPlotItemFactory::Fp3DItemCreate > s_Item3DCreateFp = initDAChart3DPlotItemFactory();
    return s_Item3DCreateFp;
}

}  // namespace DA
