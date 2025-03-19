#include "DADialogCreatePivotTable.h"
#include "ui_DADialogCreatePivotTable.h"
#include <QStandardItemModel>

namespace DA
{

class DADialogCreatePivotTable::PrivateData
{
	DA_DECLARE_PUBLIC(DADialogCreatePivotTable)
public:
	PrivateData(DADialogCreatePivotTable* p);

public:
	DAPyDataFrame mDataframe;
};

DADialogCreatePivotTable::PrivateData::PrivateData(DADialogCreatePivotTable* p) : q_ptr(p)
{
}

//===============================================================
// DADialogCreatePivotTable
//===============================================================

DADialogCreatePivotTable::DADialogCreatePivotTable(QWidget* parent)
	: QDialog(parent), ui(new Ui::DADialogCreatePivotTable), DA_PIMPL_CONSTRUCT
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

	DAPyDataFrame df = getCurrentDataFrame();
	QStringList para = df.columns();

	// QStringList para;
	para.append("age");
	para.append("workclass");
	para.append("fnlwgt");
	para.append("education");
	para.append("education-num");
	para.append("marital-status");
	para.append("occupation");
	para.append("relationship");
	para.append("race");
	para.append("sex");
	para.append("capital-gain");
	para.append("capital-loss");
	para.append("hours-per-week");
	para.append("native-country");
	para.append("salary");

	// 设置tableview表头内容
	QStringList headers;
	headers << tr("Value") << tr("Index") << tr("Columns");

	// 添加数据
	for (int row = 0; row < para.size(); ++row) {
		QStandardItem* vitem = new QStandardItem(para[ row ]);
		QStandardItem* iitem = new QStandardItem(para[ row ]);
		QStandardItem* citem = new QStandardItem(para[ row ]);

		vitem->setCheckable(true);
		vitem->setCheckState(Qt::Unchecked);

		iitem->setCheckable(true);
		iitem->setCheckState(Qt::Unchecked);

		vitem->setCheckable(true);
		vitem->setCheckState(Qt::Unchecked);

		citem->setCheckable(true);
		citem->setCheckState(Qt::Unchecked);

		model->setItem(row, 0, vitem);  // "Value" 列
		model->setItem(row, 1, iitem);  // "Index" 列
		model->setItem(row, 2, citem);  // "Columns" 列
	}

	// 添加tableview表头
	model->setHorizontalHeaderLabels(headers);

	// 利用 setModel() 方法将数据模型与 QTableView 绑定
	ui->tableViewParameter->setModel(model);
	ui->tableViewParameter->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	// 监听复选框状态变化，实现**一行互斥**
	connect(model, &QStandardItemModel::itemChanged, [ model ](QStandardItem* item) {
		int row = item->row();
		int col = item->column();

		if (item->checkState() == Qt::Checked) {
			// 如果当前列被选中，禁用本行的其他列
			for (int c = 0; c < 3; ++c) {
				if (c == col)
					continue;
				QStandardItem* otherItem = model->item(row, c);
				otherItem->setCheckState(Qt::Unchecked);
				otherItem->setFlags(otherItem->flags() & ~Qt::ItemIsEnabled);  // 禁用
			}
		} else {
			// 如果当前列取消勾选，则恢复本行所有列的可选状态
			for (int c = 0; c < 3; ++c) {
				model->item(row, c)->setFlags(model->item(row, c)->flags() | Qt::ItemIsEnabled);
			}
		}
	});
}

DAPyDataFrame DADialogCreatePivotTable::getCurrentDataFrame() const
{
	return d_ptr->mDataframe;
}

QStringList DADialogCreatePivotTable::getPivotTableValue() const
{
	QStandardItemModel* model = qobject_cast< QStandardItemModel* >(ui->tableViewParameter->model());

	if (!model)
		return QStringList();

	int rows = model->rowCount();
	// 存储选中的文本值
	QStringList values;

	for (int i = 0; i < rows; ++i) {
		QStandardItem* item = model->item(i, 0);
		if (item && item->checkState() == Qt::Checked) {
			values << item->text();
		}
	}
	return values;
}

QStringList DADialogCreatePivotTable::getPivotTableIndex() const
{
	QStandardItemModel* model = qobject_cast< QStandardItemModel* >(ui->tableViewParameter->model());

	if (!model)
		return QStringList();

	int rows = model->rowCount();
	// 存储选中的文本值
	QStringList indexs;

	for (int i = 0; i < rows; ++i) {
		QStandardItem* item = model->item(i, 1);
		if (item && item->checkState() == Qt::Checked) {
			indexs << item->text();
		}
	}
	return indexs;
}

QStringList DADialogCreatePivotTable::getPivotTableColumn() const
{
	QStandardItemModel* model = qobject_cast< QStandardItemModel* >(ui->tableViewParameter->model());

	if (!model)
		return QStringList();

	int rows = model->rowCount();
	// 存储选中的文本值
	QStringList columns;

	for (int i = 0; i < rows; ++i) {
		QStandardItem* item = model->item(i, 2);
		if (item && item->checkState() == Qt::Checked) {
			columns << item->text();
		}
	}
	return columns;
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
