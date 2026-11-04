/**
 * @file DAAppRibbonAreaAgent.cpp
 * @brief DAAppRibbonArea 的「AI 智能体」标签页实现
 *
 * 布局：| agent管理(large) | agent gallery(最大宽度 700px) | 执行agent/新建会话/会话管理(large) |
 * gallery 的每个 action 对应一条 agent 提示词，选中后由「执行agent」触发 runAgent。
 *
 * 提示词数据来自 DAAgentInterface::agentPromptOps()；画廊首次构建早于插件加载
 * （createUi 先于 initPlugins），因此额外监听 agentListChanged 做兜底刷新。
 */
#include "DAAppRibbonArea.h"
#include "DAUiObjectNames.h"
#include "AppMainWindow.h"
// Qt
#include <QIcon>
#include <utility>
#include <QMessageBox>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonGallery.h"
#include "SARibbonGalleryGroup.h"
// Agent 提示词库
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include "Dialog/DAAgentManagerDialog.h"
#include "DAAgentPrompt.h"
#include "DAAgentPromptOps.h"
// Agent 助手会话标签页宿主（session-tabs：新建会话/会话管理按钮入口）
#include "Agent/DAAgentDockWidget.h"
// api
#include "DAAppCore.h"
#include "DAAppActions.h"
#include "DAAppDockingArea.h"

using namespace DA;

/**
 * @brief 构建 AI分析 标签页
 * @param ribbon ribbon 栏（由 buildRibbon 注入）
 */
void DAAppRibbonArea::buildRibbonAgentCategory(SARibbonBar* ribbon)
{
    mCategoryAgent = ribbon->addCategoryPage(tr("AI Agent"));  // cn:AI智能体
    mCategoryAgent->setObjectName(QString::fromUtf8(UiNames::Ribbon::AgentCategory));
    mPanelAgent = mCategoryAgent->addPanel(tr("AI Agent"));  // cn:AI智能体

    // agent 管理（large button）
    mActionAgentManage = new QAction(QIcon(":/da/icon/agent-manage.svg"), tr("Agent Manager"), this);  // cn:Agent 管理
    mActionAgentManage->setToolTip(tr("Manage agents: add, edit, delete prompts"));  // cn:管理 agent：新增、修改、删除提示词
    mPanelAgent->addLargeAction(mActionAgentManage);

    // agent gallery（最大宽度 700px）
    mAgentGallery = mPanelAgent->addGallery(true);
    mAgentGallery->setMaximumWidth(700);

    // 执行 agent（large button）
    mActionRunAgent = new QAction(QIcon(":/da/icon/run-agent.svg"), tr("Run Agent"), this);  // cn:执行 Agent
    mActionRunAgent->setToolTip(
        tr("Run AI analysis with the selected agent prompt"));  // cn:使用当前选中的 agent 提示词执行 AI 分析
    mPanelAgent->addLargeAction(mActionRunAgent);

    // 会话操作（session-tabs，large button）：与 Agent 助手标题栏「+/会话管理」同一入口
    mActionAgentNewSession = new QAction(QIcon(":/DAGui/icon/session-new.svg"), tr("New Session"), this);  // cn:新建会话
    mActionAgentNewSession->setToolTip(tr("Open a new agent chat session"));  // cn:打开一个新的 Agent 会话标签页
    mPanelAgent->addLargeAction(mActionAgentNewSession);
    mActionAgentSessionManager =
        new QAction(QIcon(":/DAGui/icon/session-manager.svg"), tr("Session Manager"), this);  // cn:会话管理
    mActionAgentSessionManager->setToolTip(
        tr("Manage agent sessions: switch, rename, delete"));  // cn:管理 Agent 会话：切换、重命名、删除
    mPanelAgent->addLargeAction(mActionAgentSessionManager);

    populateAgentGallery();

    connect(mActionAgentManage, &QAction::triggered, this, &DAAppRibbonArea::onActionAgentManage);
    connect(mActionRunAgent, &QAction::triggered, this, &DAAppRibbonArea::onActionRunAgent);
    connect(mActionAgentNewSession, &QAction::triggered, this, &DAAppRibbonArea::onActionAgentNewSession);
    connect(mActionAgentSessionManager, &QAction::triggered, this, &DAAppRibbonArea::onActionAgentSessionManager);
    // gallery 首次构建早于插件加载（createUi 先于 initPlugins），插件注入的内置
    // agent 依赖此信号兜底刷新（保存/删除/插件 registerBuiltinAgent 均触发）；
    // 保留当前选中项，与管理对话框关闭后的刷新行为一致
    if (DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface()) {
        connect(agent, &DA::DAAgentInterface::agentListChanged, this, [ this ]() {
            QString prevTitle = mSelectedAgentTitle;
            populateAgentGallery();
            if (!prevTitle.isEmpty()) {
                mSelectedAgentTitle = prevTitle;
            }
        });
    }
}

/**
 * @brief 填充 agent gallery，从 DAAgentInterface::agentPromptOps 取提示词列表
 */
void DAAppRibbonArea::populateAgentGallery()
{
    if (!mAgentGallery) {
        return;
    }
    // 清空旧 action
    for (QAction* act : std::as_const(mAgentActions)) {
        act->deleteLater();
    }
    mAgentActions.clear();

    static QIcon sAgentIcon(":/da/icon/agent.svg");
    DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface();
    QList< DA::DAAgentPrompt > prompts;
    if (agent) {
        DA::DAAgentPromptOps* ops = agent->agentPromptOps();
        if (ops) {
            prompts = ops->agentPrompts();
        }
    }
    for (const DA::DAAgentPrompt& a : std::as_const(prompts)) {
        QAction* act = new QAction(sAgentIcon, a.title, this);
        // tooltip 显示完整提示词内容（富文本换行）
        QString tooltip =
            QString("<html><body><div style=\"white-space:pre-wrap; max-width:480px;\">%1</div></body></html>")
                .arg(a.content.toHtmlEscaped());
        act->setToolTip(tooltip);
        act->setData(a.title);
        mAgentActions.append(act);
    }
    if (!mAgentGalleryGroup) {
        // 首次构建：添加分组 + 样式 + 连接信号
        mAgentGalleryGroup = mAgentGallery->addCategoryActions(tr("Agent"), mAgentActions);  // cn:Agent
        mAgentGalleryGroup->setGalleryGroupStyle(SARibbonGalleryGroup::IconWithWordWrapText);
        mAgentGalleryGroup->setGridMinimumWidth(80);
        connect(mAgentGalleryGroup, &SARibbonGalleryGroup::triggered, this, &DAAppRibbonArea::onAgentGalleryTriggered);
    } else {
        // 已有分组：清空 model 重新填充
        SARibbonGalleryGroupModel* model = mAgentGalleryGroup->groupModel();
        model->clear();
        mAgentGalleryGroup->addActionItemList(mAgentActions);
    }
    // 默认选中第一个 agent
    if (!mAgentActions.isEmpty()) {
        onAgentGalleryTriggered(mAgentActions.first());
    } else {
        mSelectedAgentTitle.clear();
    }
}

/**
 * @brief agent 管理按钮：打开管理对话框，关闭后刷新 gallery
 */
void DAAppRibbonArea::onActionAgentManage()
{
    DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface();
    if (!agent) {
        return;
    }
    DA::DAAgentPromptOps* ops = agent->agentPromptOps();
    if (!ops) {
        return;
    }
    DA::DAAgentManagerDialog dlg(ops, agent, app());
    QString prevTitle = mSelectedAgentTitle;
    dlg.exec();
    // 管理结束后刷新 gallery（agentPrompts 直接读磁盘最新状态）
    populateAgentGallery();
    if (!prevTitle.isEmpty()) {
        mSelectedAgentTitle = prevTitle;
    }
}

/**
 * @brief gallery 选中项变化：记录当前选中的 agent 标题
 * @param act 被选中的 gallery action
 */
void DAAppRibbonArea::onAgentGalleryTriggered(QAction* act)
{
    if (!act) {
        return;
    }
    mSelectedAgentTitle = act->data().toString();
}

/**
 * @brief 执行 agent 按钮：调用 DAAgentInterface::runAgent 触发 AI 分析
 */
void DAAppRibbonArea::onActionRunAgent()
{
    if (mSelectedAgentTitle.isEmpty()) {
        QMessageBox::warning(app(),
                             tr("Tip"),                                           // cn:提示
                             tr("Please select an agent in the gallery first"));  // cn:请先在 gallery 中选择一个 agent
        return;
    }
    DA::DAAgentInterface* agent = DA_APP_CORE.getAgentInterface();
    if (!agent) {
        QMessageBox::warning(app(),
                             tr("Tip"),                         // cn:提示
                             tr("Agent module is not ready"));  // cn:Agent 模块未就绪
        return;
    }
    mActions->actionShowAgentArea->trigger();  // 确保 Agent dock 可见（ActionModeShow，不会 toggle 隐藏）
    agent->runAgent(mSelectedAgentTitle);
}

/**
 * @brief 新建会话按钮：确保 Agent dock 可见后走宿主「+」统一入口
 *
 * 与 Agent 助手标题栏「+」按钮同一入口（requestNewSession：已有 unbound 视图
 * 则 raise，否则创建懒建会话草稿区——首条消息发出后才落盘建会话）。
 */
void DAAppRibbonArea::onActionAgentNewSession()
{
    if (!mDockArea) {
        return;
    }
    mActions->actionShowAgentArea->trigger();  // 确保 Agent dock 可见
    if (DA::DAAgentDockWidget* dock = mDockArea->getAgentDockWidget()) {
        dock->requestNewSession();
    }
}

/**
 * @brief 会话管理按钮：确保 Agent dock 可见后走宿主「会话管理」统一入口
 *
 * 与 Agent 助手标题栏「会话管理」按钮同一入口（requestShowSessionManager：
 * 弹出 DADialogAgentSessionManager，操作经 Dock 信号链转发到接口层）。
 */
void DAAppRibbonArea::onActionAgentSessionManager()
{
    if (!mDockArea) {
        return;
    }
    mActions->actionShowAgentArea->trigger();  // 确保 Agent dock 可见
    if (DA::DAAgentDockWidget* dock = mDockArea->getAgentDockWidget()) {
        dock->requestShowSessionManager();
    }
}
