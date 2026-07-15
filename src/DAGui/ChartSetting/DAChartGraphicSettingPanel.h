#ifndef DACHARTGRAPHICSETTINGPANEL_H
#define DACHARTGRAPHICSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "DAChartItemSettingPanel.h"
#include "qwt_plot_graphicitem.h"

namespace DA
{

/**
 * @brief QwtPlotGraphicItem属性设置面板
 *
 * 为矢量图形项提供基础属性编辑界面，包含标题、Z值、坐标轴及
 * 抗锯齿渲染属性。图形内容由 QwtGraphic 决定，面板不提供内容编辑。
 *
 * @see DAChartItemSettingPanel
 * @see QwtPlotGraphicItem
 */
class DAGUI_API DAChartGraphicSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 1,
		PropZValue,
		PropXAxis,
		PropYAxis,
		PropRenderAntialiased
	};

	explicit DAChartGraphicSettingPanel(QWidget* parent = nullptr);
	~DAChartGraphicSettingPanel() override;

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

#endif  // DACHARTGRAPHICSETTINGPANEL_H
