#ifndef DAAPPSTATUSBAR_H
#define DAAPPSTATUSBAR_H
#include "DAStatusBarInterface.h"
namespace DA
{
class AppMainWindow;
class DAStatusBar;
class DAStatusBarWidget;
class DAAppStatusBar : public DAStatusBarInterface
{
public:
    explicit DAAppStatusBar(DAUIInterface* u);
    ~DAAppStatusBar();
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
    virtual void resetProgress() override;     // 重置进度条
    // 获取当前状态
    virtual bool isProgressBarVisible() const override;
    // 获取app
    AppMainWindow* app() const;
    // 重置文字
    void resetText();

private:
    void buildStatusBar(AppMainWindow* mainWindow);

private:
    AppMainWindow* m_app { nullptr };
    DAStatusBarWidget* m_statusBarWidget { nullptr };
};
}  // end DA
#endif  // DAAPPSTATUSBAR_H
