// DAAgentDockAreaTitleBar.cpp
#include "DAAgentDockAreaTitleBar.h"
#include "DAAgentDockWidget.h"
// Qt
#include <QToolButton>
#include <QIcon>

namespace DA
{

/**
 * @brief 构造标题栏：内置按钮创建完成后插入自定义按钮
 * @param parent 所属 dock area
 * @param host 宿主（按钮点击统一入口）
 */
DAAgentDockAreaTitleBar::DAAgentDockAreaTitleBar(ads::CDockAreaWidget* parent, DAAgentDockWidget* host)
    : Super(parent), mHost(host)
{
    // 插入位置：内置按钮组（tabs menu / detach / pin / close）最左侧——
    // 即 tabs menu 按钮的当前位置，布局为 [TabBar][Spacer][按钮组...]，
    // 插到 indexOf(TabsMenuButton) 使自定义按钮位于按钮组首位
    const int insertIndex = indexOf(button(ads::TitleBarButtonTabsMenu));

    mNewSessionBtn = new QToolButton(this);
    mNewSessionBtn->setObjectName(QStringLiteral("da_agentNewSessionBtn"));
    mNewSessionBtn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-new.svg")));
    mNewSessionBtn->setToolTip(tr("New Session"));  // cn:新建会话
    mNewSessionBtn->setAutoRaise(true);
    mNewSessionBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    mNewSessionBtn->setCursor(Qt::PointingHandCursor);
    mSessionManagerBtn = new QToolButton(this);
    mSessionManagerBtn->setObjectName(QStringLiteral("da_agentSessionManagerBtn"));
    mSessionManagerBtn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-manager.svg")));
    mSessionManagerBtn->setToolTip(tr("Session Manager"));  // cn:会话管理
    mSessionManagerBtn->setAutoRaise(true);
    mSessionManagerBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    mSessionManagerBtn->setCursor(Qt::PointingHandCursor);
    insertWidget(insertIndex, mSessionManagerBtn);
    insertWidget(insertIndex, mNewSessionBtn);

    if (mHost) {
        connect(mNewSessionBtn, &QToolButton::clicked, mHost, &DAAgentDockWidget::requestNewSession);
        connect(mSessionManagerBtn, &QToolButton::clicked, mHost, &DAAgentDockWidget::requestShowSessionManager);
    }
}

DAAgentDockAreaTitleBar::~DAAgentDockAreaTitleBar()
{
}

}  // namespace DA
