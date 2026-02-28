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
	~DAStatusBarWidget();
	// 消息窗口相关方法
	void showMessage(const QString& message, int timeout = 15000);
	void clearMessage();
	// 进度窗口相关方法
	void showProgressBar();
	void hideProgressBar();
	// 设置进度0-100
	void setProgress(int value);
	// 繁忙状态
	void setBusy(bool busy);
	bool isBusy() const;
	// 重置进度条
	void resetProgress();
	// 进度文本相关方法
	void setProgressText(const QString& text);
	void clearProgressText();
	QString progressText() const;
	// 获取当前状态
	bool isProgressBarVisible() const;
	QString getCurrentMessage() const;
	// switch button的显示隐藏
	void setSwitchButtonVisible(DA::DAWorkbenchFeatureType type, bool visible);
	bool isSwitchButtonVisible(DA::DAWorkbenchFeatureType type) const;
Q_SIGNALS:
	void requestSwitch(DA::DAWorkbenchFeatureType type);

protected:
	void changeEvent(QEvent* event) override;

private Q_SLOTS:
	void onMessageTimeout();
	void fadeOutMessage();

private:
	void startMessageTimer(int timeout);
	void updateWidgetStyle();
	void initConnect();
};
}  // end DA
#endif  // DASTATUSBARWIDGET_H
