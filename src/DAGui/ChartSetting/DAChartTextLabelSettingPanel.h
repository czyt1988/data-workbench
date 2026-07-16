#ifndef DACHARTTEXTLABELSETTINGPANEL_H
#define DACHARTTEXTLABELSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_textlabel.h"

namespace DA
{

/**
 * @brief QwtPlotTextLabel属性设置面板
 *
 * 为画布文本标签提供属性编辑界面，包含标题、Z值、坐标轴、文本内容、
 * 字体、颜色、对齐、边框圆角、背景画刷及外边距等属性。
 *
 * @note QwtText 通过值副本返回，修改后需调用 setText() 写回。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotTextLabel
 */
class DAGUI_API DAChartTextLabelSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropText,
		PropFont,
		PropTextColor,
		PropTextAlignment,
		PropBorderRadius,
		PropBackgroundBrush,
		PropMargin
	};

	explicit DAChartTextLabelSettingPanel(QWidget* parent = nullptr);
	~DAChartTextLabelSettingPanel() override;

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

#endif  // DACHARTTEXTLABELSETTINGPANEL_H
