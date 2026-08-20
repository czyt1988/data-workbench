#ifndef DAFIGUREDOCKWIDGETTAB_H
#define DAFIGUREDOCKWIDGETTAB_H
#include "DAGuiAPI.h"
// ADS
#include "DockWidgetTab.h"
QT_FORWARD_DECLARE_CLASS(QMenu)
namespace DA
{
class DAFigureWidget;
/**
 * @brief 绘图 dock 的自定义选项卡
 *
 * 继承 ads::CDockWidgetTab，重写 buildContextMenu，在 ADS 默认右键菜单项
 * （Detach / Pin / Close ...）之后追加自定义 action（如重命名）。
 *
 * 通过 DAFigureComponentsFactory 注册到 DAChartOperateWidget 的嵌套
 * ads::CDockManager，仅影响绘图 dock 的选项卡右键菜单，不影响顶层停靠区。
 */
class DAGUI_API DAFigureDockWidgetTab : public ads::CDockWidgetTab
{
    Q_OBJECT
public:
    using Super = ads::CDockWidgetTab;
    explicit DAFigureDockWidgetTab(ads::CDockWidget* dockWidget, QWidget* parent = nullptr);
    ~DAFigureDockWidgetTab();
protected:
    // 重写：先填充 ADS 默认菜单项，再追加自定义 action（重命名）
    QMenu* buildContextMenu(QMenu* menu) override;
private:
    // 取本 tab 所属 dock 封装的绘图窗口，无法解析时返回 nullptr
    DAFigureWidget* figureWidget() const;
};
}  // namespace DA
#endif  // DAFIGUREDOCKWIDGETTAB_H
