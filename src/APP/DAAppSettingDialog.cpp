#include "DAAppSettingDialog.h"
#include "SettingPages/DASettingPageCommon.h"
#include <QIcon>
#include "DAAppCore.h"
#include "DAAppUI.h"
#include "AppMainWindow.h"
#include "SettingPages/DAAppConfig.h"
namespace DA
{
DAAppSettingDialog::DAAppSettingDialog(QWidget* parent) : DASettingDialog(parent)
{
}

void DAAppSettingDialog::buildUI(DAAppConfig* config)
{
    DASettingPageCommon* page = new DASettingPageCommon();
    page->setAppConfig(config);
    settingWidget()->addPage(page);
    setPage(0);
}

DAAppSettingDialog::~DAAppSettingDialog()
{
}

}
