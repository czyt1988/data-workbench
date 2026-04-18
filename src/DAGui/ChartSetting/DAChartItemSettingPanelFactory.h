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

    /**
     * @brief 获取工厂单例实例
     * @return 工厂单例引用
     */
    static DAChartItemSettingPanelFactory& instance();

    /**
     * @brief 注册面板创建函数
     * @param rtti QwtPlotItem的RTTI类型值
     * @param creator 面板创建函数
     */
    void registerPanel(int rtti, FpCreatePanel creator);

    /**
     * @brief 根据RTTI创建对应面板
     * @param rtti QwtPlotItem的RTTI类型值
     * @return 创建的面板实例，失败返回nullptr
     */
    DAChartItemSettingPanel* createPanel(int rtti) const;

    /**
     * @brief 检查RTTI类型是否已注册
     * @param rtti QwtPlotItem的RTTI类型值
     * @return 已注册返回true
     */
    bool isRegistered(int rtti) const;

    /**
     * @brief 获取所有已注册的RTTI类型列表
     * @return 已注册的RTTI值列表
     */
    QList< int > registeredRttiTypes() const;

    /**
     * @brief 显式注册所有已知面板类型
     *
     * 在DAChartSettingWidget构造时调用一次，注册所有内置面板类型。
     * 包括：Curve、BarChart、IntervalCurve、Spectrogram、TradingCurve、Grid、Legend
     */
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
