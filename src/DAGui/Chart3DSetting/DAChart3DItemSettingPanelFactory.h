#ifndef DACHART3DITEMSETTINGPANELFACTORY_H
#define DACHART3DITEMSETTINGPANELFACTORY_H
#include "DAGuiAPI.h"
#include "DAChart3DItemSettingPanel.h"
#include <functional>
#include <QList>
#include <QMap>

namespace DA
{

/**
 * @brief 3D图表项设置面板工厂类（单例）
 *
 * 通过3D RTTI类型映射到对应的设置面板创建函数。
 * 插件可通过registerPanel()扩展自定义3D图表项面板。
 *
 * @see DAChart3DItemSettingPanel
 */
class DAGUI_API DAChart3DItemSettingPanelFactory
{
public:
    using FpCreatePanel = std::function< DAChart3DItemSettingPanel* () >;

    /**
     * @brief 获取工厂单例实例
     */
    static DAChart3DItemSettingPanelFactory& instance();

    /**
     * @brief 注册面板创建函数
     * @param rtti 3D RTTI类型值（如Rtti_Plot3DSurface等）
     * @param creator 面板创建函数
     */
    void registerPanel(int rtti, FpCreatePanel creator);

    /**
     * @brief 根据RTTI创建对应面板
     * @param rtti 3D RTTI类型值
     * @return 创建的面板实例，失败返回nullptr
     */
    DAChart3DItemSettingPanel* createPanel(int rtti) const;

    /**
     * @brief 检查RTTI类型是否已注册
     */
    bool isRegistered(int rtti) const;

    /**
     * @brief 获取所有已注册的RTTI类型列表
     */
    QList< int > registeredRttiTypes() const;

    /**
     * @brief 显式注册所有已知3D面板类型
     *
     * 在DAChart3DSettingWidget构造时调用一次。
     * 包括：Surface、Bar、Line三个面板。
     * @note 本计划阶段留空（具体面板类在07计划中实现），
     *       07计划完成后在此函数体中补充注册代码。
     */
    void registerAllKnown3DPanels();

private:
    DAChart3DItemSettingPanelFactory() = default;
    ~DAChart3DItemSettingPanelFactory() = default;
    DAChart3DItemSettingPanelFactory(const DAChart3DItemSettingPanelFactory&) = delete;
    DAChart3DItemSettingPanelFactory& operator=(const DAChart3DItemSettingPanelFactory&) = delete;

private:
    QMap< int, FpCreatePanel > mCreators;  ///< RTTI → 创建函数映射
};

}  // namespace DA

#endif  // DACHART3DITEMSETTINGPANELFACTORY_H
