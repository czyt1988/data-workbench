#ifndef DACHARTGRIDSETTINGPANEL_H
#define DACHARTGRIDSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_grid.h"

namespace DA
{

/**
 * @brief QwtPlotGrid属性设置面板
 *
 * 为网格线提供属性编辑界面，包含标题、Z值、坐标轴、主/次网格线笔等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotGrid
 */
class DAGUI_API DAChartGridSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropMajorPen,
		PropMinorPen
	};

	explicit DAChartGridSettingPanel(QWidget* parent = nullptr);
	~DAChartGridSettingPanel() override;

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

#endif  // DACHARTGRIDSETTINGPANEL_H
