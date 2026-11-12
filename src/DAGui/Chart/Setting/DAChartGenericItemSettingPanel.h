#ifndef DACHARTGENERICITEMSETTINGPANEL_H
#define DACHARTGENERICITEMSETTINGPANEL_H

#include "DAGuiAPI.h"
#include "Chart/Setting/DAChartItemSettingPanel.h"

namespace DA
{

/**
 * @brief 通用图元属性设置面板
 *
 * 作为 DAChartItemSettingPanelFactory 对未注册 rtti 图元的回退面板，
 * 只提供所有 QwtPlotItem 都具备的通用属性：标题、图例可见、可见性、Z值。
 * 这样选中插件自定义图元时不再停留上一个图元的面板。
 *
 * @see DAChartItemSettingPanel
 * @see DAChartItemSettingPanelFactory
 * @see QwtPlotItem
 */
class DAGUI_API DAChartGenericItemSettingPanel : public DAChartItemSettingPanel
{
	Q_OBJECT
public:
	// 属性ID枚举
	enum PropertyId {
		PropTitle = 0,
		PropLegend,
		PropVisible,
		PropZ
	};

	explicit DAChartGenericItemSettingPanel(QWidget* parent = nullptr);
	~DAChartGenericItemSettingPanel() override;

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

#endif  // DACHARTGENERICITEMSETTINGPANEL_H
