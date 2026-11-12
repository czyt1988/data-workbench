#include "Chart/Setting/DAChartCommonItemsSettingWidget.h"
#include "ui_DAChartCommonItemsSettingWidget.h"
#include "Chart/Setting/DAChartItemSettingPanelFactory.h"
#include "Chart/Setting/DAChartItemSettingPanel.h"

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
			// 防御性判断：工厂已保证返回非空（未注册rtti回退通用面板），此处理论上不会命中
			qWarning("DAChartCommonItemsSettingWidget::updateUI: factory returned null panel for rtti %d", rtti);
			return;
		}
		d->mPanelCache[rtti] = panel;
		ui->stackedWidget->addWidget(panel);
	}

	// 3. 切换到对应面板并设置item
	ui->stackedWidget->setCurrentWidget(panel);
	panel->setPlotItem(item);
}

/**
 * @brief 获取当前显示的图元设置面板
 * @return 当前面板指针，堆叠窗口为空时返回nullptr
 */
DAChartItemSettingPanel* DAChartCommonItemsSettingWidget::currentItemPanel() const
{
	return qobject_cast< DAChartItemSettingPanel* >(ui->stackedWidget->currentWidget());
}

}  // end DA
