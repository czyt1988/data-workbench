#include "DAChartItemSettingPanelFactory.h"
#include "DAChartCurveSettingPanel.h"
#include "DAChartBarSettingPanel.h"
#include "DAChartErrorBarSettingPanel.h"
#include "DAChartSpectrogramSettingPanel.h"
#include "DAChartTradingCurveSettingPanel.h"
#include "DAChartGridSettingPanel.h"
#include "DAChartLegendSettingPanel.h"
#include "qwt_plot_item.h"

namespace DA
{

/**
 * @brief 获取工厂单例实例
 */
DAChartItemSettingPanelFactory& DAChartItemSettingPanelFactory::instance()
{
    static DAChartItemSettingPanelFactory sInstance;
    return sInstance;
}

/**
 * @brief 注册面板创建函数
 * @param rtti QwtPlotItem的RTTI类型值
 * @param creator 面板创建函数
 */
void DAChartItemSettingPanelFactory::registerPanel(int rtti, FpCreatePanel creator)
{
    if (creator) {
        mCreators[rtti] = creator;
    }
}

/**
 * @brief 根据RTTI创建对应面板
 * @param rtti QwtPlotItem的RTTI类型值
 * @return 创建的面板实例，失败返回nullptr
 */
DAChartItemSettingPanel* DAChartItemSettingPanelFactory::createPanel(int rtti) const
{
    auto it = mCreators.constFind(rtti);
    if (it != mCreators.constEnd() && it.value()) {
        return it.value()();
    }
    return nullptr;
}

/**
 * @brief 检查RTTI类型是否已注册
 * @param rtti QwtPlotItem的RTTI类型值
 * @return 已注册返回true
 */
bool DAChartItemSettingPanelFactory::isRegistered(int rtti) const
{
    return mCreators.contains(rtti);
}

/**
 * @brief 获取所有已注册的RTTI类型列表
 * @return 已注册的RTTI值列表
 */
QList< int > DAChartItemSettingPanelFactory::registeredRttiTypes() const
{
    return mCreators.keys();
}

/**
 * @brief 显式注册所有已知面板类型
 *
 * 注册内置的Qwt图表项面板类型，包括曲线、柱状图、误差棒、频谱图、
 * 交易曲线、网格、图例等。
 */
void DAChartItemSettingPanelFactory::registerAllKnownPanels()
{
    // QwtPlotCurve → 曲线设置面板
    registerPanel(QwtPlotItem::Rtti_PlotCurve, []() {
        return new DAChartCurveSettingPanel();
    });

    // QwtPlotBarChart → 柱状图设置面板
    registerPanel(QwtPlotItem::Rtti_PlotBarChart, []() {
        return new DAChartBarSettingPanel();
    });

    // QwtPlotIntervalCurve → 误差棒设置面板
    registerPanel(QwtPlotItem::Rtti_PlotIntervalCurve, []() {
        return new DAChartErrorBarSettingPanel();
    });

    // QwtPlotSpectrogram → 频谱图设置面板
    registerPanel(QwtPlotItem::Rtti_PlotSpectrogram, []() {
        return new DAChartSpectrogramSettingPanel();
    });

    // QwtPlotTradingCurve → 交易曲线设置面板
    registerPanel(QwtPlotItem::Rtti_PlotTradingCurve, []() {
        return new DAChartTradingCurveSettingPanel();
    });

    // QwtPlotGrid → 网格设置面板
    registerPanel(QwtPlotItem::Rtti_PlotGrid, []() {
        return new DAChartGridSettingPanel();
    });

    // QwtPlotLegendItem → 图例设置面板
    registerPanel(QwtPlotItem::Rtti_PlotLegend, []() {
        return new DAChartLegendSettingPanel();
    });
}

}  // end namespace DA
