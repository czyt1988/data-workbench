#ifndef DACHARTBARSETTINGPANEL_H
#define DACHARTBARSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_abstract_barchart.h"

namespace DA
{

/**
 * @brief QwtPlotBarChart属性设置面板
 *
 * 为条形图提供属性编辑界面，包含标题、Z值、坐标轴、图例模式、填充画刷、
 * 边框笔、基线、布局策略、布局提示、间距、边距等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotBarChart
 */
class DAGUI_API DAChartBarSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropLegendMode,
		PropEnableFill,
		PropFillBrush,
		PropEnableEdge,
		PropEdgePen,
		PropBaseline,
		PropLayoutPolicy,
		PropLayoutHint,
		PropSpacing,
		PropMargin
	};

	explicit DAChartBarSettingPanel(QWidget* parent = nullptr);
	~DAChartBarSettingPanel() override;

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

#endif  // DACHARTBARSETTINGPANEL_H
