#ifndef DACHARTTRADINGCURVESETTINGPANEL_H
#define DACHARTTRADINGCURVESETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_tradingcurve.h"

namespace DA
{

/**
 * @brief QwtPlotTradingCurve属性设置面板
 *
 * 为交易曲线提供属性编辑界面，包含标题、Z值、坐标轴、符号属性(Bar/Stick)、
 * 上涨画刷、下跌画刷、方向、符号宽度、最小价格、最大价格等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotTradingCurve
 */
class DAGUI_API DAChartTradingCurveSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropSymbolAttribute,
		PropIncreasingBrush,
		PropDecreasingBrush,
		PropOrientation,
		PropSymbolExtent,
		PropMinPrice,
		PropMaxPrice
	};

	explicit DAChartTradingCurveSettingPanel(QWidget* parent = nullptr);
	~DAChartTradingCurveSettingPanel() override;

protected:
	// 构建属性面板
	void buildPropertyPanel() override;

	// 从QwtPlotItem更新界面
	void updateUI(QwtPlotItem* item) override;

protected Q_SLOTS:
	// 属性值变化处理
	void onPropertyValueChanged(int propertyId);
};

}  // end namespace DA

#endif  // DACHARTTRADINGCURVESETTINGPANEL_H
