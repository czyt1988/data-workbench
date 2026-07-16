#ifndef DACHARTZONESETTINGPANEL_H
#define DACHARTZONESETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_zoneitem.h"

namespace DA
{

/**
 * @brief QwtPlotZoneItem属性设置面板
 *
 * 为坐标轴区间高亮带提供属性编辑界面，包含标题、Z值、坐标轴、方向、
 * 区间范围、边框画笔及填充画刷等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotZoneItem
 */
class DAGUI_API DAChartZoneSettingPanel : public DAChartItemSettingPanel
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
		PropIntervalMin,
		PropIntervalMax,
		PropPen,
		PropBrush
	};

	explicit DAChartZoneSettingPanel(QWidget* parent = nullptr);
	~DAChartZoneSettingPanel() override;

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

#endif  // DACHARTZONESETTINGPANEL_H
