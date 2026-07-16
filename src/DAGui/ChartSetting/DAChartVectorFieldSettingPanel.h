#ifndef DACHARTVECTORFIELDSETTINGPANEL_H
#define DACHARTVECTORFIELDSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_vectorfield.h"

namespace DA
{

/**
 * @brief QwtPlotVectorField属性设置面板
 *
 * 为矢量场图提供属性编辑界面，包含标题、Z值、坐标轴、画笔、画刷、
 * 箭头原点、幅值映射模式、箭头长度范围、缩放因子、过滤及栅格等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotVectorField
 */
class DAGUI_API DAChartVectorFieldSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropPen,
		PropBrush,
		PropIndicatorOrigin,
		PropMagnitudeAsColor,
		PropMagnitudeAsLength,
		PropMinArrowLength,
		PropMaxArrowLength,
		PropMagnitudeScaleFactor,
		PropFilterVectors,
		PropRasterSizeW,
		PropRasterSizeH
	};

	explicit DAChartVectorFieldSettingPanel(QWidget* parent = nullptr);
	~DAChartVectorFieldSettingPanel() override;

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

#endif  // DACHARTVECTORFIELDSETTINGPANEL_H
