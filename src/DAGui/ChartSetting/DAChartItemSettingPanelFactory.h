#ifndef DACHARTITEMSETTINGPANELFACTORY_H
#define DACHARTITEMSETTINGPANELFACTORY_H
#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include <functional>
#include <QList>

namespace DA
{

/**
 * @brief Qwt图表项设置面板工厂类（单例）
 *
 * 通过RTTI类型映射到对应的设置面板创建函数，消除DAChartCommonItemsSettingWidget中的switch-case路由。
 * 插件可通过registerPanel()扩展自定义图表项面板。
 *
 * @see DAChartItemSettingPanel
 *
 * @code
 * // 获取所有已注册的RTTI类型
 * QList<int> rttis = DAChartItemSettingPanelFactory::instance().registeredRttiTypes();
 *
 * // 根据RTTI创建面板
 * DAChartItemSettingPanel* panel = DAChartItemSettingPanelFactory::instance().createPanel(QwtPlotItem::Rtti_PlotCurve);
 *
 * // 注册自定义面板
 * DAChartItemSettingPanelFactory::instance().registerPanel(CustomRtti, [](){ return new MyCustomSettingPanel(); });
 * @endcode
 */
class DAGUI_API DAChartItemSettingPanelFactory
{
public:
    using FpCreatePanel = std::function< DAChartItemSettingPanel* () >;

    // 获取工厂单例实例
    static DAChartItemSettingPanelFactory& instance();

    // 注册面板创建函数
    void registerPanel(int rtti, FpCreatePanel creator);

    // 根据RTTI创建对应面板
    DAChartItemSettingPanel* createPanel(int rtti) const;

    // 检查RTTI类型是否已注册
    bool isRegistered(int rtti) const;

    // 获取所有已注册的RTTI类型列表
    QList< int > registeredRttiTypes() const;

    // 显式注册所有已知面板类型
    void registerAllKnownPanels();

private:
    // 构造函数私有化，单例模式
    DAChartItemSettingPanelFactory() = default;
    ~DAChartItemSettingPanelFactory() = default;
    // 禁止拷贝和赋值
    DAChartItemSettingPanelFactory(const DAChartItemSettingPanelFactory&) = delete;
    DAChartItemSettingPanelFactory& operator=(const DAChartItemSettingPanelFactory&) = delete;

private:
    QMap< int, FpCreatePanel > mCreators; ///< RTTI → 创建函数映射
};

}  // end namespace DA

#endif  // DACHARTITEMSETTINGPANELFACTORY_H
