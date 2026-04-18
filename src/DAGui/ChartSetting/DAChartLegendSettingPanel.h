#ifndef DACHARTLEGENDSETTINGPANEL_H
#define DACHARTLEGENDSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_legenditem.h"

namespace DA
{

/**
 * @brief QwtPlotLegendItem属性设置面板
 *
 * 为图例项提供属性编辑界面，包含标题、Z值、对齐方式、偏移、边距、间距、
 * 最大列数、边框圆角、边框笔、字体、字体颜色、背景画刷等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotLegendItem
 */
class DAGUI_API DAChartLegendSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropAlignment,
		PropHorizontalOffset,
		PropVerticalOffset,
		PropMargin,
		PropSpacing,
		PropItemMargin,
		PropItemSpacing,
		PropMaxColumns,
		PropBorderRadius,
		PropBorderPen,
		PropFont,
		PropFontColor,
		PropBackgroundBrush
	};

	explicit DAChartLegendSettingPanel(QWidget* parent = nullptr);
	~DAChartLegendSettingPanel() override;

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

#endif  // DACHARTLEGENDSETTINGPANEL_H
