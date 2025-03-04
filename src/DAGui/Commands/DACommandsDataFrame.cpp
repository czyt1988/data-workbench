#include "DACommandsDataFrame.h"
#include "Models/DAPyDataFrameTableModule.h"
#include "DAPyScripts.h"
#include <QHeaderView>

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DACommandDataFrame_iat
//===================================================
DACommandDataFrame_iat::DACommandDataFrame_iat(const DAPyDataFrame& df,
                                               int row,
                                               int col,
                                               const QVariant& olddata,
                                               const QVariant& newdata,
                                               DAPyDataFrameTableModule* model,
                                               QUndoCommand* par)
    : DACommandWithRedoCount(par), mDataframe(df), mRow(row), mCol(col), mOldData(olddata), mNewData(newdata), mModel(model)
{
	setText(QObject::tr("set dataframe data"));  // cn:改变单元格数据
}

void DACommandDataFrame_iat::undo()
{

	mDataframe.iat(mRow, mCol, mOldData);
	if (mModel) {
		mModel->refresh(mRow, mCol);
	}
}

bool DACommandDataFrame_iat::exec()
{
	if (!mDataframe.iat(mRow, mCol, mNewData)) {
		return false;
	}
	if (mModel) {
		mModel->refresh(mRow, mCol);
	}
	return true;
}

///////////////////////////////

DACommandDataFrame_insertNanRow::DACommandDataFrame_insertNanRow(const DAPyDataFrame& df,
                                                                 int row,
                                                                 DAPyDataFrameTableModule* model,
                                                                 QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mRow(row), mModel(model)
{
	setText(QObject::tr("insert row"));  // cn:插入一行
}

void DACommandDataFrame_insertNanRow::undo()
{
	load();
	if (mModel) {
		mModel->rowBeginRemove({ mRow });
		mModel->rowEndRemove();
	}
}

bool DACommandDataFrame_insertNanRow::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	if (!pydf.insert_nanrow(dataframe(), mRow)) {
		return false;
	}
	if (mModel) {
		mModel->rowBeginInsert({ mRow });
		mModel->rowEndInsert();
	}
	return true;
}

///////////////////////////////

/**
 * @brief 插入列
 *
 * da_insert_column(df=df, col=col, name=name,defaultvalue=defaultvalue)
 * @param df
 * @param col
 * @param name
 * @param defaultvalue
 * @param model
 * @param par
 */
DACommandDataFrame_insertColumn::DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
                                                                 int col,
                                                                 const QString& name,
                                                                 const QVariant& defaultvalue,
                                                                 DAPyDataFrameTableModule* model,
                                                                 QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIsRangeMode(false), mCol(col), mName(name), mDefaultvalue(defaultvalue), mModel(model)
{
	setText(QObject::tr("insert column \"%1\"").arg(name));  // cn: 插入列“%1”
}

/**
 * @brief DACommandDataFrame_insertColumn::DACommandDataFrame_insertColumn
 *
 * da_insert_column(df=df, col=col, name=name,start=start,stop=stop)
 * da_insert_column(df=df, col=col, name=name,dtype=np.datetime64,start='2020-01-01',stop='2021-01-01')
 * @param df
 * @param col
 * @param name
 * @param start
 * @param stop
 * @param model
 * @param par
 */
DACommandDataFrame_insertColumn::DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
                                                                 int col,
                                                                 const QString& name,
                                                                 const QVariant& start,
                                                                 const QVariant& stop,
                                                                 DAPyDataFrameTableModule* model,
                                                                 QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIsRangeMode(true), mCol(col), mName(name), mStart(start), mStop(stop), mModel(model)
{
	setText(QObject::tr("insert column \"%1\"").arg(name));  // cn: 插入列“%1”
}

void DACommandDataFrame_insertColumn::undo()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	mInsertedSeries            = pydf.itake_column(dataframe(), mCol);
	if (mModel) {
		// 此操作会删除一列，添加一列，整个modelreflash
		mModel->columnBeginRemove({ mCol });
		mModel->columnEndRemove();
	}
}

bool DACommandDataFrame_insertColumn::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	if (mInsertedSeries.isNone()) {
		if (mIsRangeMode) {
			if (!pydf.insert_column(dataframe(), mCol, mName, mStart, mStop)) {
				return false;
			}
		} else {
			if (!pydf.insert_column(dataframe(), mCol, mName, mDefaultvalue)) {
				return false;
			}
		}
	} else {
		// 说明已经插入了series
		if (!pydf.insert_at(dataframe(), mCol, mInsertedSeries)) {
			return false;
		}
	}

	if (mModel) {
		// 此操作会删除一列，添加一列，整个modelreflash
		mModel->columnBeginInsert({ mCol });
		mModel->columnEndInsert();
	}
	return true;
}

////////////////////////////////

/**
 * @brief 删除行
 * @param df
 * @param index
 * @param par
 */
DACommandDataFrame_dropIRow::DACommandDataFrame_dropIRow(const DAPyDataFrame& df,
                                                         const QList< int >& index,
                                                         DAPyDataFrameTableModule* model,
                                                         QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIndex(index), mModel(model)
{
	setText(QObject::tr("drop dataframe rows"));  // cn:移除dataframe行
}

void DACommandDataFrame_dropIRow::undo()
{
	load();
	if (mModel) {
		mModel->rowBeginInsert(mIndex);
		mModel->rowEndInsert();
	}
}

bool DACommandDataFrame_dropIRow::exec()
{
	DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
	if (!py.drop_irow(mDataframe, mIndex)) {
		return false;
	}
	if (mModel) {
		mModel->rowBeginRemove(mIndex);
		mModel->rowEndRemove();
	}
	return true;
}

////////////////////

DACommandDataFrame_dropIColumn::DACommandDataFrame_dropIColumn(const DAPyDataFrame& df,
                                                               const QList< int >& index,
                                                               DAPyDataFrameTableModule* model,
                                                               QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIndex(index), mModel(model)
{
	setText(QObject::tr("drop dataframe columns"));  // cn:移除dataframe列
}

void DACommandDataFrame_dropIColumn::undo()
{
	load();
	if (mModel) {
		mModel->columnBeginInsert(mIndex);
		mModel->columnEndInsert();
	}
}

bool DACommandDataFrame_dropIColumn::exec()
{
	DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
	if (!py.drop_icolumn(mDataframe, mIndex)) {
		return false;
	}
	if (mModel) {
		mModel->columnBeginRemove(mIndex);
		mModel->columnEndRemove();
	}
	return true;
}

////////////////////

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                                                   const QList< QString >& cols,
                                                                   QHeaderView* hv,
                                                                   QUndoCommand* par)
    : DACommandWithRedoCount(par), mDataframe(df), mCols(cols), mHeaderView(hv)
{
    mOldcols = df.columns();
}

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                                                   const QList< QString >& cols,
                                                                   const QList< QString >& oldcols,
                                                                   QHeaderView* hv,
                                                                   QUndoCommand* par)
    : DACommandWithRedoCount(par), mDataframe(df), mCols(cols), mHeaderView(hv), mOldcols(oldcols)
{
}

void DACommandDataFrame_renameColumns::undo()
{
	mDataframe.columns(mOldcols);
	if (mHeaderView) {
		mHeaderView->headerDataChanged(Qt::Horizontal, 0, mCols.size() - 1);
	}
}

bool DACommandDataFrame_renameColumns::exec()
{
	if (!mDataframe.columns(mCols)) {
		return false;
	}
	if (mHeaderView) {
		mHeaderView->headerDataChanged(Qt::Horizontal, 0, mCols.size() - 1);
	}
	return true;
}

////////////////////////////

DACommandDataFrame_astype::DACommandDataFrame_astype(const DAPyDataFrame& df,
                                                     const QList< int >& index,
                                                     const DAPyDType& dt,
                                                     DAPyDataFrameTableModule* model,
                                                     QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIndex(index), mDtype(dt), mModel(model)
{
	setText(QObject::tr("change column type"));  // cn:改变列数据类型
}

void DACommandDataFrame_astype::undo()
{
	load();
	if (mModel) {
		for (int c : qAsConst(mIndex)) {
			mModel->refreshColumn(c);
		}
	}
}

bool DACommandDataFrame_astype::exec()
{
	if (mDtype.isNone()) {
		return false;
	}
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	if (!pydf.astype(mDataframe, mIndex, mDtype)) {
		return false;
	}
	if (mModel) {
		for (int c : qAsConst(mIndex)) {
			mModel->refreshColumn(c);
		}
	}
	return true;
}

///////////////////////////

DACommandDataFrame_setnan::DACommandDataFrame_setnan(const DAPyDataFrame& df,
                                                     const QList< int >& rows,
                                                     const QList< int >& columns,
                                                     DAPyDataFrameTableModule* model,
                                                     QUndoCommand* par)
    : DACommandWithRedoCount(par), mDataframe(df), mRows(rows), mColumns(columns), mModel(model)
{
	// 先把原来数据提取出来
	for (int i = 0; i < mRows.size(); ++i) {
		mOlddatas.append(df.iatObj(rows[ i ], columns[ i ]));
	}
}

void DACommandDataFrame_setnan::undo()
{
	for (int i = 0; i < mRows.size(); ++i) {
		mDataframe.iat(mRows[ i ], mColumns[ i ], mOlddatas[ i ]);
		mModel->refresh(mRows[ i ], mColumns[ i ]);
	}
}

bool DACommandDataFrame_setnan::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	if (!pydf.setnan(mDataframe, mRows, mColumns)) {
		return false;
	}
	if (mModel) {
		for (int i = 0; i < mRows.size(); ++i) {
			mModel->refresh(mRows[ i ], mColumns[ i ]);
		}
	}
	return true;
}
////////////////////////////

DACommandDataFrame_dropna::DACommandDataFrame_dropna(const DAPyDataFrame& df,
                                                     DAPyDataFrameTableModule* model,
                                                     int axis,
                                                     const QString& how,
                                                     const QList< int >& index,
                                                     int thresh,
                                                     QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mModel(model), mAxis(axis), mHow(how), mIndex(index), mThresh(thresh)
{
    setText(QObject::tr("drop nan"));  // cn:改变列数据为数值
}

void DACommandDataFrame_dropna::undo()
{
	load();
	// 说明删除了空行
	if (mModel) {
		if (mDropedCount != 0) {
			mModel->refresh();
		}
	}
}

bool DACommandDataFrame_dropna::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	std::size_t lenBegin       = dataframe().size();
	if (!pydf.dropna(dataframe(), mAxis, mHow, mIndex, mThresh)) {
		return false;
	}
	std::size_t lenEnd = dataframe().size();
	mDropedCount       = lenBegin - lenEnd;

	// 说明删除了空行
	if (mModel) {
		if (mDropedCount != 0) {
			mModel->refresh();
		}
	}
	return true;
}

int DACommandDataFrame_dropna::getDropedCount() const
{
	return mDropedCount;
}

///////////////////

DACommandDataFrame_fillna::DACommandDataFrame_fillna(const DAPyDataFrame& df,
                                                     DAPyDataFrameTableModule* model,
                                                     int filltype,
                                                     float value,
                                                     const QString& method,
                                                     QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mModel(model), mFilltype(filltype), mValue(value), mMethod(method)
{
    setText(QObject::tr("fill nan"));  // cn:填充缺失值
}

void DACommandDataFrame_fillna::undo()
{
	load();
	// 说明填充了空行
	if (mModel) {
		mModel->refresh();
	}
}

bool DACommandDataFrame_fillna::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();

	if (!pydf.fillna(dataframe(), mFilltype, mValue, mMethod)) {
		return false;
	}

	// 说明填充了空行
	if (mModel) {
		mModel->refresh();
	}
	return true;
}

///////////////////

DACommandDataFrame_castNum::DACommandDataFrame_castNum(const DAPyDataFrame& df,
                                                       const QList< int >& index,
                                                       const pybind11::dict& args,
                                                       DAPyDataFrameTableModule* model,
                                                       QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIndex(index), mArgs(args), mModel(model)
{
	setText(QObject::tr("cast column to num"));  // cn:改变列数据为数值
}

void DACommandDataFrame_castNum::undo()
{
	load();
	if (mModel) {
		for (int c : qAsConst(mIndex)) {
			mModel->refreshColumn(c);
		}
	}
}

bool DACommandDataFrame_castNum::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	if (!pydf.cast_to_num(mDataframe, mIndex, mArgs)) {
		return false;
	}
	if (mModel) {
		for (int c : mIndex) {
			mModel->refreshColumn(c);
		}
	}
	return true;
}

///////////////////////////////

DACommandDataFrame_castDatetime::DACommandDataFrame_castDatetime(const DAPyDataFrame& df,
                                                                 const QList< int >& index,
                                                                 const pybind11::dict& args,
                                                                 DAPyDataFrameTableModule* model,
                                                                 QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIndex(index), mArgs(args), mModel(model)
{
	setText(QObject::tr("cast column to datetime"));  // cn:改变列数据为日期
}

void DACommandDataFrame_castDatetime::undo()
{
	load();
	if (mModel) {
		for (int c : qAsConst(mIndex)) {
			mModel->refreshColumn(c);
		}
	}
}

bool DACommandDataFrame_castDatetime::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	if (!pydf.cast_to_datetime(dataframe(), mIndex, mArgs)) {
		return false;
	}
	if (mModel) {
		for (int c : qAsConst(mIndex)) {
			mModel->refreshColumn(c);
		}
	}
	return true;
}

////////////////////////

DACommandDataFrame_setIndex::DACommandDataFrame_setIndex(const DAPyDataFrame& df,
                                                         const QList< int >& index,
                                                         QHeaderView* hv,
                                                         DAPyDataFrameTableModule* model,
                                                         QUndoCommand* par)
    : DACommandWithTemplateData(df, par), mIndex(index), mModel(model)
{
	setText(QObject::tr("set column to index"));  // cn:转换列为索引
}

void DACommandDataFrame_setIndex::undo()
{
	load();
	if (mModel) {
		mModel->refresh();
	}
}

bool DACommandDataFrame_setIndex::exec()
{
	DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
	if (!pydf.set_index(dataframe(), mIndex)) {
		return false;
	}
	if (mModel) {
		// 此操作会删除一列，添加一列，整个modelreflash
		mModel->refresh();
	}
	return true;
}

}  // end DA
