#ifndef DACHARTMARKERSETTINGPANEL_H
#define DACHARTMARKERSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_marker.h"

namespace DA
{

/**
 * @brief QwtPlotMarker属性设置面板
 *
 * 为标记线/标记点提供属性编辑界面，包含标题、Z值、坐标轴、位置、
 * 线条样式、画笔、标签文本/对齐/方向及间距等属性。
 *
 * @note DADataProbeMarker 继承自 QwtPlotMarker 但有独立 RTTI，
 *       走 DAChartDataProbeMarkerSettingPanel，与本面板互不影响。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotMarker
 */
class DAGUI_API DAChartMarkerSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropLineStyle,
		PropLinePen,
		PropLabel,
		PropLabelAlignment,
		PropLabelOrientation,
		PropSpacing,
		PropXValue,
		PropYValue
	};

	explicit DAChartMarkerSettingPanel(QWidget* parent = nullptr);
	~DAChartMarkerSettingPanel() override;

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

#endif  // DACHARTMARKERSETTINGPANEL_H
