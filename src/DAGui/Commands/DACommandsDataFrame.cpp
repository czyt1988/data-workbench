#include "DACommandsDataFrame.h"
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
                                               QUndoCommand* par)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mRow(row), mCol(col), mOldData(olddata), mNewData(newdata)
{
    setText(QObject::tr("set dataframe data"));  // cn:改变单元格数据
}

void DACommandDataFrame_iat::undo()
{

    mDataframe.iat(mRow, mCol, mOldData);
    callback();
}

bool DACommandDataFrame_iat::exec()
{
    if (!mDataframe.iat(mRow, mCol, mNewData)) {
        return false;
    }
    callback();
    return true;
}

///////////////////////////////

DACommandDataFrame_insertNanRow::DACommandDataFrame_insertNanRow(const DAPyDataFrame& df, int row, QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mRow(row)
{
    setText(QObject::tr("insert row"));  // cn:插入一行
}

void DACommandDataFrame_insertNanRow::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_insertNanRow::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.insert_nanrow(dataframe(), mRow)) {
        return false;
    }
    callback();
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
                                                                 QUndoCommand* par)
    : DACommandWithTemporaryData(df, par)
    , DACallBackInterface()
    , mIsRangeMode(false)
    , mCol(col)
    , mName(name)
    , mDefaultvalue(defaultvalue)
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
                                                                 QUndoCommand* par)
    : DACommandWithTemporaryData(df, par)
    , DACallBackInterface()
    , mIsRangeMode(true)
    , mCol(col)
    , mName(name)
    , mStart(start)
    , mStop(stop)
{
    setText(QObject::tr("insert column \"%1\"").arg(name));  // cn: 插入列“%1”
}

void DACommandDataFrame_insertColumn::undo()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    mInsertedSeries            = pydf.itake_column(dataframe(), mCol);
    callback();
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

    callback();
    return true;
}

////////////////////////////////

/**
 * @brief 删除行
 * @param df
 * @param index
 * @param par
 */
DACommandDataFrame_dropIRow::DACommandDataFrame_dropIRow(const DAPyDataFrame& df, const QList< int >& index, QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index)
{
    setText(QObject::tr("drop dataframe rows"));  // cn:移除dataframe行
}

void DACommandDataFrame_dropIRow::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_dropIRow::exec()
{
    DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
    if (!py.drop_irow(mDataframe, mIndex)) {
        return false;
    }
    callback();
    return true;
}

////////////////////

DACommandDataFrame_dropIColumn::DACommandDataFrame_dropIColumn(const DAPyDataFrame& df,
                                                               const QList< int >& index,
                                                               QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index)
{
    setText(QObject::tr("drop dataframe columns"));  // cn:移除dataframe列
}

void DACommandDataFrame_dropIColumn::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_dropIColumn::exec()
{
    DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
    if (!py.drop_icolumn(mDataframe, mIndex)) {
        return false;
    }
    callback();
    return true;
}

////////////////////

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                                                   const QList< QString >& cols,
                                                                   QHeaderView* hv,
                                                                   QUndoCommand* par)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mCols(cols), mHeaderView(hv)
{
    mOldcols = df.columns();
}

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                                                   const QList< QString >& cols,
                                                                   const QList< QString >& oldcols,
                                                                   QHeaderView* hv,
                                                                   QUndoCommand* par)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mCols(cols), mHeaderView(hv), mOldcols(oldcols)
{
}

void DACommandDataFrame_renameColumns::undo()
{
    mDataframe.columns(mOldcols);
    if (mHeaderView) {
        mHeaderView->headerDataChanged(Qt::Horizontal, 0, mCols.size() - 1);
    }
    callback();
}

bool DACommandDataFrame_renameColumns::exec()
{
    if (!mDataframe.columns(mCols)) {
        return false;
    }
    if (mHeaderView) {
        mHeaderView->headerDataChanged(Qt::Horizontal, 0, mCols.size() - 1);
    }
    callback();
    return true;
}

////////////////////////////

DACommandDataFrame_astype::DACommandDataFrame_astype(const DAPyDataFrame& df,
                                                     const QList< int >& index,
                                                     const DAPyDType& dt,
                                                     QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index), mDtype(dt)
{
    setText(QObject::tr("change column type"));  // cn:改变列数据类型
}

void DACommandDataFrame_astype::undo()
{
    load();
    callback();
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
    callback();
    return true;
}

///////////////////////////

DACommandDataFrame_setnan::DACommandDataFrame_setnan(const DAPyDataFrame& df,
                                                     const QList< int >& rows,
                                                     const QList< int >& columns,
                                                     QUndoCommand* par)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mRows(rows), mColumns(columns)
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
    }
    callback();
}

bool DACommandDataFrame_setnan::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.setnan(mDataframe, mRows, mColumns)) {
        return false;
    }
    callback();
    return true;
}

///////////////////

//----------------------------------------------------
//
//----------------------------------------------------
DACommandDataFrame_evalDatas::DACommandDataFrame_evalDatas(const DAPyDataFrame& df, const QString& exper, QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mExper(exper)
{
    setText(QObject::tr("eval datas"));  // cn:列运算
}

void DACommandDataFrame_evalDatas::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_evalDatas::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.evalDatas(dataframe(), mExper)) {
        return false;
    }
    callback();
    return true;
}
//----------------------------------------------------
//
//----------------------------------------------------
///////////////////

DACommandDataFrame_querydatas::DACommandDataFrame_querydatas(const DAPyDataFrame& df, const QString& exper, QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mExper(exper)
{
    setText(QObject::tr("query datas"));  // cn:条件查询
}

void DACommandDataFrame_querydatas::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_querydatas::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.queryDatas(dataframe(), mExper)) {
        return false;
    }
    callback();
    return true;
}

///////////////////

DACommandDataFrame_filterByColumn::DACommandDataFrame_filterByColumn(const DAPyDataFrame& df,
                                                                     double lowervalue,
                                                                     double uppervalue,
                                                                     const QString& index,
                                                                     QUndoCommand* par)
    : DACommandWithTemporaryData(df, par)
    , DACallBackInterface()
    , mlowervalue(lowervalue)
    , mUppervalue(uppervalue)
    , mIndex(index)
{
    setText(QObject::tr("data select"));  // cn:数据过滤
}

void DACommandDataFrame_filterByColumn::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_filterByColumn::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.dataselect(dataframe(), mlowervalue, mUppervalue, mIndex)) {
        return false;
    }
    callback();
    return true;
}

///////////////////

DACommandDataFrame_sort::DACommandDataFrame_sort(const DAPyDataFrame& df,
                                                 const QString& by,
                                                 const bool ascending,
                                                 QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mBy(by), mAscending(ascending)
{
    setText(QObject::tr("sort datas"));  // cn:对相关数据进行排序
}

void DACommandDataFrame_sort::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_sort::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.sort(dataframe(), mBy, mAscending)) {
        return false;
    }
    callback();
    return true;
}

///////////////////

DACommandDataFrame_castNum::DACommandDataFrame_castNum(const DAPyDataFrame& df,
                                                       const QList< int >& index,
                                                       const pybind11::dict& args,
                                                       QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index), mArgs(args)
{
    setText(QObject::tr("cast column to num"));  // cn:改变列数据为数值
}

void DACommandDataFrame_castNum::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_castNum::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.cast_to_num(mDataframe, mIndex, mArgs)) {
        return false;
    }
    callback();
    return true;
}

///////////////////////////////

DACommandDataFrame_castDatetime::DACommandDataFrame_castDatetime(const DAPyDataFrame& df,
                                                                 const QList< int >& index,
                                                                 const pybind11::dict& args,
                                                                 QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index), mArgs(args)
{
    setText(QObject::tr("cast column to datetime"));  // cn:改变列数据为日期
}

void DACommandDataFrame_castDatetime::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_castDatetime::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.cast_to_datetime(dataframe(), mIndex, mArgs)) {
        return false;
    }
    callback();
    return true;
}

////////////////////////

DACommandDataFrame_setIndex::DACommandDataFrame_setIndex(const DAPyDataFrame& df, const QList< int >& index, QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index)
{
    setText(QObject::tr("set column to index"));  // cn:转换列为索引
}

void DACommandDataFrame_setIndex::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_setIndex::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.set_index(dataframe(), mIndex)) {
        return false;
    }
    callback();
    return true;
}

}  // end DA
