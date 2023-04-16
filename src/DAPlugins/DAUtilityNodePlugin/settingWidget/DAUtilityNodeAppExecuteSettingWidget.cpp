#include "DAUtilityNodeAppExecuteSettingWidget.h"
#include "ui_DAUtilityNodeAppExecuteSettingWidget.h"

DAUtilityNodeAppExecuteSettingWidget::DAUtilityNodeAppExecuteSettingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DAUtilityNodeAppExecuteSettingWidget)
{
    ui->setupUi(this);
}

DAUtilityNodeAppExecuteSettingWidget::~DAUtilityNodeAppExecuteSettingWidget()
{
    delete ui;
}
