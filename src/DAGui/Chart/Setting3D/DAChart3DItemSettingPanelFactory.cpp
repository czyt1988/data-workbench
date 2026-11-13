#include "Chart/Setting3D/DAChart3DItemSettingPanelFactory.h"
#include "Chart/Setting3D/DAChart3DSurfaceSettingPanel.h"
#include "Chart/Setting3D/DAChart3DBarSettingPanel.h"
#include "Chart/Setting3D/DAChart3DLineSettingPanel.h"
#include "Chart/Setting3D/DAChart3DGenericItemSettingPanel.h"
#include "qwt3d_types.h"

namespace DA
{

/**
 * @brief 获取工厂单例实例
 */
DAChart3DItemSettingPanelFactory& DAChart3DItemSettingPanelFactory::instance()
{
    static DAChart3DItemSettingPanelFactory sInstance;
    return sInstance;
}

/**
 * @brief 注册面板创建函数
 */
void DAChart3DItemSettingPanelFactory::registerPanel(int rtti, FpCreatePanel creator)
{
    if (creator) {
        mCreators[rtti] = creator;
    }
}

/**
 * @brief 根据RTTI创建对应面板
 *
 * 未注册的RTTI（如插件自定义3D图元）不会返回nullptr，而是回退到
 * @ref DAChart3DGenericItemSettingPanel 通用面板，避免设置面板停留在上一个图元（SF-08）。
 * @param rtti 3D RTTI类型值
 * @return 创建的面板实例（未注册时返回通用回退面板）
 */
DAChart3DItemSettingPanel* DAChart3DItemSettingPanelFactory::createPanel(int rtti) const
{
    auto it = mCreators.constFind(rtti);
    if (it != mCreators.constEnd() && it.value()) {
        return it.value()();
    }
    // 未注册（或注册的创建函数失效）：回退到通用3D图元面板
    return new DAChart3DGenericItemSettingPanel();
}

/**
 * @brief 检查RTTI类型是否已注册
 */
bool DAChart3DItemSettingPanelFactory::isRegistered(int rtti) const
{
    return mCreators.contains(rtti);
}

/**
 * @brief 获取所有已注册的RTTI类型列表
 */
QList< int > DAChart3DItemSettingPanelFactory::registeredRttiTypes() const
{
    return mCreators.keys();
}

/**
 * @brief 显式注册所有已知3D面板类型
 *
 * 注册内置的3D图表项面板类型，包括 Surface、Bar、Line；
 * Qwt3DWaterfall 暂无专属面板，显式注册到通用面板（未注册 rtti 也会回退通用面板）。
 */
void DAChart3DItemSettingPanelFactory::registerAllKnown3DPanels()
{
    // Qwt3DSurface → Surface 设置面板
    registerPanel(Rtti_Plot3DSurface, []() {
        return new DAChart3DSurfaceSettingPanel();
    });

    // Qwt3DBar → Bar 设置面板
    registerPanel(Rtti_Plot3DBar, []() {
        return new DAChart3DBarSettingPanel();
    });

    // Qwt3DLine → Line 设置面板
    registerPanel(Rtti_Plot3DLine, []() {
        return new DAChart3DLineSettingPanel();
    });

    // Qwt3DWaterfall → 通用设置面板（信号分析瀑布视图在用，暂无专属面板）
    registerPanel(Rtti_Plot3DWaterfall, []() {
        return new DAChart3DGenericItemSettingPanel();
    });
}

}  // namespace DA
