#include "DASettingDialog.h"
#include "ui_DASettingDialog.h"
#include "DAWaitCursorScoped.h"
namespace DA
{

DASettingDialog::DASettingDialog(QWidget* parent) : QDialog(parent), ui(new Ui::DASettingDialog)
{
    ui->setupUi(this);
    connect(ui->pushButtonOK, &QPushButton::clicked, this, &DASettingDialog::onPushButtonOKClicked);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &DASettingDialog::reject);
    connect(ui->pushButtonApply, &QPushButton::clicked, this, &DASettingDialog::onPushButtonApplyClicked);
}

DASettingDialog::~DASettingDialog()
{
    delete ui;
}

/**
 * @brief 获取设置页
 * @return
 */
DASettingWidget* DASettingDialog::settingWidget() const
{
    return ui->settingWidget;
}

void DASettingDialog::onPushButtonOKClicked()
{
    onPushButtonApplyClicked();
    accept();
    emit needSave();
}

void DASettingDialog::onPushButtonApplyClicked()
{
    DA_WAIT_CURSOR_SCOPED();
    ui->settingWidget->applyAll();
    emit needSave();
}
}  // DA
