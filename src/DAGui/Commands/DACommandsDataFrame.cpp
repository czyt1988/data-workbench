#include "DACommandsDataFrame.h"
#include "DAPyScripts.h"
#include "DASlowLog.h"
#include "DATableKeyRemap.h"
#include "Models/DADataTableModel.h"
#include <algorithm>
#include <climits>

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DACommandDataFrame_iat
//===================================================
DACommandDataFrame_iat::DACommandDataFrame_iat(
    const DAPyDataFrame& df, int row, int col, const QVariant& olddata, const QVariant& newdata, QUndoCommand* par
)
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
    directionalCallback(true);
}

bool DACommandDataFrame_insertNanRow::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.insert_nanrow(dataframe(), mRow)) {
        return false;
    }
    callback();
    directionalCallback(false);
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
DACommandDataFrame_insertColumn::DACommandDataFrame_insertColumn(
    const DAPyDataFrame& df, int col, const QString& name, const QVariant& defaultvalue, QUndoCommand* par
)
    : DACommandWithTemporaryData(df, par)
    , DACallBackInterface()
    , mIsRangeMode(false)
    , mCol(col)
    , mName(name)
    , mDefaultvalue(defaultvalue)
{
    setText(QObject::tr("insert column \"%1\"").arg(name));  // cn:插入列“%1”
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
DACommandDataFrame_insertColumn::DACommandDataFrame_insertColumn(
    const DAPyDataFrame& df, int col, const QString& name, const QVariant& start, const QVariant& stop, QUndoCommand* par
)
    : DACommandWithTemporaryData(df, par)
    , DACallBackInterface()
    , mIsRangeMode(true)
    , mCol(col)
    , mName(name)
    , mStart(start)
    , mStop(stop)
{
    setText(QObject::tr("insert column \"%1\"").arg(name));  // cn:插入列“%1”
}

void DACommandDataFrame_insertColumn::undo()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    mInsertedSeries            = pydf.itake_column(dataframe(), mCol);
    callback();
    directionalCallback(true);
}

bool DACommandDataFrame_insertColumn::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
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
    directionalCallback(false);
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
    directionalCallback(true);
}

bool DACommandDataFrame_dropIRow::exec()
{
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    if (!py.drop_irow(mDataframe, mIndex)) {
        return false;
    }
    callback();
    directionalCallback(false);
    return true;
}

////////////////////

DACommandDataFrame_dropIColumn::DACommandDataFrame_dropIColumn(const DAPyDataFrame& df, const QList< int >& index, QUndoCommand* par)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index)
{
    setText(QObject::tr("drop dataframe columns"));  // cn:移除dataframe列
}

void DACommandDataFrame_dropIColumn::undo()
{
    load();
    callback();
    directionalCallback(true);
}

bool DACommandDataFrame_dropIColumn::exec()
{
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    if (!py.drop_icolumn(mDataframe, mIndex)) {
        return false;
    }
    callback();
    directionalCallback(false);
    return true;
}

////////////////////

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(
    const DAPyDataFrame& df, const QList< QString >& cols, DADataTableModel* model, QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mCols(cols), mModel(model)
{
    // 005 T1.21 红线5：撤销面板不允许空白条目
    setText(QObject::tr("rename columns"));  // cn:重命名列
    mOldcols = df.columns();
}

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(
    const DAPyDataFrame& df, const QList< QString >& cols, const QList< QString >& oldcols, DADataTableModel* model, QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mCols(cols), mModel(model), mOldcols(oldcols)
{
    // 005 T1.21 红线5：撤销面板不允许空白条目
    setText(QObject::tr("rename columns"));  // cn:重命名列
}

void DACommandDataFrame_renameColumns::undo()
{
    mDataframe.columns(mOldcols);
    if (mModel) {
        mModel->notifyHeaderDataChanged(Qt::Horizontal, 0, mCols.size() - 1);
    }
    callback();
    directionalCallback(true);  // 004 T1.23：列名引用的计算列定义随撤销反向迁移
}

bool DACommandDataFrame_renameColumns::exec()
{
    if (!mDataframe.columns(mCols)) {
        return false;
    }
    if (mModel) {
        mModel->notifyHeaderDataChanged(Qt::Horizontal, 0, mCols.size() - 1);
    }
    callback();
    directionalCallback(false);  // 004 T1.23：列名引用的计算列定义随改名迁移
    return true;
}

////////////////////////////

DACommandDataFrame_astype::DACommandDataFrame_astype(
    const DAPyDataFrame& df, const QList< int >& index, const DAPyDType& dt, QUndoCommand* par
)
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
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.astype(mDataframe, mIndex, mDtype)) {
        return false;
    }
    callback();
    return true;
}

///////////////////////////

DACommandDataFrame_setnan::DACommandDataFrame_setnan(
    const DAPyDataFrame& df, const QList< int >& rows, const QList< int >& columns, QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mRows(rows), mColumns(columns)
{
    // 005 T1.21 红线5：撤销面板不允许空白条目
    setText(QObject::tr("clear cells"));  // cn:清除单元格
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
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.setnan(mDataframe, mRows, mColumns)) {
        return false;
    }
    callback();
    return true;
}

///////////////////

DACommandDataFrame_paste::DACommandDataFrame_paste(
    const DAPyDataFrame& df, const QList< int >& rows, const QList< int >& columns, const QList< QVariant >& newdatas, QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mRows(rows), mColumns(columns), mNewdatas(newdatas)
{
    // 构造时提取旧值，供 undo 写回
    for (int i = 0; i < mRows.size(); ++i) {
        mOlddatas.append(df.iatObj(rows[ i ], columns[ i ]));
    }
    setText(QObject::tr("paste data"));  // cn:粘贴数据
}

void DACommandDataFrame_paste::undo()
{
    for (int i = 0; i < mRows.size(); ++i) {
        mDataframe.iat(mRows[ i ], mColumns[ i ], mOlddatas[ i ]);
    }
    callback();
}

bool DACommandDataFrame_paste::exec()
{
    // 逐格写入，任一格失败即返回 false（QUndoStack 会触发 undo 回滚）
    for (int i = 0; i < mRows.size(); ++i) {
        if (!mDataframe.iat(mRows[ i ], mColumns[ i ], mNewdatas[ i ])) {
            return false;
        }
    }
    callback();
    return true;
}

///////////////////

//----------------------------------------------------
// DACommandDataFrame_evalColumn（004 T1.23 计算列，列级记录撤销）
// 注：旧 DACommandDataFrame_evalDatas（DACommandWithTemporaryData 整表快照基类、
// 无任何调用方的死代码）已按 004 计划删除——D4 停用快照范式，新命令走列级记录
//----------------------------------------------------

/**
 * @brief 构造（首次 exec 时才求值并捕获旧列快照）
 * @param df 数据
 * @param targetColumn 目标列名
 * @param body 表达式体（RHS）
 * @param par 父命令
 */
DACommandDataFrame_evalColumn::DACommandDataFrame_evalColumn(const DAPyDataFrame& df, const QString& targetColumn, const QString& body, QUndoCommand* par)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mTargetColumn(targetColumn), mBody(body)
{
    setText(QObject::tr("compute column \"%1\"").arg(targetColumn));  // cn:计算列“%1”
}

QString DACommandDataFrame_evalColumn::lastErrorMessage() const
{
    return mLastError;
}

/**
 * @brief 撤销：hadColumn 时按列名还原旧列快照，否则删除新建的目标列
 *
 * 列位置在 undo 时按列名解析（R20 列名引用；undo 前其上叠加的命令均已回退，
 * 表结构即本命令 exec 后的状态，按名定位稳定）。
 */
void DACommandDataFrame_evalColumn::undo()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (mHadColumn) {
        pydf.setColumn(mDataframe, mTargetColumn, mOldColumn);
    } else {
        const QList< QString > cols = mDataframe.columns();
        const int pos               = cols.indexOf(mTargetColumn);
        if (pos >= 0) {
            pydf.drop_icolumn(mDataframe, { pos });
        }
    }
    callback();
    directionalCallback(true);
}

/**
 * @brief 执行（含首次与 redo）：求值表达式写入目标列；首次成功时捕获旧列快照
 * @return 求值成功返回 true
 */
bool DACommandDataFrame_evalColumn::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    const DAPyScriptsDataFrame::EvalColumnResult r = pydf.evalColumn(mDataframe, mTargetColumn, mBody);
    if (!r.ok) {
        mLastError = r.errorMessage;
        return false;
    }
    if (!mSnapshotTaken) {
        mHadColumn     = r.hadColumn;
        mOldColumn     = r.oldColumn;
        mSnapshotTaken = true;
    }
    callback();
    directionalCallback(false);
    return true;
}

//----------------------------------------------------
// DACommandComputedColumn_removeDefinition（004 T1.23 移除公式保留数据）
//----------------------------------------------------

/**
 * @brief 构造：从 store 捕获整条目（定义+状态）
 * @param store 计算列子存储（会话级，QPointer 防悬挂）
 * @param target 目标列名
 * @param par 父命令
 */
DACommandComputedColumn_removeDefinition::DACommandComputedColumn_removeDefinition(
    DATableComputedColumnStore* store, const QString& target, QUndoCommand* par
)
    : QUndoCommand(par), DACallBackInterface(), mStore(store)
{
    if (mStore) {
        mCaptured = mStore->entryOf(target, mEntry);
    }
    setText(QObject::tr("remove formula of column \"%1\" (keep data)").arg(target));  // cn:移除列“%1”的公式（保留数据）
}

void DACommandComputedColumn_removeDefinition::undo()
{
    if (mStore && mCaptured) {
        mStore->addDefinition(mEntry.def, mEntry.state);
    }
    callback();
}

void DACommandComputedColumn_removeDefinition::redo()
{
    if (mStore && mCaptured) {
        mStore->removeDefinition(mEntry.def.targetColumnName);
    }
    callback();
}

bool DACommandComputedColumn_removeDefinition::isCaptured() const
{
    return mCaptured;
}


///////////////////

DACommandDataFrame_castNum::DACommandDataFrame_castNum(
    const DAPyDataFrame& df, const QList< int >& index, const pybind11::dict& args, QUndoCommand* par
)
    : DACommandWithTemporaryData(df, par), DACallBackInterface(), mIndex(index), mArgs(args)
{
    setText(QObject::tr("cast column to number"));  // cn:转换列数据为数值
}

void DACommandDataFrame_castNum::undo()
{
    load();
    callback();
}

bool DACommandDataFrame_castNum::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.cast_to_num(mDataframe, mIndex, mArgs)) {
        return false;
    }
    callback();
    return true;
}

///////////////////////////////

DACommandDataFrame_castDatetime::DACommandDataFrame_castDatetime(
    const DAPyDataFrame& df, const QList< int >& index, const pybind11::dict& args, QUndoCommand* par
)
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
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
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
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.set_index(dataframe(), mIndex)) {
        return false;
    }
    callback();
    return true;
}

////////////////////////

/**
 * @brief 表头排序命令（破坏式重排 + 置换撤销，D1/R13）
 *
 * 首次 exec() 调多键排序算子计算置换并持有；exec()（含 redo）= 正置换重排，
 * undo() = 逆置换重排。全程 O(n) 索引操作、无数据复制、无快照。
 * @param df 数据
 * @param keys 排序键（列位置+是否升序），顺序即优先级
 * @param par
 */
DACommandDataFrame_sortRows::DACommandDataFrame_sortRows(
    const DAPyDataFrame& df, const QVector< QPair< int, bool > >& keys, QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mKeys(keys)
{
    setText(QObject::tr("Sort rows"));  // cn:排序
}

void DACommandDataFrame_sortRows::undo()
{
    DA_SLOW_LOG_SCOPE("dagui.command.sortRows.undo");
    if (mPerm.isEmpty()) {
        return;  // exec 未成功过，无置换可撤
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.reorderRows(mDataframe, mInvPerm)) {
        return;
    }
    mLastApplied = mInvPerm;
    // 顺序（设计稿 §4.1）：先样式键重映射（directionalCallback 里 onRowsReordered），
    // 再模型刷新/选区/视口跟随（callback 里，由调用方接线）
    directionalCallback(true);
    callback();
}

bool DACommandDataFrame_sortRows::exec()
{
    DA_SLOW_LOG_SCOPE("dagui.command.sortRows.exec");
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (mPerm.isEmpty()) {
        // 首次：算子计算置换并就地重排（多键稳定排序，NaN/NaT 恒排最后）
        QVector< int > perm;
        if (!pydf.sort(mDataframe, mKeys, &perm)) {
            return false;  // 混合类型不可比较 / 列越界等，算子侧已记录异常
        }
        if (perm.isEmpty()) {
            return false;  // 空表无可排（调用方应已拦截，防御性处理）
        }
        mPerm    = perm;
        mInvPerm = DATableKeyRemap::inversePermutation(mPerm);
    } else if (!pydf.reorderRows(mDataframe, mPerm)) {
        return false;
    }
    mLastApplied = mPerm;
    directionalCallback(false);
    callback();
    return true;
}

const QVector< int >& DACommandDataFrame_sortRows::permutation() const
{
    return mPerm;
}

const QVector< int >& DACommandDataFrame_sortRows::lastAppliedPermutation() const
{
    return mLastApplied;
}

////////////////////////

/**
 * @brief 查找替换命令（值记录撤销，D4：全部替换 = 一个撤销单元）
 *
 * 数据在构造前已由调用方经 findReplace 就地替换完成，本命令只持有变更清单：
 * 首次 push 的 redo 被 DACommandWithRedoCount 跳过（不重复替换），此后
 * redo=写回新值、undo=写回旧值（da_write_cells 按列分组批量写回，dtype 保持）。
 * @param df 数据
 * @param rows 变更行（数据源绝对行号，与 cols/oldValues/newValues 一一对应）
 * @param cols 变更列
 * @param oldValues 旧值 Python list（undo 写回）
 * @param newValues 新值 Python list（redo 写回）
 * @param par
 */
DACommandDataFrame_replace::DACommandDataFrame_replace(
    const DAPyDataFrame& df,
    const QVector< int >& rows,
    const QVector< int >& cols,
    const pybind11::list& oldValues,
    const pybind11::list& newValues,
    QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mRows(rows), mCols(cols), mOldValues(oldValues), mNewValues(newValues)
{
    setText(QObject::tr("Replace all"));  // cn:全部替换
}

void DACommandDataFrame_replace::undo()
{
    if (mRows.isEmpty()) {
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.writeCells(mDataframe, mRows, mCols, mOldValues)) {
        return;
    }
    callback();
}

bool DACommandDataFrame_replace::exec()
{
    if (mRows.isEmpty()) {
        return false;  // 无变更不入栈（调用方已拦截，防御性处理）
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    if (!pydf.writeCells(mDataframe, mRows, mCols, mNewValues)) {
        return false;
    }
    callback();
    return true;
}

int DACommandDataFrame_replace::changeCount() const
{
    return mRows.size();
}

const QVector< int >& DACommandDataFrame_replace::changedRows() const
{
    return mRows;
}

const QVector< int >& DACommandDataFrame_replace::changedCols() const
{
    return mCols;
}

//----------------------------------------------------
// DACommandDataFrame_splitColumn（004 T1.9 分列，列级记录撤销）
//----------------------------------------------------

/**
 * @brief 构造
 * @param df 数据
 * @param params 分列参数（与对话框预览同一份）
 * @param oldSourceColumn 源列快照（调用方构造前经 getColumnCopy 提取）
 * @param par 父命令
 */
DACommandDataFrame_splitColumn::DACommandDataFrame_splitColumn(
    const DAPyDataFrame& df, const SplitParams& params, const DAPySeries& oldSourceColumn, QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mParams(params), mOldSource(oldSourceColumn)
{
    const QString srcName = df.columns().value(params.col);
    setText(QObject::tr("split column \"%1\"").arg(srcName));  // cn:分列“%1”
}

QStringList DACommandDataFrame_splitColumn::insertedNames() const
{
    return mInsertedNames;
}

QString DACommandDataFrame_splitColumn::lastErrorMessage() const
{
    return mLastError;
}

/**
 * @brief 撤销：删除结果列（按名解析、逐个删）+ 源列快照写回原位置
 *
 * 防御性还原：源列在拆分中未被改动，但按计划（§4.3-4）仍写回快照，
 * 保证列顺序/列数/内容完全还原（含 dtype）。
 */
void DACommandDataFrame_splitColumn::undo()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    for (const QString& n : std::as_const(mInsertedNames)) {
        const QList< QString > cur = mDataframe.columns();
        const int pos              = cur.indexOf(n);
        if (pos >= 0) {
            pydf.drop_icolumn(mDataframe, { pos });
        }
    }
    if (!mOldSource.isNone() && mParams.col >= 0 && mParams.col < mDataframe.columns().size()) {
        pydf.drop_icolumn(mDataframe, { mParams.col });
        pydf.insert_at(mDataframe, mParams.col, mOldSource);
    }
    callback();
    directionalCallback(true);
}

/**
 * @brief 执行（含 redo）：重放拆分；首次成功记录结果列名清单
 */
bool DACommandDataFrame_splitColumn::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    const QStringList names    = pydf.splitColumn(mDataframe,
                                               mParams.col,
                                               mParams.mode,
                                               mParams.delimiter,
                                               mParams.pattern,
                                               mParams.widths,
                                               mParams.maxColumns,
                                               mParams.overflow,
                                               mParams.inferNumeric,
                                               mParams.newNames,
                                               &mLastError);
    if (names.isEmpty()) {
        return false;
    }
    if (mInsertedNames.isEmpty()) {
        mInsertedNames = names;
    }
    callback();
    directionalCallback(false);
    return true;
}

//----------------------------------------------------
// DACommandDataFrame_mergeColumns（004 T1.9 合并列，列级记录撤销）
//----------------------------------------------------

/**
 * @brief 构造
 * @param df 数据
 * @param params 合并参数（cols 顺序=拼接顺序）
 * @param oldSourceColumns 源列快照（与 params.cols 一一对应）
 * @param par 父命令
 */
DACommandDataFrame_mergeColumns::DACommandDataFrame_mergeColumns(
    const DAPyDataFrame& df, const MergeParams& params, const QList< DAPySeries >& oldSourceColumns, QUndoCommand* par
)
    : DACommandWithRedoCount(par), DACallBackInterface(), mDataframe(df), mParams(params), mOldSources(oldSourceColumns)
{
    setText(QObject::tr("merge %1 columns into \"%2\"").arg(params.cols.size()).arg(params.name));  // cn:合并 %1 列为“%2”
}

int DACommandDataFrame_mergeColumns::insertPosition() const
{
    int pos = INT_MAX;
    for (int c : mParams.cols) {
        pos = qMin(pos, c);
    }
    return (pos == INT_MAX) ? -1 : pos;
}

QString DACommandDataFrame_mergeColumns::lastErrorMessage() const
{
    return mLastError;
}

/**
 * @brief 撤销：删除结果列 + 按原位置升序写回全部源列快照
 */
void DACommandDataFrame_mergeColumns::undo()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    // 删结果列（按名解析位置）
    const QList< QString > cur = mDataframe.columns();
    const int mergedPos        = cur.indexOf(mParams.name);
    if (mergedPos >= 0) {
        pydf.drop_icolumn(mDataframe, { mergedPos });
    }
    // 源列按位置升序回插（升序保证各快照落回原位置）
    QList< int > order;
    order.reserve(mParams.cols.size());
    for (int i = 0; i < mParams.cols.size(); ++i) {
        order.append(i);
    }
    std::sort(order.begin(), order.end(), [ this ](int a, int b) {
        return mParams.cols.at(a) < mParams.cols.at(b);
    });
    for (int i : std::as_const(order)) {
        if (i < mOldSources.size() && !mOldSources.at(i).isNone()) {
            pydf.insert_at(mDataframe, mParams.cols.at(i), mOldSources.at(i));
        }
    }
    callback();
    directionalCallback(true);
}

/**
 * @brief 执行（含 redo）：重放合并（源列须在位——undo 已还原）
 */
bool DACommandDataFrame_mergeColumns::exec()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getDataFrame();
    const int pos              = pydf.mergeColumns(mDataframe, mParams.cols, mParams.sep, mParams.skipNa, mParams.name, &mLastError);
    if (pos < 0) {
        return false;
    }
    callback();
    directionalCallback(false);
    return true;
}

}  // end DA
