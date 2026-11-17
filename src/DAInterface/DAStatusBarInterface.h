#ifndef DASTATUSBARINTERFACE_H
#define DASTATUSBARINTERFACE_H
#include "DAInterfaceAPI.h"
#include "DAUIExtendInterface.h"
class QAction;
namespace DA
{
class DAINTERFACE_API DAStatusBarInterface : public DAUIExtendInterface
{
    Q_OBJECT
public:
    explicit DAStatusBarInterface(DAUIInterface* u);
    virtual ~DAStatusBarInterface() override;
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
    // 设置是否显示switch button 组，switch button组用于快速切换绘图、数据、工作流三个部分，在一些场景需要隐藏
    virtual void setSwitchButtonVisible(DA::DAWorkbenchFeatureType type, bool visible) = 0;
    virtual bool isSwitchButtonVisible(DA::DAWorkbenchFeatureType type) const          = 0;
    //↓↓↓ 新增虚函数一律追加在末尾，避免改变既有虚表索引（老版本插件二进制兼容）↓↓↓
    // 进度条右侧的「停止」按钮：绑定外部 QAction（文本用作 tooltip，图标即按钮外观）；
    // 按钮随进度条一起显示/隐藏，只占固定小尺寸，不改变状态栏高度；传 nullptr 解绑并销毁按钮
    virtual void setProgressCancelAction(QAction* action) = 0;
    // 获取当前绑定的取消动作（未绑定时返回 nullptr）
    virtual QAction* progressCancelAction() const = 0;
    // 显示/隐藏进度条旁的取消按钮（未绑定动作时无效果）。只有调用方自己显示进度时才置 true，
    // 避免别的调用方 showProgressBar() 时误显示本插件绑定的停止按钮
    virtual void setProgressCancelVisible(bool visible) = 0;
};
}  // end DA
#endif  // DASTATUSBARINTERFACE_H
