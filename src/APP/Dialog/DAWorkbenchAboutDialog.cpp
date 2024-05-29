#include "DAWorkbenchAboutDialog.h"
#include "ui_DAWorkbenchAboutDialog.h"
#include "SARibbonGlobal.h"
#include "spdlog/version.h"
#include "qwt_global.h"
#include "DAGlobals.h"
#if DA_ENABLE_PYTHON
#include "pybind11/detail/common.h"
#endif
namespace DA{
DAWorkbenchAboutDialog::DAWorkbenchAboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DAWorkbenchAboutDialog)
{
    ui->setupUi(this);
    makeAboutInfo();
}

DAWorkbenchAboutDialog::~DAWorkbenchAboutDialog()
{
    delete ui;
}

void DAWorkbenchAboutDialog::makeAboutInfo()
{
    ui->textEdit->append(tr("DAWorkBench,LGPL, Version:%1.%2.%3").arg(DA_VERSION_MAJ).arg(DA_VERSION_MIN).arg(DA_VERSION_PAR));
    ui->textEdit->append(tr("email:czy.t@163.com"));
    ui->textEdit->append(tr("github:https://github.com/czyt1988/data-workbench"));
    ui->textEdit->append(tr(""));
    ui->textEdit->append(tr("third party list:"));
    ui->textEdit->append(tr("SARibbon, MIT, Version:%1.%2.%3").arg(SA_RIBBON_BAR_VERSION_MAJ).arg(SA_RIBBON_BAR_VERSION_MIN).arg(SA_RIBBON_BAR_VERSION_PAT));
    ui->textEdit->append(tr("spdlog, MIT, Version:%1.%2.%3").arg(SPDLOG_VER_MAJOR).arg(SPDLOG_VER_MINOR).arg(SPDLOG_VER_PATCH));
    ui->textEdit->append(tr("Qt-Advanced-Docking-System, LGPL v2.1, Version:"));
    ui->textEdit->append(tr("qwt, LGPL, Version:%1").arg(QWT_VERSION_STR));
#if DA_ENABLE_PYTHON
    ui->textEdit->append(tr(""));
    ui->textEdit->append(tr("This is a Python dependent version"));//cn:这是依赖python的版本
    ui->textEdit->append(tr("pybind11,BSD,Version:%1.%2.%3").arg(PYBIND11_VERSION_MAJOR).arg(PYBIND11_VERSION_MINOR).arg(PYBIND11_VERSION_PATCH));//cn:这是依赖python的版本
#endif
}
}//end DA
