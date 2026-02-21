#ifndef DAPLOTCOMMONITEMSSETTINGWIDGET_H
#define DAPLOTCOMMONITEMSSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractPlotItemSettingWidget.h"
#include <QWidget>
namespace QIM
{
class QImPlotItemNode;
}
namespace Ui
{
class DAPlotCommonItemsSettingWidget;
}
namespace DA
{
/**
 * @brief 这是一个通用的设置窗口，集成了已有的所有设置窗口
 */
class DAGUI_API DAPlotCommonItemsSettingWidget : public DAAbstractPlotItemSettingWidget
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPlotCommonItemsSettingWidget)
public:
	explicit DAPlotCommonItemsSettingWidget(QWidget* parent = nullptr);
	~DAPlotCommonItemsSettingWidget();
	//
	virtual void updateUI(QIM::QImPlotItemNode* item) override;

private:
	Ui::DAPlotCommonItemsSettingWidget* ui;
};
}  // end DA
#endif  // DAPLOTCOMMONITEMSSETTINGWIDGET_H
