#ifndef DACHARTSHAPESETTINGPANEL_H
#define DACHARTSHAPESETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_shapeitem.h"

namespace DA
{

/**
 * @brief QwtPlotShapeItem属性设置面板
 *
 * 为任意形状项提供属性编辑界面，包含标题、Z值、坐标轴、边框画笔、
 * 填充画刷、渲染容差、多边形裁剪及图例模式等属性。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotShapeItem
 */
class DAGUI_API DAChartShapeSettingPanel : public DAChartItemSettingPanel
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
		PropRenderTolerance,
		PropClipPolygons,
		PropLegendMode
	};

	explicit DAChartShapeSettingPanel(QWidget* parent = nullptr);
	~DAChartShapeSettingPanel() override;

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

#endif  // DACHARTSHAPESETTINGPANEL_H
