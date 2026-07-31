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
 * @brief 高级设置页：工作流超时、最近文件数、dump保留、节点/插件路径、启动画面、自动保存
 */
class DASettingPageAdvanced : public DAAbstractSettingPage
{
    Q_OBJECT

public:
    explicit DASettingPageAdvanced(QWidget* parent = nullptr);
    ~DASettingPageAdvanced();
    virtual void apply() override;
    virtual QString getSettingPageTitle() const override;
    virtual QIcon getSettingPageIcon() const override;
    bool setAppConfig(DAAppConfig* p);
private slots:
    void onDoubleSpinBoxWorkflowTimeoutValueChanged(double v);
    void onSpinBoxRecentFilesMaxValueChanged(int v);
    void onSpinBoxDumpRetentionDaysValueChanged(int v);
    void onLineEditPluginPathTextChanged(const QString& text);
    void onToolButtonPluginBrowseClicked();
    void onToolButtonAddNodePathClicked();
    void onToolButtonRemoveNodePathClicked();
    void onCheckBoxShowSplashStateChanged(int state);
    void onSpinBoxAutosaveIntervalValueChanged(int v);

private:
    Ui::DASettingPageAdvanced* ui;
    DAAppConfig* mAppConfig { nullptr };
};
}
#endif  // DASETTINGPAGEADVANCED_H
