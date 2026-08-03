#include "DAAppSettingDialog.h"
#include "SettingPages/DASettingPageGeneral.h"
#include "SettingPages/DASettingPagePython.h"
#include "SettingPages/DASettingPageLog.h"
#include "SettingPages/DASettingPageAdvanced.h"
#include "DAAgentSettingsWidget.h"
#include "DAConfigs.h"
#include <QIcon>
#include "AppMainWindow.h"
#include "SettingPages/DAAppConfig.h"
namespace DA
{
DAAppSettingDialog::DAAppSettingDialog(QWidget* parent) : DASettingDialog(parent)
{
}

void DAAppSettingDialog::buildUI(DAAppConfig* config)
{
    // 通用设置页
    DASettingPageGeneral* general = new DASettingPageGeneral();
    general->setAppConfig(config);
    settingWidget()->addPage(general);

#if DA_ENABLE_PYTHON
    // Python 环境设置页（仅启用 Python 时注册）
    DASettingPagePython* python = new DASettingPagePython();
    python->setAppConfig(config);
    settingWidget()->addPage(python);
#endif

    // 日志设置页
    DASettingPageLog* logPage = new DASettingPageLog();
    logPage->setAppConfig(config);
    settingWidget()->addPage(logPage);

    // 高级设置页
    DASettingPageAdvanced* advanced = new DASettingPageAdvanced();
    advanced->setAppConfig(config);
    settingWidget()->addPage(advanced);

    // Agent LLM 设置页
    // 本页不调用 setAppConfig(config)——配置经 QSettings 自取
    // （DAAgent 库无法链接 APP 的 DAAppConfig，见 plan-06 "持久化方案选择"）
    DAAgentSettingsWidget* agentPage = new DAAgentSettingsWidget();
    settingWidget()->addPage(agentPage);

    setPage(0);
}

DAAppSettingDialog::~DAAppSettingDialog()
{
}

}
