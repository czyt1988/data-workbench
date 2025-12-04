#ifndef DASTATUSBARINTERFACE_H
#define DASTATUSBARINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAUIExtendInterface.h"
namespace DA
{
class DAINTERFACE_API DAStatusBarInterface : public DAUIExtendInterface
{
    Q_OBJECT
public:
    explicit DAStatusBarInterface(DAUIInterface* u);
    ~DAStatusBarInterface();
    // 添加状态栏信息，状态栏信息将在状态栏显示，并间隔n秒(默认n=15)后隐藏
    virtual void showMessage(const QString& message, int timeout = 15000) = 0;
    virtual void clearMessage()                                           = 0;
    virtual void showProgressBar()                                        = 0;
    virtual void hideProgressBar()                                        = 0;
    virtual void setProgress(int value)                                   = 0;  // 0-100
    virtual void setProgressText(const QString& text)                     = 0;
    virtual void clearProgressText()                                      = 0;
    virtual void setBusy(bool busy)                                       = 0;  // 繁忙状态
    virtual bool isBusy() const                                           = 0;
    virtual void resetProgress()                                          = 0;  // 重置进度条
    // 获取当前状态
    virtual bool isProgressBarVisible() const = 0;
};
}  // end DA
#endif  // DASTATUSBARINTERFACE_H
