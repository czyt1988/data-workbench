#include "DADialogCreatePivotTable.h"
#include "ui_DADialogCreatePivotTable.h"
#include <QStandardItemModel>

#include "APP/DAAppController.h"
// Widget
#include "DADataOperateOfDataFrameWidget.h"

namespace DA
{
DADialogCreatePivotTable::DADialogCreatePivotTable(QWidget* parent)
	: QDialog(parent), ui(new Ui::DADialogCreatePivotTable)
{
	ui->setupUi(this);
	this->initCreatePivotTable();
}

DADialogCreatePivotTable::~DADialogCreatePivotTable()
{
    delete ui;
}

/**
 * @brief 初始化数据透视表界面
 */
void DADialogCreatePivotTable::initCreatePivotTable()
{
	// 准备数据模型
	QStandardItemModel* model = new QStandardItemModel();

	// 设置表头内容
	QStringList headers;
	headers << tr("Value") << tr("Index") << tr("Columns");

	// 添加表头
	model->setHorizontalHeaderLabels(headers);

	// 利用 setModel() 方法将数据模型与 QTableView 绑定
	ui->tableViewParameter->setModel(model);
}

QStringList DADialogCreatePivotTable::getPivotTableValue() const
{
	return QStringList();
}

QStringList DADialogCreatePivotTable::getPivotTableIndex() const
{
	return QStringList();
}

QStringList DADialogCreatePivotTable::getPivotTableColumn() const
{
	return QStringList();
}

QString DADialogCreatePivotTable::getPivotTableAggfunc() const
{
	return ui->comboBoxAggfunc->currentText();
}

bool DADialogCreatePivotTable::isEnableMarginsName() const
{
	return ui->checkBoxMargins->isChecked();
}

void DADialogCreatePivotTable::setEnableMargins(bool on)
{
	ui->checkBoxMargins->setChecked(on);
}

QString DADialogCreatePivotTable::getMarginsName() const
{
	return ui->lineEditMarginsName->text();
}

void DADialogCreatePivotTable::setMarginsName(QString& s)
{
	ui->lineEditMarginsName->setText(s);
}

bool DADialogCreatePivotTable::isEnableSort() const
{
	return ui->checkBoxSort->isChecked();
}

void DADialogCreatePivotTable::setEnableSort(bool on)
{
	ui->checkBoxSort->setChecked(on);
}
}
