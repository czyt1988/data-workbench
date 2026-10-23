#include "DAAppSettingDialog.h"
#include "SettingPages/DASettingPageGeneral.h"
#include "SettingPages/DASettingPagePython.h"
#include "SettingPages/DASettingPageLog.h"
#include "SettingPages/DASettingPageAdvanced.h"
#include "SettingPages/DAAgentSettingsWidget.h"
#include "SettingPages/DAAgentPermissionSettingsWidget.h"
#include "SettingPages/DASettingPageShortcuts.h"
#include "DAConfigs.h"
#include <QIcon>
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "SettingPages/DAAppConfig.h"
namespace DA
{
DAAppSettingDialog::DAAppSettingDialog(QWidget* parent) : DASettingDialog(parent)
{
}

void DAAppSettingDialog::buildUI(DAAppConfig* config, DAAppCore* core)
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

    // 快捷键设置页（数据源为 DAShortcutManager，独立于 DAAppConfig 持久化到 shortcuts.json）
    DASettingPageShortcuts* shortcutsPage = new DASettingPageShortcuts();
    shortcutsPage->setShortcutManager(core->getShortcutManager());
    settingWidget()->addPage(shortcutsPage);

    // Agent LLM 设置页（本页注入 setAgentInterface 而非 setAppConfig）
    DAAgentSettingsWidget* agentPage = new DAAgentSettingsWidget();
    agentPage->setAgentInterface(core->getAgentInterface());
    settingWidget()->addPage(agentPage);

    // Agent 权限设置页（permission-layer P1 骨架：模式/审批超时/判官）
    DAAgentPermissionSettingsWidget* permissionPage = new DAAgentPermissionSettingsWidget();
    permissionPage->setAgentInterface(core->getAgentInterface());
    settingWidget()->addPage(permissionPage);

    setPage(0);
}

DAAppSettingDialog::~DAAppSettingDialog()
{
}

}
