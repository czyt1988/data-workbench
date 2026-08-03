#include "DAChart3DCommonItemsSettingWidget.h"
#include "ui_DAChart3DCommonItemsSettingWidget.h"
#include "DAChart3DItemSettingPanelFactory.h"
#include "DAChart3DItemSettingPanel.h"

namespace DA
{

class DAChart3DCommonItemsSettingWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChart3DCommonItemsSettingWidget)
public:
    PrivateData(DAChart3DCommonItemsSettingWidget* p);
    QMap< int, DAChart3DItemSettingPanel* > mPanelCache;  ///< 按RTTI缓存面板实例
};

DAChart3DCommonItemsSettingWidget::PrivateData::PrivateData(DAChart3DCommonItemsSettingWidget* p) : q_ptr(p)
{
}

//===============================================================
// DAChart3DCommonItemsSettingWidget
//===============================================================

DAChart3DCommonItemsSettingWidget::DAChart3DCommonItemsSettingWidget(QWidget* parent)
    : DAAbstractChart3DItemSettingWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChart3DCommonItemsSettingWidget)
{
    ui->setupUi(this);
    // 注意：面板注册在DAChart3DSettingWidget构造函数中统一调用registerAllKnown3DPanels()
}

DAChart3DCommonItemsSettingWidget::~DAChart3DCommonItemsSettingWidget()
{
    DA_D(d);
    // 清理缓存的面板
    qDeleteAll(d->mPanelCache);
    d->mPanelCache.clear();
    delete ui;
}

/**
 * @brief 根据item的rtti查找/创建/缓存面板并切换显示
 *
 * 逻辑：
 * 1. 从缓存获取面板
 * 2. 缓存未命中，通过工厂创建
 * 3. 切换到对应面板并设置item
 */
void DAChart3DCommonItemsSettingWidget::updateUI(Qwt3DPlotItem* item)
{
    DA_D(d);
    if (nullptr == item) {
        return;
    }

    int rtti = item->rtti();

    // 1. 尝试从缓存获取
    DAChart3DItemSettingPanel* panel = nullptr;
    if (d->mPanelCache.contains(rtti)) {
        panel = d->mPanelCache[rtti];
    }

    // 2. 缓存未命中，通过工厂创建
    if (nullptr == panel) {
        panel = DAChart3DItemSettingPanelFactory::instance().createPanel(rtti);
        if (nullptr == panel) {
            // 未注册的RTTI类型，不做处理
            return;
        }
        d->mPanelCache[rtti] = panel;
        ui->stackedWidget->addWidget(panel);
    }

    // 3. 切换到对应面板并设置item
    ui->stackedWidget->setCurrentWidget(panel);
    panel->setPlot3DItem(item);
}

}  // namespace DA
