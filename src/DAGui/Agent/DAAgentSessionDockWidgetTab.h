#ifndef DAAGENTSESSIONDOCKWIDGETTAB_H
#define DAAGENTSESSIONDOCKWIDGETTAB_H
#include "DAGuiAPI.h"
// ADS
#include "DockWidgetTab.h"
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace DA
{
/**
 * @brief Agent 会话 dock 的自定义选项卡
 *
 * 继承 ads::CDockWidgetTab，重写 buildContextMenu，在 ADS 默认右键菜单项
 * （Detach / Pin / Close 等）之后追加自定义 action：重命名会话 / 停止会话
 * （仅运行中）/ 删除会话。经 DAAgentSessionComponentsFactory 注册到
 * DAAgentDockWidget 的嵌套 ads::CDockManager，仅影响会话 dock。
 */
class DAGUI_API DAAgentSessionDockWidgetTab : public ads::CDockWidgetTab
{
    Q_OBJECT
public:
    using Super = ads::CDockWidgetTab;
    explicit DAAgentSessionDockWidgetTab(ads::CDockWidget* dockWidget, QWidget* parent = nullptr);
    ~DAAgentSessionDockWidgetTab();

protected:
    // 重写：先填充 ADS 默认菜单项，再追加自定义 action（重命名/停止/删除）
    QMenu* buildContextMenu(QMenu* menu) override;

private:
    // 取宿主（dock 内容的顶层宿主窗口链上的 DAAgentDockWidget），无法解析时返回 nullptr
    class DAAgentDockWidget* hostWidget() const;
    // 本 tab 所属会话 ID（dock objectName；unbound 视图返回空）
    QString sessionId() const;
};
}  // namespace DA
#endif  // DAAGENTSESSIONDOCKWIDGETTAB_H
