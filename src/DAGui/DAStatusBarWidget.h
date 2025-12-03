#ifndef DASTATUSBARWIDGET_H
#define DASTATUSBARWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QHBoxLayout>
#include <QPropertyAnimation>
namespace DA
{

/**
 * @brief 状态栏窗口
 */
class DAGUI_API DAStatusBarWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAStatusBarWidget)
public:
    explicit DAStatusBarWidget(QWidget* parent = nullptr);

    // 消息窗口相关方法
    void showMessage(const QString& message, int timeout = 5000);
    void clearMessage();
    void setMessageTimeout(int milliseconds);

    // 进度窗口相关方法
    void showProgressBar();
    void hideProgressBar();
    void setProgress(int value);  // 0-100
    void setBusy(bool busy);      // 繁忙状态
    void resetProgress();         // 重置进度条

    // 获取当前状态
    bool isProgressBarVisible() const;
    bool isBusy() const;
    QString getCurrentMessage() const;

protected:
    void changeEvent(QEvent* event) override;

private Q_SLOTS:
    void onMessageTimeout();
    void fadeOutMessage();

private:
    void startMessageTimer(int timeout);
    void updateWidgetStyle();
};
}  // end DA
#endif  // DASTATUSBARWIDGET_H
