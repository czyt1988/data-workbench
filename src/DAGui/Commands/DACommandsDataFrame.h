#ifndef DACOMMANDSDATAFRAME_H
#define DACOMMANDSDATAFRAME_H
#include <QUndoCommand>
#include <QPoint>
#include <QPair>
#include <QVector>
#include <optional>
#include <functional>
#include "DAGuiAPI.h"
#include "DACallBackInterface.h"
#include "DACommandWithRedoCount.h"
#include "DACommandWithTemporaryData.h"
#include "DAData.h"
#include "numpy/DAPyDType.h"
#include "pandas/DAPySeries.h"

namespace DA
{

class DAPyDataFrameTableModel;
class DADataTableModel;
/**
 * @file DataManager相关的命令
 *
 */

/**
 * @brief 添加变量命令
 *
 * redo会调用iat设置值，但是，如果值没有设置成功，将会把m_isSuccess设置为false，这时调用undo不做任何处理
 *
 * 因此在对一些场合，需要判断是否设置成功的地方，可以按照如下方式进行操作
 * @code
 * bool DAPyDataFrameTableModule::setData(const QModelIndex& index, const QVariant& value, int role)
 * {
 *     if (Qt::EditRole != role) {
 *         return false;
 *     }
 *     if (!index.isValid() || d_ptr->_dataframe.isNone()) {
 *         return false;
 *     }
 *     std::pair< std::size_t, std::size_t > shape = d_ptr->_dataframe.shape();
 *     if (index.row() >= (int)shape.first || index.column() >= (int)shape.second) {
 *         return false;
 *     }
 *     QVariant olddata                = d_ptr->_dataframe.iat(index.row(), index.column());
 *     DACommandDataFrame_iat* cmd_iat = new DACommandDataFrame_iat(d_ptr->_dataframe, index.row(), index.column(),
 * olddata, value); d_ptr->_undoStack->push(cmd_iat); if (!cmd_iat->isSetSuccess()) {
 *         //没设置成功，undo回退一步
 *         d_ptr->_undoStack->undo();
 *         return false;
 *     }
 *     //这里说明设置成功了
 *     return true;
 * }
 * @endcode
 */
class DAGUI_API DACommandDataFrame_iat : public DACommandWithRedoCount, public DACallBackInterface
{
public:
    DACommandDataFrame_iat(
        const DAPyDataFrame& df, int row, int col, const QVariant& olddata, const QVariant& newdata, QUndoCommand* par = nullptr
    );
    virtual void undo() override;
    virtual bool exec() override;

private:
    DAPyDataFrame mDataframe;
    int mRow;
    int mCol;
    QVariant mOldData;
    QVariant mNewData;
};

/**
 * @brief 插入一个空行
 */
class DAGUI_API DACommandDataFrame_insertNanRow : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_insertNanRow(const DAPyDataFrame& df, int row, QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    int mRow;
};

/**
 * @brief 插入列
 */
class DAGUI_API DACommandDataFrame_insertColumn : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_insertColumn(
        const DAPyDataFrame& df, int col, const QString& name, const QVariant& defaultvalue, QUndoCommand* par = nullptr
    );
    DACommandDataFrame_insertColumn(
        const DAPyDataFrame& df, int col, const QString& name, const QVariant& start, const QVariant& stop, QUndoCommand* par = nullptr
    );
    virtual void undo() override;
    virtual bool exec() override;

private:
    bool mIsRangeMode;  ///< 标记是否是一个范围生成
    int mCol;           ///< 插入的列
    QString mName;
    QVariant mDefaultvalue;
    QVariant mStart;
    QVariant mStop;
    DAPySeries mInsertedSeries;
};

/**
 * @brief 删除行
 */
class DAGUI_API DACommandDataFrame_dropIRow : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_dropIRow(const DAPyDataFrame& df, const QList< int >& index, QUndoCommand* par = nullptr);

    virtual void undo() override;
    virtual bool exec() override;

private:
    QList< int > mIndex;
};

/**
 * @brief 删除列
 */
class DAGUI_API DACommandDataFrame_dropIColumn : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_dropIColumn(const DAPyDataFrame& df, const QList< int >& index, QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    QList< int > mIndex;
};

/**
 * @brief 更改列名
 *
 * 表头刷新经模型的 notifyHeaderDataChanged 通知入口（exec/undo 后调用），
 * 不再持有 QHeaderView*（解除表头替换的外部耦合，R12）。
 */
class DAGUI_API DACommandDataFrame_renameColumns : public DACommandWithRedoCount, public DACallBackInterface
{
public:
    DACommandDataFrame_renameColumns(
        const DAPyDataFrame& df, const QList< QString >& cols, DADataTableModel* model = nullptr, QUndoCommand* par = nullptr
    );
    DACommandDataFrame_renameColumns(
        const DAPyDataFrame& df,
        const QList< QString >& cols,
        const QList< QString >& oldcols,
        DADataTableModel* model = nullptr,
        QUndoCommand* par       = nullptr
    );
    virtual void undo() override;
    virtual bool exec() override;

private:
    DAPyDataFrame mDataframe;
    QList< QString > mCols;
    DADataTableModel* mModel;  ///< 非拥有：exec/undo 后发 headerDataChanged 通知（替代旧 QHeaderView* 持有）
    QList< QString > mOldcols;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_astype : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_astype(const DAPyDataFrame& df, const QList< int >& index, const DAPyDType& dt, QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    QList< int > mIndex;
    DAPyDType mDtype;
};

/**
 * @brief 设置为nan
 */
class DAGUI_API DACommandDataFrame_setnan : public DACommandWithRedoCount, public DACallBackInterface
{
public:
    DACommandDataFrame_setnan(
        const DAPyDataFrame& df, const QList< int >& rows, const QList< int >& columns, QUndoCommand* par = nullptr
    );
    virtual void undo() override;
    virtual bool exec() override;

private:
    DAPyDataFrame mDataframe;
    QList< int > mRows;
    QList< int > mColumns;
    QList< pybind11::object > mOlddatas;
};

/**
 * @brief 批量写入单元格（粘贴）
 *
 * 参照 DACommandDataFrame_setnan 的批量旧值模式：构造时提取旧值，
 * redo 批量 iat 写新值、undo 写回旧值。行列表 mRows/mColumns 与新值列表
 * mNewdatas 一一对应（同行同列的多次写入按顺序覆盖）
 */
class DAGUI_API DACommandDataFrame_paste : public DACommandWithRedoCount, public DACallBackInterface
{
public:
    DACommandDataFrame_paste(const DAPyDataFrame& df,
                             const QList< int >& rows,
                             const QList< int >& columns,
                             const QList< QVariant >& newdatas,
                             QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    DAPyDataFrame mDataframe;
    QList< int > mRows;
    QList< int > mColumns;
    QList< QVariant > mNewdatas;
    QList< pybind11::object > mOlddatas;
};

/**
 * @brief evaldatas
 */
class DAGUI_API DACommandDataFrame_evalDatas : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_evalDatas(const DAPyDataFrame& df, const QString& exper, QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    QString mExper;
};


/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castNum : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_castNum(
        const DAPyDataFrame& df, const QList< int >& index, const pybind11::dict& args, QUndoCommand* par = nullptr
    );
    virtual void undo() override;
    virtual bool exec() override;

private:
    QList< int > mIndex;
    pybind11::dict mArgs;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castDatetime : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_castDatetime(
        const DAPyDataFrame& df, const QList< int >& index, const pybind11::dict& args, QUndoCommand* par = nullptr
    );
    virtual void undo() override;
    virtual bool exec() override;

private:
    QList< int > mIndex;
    pybind11::dict mArgs;
};

/**
 * @brief 转换列为索引
 */
class DAGUI_API DACommandDataFrame_setIndex : public DACommandWithTemporaryData, public DACallBackInterface
{
public:
    DACommandDataFrame_setIndex(const DAPyDataFrame& df, const QList< int >& index, QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;

private:
    QList< int > mIndex;
};

/**
 * @brief 表头排序命令（破坏式重排 + 置换撤销，D1/R13，禁快照）
 *
 * 首次 exec() 调多键排序算子计算置换并持有；exec()（含 redo）= 正置换重排，
 * undo() = 逆置换重排，全程 O(n) 索引操作、无数据复制、无快照。
 * 置换语义与 DATableKeyRemap 一致：perm[i] = 新表第 i 行在旧表中的行号（新→旧），
 * 逆置换 inv[旧行号] = 新位置（旧→新）。
 *
 * 每次生效后的联动由调用方经回调完成（顺序：先样式键重映射、再模型刷新与选区/视口
 * 跟随）：directionalCallback(isUndo) 里用 lastAppliedPermutation() 调
 * DATableStyleManager::onRowsReordered；callback() 里做模型 notifyDataChanged、
 * 按逆置换修正选区、视口回顶（参照 paste 命令的回调范式）。
 */
class DAGUI_API DACommandDataFrame_sortRows : public DACommandWithRedoCount, public DACallBackInterface
{
public:
    DACommandDataFrame_sortRows(const DAPyDataFrame& df, const QVector< QPair< int, bool > >& keys, QUndoCommand* par = nullptr);
    virtual void undo() override;
    virtual bool exec() override;
    // 正置换（新→旧）；首次成功 exec 前为空
    const QVector< int >& permutation() const;
    // 最近一次生效（exec/undo）实际应用的置换：exec=perm、undo=inv（样式重映射直接用此参数）
    const QVector< int >& lastAppliedPermutation() const;

private:
    DAPyDataFrame mDataframe;
    QVector< QPair< int, bool > > mKeys;  ///< 排序键（列位置+是否升序），顺序即优先级
    QVector< int > mPerm;                 ///< 正置换（新→旧），首次 exec 时由算子计算
    QVector< int > mInvPerm;              ///< 逆置换（旧→新）
    QVector< int > mLastApplied;          ///< 最近一次应用的置换
};
}  // end of namespace DA
#endif  // DACOMMANDSDATAFRAME_H
