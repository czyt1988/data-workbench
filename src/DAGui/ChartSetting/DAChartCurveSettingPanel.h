#ifndef DACHARTCURVESETTINGPANEL_H
#define DACHARTCURVESETTINGPANEL_H
#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"

namespace DA
{
/**
 * @brief QwtPlotCurve属性设置面板
 *
 * 继承DAChartItemSettingPanel，使用DAPropertyPanelWidget通用便捷方法
 * 结合基类的Qwt专有方法，构建完整的曲线属性编辑界面。
 *
 * @see DAChartItemSettingPanel
 * @see DAPropertyPanelWidget
 */
class DAGUI_API DAChartCurveSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyID {
		PID_Title = 1,
		PID_ZValue = 2,
		PID_XAxis = 3,
		PID_YAxis = 4,
		PID_CurveStyle = 5,
		PID_Pen = 6,
		PID_EnableMarker = 7,
		PID_Symbol = 8,
		PID_Fitted = 9,
		PID_Inverted = 10,
		PID_LegendShowLine = 11,
		PID_LegendShowSymbol = 12,
		PID_LegendShowBrush = 13,
		PID_EnableFill = 14,
		PID_Fill = 15,
		PID_BaseLine = 16,
		PID_Orientation = 17
	};

	explicit DAChartCurveSettingPanel(QWidget* parent = nullptr);
	~DAChartCurveSettingPanel() override;

	// 根据QwtPlotCurve更新UI
	void updateUI(QwtPlotItem* item) override;

private:
	// 构建属性面板布局
	void buildPropertyPanel() override;

private Q_SLOTS:
	// 处理属性值变化
	void onCurvePropertyValueChanged(int propertyId);
};
}  // end namespace DA

#endif  // DACHARTCURVESETTINGPANEL_H
