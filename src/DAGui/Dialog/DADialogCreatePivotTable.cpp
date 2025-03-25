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
	QStandardItemModel* mModel { nullptr };
};

DADialogCreatePivotTable::PrivateData::PrivateData(DADialogCreatePivotTable* p) : q_ptr(p)
{
}

//===============================================================
// DADialogCreatePivotTable
//===============================================================

DADialogCreatePivotTable::DADialogCreatePivotTable(QWidget* parent)
	: QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADialogCreatePivotTable)
{
	ui->setupUi(this);
}

DADialogCreatePivotTable::~DADialogCreatePivotTable()
{
	delete ui;
}

DAPyDataFrame DADialogCreatePivotTable::getDataFrame() const
{
	return d_ptr->mDataframe;
}

void DADialogCreatePivotTable::setDataframe(const DAPyDataFrame& df)
{
	d_ptr->mDataframe = df;

	// 准备数据模型
	if (!d_ptr->mModel) {
		d_ptr->mModel = new QStandardItemModel(this);
	}
	QStandardItemModel* m = d_ptr->mModel;
	QStringList para      = df.columns();
	m->clear();

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

		citem->setCheckable(true);
		citem->setCheckState(Qt::Unchecked);

		m->setItem(row, 0, vitem);  // "Value" 列
		m->setItem(row, 1, iitem);  // "Index" 列
		m->setItem(row, 2, citem);  // "Columns" 列
	}

	// 添加tableview表头
	m->setHorizontalHeaderLabels(headers);

	// 利用 setModel() 方法将数据模型与 QTableView 绑定
	ui->tableViewParameter->setModel(m);
	ui->tableViewParameter->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	// 监听复选框状态变化，实现**一行互斥**
	connect(m, &QStandardItemModel::itemChanged, [ m ](QStandardItem* item) {
		int row = item->row();
		int col = item->column();

		if (item->checkState() == Qt::Checked) {
			// 如果当前列被选中，禁用本行的其他列
			for (int c = 0; c < 3; ++c) {
				if (c == col)
					continue;
				if (QStandardItem* otherItem = m->item(row, c); otherItem) {
					otherItem->setCheckState(Qt::Unchecked);
					otherItem->setFlags(otherItem->flags() & ~Qt::ItemIsEnabled);
				}
			}
		} else {
			// 如果当前列取消勾选，则恢复本行所有列的可选状态
			for (int c = 0; c < 3; ++c) {
				if (QStandardItem* resetItem = m->item(row, c); resetItem) {
					resetItem->setFlags(resetItem->flags() | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
				}
			}
		}
	});
}

QStringList DADialogCreatePivotTable::getPivotTableValue() const
{
	QStandardItemModel* model = qobject_cast< QStandardItemModel* >(ui->tableViewParameter->model());

	// 存储选中的文本值
	QStringList values;
	if (!model) {
		return values;
	}

	int rows = model->rowCount();

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
	// 存储选中的文本值
	QStringList indexs;

	if (!model) {
		return indexs;
	}

	int rows = model->rowCount();

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

/**
 * @brief 获取聚合函数
 *
 * @todo 这里不要使用combox的文本作为聚合函数的名字，应该使用combox的data，因为文本有可能进行翻译，翻译后就会导致错误，
 * 应该如下操作:
 * @code
 * ui->comboBoxAggfunc->addItem(tr("menu"),QString("menu"));//第一个是可翻译的文本，第二个是设置到item的QVariant的data内容，
 * @endcode
 *
 * @return
 */
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

/**
 * @brief
 *
 * @todo MarginsName不应该让用户输入，而是让用户选择，可以使用带checkbox的combox,这个去github上或者哪里搜索一下有没有现成的，这个combox把列名列举出来，再加上一个all，让用户自己勾选就行
 * @return
 */
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
