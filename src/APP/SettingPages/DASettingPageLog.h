#ifndef DASETTINGPAGELOG_H
#define DASETTINGPAGELOG_H
#include "DAAbstractSettingPage.h"
#include <QWidget>
#include <QComboBox>
#include "DAGuiAPI.h"

namespace Ui
{
class DASettingPageLog;
}

namespace DA
{
class DAAppConfig;

/**
 * @brief 日志设置页：日志级别、UI队列级别、stdout、轮转参数、显示条数
 */
class DASettingPageLog : public DAAbstractSettingPage
{
    Q_OBJECT

public:
    explicit DASettingPageLog(QWidget* parent = nullptr);
    ~DASettingPageLog();
    virtual void apply() override;
    virtual QString getSettingPageTitle() const override;
    virtual QIcon getSettingPageIcon() const override;
    bool setAppConfig(DAAppConfig* p);
private slots:
    void onComboBoxLogLevelCurrentIndexChanged(int index);
    void onComboBoxQueueLevelCurrentIndexChanged(int index);
    void onCheckBoxOutputStdoutStateChanged(int state);
    void onComboBoxRotationModeCurrentIndexChanged(int index);
    void onSpinBoxMaxSizeValueChanged(int v);
    void onSpinBoxMaxFilesValueChanged(int v);
    void onSpinBoxDisplayLogsNumValueChanged(int v);

private:
    void fillLogLevelCombo(QComboBox* combo);

private:
    Ui::DASettingPageLog* ui;
    DAAppConfig* mAppConfig { nullptr };
};
}
#endif  // DASETTINGPAGELOG_H
