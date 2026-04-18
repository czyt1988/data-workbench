#ifndef DACHARTSPECTROGRAMSETTINGPANEL_H
#define DACHARTSPECTROGRAMSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_spectrogram.h"

namespace DA
{

/**
 * @brief QwtPlotSpectrogram属性设置面板
 *
 * 为频谱图提供属性编辑界面，包含标题、Z值、坐标轴、显示模式、
 * 起始颜色、结束颜色、轮廓线笔等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotSpectrogram
 */
class DAGUI_API DAChartSpectrogramSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropDisplayMode,
		PropFromColor,
		PropToColor,
		PropContourPen
	};

	explicit DAChartSpectrogramSettingPanel(QWidget* parent = nullptr);
	~DAChartSpectrogramSettingPanel() override;

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

#endif  // DACHARTSPECTROGRAMSETTINGPANEL_H
