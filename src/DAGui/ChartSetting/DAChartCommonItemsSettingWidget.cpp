#include "DAChartCommonItemsSettingWidget.h"
#include "ui_DAChartCommonItemsSettingWidget.h"
#include "DAChartItemSettingPanelFactory.h"
#include "DAChartItemSettingPanel.h"

namespace DA
{

class DAChartCommonItemsSettingWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAChartCommonItemsSettingWidget)
public:
	PrivateData(DAChartCommonItemsSettingWidget* p);
	QMap< int, DAChartItemSettingPanel* > mPanelCache;  ///< 按RTTI缓存面板实例
};

DAChartCommonItemsSettingWidget::PrivateData::PrivateData(DAChartCommonItemsSettingWidget* p) : q_ptr(p)
{
}

//===============================================================
// DAChartCommonItemsSettingWidget
//===============================================================

DAChartCommonItemsSettingWidget::DAChartCommonItemsSettingWidget(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartCommonItemsSettingWidget)
{
	ui->setupUi(this);
	// 注意：面板注册在DAChartSettingWidget构造函数中统一调用registerAllKnownPanels()
}

DAChartCommonItemsSettingWidget::~DAChartCommonItemsSettingWidget()
{
	DA_D(d);
	// 清理缓存的面板
	qDeleteAll(d->mPanelCache);
	d->mPanelCache.clear();
	delete ui;
}

void DAChartCommonItemsSettingWidget::updateUI(QwtPlotItem* item)
{
	DA_D(d);
	if (nullptr == item) {
		return;
	}

	int rtti = item->rtti();

	// 1. 尝试从缓存获取
	DAChartItemSettingPanel* panel = nullptr;
	if (d->mPanelCache.contains(rtti)) {
		panel = d->mPanelCache[rtti];
	}

	// 2. 缓存未命中，通过工厂创建
	if (nullptr == panel) {
		panel = DAChartItemSettingPanelFactory::instance().createPanel(rtti);
		if (nullptr == panel) {
			// 未注册的RTTI类型，不做处理
			return;
		}
		d->mPanelCache[rtti] = panel;
		ui->stackedWidget->addWidget(panel);
	}

	// 3. 切换到对应面板并设置item
	ui->stackedWidget->setCurrentWidget(panel);
	panel->setPlotItem(item);
}

}  // end DA
