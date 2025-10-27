#ifndef DACHARTCOMMONITEMSSETTINGWIDGET_H
#define DACHARTCOMMONITEMSSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChartItemSettingWidget.h"
#include <QWidget>
class QwtPlotItem;
namespace Ui
{
class DAChartCommonItemsSettingWidget;
}
namespace DA
{
/**
 * @brief 这是一个通用的设置窗口，集成了已有的所有设置窗口
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
