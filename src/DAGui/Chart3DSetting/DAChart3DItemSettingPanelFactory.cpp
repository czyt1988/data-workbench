#include "DAChart3DItemSettingPanelFactory.h"
#include "DAChart3DSurfaceSettingPanel.h"
#include "DAChart3DBarSettingPanel.h"
#include "DAChart3DLineSettingPanel.h"
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
 */
DAChart3DItemSettingPanel* DAChart3DItemSettingPanelFactory::createPanel(int rtti) const
{
    auto it = mCreators.constFind(rtti);
    if (it != mCreators.constEnd() && it.value()) {
        return it.value()();
    }
    return nullptr;
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
 * 注册内置的3D图表项面板类型，包括 Surface、Bar、Line。
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
}

}  // namespace DA
