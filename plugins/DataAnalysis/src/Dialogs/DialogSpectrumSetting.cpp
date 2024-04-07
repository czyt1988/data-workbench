#include "DialogSpectrumSetting.h"
#include "ui_DialogSpectrumSetting.h"

DialogSpectrumSetting::DialogSpectrumSetting(QWidget* parent) : QDialog(parent), ui(new Ui::DialogSpectrumSetting)
{
    ui->setupUi(this);
}

DialogSpectrumSetting::~DialogSpectrumSetting()
{
    delete ui;
}
