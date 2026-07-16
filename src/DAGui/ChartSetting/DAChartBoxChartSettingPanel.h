#ifndef DACHARTBOXCHARTSETTINGPANEL_H
#define DACHARTBOXCHARTSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_boxchart.h"

namespace DA
{

/**
 * @brief QwtPlotBoxChart属性设置面板
 *
 * 为箱线图（盒须图）提供属性编辑界面，包含标题、Z值、坐标轴、方向、
 * 箱体样式/宽度范围、画笔/画刷、须线样式、中位数线、均值标记及
 * 离群点抖动等属性。
 *
 * @note MedianVisible 为 false 时禁用 PropMedianPen。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotBoxChart
 */
class DAGUI_API DAChartBoxChartSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropOrientation,
		PropBoxStyle,
		PropBoxExtent,
		PropMinBoxWidth,
		PropMaxBoxWidth,
		PropPen,
		PropBrush,
		PropWhiskerStyle,
		PropMedianVisible,
		PropMedianPen,
		PropMeanVisible,
		PropOutlierJitter
	};

	explicit DAChartBoxChartSettingPanel(QWidget* parent = nullptr);
	~DAChartBoxChartSettingPanel() override;

protected:
	// 构建属性面板
	void buildPropertyPanel() override;

	// 从QwtPlotItem更新界面
	void updateUI(QwtPlotItem* item) override;

	// 根据中位数线可见性启用/禁用中位数线画笔属性
	void updateMedianPenEnabled(bool visible);

protected Q_SLOTS:
	// 属性值变化处理
	void onPropertyValueChanged(int propertyId);
};

}  // end namespace DA

#endif  // DACHARTBOXCHARTSETTINGPANEL_H
