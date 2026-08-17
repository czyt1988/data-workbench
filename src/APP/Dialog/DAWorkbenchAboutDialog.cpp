#include "DAWorkbenchAboutDialog.h"
#include "ui_DAWorkbenchAboutDialog.h"
#include "SARibbonGlobal.h"
#include "spdlog/version.h"
#include "qwt_global.h"
#include "DAGlobals.h"
// 通过此宏引入pybind11，避免qt中slot关键字冲突
#include "DAPybind11InQt.h"
namespace DA
{
DAWorkbenchAboutDialog::DAWorkbenchAboutDialog(QWidget* parent) : QDialog(parent), ui(new Ui::DAWorkbenchAboutDialog)
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
	ui->textEdit->append(
		tr("DAWorkbench, LGPL, Version: %1.%2.%3").arg(DA_VERSION_MAJOR).arg(DA_VERSION_MINOR).arg(DA_VERSION_PATCH));  // cn:DAWorkbench,LGPL,版本:%1.%2.%3
	ui->textEdit->append(tr("Email: czy.t@163.com"));  // cn:邮箱:czy.t@163.com
	ui->textEdit->append(tr("GitHub: https://github.com/czyt1988/data-workbench"));  // cn:GitHub:https://github.com/czyt1988/data-workbench
	ui->textEdit->append(tr(""));  // cn:空行
	ui->textEdit->append(tr("Third party list:"));  // cn:第三方库列表:
	ui->textEdit->append(tr("SARibbon, MIT, Version: %1.%2.%3")  // cn:SARibbon,MIT,版本:%1.%2.%3
							 .arg(SA_RIBBON_BAR_VERSION_MAJ)
							 .arg(SA_RIBBON_BAR_VERSION_MIN)
							 .arg(SA_RIBBON_BAR_VERSION_PAT));
	ui->textEdit->append(
		tr("spdlog, MIT, Version: %1.%2.%3").arg(SPDLOG_VER_MAJOR).arg(SPDLOG_VER_MINOR).arg(SPDLOG_VER_PATCH));  // cn:spdlog,MIT,版本:%1.%2.%3
	ui->textEdit->append(tr("Qt-Advanced-Docking-System, LGPL v2.1, Version:"));  // cn:Qt-Advanced-Docking-System,LGPL v2.1,版本:
	ui->textEdit->append(tr("qwt, LGPL, Version: %1").arg(QWT_VERSION_STR));  // cn:qwt,LGPL,版本:%1
	ui->textEdit->append(tr(""));  // cn:空行
	ui->textEdit->append(tr("This is a Python dependent version"));  // cn:这是依赖python的版本
	ui->textEdit->append(tr("pybind11, BSD, Version: %1.%2").arg(PYBIND11_VERSION_MAJOR).arg(PYBIND11_VERSION_MINOR));  // cn:pybind11,BSD,版本:%1.%2
}
}  // end DA
