#ifndef DACHART3DCOMMONITEMSSETTINGWIDGET_H
#define DACHART3DCOMMONITEMSSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include "DAAbstractChart3DItemSettingWidget.h"
#include <QMap>
class Qwt3DPlotItem;
namespace Ui
{
class DAChart3DCommonItemsSettingWidget;
}
namespace DA
{
class DAChart3DItemSettingPanel;

/**
 * @brief 3D通用图表项设置窗口，集成所有3D设置面板
 *
 * 使用DAChart3DItemSettingPanelFactory动态创建不同类型3D图表项的设置面板，
 * 并通过QMap缓存已创建的实例。
 *
 * @see DAChart3DItemSettingPanelFactory
 * @see DAChart3DItemSettingPanel
 */
class DAGUI_API DAChart3DCommonItemsSettingWidget : public DAAbstractChart3DItemSettingWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChart3DCommonItemsSettingWidget)
public:
    explicit DAChart3DCommonItemsSettingWidget(QWidget* parent = nullptr);
    ~DAChart3DCommonItemsSettingWidget();
    // 根据item的rtti查找/创建/缓存面板并切换显示
    virtual void updateUI(Qwt3DPlotItem* item) override;

private:
    Ui::DAChart3DCommonItemsSettingWidget* ui;
};
}  // namespace DA
#endif  // DACHART3DCOMMONITEMSSETTINGWIDGET_H
