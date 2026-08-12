#ifndef DAAPPSTATUSBAR_H
#define DAAPPSTATUSBAR_H
#include "DAStatusBarInterface.h"
class QStatusBar;
class QToolButton;
#include "DAStatusBarWidget.h"
namespace DA
{
class AppMainWindow;
class DAStatusBar;
class DAAppDockingArea;
class DAAppActions;

class DAAppStatusBar : public DAStatusBarInterface
{
    Q_OBJECT
public:
	explicit DAAppStatusBar(DAUIInterface* u);
	virtual ~DAAppStatusBar() override;
	// 发生语言变更时会触发此函数
	virtual void retranslateUi() override;
	//
	virtual void showMessage(const QString& message, int timeout = 15000) override;
	virtual void clearMessage() override;
	virtual void showProgressBar() override;
	virtual void hideProgressBar() override;
	virtual void setProgress(int value) override;  // 0-100
	virtual void setProgressText(const QString& text) override;
	virtual void clearProgressText() override;
	virtual void setBusy(bool busy) override;  // 繁忙状态
	virtual bool isBusy() const override;
	virtual void resetProgress() override;  // 重置进度条
	// 获取当前状态
	virtual bool isProgressBarVisible() const override;
	// 设置是否显示switch button 组，switch button组用于快速切换绘图、数据、工作流三个部分，在一些场景需要隐藏
	virtual void setSwitchButtonVisible(DA::DAWorkbenchFeatureType type, bool visible) override;
	virtual bool isSwitchButtonVisible(DA::DAWorkbenchFeatureType type) const override;
	// 获取app
	AppMainWindow* app() const;
	//
	void setAppDockingArea(DAAppDockingArea* dockingArea);
	void setAppActions(DAAppActions* actions);
private Q_SLOTS:
	void onRequestSwitch(DA::DAWorkbenchFeatureType type);

private:
	void buildStatusBar(AppMainWindow* mainWindow);

private:
	AppMainWindow* mApp { nullptr };
	QStatusBar* mStatusBar { nullptr };
	DAStatusBarWidget* mStatusBarWidget { nullptr };
	DAAppDockingArea* mDockingArea { nullptr };
	DAAppActions* mActions { nullptr };
	QToolButton* mShowLeftSideBarButton { nullptr };
	QToolButton* mShowRightSideBarButton { nullptr };
};
}  // end DA
#endif  // DAAPPSTATUSBAR_H
