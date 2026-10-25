#ifndef DASETTINGPAGEADVANCED_H
#define DASETTINGPAGEADVANCED_H
#include "DAAbstractSettingPage.h"
#include <QWidget>
#include "DAGuiAPI.h"

namespace Ui
{
class DASettingPageAdvanced;
}

namespace DA
{
class DAAppConfig;

/**
 * @brief 高级设置页：最近文件数、dump保留天数、启动画面、自动保存、脚本工作区
 */
class DASettingPageAdvanced : public DAAbstractSettingPage
{
    Q_OBJECT

public:
    explicit DASettingPageAdvanced(QWidget* parent = nullptr);
    virtual ~DASettingPageAdvanced() override;
    virtual void apply() override;
    virtual QString getSettingPageTitle() const override;
    virtual QIcon getSettingPageIcon() const override;
    bool setAppConfig(DAAppConfig* p);
private Q_SLOTS:
    void onSpinBoxRecentFilesMaxValueChanged(int v);
    void onSpinBoxDumpRetentionDaysValueChanged(int v);
    void onCheckBoxShowSplashStateChanged(int state);
    void onSpinBoxAutosaveIntervalValueChanged(int v);
    void onWorkspaceDirChanged(const QString& path);
    void onSpinBoxScriptTimeoutValueChanged(int v);
    void onSpinBoxScriptResultMaxCharsValueChanged(int v);

private:
    Ui::DASettingPageAdvanced* ui;
    DAAppConfig* mAppConfig { nullptr };
};
}
#endif  // DASETTINGPAGEADVANCED_H
