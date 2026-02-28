#include "DAAppStatusBar.h"
#include "DAStatusBarWidget.h"
#include <QStatusBar>
#include <QToolButton>
#include "DAAppUI.h"
#include "AppMainWindow.h"
#include "DAAppDockingArea.h"
#include "DAAppActions.h"
namespace DA
{
DAAppStatusBar::DAAppStatusBar(DAUIInterface* u) : DAStatusBarInterface(u)
{
	DAAppUI* appui = qobject_cast< DAAppUI* >(u);
	m_app          = qobject_cast< AppMainWindow* >(appui->mainWindow());
	buildStatusBar(m_app);
}

DAAppStatusBar::~DAAppStatusBar()
{
}

void DAAppStatusBar::retranslateUi()
{
}

void DAAppStatusBar::showMessage(const QString& message, int timeout)
{
	m_statusBarWidget->showMessage(message, timeout);
}

void DAAppStatusBar::clearMessage()
{
	m_statusBarWidget->clearMessage();
}

void DAAppStatusBar::showProgressBar()
{
	m_statusBarWidget->showProgressBar();
}

void DAAppStatusBar::hideProgressBar()
{
	m_statusBarWidget->hideProgressBar();
}

void DAAppStatusBar::setProgress(int value)
{
	m_statusBarWidget->setProgress(value);
}

void DAAppStatusBar::setProgressText(const QString& text)
{
	m_statusBarWidget->setProgressText(text);
}

void DAAppStatusBar::clearProgressText()
{
	m_statusBarWidget->clearProgressText();
}

void DAAppStatusBar::setBusy(bool busy)
{
	m_statusBarWidget->setBusy(busy);
}

bool DAAppStatusBar::isBusy() const
{
	return m_statusBarWidget->isBusy();
}

void DAAppStatusBar::resetProgress()
{
	m_statusBarWidget->resetProgress();
}

bool DAAppStatusBar::isProgressBarVisible() const
{
	return m_statusBarWidget->isProgressBarVisible();
}

void DAAppStatusBar::setSwitchButtonVisible(DA::DAWorkbenchFeatureType type, bool visible)
{
	m_statusBarWidget->setSwitchButtonVisible(type, visible);
}

bool DAAppStatusBar::isSwitchButtonVisible(DA::DAWorkbenchFeatureType type) const
{
	return m_statusBarWidget->isSwitchButtonVisible(type);
}

AppMainWindow* DAAppStatusBar::app() const
{
	return m_app;
}

void DAAppStatusBar::setAppDockingArea(DAAppDockingArea* dockingArea)
{
	m_dockingArea = dockingArea;
}

void DAAppStatusBar::setAppActions(DAAppActions* actions)
{
	m_actions = actions;
	// 设置完action后，构建action相关的按钮
	m_showLeftSideBarButton = new QToolButton(m_statusBar);
	m_showLeftSideBarButton->setAutoRaise(true);
	m_showLeftSideBarButton->setDefaultAction(actions->actionShowLeftSideBar);
	m_statusBar->insertWidget(0, m_showLeftSideBarButton);
	m_showRightSideBarButton = new QToolButton(m_statusBar);
	m_showRightSideBarButton->setAutoRaise(true);
	m_showRightSideBarButton->setDefaultAction(actions->actionShowRightSideBar);
	m_statusBar->addPermanentWidget(m_showRightSideBarButton);
}

void DAAppStatusBar::buildStatusBar(AppMainWindow* mainWindow)
{
	m_statusBar       = new QStatusBar(mainWindow);
	m_statusBarWidget = new DAStatusBarWidget(m_statusBar);
	m_statusBar->addWidget(m_statusBarWidget, 1);
	mainWindow->setStatusBar(m_statusBar);
	connect(m_statusBarWidget, &DAStatusBarWidget::requestSwitch, this, &DAAppStatusBar::onRequestSwitch);
}

void DAAppStatusBar::onRequestSwitch(DA::DAWorkbenchFeatureType type)
{
	if (!m_dockingArea) {
		return;
	}
	m_dockingArea->raiseFeatureArea(type);
}
}
