#ifndef DACHARTSCALESETTINGPANEL_H
#define DACHARTSCALESETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_scaleitem.h"

namespace DA
{

/**
 * @brief QwtPlotScaleItem属性设置面板
 *
 * 为画布刻度尺提供属性编辑界面，包含标题、Z值、坐标轴、对齐方式、
 * 位置、边框距离、字体及刻度同步等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotScaleItem
 */
class DAGUI_API DAChartScaleSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropAlignment,
		PropPosition,
		PropBorderDistance,
		PropFont,
		PropScaleDivFromAxis
	};

	explicit DAChartScaleSettingPanel(QWidget* parent = nullptr);
	~DAChartScaleSettingPanel() override;

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

#endif  // DACHARTSCALESETTINGPANEL_H
