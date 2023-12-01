#include "DACommandsDataFrame.h"
#include "Models/DAPyDataFrameTableModule.h"
#include "DAPyScripts.h"
#include <QHeaderView>

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

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
    : DACommandWithRedoCount(par), m_dataframe(df), m_row(row), m_col(col), m_oldData(olddata), m_newData(newdata), m_model(model)
{
    setText(QObject::tr("set dataframe data"));  // cn:改变单元格数据
}

void DACommandDataFrame_iat::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    setSuccess(m_dataframe.iat(m_row, m_col, m_newData));
    if (m_model) {
        m_model->refresh(m_row, m_col);
    }
}

void DACommandDataFrame_iat::undo()
{
    if (isSetSuccess()) {
        m_dataframe.iat(m_row, m_col, m_oldData);
        if (m_model) {
            m_model->refresh(m_row, m_col);
        }
    }
}

///////////////////////////////

DACommandDataFrame_insertNanRow::DACommandDataFrame_insertNanRow(const DAPyDataFrame& df, int row, DAPyDataFrameTableModule* model, QUndoCommand* par)
    : DACommandWithTemplateData(df, par), m_row(row), m_model(model)
{
    setText(QObject::tr("insert row"));  // cn:插入一行
}

void DACommandDataFrame_insertNanRow::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.insert_nanrow(dataframe(), m_row)) {
        return;
    }
    setSuccess();
    if (m_model) {
        m_model->rowBeginInsert({ m_row });
        m_model->rowEndInsert();
    }
}

void DACommandDataFrame_insertNanRow::undo()
{
    load();
    if (m_model) {
        m_model->rowBeginRemove({ m_row });
        m_model->rowEndRemove();
    }
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
    : DACommandWithTemplateData(df, par), m_isRangeMode(false), m_col(col), m_name(name), m_defaultvalue(defaultvalue), m_model(model)
{
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
    : DACommandWithTemplateData(df, par), m_isRangeMode(true), m_col(col), m_name(name), m_start(start), m_stop(stop), m_model(model)
{
}

void DACommandDataFrame_insertColumn::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (m_insertedSeries.isNone()) {
        if (m_isRangeMode) {
            if (!pydf.insert_column(dataframe(), m_col, m_name, m_start, m_stop)) {
                return;
            }
        } else {
            if (!pydf.insert_column(dataframe(), m_col, m_name, m_defaultvalue)) {
                return;
            }
        }
    } else {
        //说明已经插入了series
        pydf.insert_at(dataframe(), m_col, m_insertedSeries);
    }

    setSuccess();
    if (m_model) {
        //此操作会删除一列，添加一列，整个modelreflash
        m_model->columnBeginInsert({ m_col });
        m_model->columnEndInsert();
    }
}

void DACommandDataFrame_insertColumn::undo()
{
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    m_insertedSeries           = pydf.itake_column(dataframe(), m_col);
    if (m_model) {
        //此操作会删除一列，添加一列，整个modelreflash
        m_model->columnBeginRemove({ m_col });
        m_model->columnEndRemove();
    }
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
    : DACommandWithTemplateData(df, par), m_index(index), m_model(model)
{
    setText(QObject::tr("drop dataframe rows"));  // cn:移除dataframe行
}

void DACommandDataFrame_dropIRow::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
    if (!py.drop_irow(m_dataframe, m_index)) {
        return;
    }
    setSuccess();
    if (m_model) {
        m_model->rowBeginRemove(m_index);
        m_model->rowEndRemove();
    }
}

void DACommandDataFrame_dropIRow::undo()
{
    load();
    if (m_model) {
        m_model->rowBeginInsert(m_index);
        m_model->rowEndInsert();
    }
}

////////////////////

DACommandDataFrame_dropIColumn::DACommandDataFrame_dropIColumn(const DAPyDataFrame& df,
                                                               const QList< int >& index,
                                                               DAPyDataFrameTableModule* model,
                                                               QUndoCommand* par)
    : DACommandWithTemplateData(df, par), m_index(index), m_model(model)
{
    setText(QObject::tr("drop dataframe columns"));  // cn:移除dataframe列
}

void DACommandDataFrame_dropIColumn::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
    if (!py.drop_icolumn(m_dataframe, m_index)) {
        return;
    }
    setSuccess();
    if (m_model) {
        m_model->columnBeginRemove(m_index);
        m_model->columnEndRemove();
    }
}

void DACommandDataFrame_dropIColumn::undo()
{
    load();
    if (m_model) {
        m_model->columnBeginInsert(m_index);
        m_model->columnEndInsert();
    }
}

////////////////////

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                                                   const QList< QString >& cols,
                                                                   QHeaderView* hv,
                                                                   QUndoCommand* par)
    : QUndoCommand(par), m_dataframe(df), m_cols(cols), m_headerView(hv)
{
    m_oldcols = df.columns();
}

DACommandDataFrame_renameColumns::DACommandDataFrame_renameColumns(const DAPyDataFrame& df,
                                                                   const QList< QString >& cols,
                                                                   const QList< QString >& oldcols,
                                                                   QHeaderView* hv,
                                                                   QUndoCommand* par)
    : QUndoCommand(par), m_dataframe(df), m_cols(cols), m_headerView(hv), m_oldcols(oldcols)
{
}

void DACommandDataFrame_renameColumns::redo()
{
    m_dataframe.columns(m_cols);
    if (m_headerView) {
        m_headerView->headerDataChanged(Qt::Horizontal, 0, m_cols.size() - 1);
    }
}

void DACommandDataFrame_renameColumns::undo()
{
    m_dataframe.columns(m_oldcols);
    if (m_headerView) {
        m_headerView->headerDataChanged(Qt::Horizontal, 0, m_cols.size() - 1);
    }
}

////////////////////////////

DACommandDataFrame_astype::DACommandDataFrame_astype(const DAPyDataFrame& df,
                                                     const QList< int >& index,
                                                     const DAPyDType& dt,
                                                     DAPyDataFrameTableModule* model,
                                                     QUndoCommand* par)
    : DACommandWithTemplateData(df, par), m_index(index), m_dtype(dt), m_model(model)
{
    setText(QObject::tr("change column type"));  // cn:改变列数据类型
}

void DACommandDataFrame_astype::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    if (m_dtype.isNone()) {
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.astype(m_dataframe, m_index, m_dtype)) {
        return;
    }
    setSuccess();
    if (m_model) {
        for (int c : m_index) {
            m_model->refreshColumn(c);
        }
    }
}

void DACommandDataFrame_astype::undo()
{
    load();
    if (m_model) {
        for (int c : m_index) {
            m_model->refreshColumn(c);
        }
    }
}

///////////////////////////

DACommandDataFrame_setnan::DACommandDataFrame_setnan(const DAPyDataFrame& df,
                                                     const QList< int >& rows,
                                                     const QList< int >& columns,
                                                     DAPyDataFrameTableModule* model,
                                                     QUndoCommand* par)
    : DACommandWithRedoCount(par), m_dataframe(df), m_rows(rows), m_columns(columns), m_model(model)
{
    //先把原来数据提取出来
    for (int i = 0; i < m_rows.size(); ++i) {
        m_olddatas.append(df.iatObj(rows[ i ], columns[ i ]));
    }
}

void DACommandDataFrame_setnan::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.setnan(m_dataframe, m_rows, m_columns)) {
        return;
    }
    setSuccess();
    if (m_model) {
        for (int i = 0; i < m_rows.size(); ++i) {
            m_model->refresh(m_rows[ i ], m_columns[ i ]);
        }
    }
}

void DACommandDataFrame_setnan::undo()
{
    for (int i = 0; i < m_rows.size(); ++i) {
        m_dataframe.iat(m_rows[ i ], m_columns[ i ], m_olddatas[ i ]);
        m_model->refresh(m_rows[ i ], m_columns[ i ]);
    }
}

///////////////////

DACommandDataFrame_castNum::DACommandDataFrame_castNum(const DAPyDataFrame& df,
                                                       const QList< int >& index,
                                                       const pybind11::dict& args,
                                                       DAPyDataFrameTableModule* model,
                                                       QUndoCommand* par)
    : DACommandWithTemplateData(df, par), m_index(index), m_args(args), m_model(model)
{
    setText(QObject::tr("cast column to num"));  // cn:改变列数据为数值
}

void DACommandDataFrame_castNum::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.cast_to_num(m_dataframe, m_index, m_args)) {
        return;
    }
    setSuccess();
    if (m_model) {
        for (int c : m_index) {
            m_model->refreshColumn(c);
        }
    }
}

void DACommandDataFrame_castNum::undo()
{
    load();
    if (m_model) {
        for (int c : m_index) {
            m_model->refreshColumn(c);
        }
    }
}

///////////////////////////////

DACommandDataFrame_castDatetime::DACommandDataFrame_castDatetime(const DAPyDataFrame& df,
                                                                 const QList< int >& index,
                                                                 const pybind11::dict& args,
                                                                 DAPyDataFrameTableModule* model,
                                                                 QUndoCommand* par)
    : DACommandWithTemplateData(df, par), m_index(index), m_args(args), m_model(model)
{
    setText(QObject::tr("cast column to datetime"));  // cn:改变列数据为日期
}

void DACommandDataFrame_castDatetime::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.cast_to_datetime(m_dataframe, m_index, m_args)) {
        return;
    }
    setSuccess();
    if (m_model) {
        for (int c : m_index) {
            m_model->refreshColumn(c);
        }
    }
}

void DACommandDataFrame_castDatetime::undo()
{
    load();
    if (m_model) {
        for (int c : m_index) {
            m_model->refreshColumn(c);
        }
    }
}

////////////////////////

DACommandDataFrame_setIndex::DACommandDataFrame_setIndex(const DAPyDataFrame& df,
                                                         const QList< int >& index,
                                                         QHeaderView* hv,
                                                         DAPyDataFrameTableModule* model,
                                                         QUndoCommand* par)
    : DACommandWithTemplateData(df, par), m_index(index), m_headerView(hv), m_model(model)
{
    setText(QObject::tr("set column to index"));  // cn:转换列为索引
}

void DACommandDataFrame_setIndex::redo()
{
    addRedoCnt();
    if (isEqualTwo()) {
        //第二次执行跳过，推入栈
        return;
    }
    DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
    if (!pydf.set_index(dataframe(), m_index)) {
        return;
    }
    setSuccess();
    if (m_model) {
        //此操作会删除一列，添加一列，整个modelreflash
        m_model->refresh();
    }
}

void DACommandDataFrame_setIndex::undo()
{
    load();
    if (m_model) {
        m_model->refresh();
    }
}

////////////////////////////
