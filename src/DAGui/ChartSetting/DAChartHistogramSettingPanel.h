#ifndef DACHARTHISTOGRAMSETTINGPANEL_H
#define DACHARTHISTOGRAMSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_histogram.h"

namespace DA
{

/**
 * @brief QwtPlotHistogram属性设置面板
 *
 * 为直方图提供属性编辑界面，包含标题、Z值、坐标轴、方向、样式、
 * 画笔、画刷及基线等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotHistogram
 */
class DAGUI_API DAChartHistogramSettingPanel : public DAChartItemSettingPanel
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
		PropStyle,
		PropPen,
		PropBrush,
		PropBaseline
	};

	explicit DAChartHistogramSettingPanel(QWidget* parent = nullptr);
	~DAChartHistogramSettingPanel() override;

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

#endif  // DACHARTHISTOGRAMSETTINGPANEL_H
