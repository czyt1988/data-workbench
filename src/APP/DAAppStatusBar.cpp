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
	mApp          = qobject_cast< AppMainWindow* >(appui->mainWindow());
	buildStatusBar(mApp);
}

DAAppStatusBar::~DAAppStatusBar()
{
}

void DAAppStatusBar::retranslateUi()
{
}

void DAAppStatusBar::showMessage(const QString& message, int timeout)
{
	mStatusBarWidget->showMessage(message, timeout);
}

void DAAppStatusBar::clearMessage()
{
	mStatusBarWidget->clearMessage();
}

void DAAppStatusBar::showProgressBar()
{
	mStatusBarWidget->showProgressBar();
}

void DAAppStatusBar::hideProgressBar()
{
	mStatusBarWidget->hideProgressBar();
}

void DAAppStatusBar::setProgress(int value)
{
	mStatusBarWidget->setProgress(value);
}

void DAAppStatusBar::setProgressText(const QString& text)
{
	mStatusBarWidget->setProgressText(text);
}

void DAAppStatusBar::clearProgressText()
{
	mStatusBarWidget->clearProgressText();
}

void DAAppStatusBar::setBusy(bool busy)
{
	mStatusBarWidget->setBusy(busy);
}

bool DAAppStatusBar::isBusy() const
{
	return mStatusBarWidget->isBusy();
}

void DAAppStatusBar::resetProgress()
{
	mStatusBarWidget->resetProgress();
}

bool DAAppStatusBar::isProgressBarVisible() const
{
	return mStatusBarWidget->isProgressBarVisible();
}

void DAAppStatusBar::setSwitchButtonVisible(DA::DAWorkbenchFeatureType type, bool visible)
{
	mStatusBarWidget->setSwitchButtonVisible(type, visible);
}

bool DAAppStatusBar::isSwitchButtonVisible(DA::DAWorkbenchFeatureType type) const
{
	return mStatusBarWidget->isSwitchButtonVisible(type);
}

AppMainWindow* DAAppStatusBar::app() const
{
	return mApp;
}

void DAAppStatusBar::setAppDockingArea(DAAppDockingArea* dockingArea)
{
	mDockingArea = dockingArea;
}

void DAAppStatusBar::setAppActions(DAAppActions* actions)
{
	mActions = actions;
	// 设置完action后，构建action相关的按钮
	mShowLeftSideBarButton = new QToolButton(mStatusBar);
	mShowLeftSideBarButton->setAutoRaise(true);
	mShowLeftSideBarButton->setDefaultAction(actions->actionShowLeftSideBar);
	mStatusBar->insertWidget(0, mShowLeftSideBarButton);
	mShowRightSideBarButton = new QToolButton(mStatusBar);
	mShowRightSideBarButton->setAutoRaise(true);
	mShowRightSideBarButton->setDefaultAction(actions->actionShowRightSideBar);
	mStatusBar->addPermanentWidget(mShowRightSideBarButton);
}

void DAAppStatusBar::buildStatusBar(AppMainWindow* mainWindow)
{
	mStatusBar       = new QStatusBar(mainWindow);
	mStatusBarWidget = new DAStatusBarWidget(mStatusBar);
	mStatusBar->addWidget(mStatusBarWidget, 1);
	mainWindow->setStatusBar(mStatusBar);
	connect(mStatusBarWidget, &DAStatusBarWidget::requestSwitch, this, &DAAppStatusBar::onRequestSwitch);
}

void DAAppStatusBar::onRequestSwitch(DA::DAWorkbenchFeatureType type)
{
	if (!mDockingArea) {
		return;
	}
	mDockingArea->raiseFeatureArea(type);
}
}
