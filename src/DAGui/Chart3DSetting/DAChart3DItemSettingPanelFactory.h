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

    // 获取工厂单例实例
    static DAChart3DItemSettingPanelFactory& instance();

    // 注册面板创建函数
    void registerPanel(int rtti, FpCreatePanel creator);

    // 根据RTTI创建对应面板
    DAChart3DItemSettingPanel* createPanel(int rtti) const;

    // 检查RTTI类型是否已注册
    bool isRegistered(int rtti) const;

    // 获取所有已注册的RTTI类型列表
    QList< int > registeredRttiTypes() const;

    // 显式注册所有已知3D面板类型
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
