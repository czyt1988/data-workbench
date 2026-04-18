#ifndef DACHARTERRORBARSETTINGPANEL_H
#define DACHARTERRORBARSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_intervalcurve.h"

namespace DA
{

/**
 * @brief QwtPlotIntervalCurve属性设置面板
 *
 * 为误差棒提供属性编辑界面，包含标题、Z值、坐标轴、启用误差棒、
 * 误差棒样式(Bar/Stick)、误差棒笔、启用填充、填充画刷、方向、曲线笔等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotIntervalCurve
 */
class DAGUI_API DAChartErrorBarSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropEnableErrorBar,
		PropErrorBarStyle,
		PropErrorBarPen,
		PropEnableFill,
		PropFillBrush,
		PropOrientation,
		PropCurvePen
	};

	explicit DAChartErrorBarSettingPanel(QWidget* parent = nullptr);
	~DAChartErrorBarSettingPanel() override;

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

#endif  // DACHARTERRORBARSETTINGPANEL_H
