#include "DAAppSettingDialog.h"
#include "DASettingPageCommon.h"
#include <QIcon>
#include "DAAppCore.h"
#include "DAAppUI.h"
#include "AppMainWindow.h"
#include "DAAppConfig.h"
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
}

DAAppSettingDialog::~DAAppSettingDialog()
{
}

}
