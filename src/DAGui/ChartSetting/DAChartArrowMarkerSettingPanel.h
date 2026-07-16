#ifndef DACHARTARROWMARKERSETTINGPANEL_H
#define DACHARTARROWMARKERSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_arrowmarker.h"

namespace DA
{

/**
 * @brief QwtPlotArrowMarker属性设置面板
 *
 * 为箭头标记提供属性编辑界面，包含标题、Z值、坐标轴、定位模式
 * (显式起止点 / 起点+长度+角度)、起点/终点坐标、长度、角度、线条画笔、
 * 头部与尾部端点样式/尺寸/画笔/画刷等属性。
 *
 * @note PositionMode 切换时会启用/禁用相关属性:
 *       ExplicitPoints 模式启用 EndX/EndY,禁用 Length/Angle;
 *       StartLengthAngle 模式反之。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotArrowMarker
 */
class DAGUI_API DAChartArrowMarkerSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropPositionMode,
		PropStartX,
		PropStartY,
		PropEndX,
		PropEndY,
		PropLength,
		PropAngle,
		PropLinePen,
		PropHeadStyle,
		PropHeadSize,
		PropHeadPen,
		PropHeadBrush,
		PropTailStyle,
		PropTailSize,
		PropTailPen,
		PropTailBrush
	};

	explicit DAChartArrowMarkerSettingPanel(QWidget* parent = nullptr);
	~DAChartArrowMarkerSettingPanel() override;

protected:
	// 构建属性面板
	void buildPropertyPanel() override;

	// 从QwtPlotItem更新界面
	void updateUI(QwtPlotItem* item) override;

	// 根据定位模式启用/禁用相关属性
	void updatePositionModeVisibility(QwtPlotArrowMarker::PositionMode mode);

protected Q_SLOTS:
	// 属性值变化处理
	void onPropertyValueChanged(int propertyId);
};

}  // end namespace DA

#endif  // DACHARTARROWMARKERSETTINGPANEL_H
