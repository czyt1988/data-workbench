#include "DADialogAppSetting.h"
#include "ui_DADialogAppSetting.h"
#include "DASettingPageCommon.h"
namespace DA
{
DADialogAppSetting::DADialogAppSetting(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogAppSetting)
{
    ui->setupUi(this);
    ui->widgetSetting->addPage(QIcon(), tr("Common"), new DASettingPageCommon());
    connect(ui->pushButtonApply, &QPushButton::clicked, this, &DADialogAppSetting::onSettingApply);
    connect(ui->pushButtonOK, &QPushButton::clicked, this, &DADialogAppSetting::onAccepted);
}

DADialogAppSetting::~DADialogAppSetting()
{
    delete ui;
}

void DADialogAppSetting::onSettingApply()
{
    ui->widgetSetting->applyAll();
}

void DADialogAppSetting::onAccepted()
{
    ui->widgetSetting->applyAll();
    QDialog::accept();
}
}
