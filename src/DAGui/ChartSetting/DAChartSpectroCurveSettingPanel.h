#ifndef DACHARTSPECTROCURVESETTINGPANEL_H
#define DACHARTSPECTROCURVESETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_spectrocurve.h"

namespace DA
{

/**
 * @brief QwtPlotSpectroCurve属性设置面板
 *
 * 为三维数据投影曲线提供属性编辑界面，包含标题、Z值、坐标轴、方向、
 * 笔宽、颜色范围及裁剪点等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotSpectroCurve
 */
class DAGUI_API DAChartSpectroCurveSettingPanel : public DAChartItemSettingPanel
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
		PropPenWidth,
		PropColorRangeMin,
		PropColorRangeMax,
		PropClipPoints
	};

	explicit DAChartSpectroCurveSettingPanel(QWidget* parent = nullptr);
	~DAChartSpectroCurveSettingPanel() override;

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

#endif  // DACHARTSPECTROCURVESETTINGPANEL_H
