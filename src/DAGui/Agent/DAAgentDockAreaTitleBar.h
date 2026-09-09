#ifndef DAAGENTDOCKAREATITLEBAR_H
#define DAAGENTDOCKAREATITLEBAR_H
#include "DAGuiAPI.h"
// ADS
#include "DockAreaTitleBar.h"
class QToolButton;

namespace DA
{
class DAAgentDockWidget;

/**
 * @brief Agent 会话区域的嵌套 dock 标题栏
 *
 * 继承 ads::CDockAreaTitleBar，在标签页右侧内置按钮组（list all tabs /
 * detach group / pin / close）的最左侧插入两个自定义 QToolButton：
 * 「+ 新建会话」与「会话管理」，点击经宿主 DAAgentDockWidget 的统一入口处理
 *（requestNewSession / requestShowSessionManager）。
 *
 * 经 DAAgentSessionComponentsFactory::createDockAreaTitleBar 注入，仅影响
 * Agent 会话嵌套管理器内的 dock area，不影响顶层停靠区。
 */
class DAGUI_API DAAgentDockAreaTitleBar : public ads::CDockAreaTitleBar
{
    Q_OBJECT
public:
    using Super = ads::CDockAreaTitleBar;
    explicit DAAgentDockAreaTitleBar(ads::CDockAreaWidget* parent, DAAgentDockWidget* host);
    ~DAAgentDockAreaTitleBar();

private:
    DAAgentDockWidget* mHost;  ///< 宿主（按钮点击入口），生命周期长于标题栏（父链）
    QToolButton* mNewSessionBtn = nullptr;
    QToolButton* mSessionManagerBtn = nullptr;
};
}  // namespace DA
#endif  // DAAGENTDOCKAREATITLEBAR_H
