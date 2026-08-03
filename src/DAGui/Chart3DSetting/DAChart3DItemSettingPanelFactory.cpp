#include "DAChart3DItemSettingPanelFactory.h"
// 07计划完成后在此添加具体面板的头文件:
// #include "DAChart3DSurfaceSettingPanel.h"
// #include "DAChart3DBarSettingPanel.h"
// #include "DAChart3DLineSettingPanel.h"
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
 * TODO: 07计划实现具体面板后，取消以下注释并注册：
 *
 * registerPanel(Rtti_Plot3DSurface, []() {
 *     return new DAChart3DSurfaceSettingPanel();
 * });
 * registerPanel(Rtti_Plot3DBar, []() {
 *     return new DAChart3DBarSettingPanel();
 * });
 * registerPanel(Rtti_Plot3DLine, []() {
 *     return new DAChart3DLineSettingPanel();
 * });
 */
void DAChart3DItemSettingPanelFactory::registerAllKnown3DPanels()
{
    // 本计划阶段：留空，待07计划完成后补充注册代码
}

}  // namespace DA
