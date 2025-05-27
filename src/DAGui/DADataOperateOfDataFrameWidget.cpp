#include "DADataOperateOfDataFrameWidget.h"
#include "ui_DADataOperateOfDataFrameWidget.h"
#include "Models/DAPyDataFrameTableModel.h"
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
#include "Dialog/DADialogDataFrameFillna.h"
#include "Dialog/DADialogDataFrameFillInterpolate.h"
#include "Dialog/DADialogDataFrameClipOutlier.h"
#include "Dialog/DADialogDataFrameEvalDatas.h"
#include "Dialog/DADialogDataFrameQueryDatas.h"
#include "Dialog/DADialogDataFrameDataSearch.h"
#include "Dialog/DADialogCreatePivotTable.h"
#include "Dialog/DADialogDataFrameSort.h"

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

	mModel = new DAPyDataFrameTableModel(getUndoStack(), this);

	ui->tableView->setModel(mModel);
	// 关闭不必要的绘制特性
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
	if (d.isDataFrame()) {
		ui->tableView->setDataFrame(d.toDataFrame());
	}
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
	if (!cmd->exec()) {
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
	if (!cmd->exec()) {
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
	if (!cmd->exec()) {
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
	if (!cmd->exec()) {
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
	if (!cmd->exec()) {
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
	if (!cmd->exec()) {
		return;
	}
	//! 通知datamanager
	if (DADataManager* mgr = mData.getDataManager()) {
		mgr->callDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
	}

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
	if (!cmd->exec()) {
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
	if (!cmd->exec()) {
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
	if (!cmd->exec()) {
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
	std::unique_ptr< DACommandDataFrame_setIndex > cmd =
		std::make_unique< DACommandDataFrame_setIndex >(df, colsIndex, ui->tableView->verticalHeader(), mModel);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 删除缺失值
 * @param how 可选参数，表示删除的条件。默认值为’any’，表示只要存在一个缺失值就删除整行或整列；设置为’all’表示只有当整行或整列都是缺失值时才删除
 * @param thresh 可选参数，表示在删除之前需要满足的非缺失值的最小数量。如果行或列中的非缺失值数量小于等于thresh，则会被删除，-1代表不生效
 * @return 返回删除的数量，0代表没有删除任何内容
 */
int DADataOperateOfDataFrameWidget::dropna(const QString& how, std::optional< int > thresh)
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return 0;
	}
	int axis = 0;
	QList< int > index;
	if (isDataframeTableHaveSelection()) {
		// 先看看是否选中了列
		index = getFullySelectedDataframeColumns();
		if (!index.isEmpty()) {
			// 说明单独选中了一列，这时只针对列进行dropna
			axis = 0;
		} else {
			// 如果没有，就看看是否选中了行
			index = getFullySelectedDataframeRows();
			if (!index.isEmpty()) {
				// 说明单独选中了一些行，这时就是删除这些选中行里包含空的列
				axis = 1;
			}
		}
	}
	return dropna(df, axis, how, index, thresh);
}

/**
 * @brief 删除缺失值
 * @param axis 删除轴向0，代表删除行，1代表删除列
 * @param how 可选参数，表示删除的条件。默认值为’any’，表示只要存在一个缺失值就删除整行或整列；设置为’all’表示只有当整行或整列都是缺失值时才删除
 * @param index 可选参数，用于指定要检查缺失值的特定列名或行索引。
 * @param thresh 可选参数，表示在删除之前需要满足的非缺失值的最小数量。如果行或列中的非缺失值数量小于等于thresh，则会被删除，-1代表不生效
 * @return 返回删除的数量，0代表没有删除任何内容
 */
int DADataOperateOfDataFrameWidget::dropna(const DAPyDataFrame& df,
                                           int axis,
                                           const QString& how,
                                           const QList< int > index,
                                           std::optional< int > thresh)
{

	std::unique_ptr< DACommandDataFrame_dropna > cmd =
		std::make_unique< DACommandDataFrame_dropna >(df, mModel, axis, how, index, thresh);
	if (!cmd->exec()) {
		return false;
	}
	int dropcnt = cmd->getDropedCount();
	if (dropcnt == 0) {
		// 说明没有删除任何内容，也返回0
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return dropcnt;
}

/**
 * @brief 填充缺失值
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::fillna()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	if (!mDialogDataFrameFillna) {
		mDialogDataFrameFillna = new DADialogDataFrameFillna(this);
	}
	if (QDialog::Accepted != mDialogDataFrameFillna->exec()) {
		// 说明用户取消
		return false;
	}
	// 获取填充值
	double value   = mDialogDataFrameFillna->getFillNanValue();
	int limitCount = -1;  // 如果-1证明没有设置
	if (mDialogDataFrameFillna->isEnableLimitCount()) {
		limitCount = mDialogDataFrameFillna->getLimitCount();
	}
	return fillna(df, value, limitCount);
}

/**
 * @brief 填充缺失值
 * @param value 可选参数，将缺失值填充为某特定值
 * @param limit 可选参数，表示填充行数/列数的限制。
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::fillna(const DAPyDataFrame& df, double value, int limit)
{
	std::unique_ptr< DACommandDataFrame_fillna > cmd =
		std::make_unique< DACommandDataFrame_fillna >(df, mModel, value, limit);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 插值法填充缺失值
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::interpolate()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	if (!mDialogDataFrameFillInterpolate) {
		mDialogDataFrameFillInterpolate = new DADialogDataFrameFillInterpolate(this);
	}
	if (QDialog::Accepted != mDialogDataFrameFillInterpolate->exec()) {
		// 说明用户取消
		return false;
	}
	// 获取插值填充方法
	QString method = mDialogDataFrameFillInterpolate->getInterpolateMethod();
	// 获取插值方向
	int axis = mDialogDataFrameFillInterpolate->getInterPolateAxis();
	// 获取多项式插值次数
	int order      = mDialogDataFrameFillInterpolate->getInterpolateOrder();
	int limitCount = -1;  // 如果-1证明没有设置
	if (mDialogDataFrameFillInterpolate->isEnableLimitCount()) {
		limitCount = mDialogDataFrameFillInterpolate->getLimitCount();
	}
	return interpolate(df, method, order, limitCount);
}

bool DADataOperateOfDataFrameWidget::interpolate(const DAPyDataFrame& df, const QString& method, int order, int limit)
{
	std::unique_ptr< DACommandDataFrame_interpolate > cmd =
		std::make_unique< DACommandDataFrame_interpolate >(df, mModel, method, order, limit);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 前向填充缺失值
 * @return 返回删除的数量，0代表没有删除任何内容
 */
bool DADataOperateOfDataFrameWidget::ffillna()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	int axis = 0;
	return ffillna(df, axis, -1);
}

/**
 * @brief 前向填充缺失值
 * @param axis 填充轴向，0代表按行填充，1代表按列填充
 * @return 返回填充的数量，0代表没有填充任何内容
 */
bool DADataOperateOfDataFrameWidget::ffillna(const DAPyDataFrame& df, int axis, int limit)
{
	std::unique_ptr< DACommandDataFrame_ffillna > cmd =
		std::make_unique< DACommandDataFrame_ffillna >(df, mModel, axis, limit);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 后向填充缺失值
 * @return 返回删除的数量，0代表没有删除任何内容
 */
bool DADataOperateOfDataFrameWidget::bfillna()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	int axis = 0;
	return bfillna(df, axis, -1);
}

/**
 * @brief 后向填充缺失值
 * @param axis 填充轴向，0代表按行填充，1代表按列填充
 * @return 返回填充的数量，0代表没有填充任何内容
 */
bool DADataOperateOfDataFrameWidget::bfillna(const DAPyDataFrame& df, int axis, int limit)
{
	std::unique_ptr< DACommandDataFrame_bfillna > cmd =
		std::make_unique< DACommandDataFrame_bfillna >(df, mModel, axis, limit);

	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 删除重复值
 * @param keep 可选参数，表示指定保留哪个重复的行。默认值为'first'，表示保留第一次出现的重复行；设置为'last'表示保留最后一次出现的重复行,False表示删除所有重复的行。
 * @return 返回删除的数量，0代表没有删除任何内容
 */
int DADataOperateOfDataFrameWidget::dropduplicates(const QString& keep)
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return 0;
	}
	QList< int > index;
	if (isDataframeTableHaveSelection()) {
		// 获取选中的列
		index = getFullySelectedDataframeColumns();
	}
	return dropduplicates(df, keep, index);
}

/**
 * @brief 删除重复值
 * @param keep 可选参数，表示指定保留哪个重复的行。默认值为'first'，表示保留第一次出现的重复行；设置为'last'表示保留最后一次出现的重复行,False表示删除所有重复的行。
 * @param index 可选参数，用于指定用于判断重复的列或列列表。如果为 None，则使用所有列。
 * @return 返回删除的数量，0代表没有删除任何内容
 */
int DADataOperateOfDataFrameWidget::dropduplicates(const DAPyDataFrame& df, const QString& keep, const QList< int > index)
{
	std::unique_ptr< DACommandDataFrame_dropduplicates > cmd =
		std::make_unique< DACommandDataFrame_dropduplicates >(df, mModel, keep, index);
	if (!cmd->exec()) {
		return 0;
	}
	int dropcnt = cmd->getDropedCount();
	if (dropcnt == 0) {
		// 说明没有删除任何内容，也返回0
		return 0;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return dropcnt;
}

/**
 * @brief n倍标准差法删除异常值
 * @return 返回删除的数量，0代表没有删除任何内容
 */
int DADataOperateOfDataFrameWidget::nstdfilteroutlier(double n)
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	int axis = 1;
	QList< int > index;
	if (isDataframeTableHaveSelection()) {
		// 先看看是否选中了列
		index = getFullySelectedDataframeColumns();
	}
	return nstdfilteroutlier(df, n, axis, index);
}

/**
 * @brief n倍标准差法删除异常值
 * @param axis 填充轴向，0代表按行填充，1代表按列填充
 * @return 返回填充的数量，0代表没有填充任何内容
 */
int DADataOperateOfDataFrameWidget::nstdfilteroutlier(const DAPyDataFrame& df, double n, int axis, const QList< int > index)
{
	std::unique_ptr< DACommandDataFrame_nstdfilteroutlier > cmd =
		std::make_unique< DACommandDataFrame_nstdfilteroutlier >(df, mModel, n, axis, index);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 替换规定界限外的异常值
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::clipoutlier()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	//	if (!mDialogDataFrameQueryDatas) {
	//		mDialogDataFrameQueryDatas = new DADialogDataFrameQueryDatas(this);
	//	}
	if (!mDialogDataFrameClipOutlier) {
		mDialogDataFrameClipOutlier = new DADialogDataFrameClipOutlier(this);
	}
	if (QDialog::Accepted != mDialogDataFrameClipOutlier->exec()) {
		// 说明用户取消
		return false;
	}
	// 获取过滤数据上下限
	double lowervalue = mDialogDataFrameClipOutlier->getLowerValue();
	double uppervalue = mDialogDataFrameClipOutlier->getUpperValue();

	int axis = 0;

	return clipoutlier(df, lowervalue, uppervalue, axis);
}

/**
 * @brief 替换规定界限外的异常值
 * @param lower 可选参数，下界值
 * @param upper 可选参数，上界值。
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::clipoutlier(const DAPyDataFrame& df, double lower, double upper, int axis)
{
	std::unique_ptr< DACommandDataFrame_clipoutlier > cmd =
		std::make_unique< DACommandDataFrame_clipoutlier >(df, mModel, lower, upper, axis);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 过滤给定条件外的数据
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::queryDatas()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	if (!mDialogDataFrameQueryDatas) {
		mDialogDataFrameQueryDatas = new DADialogDataFrameQueryDatas(this);
	}
	if (QDialog::Accepted != mDialogDataFrameQueryDatas->exec()) {
		// 说明用户取消
		return false;
	}
	// 获取填充值
	QString exper = mDialogDataFrameQueryDatas->getExpr();
	return queryDatas(df, exper);
}

/**
 * @brief 过滤给定条件外的数据。
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::queryDatas(const DAPyDataFrame& df, const QString& exper)
{
	std::unique_ptr< DACommandDataFrame_querydatas > cmd =
		std::make_unique< DACommandDataFrame_querydatas >(df, exper, mModel);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 检索给定的数据
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::searchData()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	if (!mDialogDataFrameDataSearch) {
		mDialogDataFrameDataSearch = new DADialogDataFrameDataSearch(this);
	}
	if (QDialog::Accepted != mDialogDataFrameDataSearch->exec()) {
		// 说明用户取消
		return false;
	}
	// 获取填充值
	QString exper = mDialogDataFrameDataSearch->getExpr();
	return queryDatas(df, exper);
}

/**
 * @brief 检索给定的数据
 * @param exper 可选参数
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::searchData(const DAPyDataFrame& df, const QString& exper)
{
	std::unique_ptr< DACommandDataFrame_searchdata > cmd =
		std::make_unique< DACommandDataFrame_searchdata >(df, exper, mModel);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 列运算
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::evalDatas()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	if (!mDialogDataFrameEvalDatas) {
		mDialogDataFrameEvalDatas = new DADialogDataFrameEvalDatas(this);
	}
	if (QDialog::Accepted != mDialogDataFrameEvalDatas->exec()) {
		// 说明用户取消
		return false;
	}
	// 获取填充值
	QString exper = mDialogDataFrameEvalDatas->getExpr();
	return evalDatas(df, exper);
}

/**
 * @brief 列运算
 * @param df
 * @param exper
 * @return
 */
bool DADataOperateOfDataFrameWidget::evalDatas(const DAPyDataFrame& df, const QString& exper)
{
	std::unique_ptr< DACommandDataFrame_evalDatas > cmd =
		std::make_unique< DACommandDataFrame_evalDatas >(df, exper, mModel);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 数据排序
 * @return
 */
bool DADataOperateOfDataFrameWidget::sortDatas()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}

	if (!mDADialogDataFrameSort) {
		mDADialogDataFrameSort = new DADialogDataFrameSort(this);
	}

	mDADialogDataFrameSort->setDataframe(df);

	// 获取选中的列
	if (isDataframeTableHaveSelection())
		mDADialogDataFrameSort->setSortBy(getSelectedOneDataframeColumn());

	if (QDialog::Accepted != mDADialogDataFrameSort->exec())
		return false;

	// 获取排序参数
	QString by     = mDADialogDataFrameSort->getSortBy();
	bool ascending = mDADialogDataFrameSort->getSortType();
	return sortDatas(df, by, ascending);
}

/**
 * @brief 数据排序
 * @return
 */
bool DADataOperateOfDataFrameWidget::sortDatas(const DAPyDataFrame& df, const QString& by, const bool ascending)
{
	std::unique_ptr< DACommandDataFrame_sort > cmd = std::make_unique< DACommandDataFrame_sort >(df, by, ascending, mModel);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
	return true;
}

/**
 * @brief 过滤给定条件外的数据
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::filterByColumn()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return false;
	}
	if (!mDialogDataFrameClipOutlier)
		mDialogDataFrameClipOutlier = new DADialogDataFrameClipOutlier(this);

	mDialogDataFrameClipOutlier->setDataframe(df);

	// 获取选中的列
	if (isDataframeTableHaveSelection())
		mDialogDataFrameClipOutlier->setFilterData(getSelectedOneDataframeColumn());

	if (QDialog::Accepted != mDialogDataFrameClipOutlier->exec())
		return false;

	// 获取过滤参数
	QString index     = mDialogDataFrameClipOutlier->getFilterData();
	double lowervalue = mDialogDataFrameClipOutlier->getLowerValue();
	double uppervalue = mDialogDataFrameClipOutlier->getUpperValue();

	return filterByColumn(df, lowervalue, uppervalue, index);
}

/**
 * @brief 过滤给定条件外的数据
 * @param lower 可选参数，下界值
 * @param upper 可选参数，上界值。
 * @return 成功返回true,反之返回false
 */
bool DADataOperateOfDataFrameWidget::filterByColumn(const DAPyDataFrame& df, double lower, double upper, const QString& index)
{
	std::unique_ptr< DACommandDataFrame_filterByColumn > cmd =
		std::make_unique< DACommandDataFrame_filterByColumn >(df, lower, upper, index, mModel);
	if (!cmd->exec()) {
		return false;
	}
	getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
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

/**
 * @brief 创建数据透视表。
 * @return
 */

DAPyDataFrame DADataOperateOfDataFrameWidget::createPivotTable()
{
	DAPyDataFrame df = getDataframe();
	if (df.isNone()) {
		return DAPyDataFrame();
	}
	if (!mDialogCreatePivotTable) {
		mDialogCreatePivotTable = new DADialogCreatePivotTable(this);
	}
	mDialogCreatePivotTable->setDataframe(df);
	if (QDialog::Accepted != mDialogCreatePivotTable->exec()) {
		// 说明用户取消
		return DAPyDataFrame();
	}
	// 获取创建透视表的参数
	QStringList value   = mDialogCreatePivotTable->getPivotTableValue();
	QStringList index   = mDialogCreatePivotTable->getPivotTableIndex();
	QStringList columns = mDialogCreatePivotTable->getPivotTableColumn();
	QString aggfunc     = mDialogCreatePivotTable->getPivotTableAggfunc();
	bool margins        = mDialogCreatePivotTable->isEnableMarginsName();
	QString marginsName = mDialogCreatePivotTable->getMarginsName();
	bool sort           = mDialogCreatePivotTable->isEnableSort();

	// 如果用户没有选定分组，则返回空
	if (index.empty())
		return DAPyDataFrame();

	return createPivotTable(df, value, index, columns, aggfunc, margins, marginsName, sort);
}

/**
 * @brief 创建数据透视表。
 * @param value 可选参数，要进行汇总的数据值
 * @param index 可选参数，被分析的特征，列、数组、列表
 * @param columns 可选参数，进行分组的特征，列、数组、列表
 * @param aggfunc 可选参数，聚合函数，计算类型
 * @param margins 可选参数，行列数据的统计
 * @param marginsName 可选参数，行列数据的统计的名称
 * @param sort 可选参数，聚合后的结果排序
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::createPivotTable(const DAPyDataFrame& df,
                                                               const QStringList value,
                                                               const QStringList index,
                                                               const QStringList columns,
                                                               const QString& aggfunc,
                                                               bool margins,
                                                               const QString& marginsName,
                                                               bool sort)
{

	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();

	DAPyDataFrame df_pivottable = pydf.pivotTable(df, value, index, columns, aggfunc, margins, marginsName, sort);
	return df_pivottable;
}

/**
 * @brief dataframe表格是否有选中项
 * @return
 */
bool DADataOperateOfDataFrameWidget::isDataframeTableHaveSelection() const
{
	const QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
	if (!selectionModel) {
		return false;
	}
	return selectionModel->hasSelection();
}

/**
 * @brief 返回当前选中单元格所包含的列数，列数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeCoumns(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return QList< int >();
	}
	QSet< int > res;
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
				res.insert(i.column());
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		for (const QModelIndex& i : selindexs) {
			res.insert(i.column());
		}
	}
	return res.values();
}

/**
 * @brief 返回当前选中单元格所包含的行数，行数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeRows(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return QList< int >();
	}
	QSet< int > res;
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
				res.insert(i.row());
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		for (const QModelIndex& i : selindexs) {
			res.insert(i.row());
		}
	}
	return res.values();
}

/**
 * @brief 返回表格中完全选中的一整列的列数
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getFullySelectedDataframeColumns(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return QList< int >();
	}
	QSet< int > res;
	QModelIndexList selindexs = selModel->selectedColumns();
	if (ensureInDataframe) {
		// 确保返回的列数都在dataframe里
		DAPyDataFrame df = getDataframe();
		if (df.isNone()) {
			return QList< int >();
		}
		auto shape = df.shape();
		for (const QModelIndex& i : selindexs) {
			if (i.column() < (int)shape.second) {
				res.insert(i.column());
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		for (const QModelIndex& i : selindexs) {
			res.insert(i.column());
		}
	}
	return res.values();
}

/**
 * @brief 返回表格中完全选中的一整行的行数
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getFullySelectedDataframeRows(bool ensureInDataframe) const
{
	QItemSelectionModel* selModel = ui->tableView->selectionModel();
	if (!selModel) {
		return QList< int >();
	}
	QSet< int > res;
	QModelIndexList selindexs = selModel->selectedRows();
	if (ensureInDataframe) {
		// 确保返回的列数都在dataframe里
		DAPyDataFrame df = getDataframe();
		if (df.isNone()) {
			return QList< int >();
		}
		auto shape = df.shape();
		for (const QModelIndex& i : selindexs) {
			if (i.row() < (int)shape.first) {
				res.insert(i.row());
			}
		}
	} else {
		// 不确保返回的列数都在dataframe里
		for (const QModelIndex& i : selindexs) {
			res.insert(i.row());
		}
	}
	return res.values();
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
	DAPyDType t;
	try {
		t = df.dtypes(index.column());
	} catch (const std::exception& e) {
		qCritical() << e.what();
	}

	emit selectTypeChanged({ index.column() }, t);
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
