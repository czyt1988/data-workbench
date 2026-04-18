#ifndef DACHARTCOMMONITEMSSETTINGWIDGET_H
#define DACHARTCOMMONITEMSSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartItemSettingWidget.h"
#include <QMap>
class QwtPlotItem;
namespace Ui
{
class DAChartCommonItemsSettingWidget;
}
namespace DA
{
class DAChartItemSettingPanel;

/**
 * @brief 这是一个通用的设置窗口，集成了已有的所有设置窗口
 *
 * 使用DAChartItemSettingPanelFactory动态创建不同类型图表项的设置面板，
 * 并通过QMap缓存已创建的实例避免频繁创建/销毁。
 *
 * @see DAChartItemSettingPanelFactory
 * @see DAChartItemSettingPanel
 */
class DAGUI_API DAChartCommonItemsSettingWidget : public DAAbstractChartItemSettingWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAChartCommonItemsSettingWidget)
public:
	explicit DAChartCommonItemsSettingWidget(QWidget* parent = nullptr);
	~DAChartCommonItemsSettingWidget();
	//
    virtual void updateUI(QwtPlotItem* item) override;

private:
	Ui::DAChartCommonItemsSettingWidget* ui;
};
}  // end DA
#endif  // DACHARTCOMMONITEMSSETTINGWIDGET_H
