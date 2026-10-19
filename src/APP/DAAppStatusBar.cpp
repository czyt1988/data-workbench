#include "DAAppStatusBar.h"
#include "DAStatusBarWidget.h"
#include "DAAppUI.h"
#include "AppMainWindow.h"
#include "DAAppDockingArea.h"
#include "DAAppActions.h"
#include "DATableStatsWidget.h"
#include "DADataOperateWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include <QStatusBar>
#include <QToolButton>
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
	// 状态栏选区统计部件（007）：建部件 + 挂载 + 连当前数据页切换（只做接线，
	// 不在构造期硬取当前页——setAppDockingArea 可能早于任何数据页创建）
	if (!mTableStatsWidget && mStatusBarWidget) {
		mTableStatsWidget = new DATableStatsWidget(mStatusBarWidget);
		mStatusBarWidget->setExtraPermanentWidget(mTableStatsWidget);
	}
	if (mDockingArea) {
		DADataOperateWidget* opw = mDockingArea->getDataOperateWidget();
		if (opw) {
			// 当前数据页切换：跟随绑定（失败 = nullptr = 解绑隐藏）
			connect(opw, &DADataOperateWidget::currentDataTableWidgetChanged, this,
			        [ this ](DADataOperatePageWidget* page, int) {
				        mTableStatsWidget->setPage(qobject_cast< DADataOperateOfDataFrameWidget* >(page));
			        });
			// 页面删除前先解绑：dataTableRemoving 在删页前发；命中当前绑定页就解绑
			//（不靠部件里的 QPointer 碰运气——销毁路径不保证先置空再触发去抖到期）
			connect(opw, &DADataOperateWidget::dataTableRemoving, this, [ this ](DADataOperatePageWidget* page) {
				if (mTableStatsWidget && mTableStatsWidget->page() == page) {
					mTableStatsWidget->setPage(nullptr);
				}
			});
			// 首次绑定兜底（setAppDockingArea 可能晚于某个数据页的创建；无页时 nullptr → 解绑隐藏）
			mTableStatsWidget->setPage(opw->getCurrentDataFrameWidget());
		}
	}
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
