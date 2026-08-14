#include "DAAppSettingDialog.h"
#include "SettingPages/DASettingPageGeneral.h"
#include "SettingPages/DASettingPagePython.h"
#include "SettingPages/DASettingPageLog.h"
#include "SettingPages/DASettingPageAdvanced.h"
#include "SettingPages/DAAgentSettingsWidget.h"
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

    // Python 环境设置页（仅启用 Python 时注册）
    DASettingPagePython* python = new DASettingPagePython();
    python->setAppConfig(config);
    settingWidget()->addPage(python);

    // 日志设置页
    DASettingPageLog* logPage = new DASettingPageLog();
    logPage->setAppConfig(config);
    settingWidget()->addPage(logPage);

    // 高级设置页
    DASettingPageAdvanced* advanced = new DASettingPageAdvanced();
    advanced->setAppConfig(config);
    settingWidget()->addPage(advanced);

    // Agent LLM 设置页（与其它设置页统一经 config-> 取接口；本页注入 setAgentInterface 而非 setAppConfig，
    // 取接口路径 config->getCore()->getAgentInterface() 与其它页 page->setAppConfig(config) 同样经 config-> 取数据）
    DAAgentSettingsWidget* agentPage = new DAAgentSettingsWidget();
    agentPage->setAgentInterface(config->getCore()->getAgentInterface());
    settingWidget()->addPage(agentPage);

    setPage(0);
}

DAAppSettingDialog::~DAAppSettingDialog()
{
}

}
