#ifndef DACHARTMULTIBARSETTINGPANEL_H
#define DACHARTMULTIBARSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_multi_barchart.h"

namespace DA
{

/**
 * @brief QwtPlotMultiBarChart属性设置面板
 *
 * 为多组柱状图提供属性编辑界面，包含标题、Z值、坐标轴、方向、
 * 图表样式（分组/堆叠）、布局策略、间距、边距及基线等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotMultiBarChart
 */
class DAGUI_API DAChartMultiBarSettingPanel : public DAChartItemSettingPanel
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
		PropChartStyle,
		PropLayoutPolicy,
		PropLayoutHint,
		PropSpacing,
		PropMargin,
		PropBaseline
	};

	explicit DAChartMultiBarSettingPanel(QWidget* parent = nullptr);
	~DAChartMultiBarSettingPanel() override;

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

#endif  // DACHARTMULTIBARSETTINGPANEL_H
