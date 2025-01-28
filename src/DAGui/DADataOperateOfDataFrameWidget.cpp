#include "DADataOperateOfDataFrameWidget.h"
#include "ui_DADataOperateOfDataFrameWidget.h"
#include "Models/DAPyDataFrameTableModule.h"
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
// stl
#include <memory>
// qt
#include <QTableView>
#include <QHeaderView>
#include <QSet>
#include <QMessageBox>
// python
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
// cmd
#include "Commands/DACommandsDataFrame.h"
// Dialog
#include "Dialog/DARenameColumnsNameDialog.h"
#include "Dialog/DADialogDataframeColumnCastToNumeric.h"
#include "Dialog/DADialogDataframeColumnCastToDatetime.h"
#include "Dialog/DADialogInsertNewColumn.h"

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DADataOperateOfDataFrameWidget
//===================================================
int DADataOperateOfDataFrameWidget::getDataOperatePageType() const
{
    return DADataOperatePageWidget::DataOperateOfDataFrame;
}

DADataOperateOfDataFrameWidget::DADataOperateOfDataFrameWidget(const DAData& d, QWidget* parent)
    : DADataOperatePageWidget(parent)
    , ui(new Ui::DADataOperateOfDataFrameWidget)
    , mDialogCastNumArgs(nullptr)
    , mDialogCastDatetimeArgs(nullptr)
{
    ui->setupUi(this);

	mModel = new DAPyDataFrameTableModule(getUndoStack(), this);
	ui->tableView->setModel(mModel);
	QFontMetrics fm = fontMetrics();
	ui->tableView->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.2);

	setDAData(d);
	connect(ui->tableView, &QTableView::clicked, this, &DADataOperateOfDataFrameWidget::onTableViewClicked);
}

DADataOperateOfDataFrameWidget::~DADataOperateOfDataFrameWidget()
{
    delete ui;
}

/**
 * @brief 是否存在data
 * @return
 */
bool DADataOperateOfDataFrameWidget::haveData() const
{
    return mData.isDataFrame();
}

/**
 * @brief 获取dataframe
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::getDataframe() const
{
    return mData.toDataFrame();
}

/**
 * @brief 获取Data的引用
 * @return
 */
const DAData& DADataOperateOfDataFrameWidget::data() const
{
    return mData;
}

void DADataOperateOfDataFrameWidget::setDAData(const DA::DAData& d)
{
	mData = d;
	mModel->setDAData(d);
}

void DADataOperateOfDataFrameWidget::insertRowAboveBySelect()
{
	int r = getSelectedOneDataframeRow();
	if (r < 0) {
		return;
	}
	insertRowAt(r);
}

void DADataOperateOfDataFrameWidget::insertRowBelowBySelect()
{
	int r = getSelectedOneDataframeRow();
	if (r < 0) {
		return;
	}
	insertRowAt(r + 1);
}

/**
 * @brief 在所选位置插入行
 * @param row
 */
void DADataOperateOfDataFrameWidget::insertRowAt(int row)
{
	std::unique_ptr< DACommandDataFrame_insertNanRow > cmd(
		new DACommandDataFrame_insertNanRow(mData.toDataFrame(), row, mModel));
	if (!cmd->isSuccess()) {
		return;
	}
	getUndoStack()->push(cmd.release());
}

/**
 * @brief 选中列右边插入新列
 */
void DADataOperateOfDataFrameWidget::insertColumnRightBySelect()
{
	int c = getSelectedOneDataframeColumn();
	if (c < 0) {
		return;
	}
	insertColumnAt(c + 1);
}

/**
 * @brief 选中列左边插入新列
 */
void DADataOperateOfDataFrameWidget::insertColumnLeftBySelect()
{
	int c = getSelectedOneDataframeColumn();
	if (c < 0) {
		return;
	}
	insertColumnAt(c);
}

/**
 * @brief 在col位置插入列
 * @param col
 */
void DADataOperateOfDataFrameWidget::insertColumnAt(int col)
{
	DADialogInsertNewColumn dlg(this);
	if (QDialog::Accepted != dlg.exec()) {
		return;
	}
	std::unique_ptr< DACommandDataFrame_insertColumn > cmd;
	QString name = dlg.getName();
	if (name.isEmpty()) {
		QMessageBox::warning(this,
							 tr("warning"),                                                     // cn: 警告
							 tr("The name of the new column to be inserted must be specified")  // cn:必须指定列的名字
		);
		return;
	}
	DAPyDType dt = dlg.getDType();
	if (dlg.isRangeMode()) {
		cmd.reset(new DACommandDataFrame_insertColumn(
			mData.toDataFrame(), col, name, dlg.getStartValue(), dlg.getStopValue(), mModel));
	} else {
		cmd.reset(new DACommandDataFrame_insertColumn(mData.toDataFrame(), col, name, dlg.getDefaultValue(), mModel));
	}
	if (!cmd->isSuccess()) {
		return;
	}
	getUndoStack()->push(cmd.release());
}

/**
 * @brief 移除选中的行
 * @return 返回成功移除的行数
 */
int DADataOperateOfDataFrameWidget::removeSelectRow()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return 0;
	}
	QList< int > rows = getSelectedDataframeRows();
	if (rows.size() <= 0) {
		qWarning() << tr("please select valid data cells");  // cn:请选择正确的行
		return 0;
	}
	std::unique_ptr< DACommandDataFrame_dropIRow > cmd(new DACommandDataFrame_dropIRow(mData.toDataFrame(), rows, mModel));
	if (!cmd->isSuccess()) {
		return 0;
	}
	getUndoStack()->push(cmd.release());
	return rows.size();
}

/**
 * @brief 移除选中的列
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectColumn()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return 0;
	}
	QList< int > columns = getSelectedDataframeCoumns();
	if (columns.size() <= 0) {
		qWarning() << tr("please select valid column");  // cn:请选择正确的列
		return 0;
	}
	std::unique_ptr< DACommandDataFrame_dropIColumn > cmd(
		new DACommandDataFrame_dropIColumn(mData.toDataFrame(), columns, mModel));
	if (!cmd->isSuccess()) {
		return 0;
	}
	getUndoStack()->push(cmd.release());
	return columns.size();
}

/**
 * @brief 设置选中单元格为nan,返回设置成功的个数
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectCell()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return 0;
	}
	const QList< QPoint > cells = getSelectedDataframeCells();
	if (cells.size() <= 0) {
		qWarning() << tr("please select valid cell");  // cn:请选择正确的单元格
		return 0;
	}
	QList< int > rows, cols;
	rows.reserve(cells.size());
	cols.reserve(cells.size());
	for (const auto p : cells) {
		rows.append(p.x());
		cols.append(p.y());
	}
	std::unique_ptr< DACommandDataFrame_setnan > cmd(new DACommandDataFrame_setnan(df, rows, cols, mModel));
	if (!cmd->isSuccess()) {
		return 0;
	}
	getUndoStack()->push(cmd.release());
	return cells.size();
}

/**
 * @brief 激活此窗口的UndoStack
 */

/**
 * @brief 更改列名
 */
void DADataOperateOfDataFrameWidget::renameColumns()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return;
	}
	QList< QString > oldcols = df.columns();
	if (oldcols.size() <= 0) {
		qWarning() << tr("table have not column");  // cn:表格没有列
		return;
	}
	DARenameColumnsNameDialog dlg(this);
	dlg.setDataName(mData.getName());
	dlg.setColumnsName(oldcols);
	if (QDialog::Accepted != dlg.exec()) {
		return;
	}
	QList< QString > cols = dlg.getColumnsName();
	// 唯一判断在DARenameColumnsNameDialog里进行
	//  header提前获取
	QHeaderView* hv                       = ui->tableView->horizontalHeader();
	DACommandDataFrame_renameColumns* cmd = new DACommandDataFrame_renameColumns(df, cols, oldcols, hv);
	getUndoStack()->push(cmd);
}

/**
 * @brief 设置选择列的数据类型
 * @param dtypeName
 * @return 成功改变类型返回true
 */
bool DADataOperateOfDataFrameWidget::changeSelectColumnType(const DAPyDType& dt)
{
	qDebug() << "changeSelectColumnType:" << dt;
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		emit selectTypeChanged({}, DAPyDType());
		return false;
	}
	QList< int > selColumns = getSelectedDataframeCoumns();
	if (selColumns.size() <= 0) {
		qWarning() << tr("please select valid column");  // cn:请选择正确的列
		emit selectTypeChanged({}, DAPyDType());
		return false;
	}
	std::unique_ptr< DACommandDataFrame_astype > cmd(new DACommandDataFrame_astype(df, selColumns, dt, mModel));
	if (!cmd->isSuccess()) {
		// 说明没有设置成功
		emit selectTypeChanged({}, DAPyDType());
		return false;
	}
	getUndoStack()->push(cmd.release());  // 成功，push会执行redo但会跳过
	emit selectTypeChanged(selColumns, dt);
	// 这里说明设置成功了
	return true;
}

/**
 * @brief 把选择的列转换为数值
 */
void DADataOperateOfDataFrameWidget::castSelectToNum()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return;
	}
	QList< int > colsIndex = getSelectedDataframeCoumns();
	if (colsIndex.size() <= 0) {
		qWarning() << tr("please select valid column");  // cn:请选择正确的列
		return;
	}
	if (mDialogCastNumArgs == nullptr) {
		mDialogCastNumArgs = new DADialogDataframeColumnCastToNumeric(this);
	}
	if (QDialog::Accepted != mDialogCastNumArgs->exec()) {
		return;
	}
	DAPyDType dt        = df.dtypes(colsIndex.first());
	pybind11::dict args = mDialogCastNumArgs->getArgs();
	std::unique_ptr< DACommandDataFrame_castNum > cmd(new DACommandDataFrame_castNum(df, colsIndex, args, mModel));
	if (!cmd->isSuccess()) {
		return;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	// 如果类型改变了刷新类型
	DAPyDType dt2 = df.dtypes(colsIndex.first());
	if (dt != dt2) {
		emit selectTypeChanged(colsIndex, dt2);
	}
}

/**
 * @brief 把选择的列转换为日期
 */
void DADataOperateOfDataFrameWidget::castSelectToDatetime()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return;
	}
	QList< int > colsIndex = getSelectedDataframeCoumns();
	if (colsIndex.size() <= 0) {
		qWarning() << tr("please select valid column");  // cn:请选择正确的列
		return;
	}
	if (mDialogCastDatetimeArgs == nullptr) {
		mDialogCastDatetimeArgs = new DADialogDataframeColumnCastToDatetime(this);
	}
	if (QDialog::Accepted != mDialogCastDatetimeArgs->exec()) {
		return;
	}
	DAPyDType dt        = df.dtypes(colsIndex.first());
	pybind11::dict args = mDialogCastDatetimeArgs->getArgs();
	std::unique_ptr< DACommandDataFrame_castDatetime > cmd(new DACommandDataFrame_castDatetime(df, colsIndex, args, mModel));
	if (!cmd->isSuccess()) {
		return;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	// 如果类型改变了刷新类型
	DAPyDType dt2 = df.dtypes(colsIndex.first());
	if (dt != dt2) {
		emit selectTypeChanged(colsIndex, dt2);
	}
}

/**
 * @brief 把选择的列转换为索引
 * @return
 */
bool DADataOperateOfDataFrameWidget::changeSelectColumnToIndex()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	QList< int > colsIndex = getSelectedDataframeCoumns();
	if (colsIndex.size() <= 0) {
		qWarning() << tr("please select valid column");  // cn:请选择正确的列
		return false;
	}
	std::unique_ptr< DACommandDataFrame_setIndex > cmd(
		new DACommandDataFrame_setIndex(df, colsIndex, ui->tableView->verticalHeader(), mModel));
	if (!cmd->isSuccess()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 删除缺失值
 */
bool DADataOperateOfDataFrameWidget::dropna()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}

	return true;
}

/**
 * @brief 创建一个数据描述
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::createDataDescribe()
{
	if (!mData.isDataFrame()) {
		return DAPyDataFrame();
	}
	DAPyDataFrame df_describe = mData.toDataFrame().describe();
	return df_describe;
}

QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeCoumns(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return QList< int >();
	}
	QSet< int > columns;
	QModelIndexList selindexs = selModel->selectedIndexes();
	if (ensureInDataframe) {
		// 确保返回的列数都在dataframe里
		DAPyDataFrame df = getDataframe();
		if (df.isNone()) {
			return QList< int >();
		}
		auto shape = df.shape();
		for (const QModelIndex& i : selindexs) {
			if (i.column() < (int)shape.second) {
				columns.insert(i.column());
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		for (const QModelIndex& i : selindexs) {
			columns.insert(i.column());
		}
	}
	return columns.values();
}

QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeRows(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return QList< int >();
	}
	QSet< int > rows;
	QModelIndexList selindexs = selModel->selectedIndexes();
	if (ensureInDataframe) {
		// 确保返回的列数都在dataframe里
		DAPyDataFrame df = getDataframe();
		if (df.isNone()) {
			return QList< int >();
		}
		auto shape = df.shape();
		for (const QModelIndex& i : selindexs) {
			if (i.row() < (int)shape.first) {
				rows.insert(i.row());
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		for (const QModelIndex& i : selindexs) {
			rows.insert(i.row());
		}
	}
	return rows.values();
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeRow(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return -1;
	}
	QModelIndexList selindexs = selModel->selectedIndexes();
	if (selindexs.isEmpty()) {
		return -1;
	}
	if (ensureInDataframe) {
		// 确保返回的列数都在dataframe里
		DAPyDataFrame df = getDataframe();
		if (df.isNone()) {
			return -1;
		}
		auto shape = df.shape();
		for (const QModelIndex& i : selindexs) {
			if (i.row() < (int)shape.first) {
				return i.row();
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		return selindexs.first().row();
	}
	return -1;
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeColumn(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return -1;
	}
	QModelIndexList selindexs = selModel->selectedIndexes();
	if (selindexs.isEmpty()) {
		return -1;
	}
	if (ensureInDataframe) {
		// 确保返回的列数都在dataframe里
		DAPyDataFrame df = getDataframe();
		if (df.isNone()) {
			return -1;
		}
		auto shape = df.shape();
		for (const QModelIndex& i : selindexs) {
			if (i.column() < (int)shape.second) {
				return i.column();
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		return selindexs.first().column();
	}
	return -1;
}

/**
 * @brief 获取当前表格操作选中的数据
 *
 * 如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，
 * 如果用户选中了多列，那么每列作为一个DAData，最后组成一个QList<DAData>返回,如果用户打开了表格，但没选择任何列，这个函数返回一个空list
 *
 * 如果用户没有选择列，但选中了单元格，那么相当于选中了单元格对应的列
 *
 * 如果什么都没选中，那么返回一个空的list
 *
 * @return
 */
QList< DAData > DADataOperateOfDataFrameWidget::getSlectedSeries() const
{
	QList< DAData > res;
	const QList< int > indexs = getSelectedDataframeCoumns(true);
	if (indexs.isEmpty()) {
		return res;
	}
	DAPyDataFrame df = mData.toDataFrame();
	auto shape       = df.shape();
	for (int i : indexs) {
		if (i < shape.second) {
			auto series = df[ i ];
			DAData d(series);
			res.append(d);
		}
	}
	return res;
}

QList< QPoint > DADataOperateOfDataFrameWidget::getSelectedDataframeCells(bool ensureInDataframe) const
{
	QList< QPoint > res;
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return res;
	}
	QList< int > rows;
	QList< int > cols;
	QModelIndexList selindexs = selModel->selectedIndexes();
	if (ensureInDataframe) {
		// 确保返回的列数都在dataframe里
		DAPyDataFrame df = getDataframe();
		if (df.isNone()) {
			return res;
		}
		auto shape = df.shape();
		for (const QModelIndex& index : selindexs) {
			if (index.row() < (int)shape.first && index.column() < (int)shape.second) {
				res.append(QPoint(index.row(), index.column()));
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		for (const QModelIndex& index : selindexs) {
			res.append(QPoint(index.row(), index.column()));
		}
	}
	return res;
}

/**
 * @brief 表格点击
 * @param index
 */
void DADataOperateOfDataFrameWidget::onTableViewClicked(const QModelIndex& index)
{
	if (!mData.isDataFrame()) {
		emit selectTypeChanged({ index.column() }, DAPyDType());
		return;
	}
	DAPyDataFrame df = mData.toDataFrame();
	if (index.column() >= (int)df.shape().second) {
		emit selectTypeChanged({ index.column() }, DAPyDType());
		return;
	}
	emit selectTypeChanged({ index.column() }, df.dtypes(index.column()));
}

void DADataOperateOfDataFrameWidget::changeEvent(QEvent* e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}
