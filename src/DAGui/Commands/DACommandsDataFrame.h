#ifndef DACOMMANDSDATAFRAME_H
#define DACOMMANDSDATAFRAME_H
#include <QUndoCommand>
#include <QPoint>
#include "DAGuiAPI.h"
#include "DACommandWithRedoCount.h"
#include "DACommandWithTemplateData.h"
#include "DAData.h"
#include "numpy/DAPyDType.h"
#include "pandas/DAPySeries.h"
class QHeaderView;

namespace DA
{
class DAPyDataFrameTableModule;
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
class DAGUI_API DACommandDataFrame_iat : public DACommandWithRedoCount
{
public:
    DACommandDataFrame_iat(const DAPyDataFrame& df,
                           int row,
                           int col,
                           const QVariant& olddata,
                           const QVariant& newdata,
                           DAPyDataFrameTableModule* model = nullptr,
                           QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    DAPyDataFrame m_dataframe;
    int m_row;
    int m_col;
    QVariant m_oldData;
    QVariant m_newData;
    DAPyDataFrameTableModule* m_model;
};

/**
 * @brief 插入一个空行
 */
class DAGUI_API DACommandDataFrame_insertNanRow : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_insertNanRow(const DAPyDataFrame& df,
                                    int row,
                                    DAPyDataFrameTableModule* model = nullptr,
                                    QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    int m_row;
    DAPyDataFrameTableModule* m_model;
};

/**
 * @brief 插入列
 */
class DAGUI_API DACommandDataFrame_insertColumn : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
                                    int col,
                                    const QString& name,
                                    const QVariant& defaultvalue,
                                    DAPyDataFrameTableModule* model = nullptr,
                                    QUndoCommand* par               = nullptr);
    DACommandDataFrame_insertColumn(const DAPyDataFrame& df,
                                    int col,
                                    const QString& name,
                                    const QVariant& start,
                                    const QVariant& stop,
                                    DAPyDataFrameTableModule* model = nullptr,
                                    QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    bool m_isRangeMode;  ///< 标记是否是一个范围生成
    int m_col;           ///< 插入的列
    QString m_name;
    QVariant m_defaultvalue;
    QVariant m_start;
    QVariant m_stop;
    DAPyDataFrameTableModule* m_model;
    DAPySeries m_insertedSeries;
};

/**
 * @brief 删除行
 */
class DAGUI_API DACommandDataFrame_dropIRow : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_dropIRow(const DAPyDataFrame& df,
                                const QList< int >& index,
                                DAPyDataFrameTableModule* model = nullptr,
                                QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    QList< int > m_index;
    DAPyDataFrameTableModule* m_model;
};

/**
 * @brief 删除列
 */
class DAGUI_API DACommandDataFrame_dropIColumn : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_dropIColumn(const DAPyDataFrame& df,
                                   const QList< int >& index,
                                   DAPyDataFrameTableModule* model = nullptr,
                                   QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    QList< int > m_index;
    DAPyDataFrameTableModule* m_model;
};

/**
 * @brief 更改列名
 */
class DAGUI_API DACommandDataFrame_renameColumns : public QUndoCommand
{
public:
    DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                     const QList< QString >& cols,
                                     QHeaderView* hv   = nullptr,
                                     QUndoCommand* par = nullptr);
    DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                     const QList< QString >& cols,
                                     const QList< QString >& oldcols,
                                     QHeaderView* hv   = nullptr,
                                     QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

private:
    DAPyDataFrame m_dataframe;
    QList< QString > m_cols;
    QHeaderView* m_headerView;
    QList< QString > m_oldcols;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_astype : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_astype(const DAPyDataFrame& df,
                              const QList< int >& index,
                              const DAPyDType& dt,
                              DAPyDataFrameTableModule* model = nullptr,
                              QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    QList< int > m_index;
    DAPyDType m_dtype;
    DAPyDataFrameTableModule* m_model;
};

/**
 * @brief 设置为nan
 */
class DAGUI_API DACommandDataFrame_setnan : public DACommandWithRedoCount
{
public:
    DACommandDataFrame_setnan(const DAPyDataFrame& df,
                              const QList< int >& rows,
                              const QList< int >& columns,
                              DAPyDataFrameTableModule* model = nullptr,
                              QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    DAPyDataFrame m_dataframe;
    QList< int > m_rows;
    QList< int > m_columns;
    DAPyDataFrameTableModule* m_model;
    QList< pybind11::object > m_olddatas;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castNum : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_castNum(const DAPyDataFrame& df,
                               const QList< int >& index,
                               const pybind11::dict& args,
                               DAPyDataFrameTableModule* model = nullptr,
                               QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    QList< int > m_index;
    pybind11::dict m_args;
    DAPyDataFrameTableModule* m_model;
};

/**
 * @brief 转换列的数据类型
 */
class DAGUI_API DACommandDataFrame_castDatetime : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_castDatetime(const DAPyDataFrame& df,
                                    const QList< int >& index,
                                    const pybind11::dict& args,
                                    DAPyDataFrameTableModule* model = nullptr,
                                    QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    DAPyDataFrame m_dataframe;
    QList< int > m_index;
    pybind11::dict m_args;
    DAPyDataFrameTableModule* m_model;
};

/**
 * @brief 转换列为索引
 */
class DAGUI_API DACommandDataFrame_setIndex : public DACommandWithTemplateData
{
public:
    DACommandDataFrame_setIndex(const DAPyDataFrame& df,
                                const QList< int >& index,
                                QHeaderView* hv                 = nullptr,
                                DAPyDataFrameTableModule* model = nullptr,
                                QUndoCommand* par               = nullptr);
    void redo() override;
    void undo() override;

private:
    QList< int > m_index;
    QHeaderView* m_headerView;
    DAPyDataFrameTableModule* m_model;
};
}  // end of namespace DA
#endif  // DACOMMANDSDATAFRAME_H
