#ifndef DACHARTDATAPROBEMARKERSETTINGPANEL_H
#define DACHARTDATAPROBEMARKERSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "DADataProbeMarker.h"

namespace DA
{

/**
 * @brief DADataProbeMarker属性设置面板
 *
 * 为数据探针标记提供属性编辑界面，包含探针名称、Z值、探针位置值、
 * 标签位置、标签样式、标签可见性、探针颜色等属性。
 *
 * @see DAChartItemSettingPanel
 * @see DADataProbeMarker
 */
class DAGUI_API DAChartDataProbeMarkerSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropProbeValue,
		PropLabelPosition,
		PropLabelStyle,
		PropLabelVisible,
		PropProbeColor
	};

	explicit DAChartDataProbeMarkerSettingPanel(QWidget* parent = nullptr);
	~DAChartDataProbeMarkerSettingPanel() override;

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

#endif  // DACHARTDATAPROBEMARKERSETTINGPANEL_H
