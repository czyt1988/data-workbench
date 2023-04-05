#include "DADataOperateOfDataFrameWidget.h"
#include "ui_DADataOperateOfDataFrameWidget.h"
#include "DAPyDataFrameTableModule.h"
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
#include "DACommandsDataFrame.h"
// Dialog
#include "DARenameColumnsNameDialog.h"
#include "DADialogDataframeColumnCastToNumeric.h"
#include "DADialogDataframeColumnCastToDatetime.h"
#include "DADialogInsertNewColumn.h"

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
    : DADataOperatePageWidget(parent), ui(new Ui::DADataOperateOfDataFrameWidget), _dlgCastNumArgs(nullptr), _dlgCastDatetimeArgs(nullptr)
{
    ui->setupUi(this);
    _undoStack.setUndoLimit(20);
    _undoStack.setActive();
    _model = new DAPyDataFrameTableModule(&_undoStack, this);
    ui->tableView->setModel(_model);
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
    return _data.isDataFrame();
}

/**
 * @brief 获取dataframe
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::getDataframe() const
{
    return _data.toDataFrame();
}

/**
 * @brief 获取Data的引用
 * @return
 */
const DAData& DADataOperateOfDataFrameWidget::data() const
{
    return _data;
}

void DADataOperateOfDataFrameWidget::setDAData(const DA::DAData& d)
{
    _data = d;
    _model->setDAData(d);
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
    std::unique_ptr< DACommandDataFrame_insertNanRow > cmd(new DACommandDataFrame_insertNanRow(_data.toDataFrame(), row, _model));
    cmd->redo();
    if (!cmd->isSetSuccess()) {
        return;
    }
    _undoStack.push(cmd.release());
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
        cmd.reset(new DACommandDataFrame_insertColumn(_data.toDataFrame(), col, name, dlg.getStartValue(), dlg.getStopValue(), _model));
    } else {
        cmd.reset(new DACommandDataFrame_insertColumn(_data.toDataFrame(), col, name, dlg.getDefaultValue(), _model));
    }
    cmd->redo();
    if (!cmd->isSetSuccess()) {
        return;
    }
    _undoStack.push(cmd.release());
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
    std::unique_ptr< DACommandDataFrame_dropIRow > cmd(new DACommandDataFrame_dropIRow(_data.toDataFrame(), rows, _model));
    cmd->redo();
    if (!cmd->isSetSuccess()) {
        return 0;
    }
    _undoStack.push(cmd.release());
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
    std::unique_ptr< DACommandDataFrame_dropIColumn > cmd(new DACommandDataFrame_dropIColumn(_data.toDataFrame(), columns, _model));
    cmd->redo();
    if (!cmd->isSetSuccess()) {
        return 0;
    }
    _undoStack.push(cmd.release());
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
    QPair< QList< int >, QList< int > > rowcols = getSelectedDataframeCells();
    if (rowcols.first.size() <= 0) {
        qWarning() << tr("please select valid cell");  // cn:请选择正确的单元格
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_setnan > cmd(new DACommandDataFrame_setnan(df, rowcols.first, rowcols.second, _model));
    cmd->redo();
    if (!cmd->isSetSuccess()) {
        return 0;
    }
    _undoStack.push(cmd.release());
    return rowcols.first.size();
}

/**
 * @brief 激活此窗口的UndoStack
 */
void DADataOperateOfDataFrameWidget::activeUndoStack()
{
    qDebug() << "DADataOperateOfDataFrameWidget activeUndoStack ";
    if (!_undoStack.isActive()) {
        _undoStack.setActive();
    }
}

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
    dlg.setDataName(_data.getName());
    dlg.setColumnsName(oldcols);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    QList< QString > cols = dlg.getColumnsName();
    //唯一判断在DARenameColumnsNameDialog里进行
    // header提前获取
    QHeaderView* hv                       = ui->tableView->horizontalHeader();
    DACommandDataFrame_renameColumns* cmd = new DACommandDataFrame_renameColumns(df, cols, oldcols, hv);
    _undoStack.push(cmd);
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
    std::unique_ptr< DACommandDataFrame_astype > cmd(new DACommandDataFrame_astype(df, selColumns, dt, _model));
    cmd->redo();
    if (!cmd->isSetSuccess()) {
        //说明没有设置成功
        emit selectTypeChanged({}, DAPyDType());
        return false;
    }
    _undoStack.push(cmd.release());  //成功，push会执行redo但会跳过
    emit selectTypeChanged(selColumns, dt);
    //这里说明设置成功了
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
    if (_dlgCastNumArgs == nullptr) {
        _dlgCastNumArgs = new DADialogDataframeColumnCastToNumeric(this);
    }
    if (QDialog::Accepted != _dlgCastNumArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypes(colsIndex.first());
    pybind11::dict args = _dlgCastNumArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castNum > cmd(new DACommandDataFrame_castNum(df, colsIndex, args, _model));
    cmd->redo();  //先执行
    if (!cmd->isSetSuccess()) {
        return;
    }
    _undoStack.push(cmd.release());  //推入后不会执行redo逻辑部分
    //如果类型改变了刷新类型
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
    if (_dlgCastDatetimeArgs == nullptr) {
        _dlgCastDatetimeArgs = new DADialogDataframeColumnCastToDatetime(this);
    }
    if (QDialog::Accepted != _dlgCastDatetimeArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypes(colsIndex.first());
    pybind11::dict args = _dlgCastDatetimeArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castDatetime > cmd(new DACommandDataFrame_castDatetime(df, colsIndex, args, _model));
    cmd->redo();  //先执行
    if (!cmd->isSetSuccess()) {
        return;
    }
    _undoStack.push(cmd.release());  //推入后不会执行redo逻辑部分
    //如果类型改变了刷新类型
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
            new DACommandDataFrame_setIndex(df, colsIndex, ui->tableView->verticalHeader(), _model));
    cmd->redo();  //先执行
    if (!cmd->isSetSuccess()) {
        return false;
    }
    _undoStack.push(cmd.release());  //推入后不会执行redo逻辑部分
    return true;
}

/**
 * @brief 创建一个数据描述
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::createDataDescribe()
{
    if (!_data.isDataFrame()) {
        return DAPyDataFrame();
    }
    DAPyDataFrame df_describe = _data.toDataFrame().describe();
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
        //确保返回的列数都在dataframe里
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
        //不确保返回的列数都在dataframe里
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
        //确保返回的列数都在dataframe里
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
        //不确保返回的列数都在dataframe里
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
        //确保返回的列数都在dataframe里
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
        //不确保返回的列数都在dataframe里
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
        //确保返回的列数都在dataframe里
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
        //不确保返回的列数都在dataframe里
        return selindexs.first().column();
    }
    return -1;
}

QPair< QList< int >, QList< int > > DADataOperateOfDataFrameWidget::getSelectedDataframeCells(bool ensureInDataframe) const
{
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return QPair< QList< int >, QList< int > >();
    }
    QList< int > rows;
    QList< int > cols;
    QModelIndexList selindexs = selModel->selectedIndexes();
    if (ensureInDataframe) {
        //确保返回的列数都在dataframe里
        DAPyDataFrame df = getDataframe();
        if (df.isNone()) {
            return QPair< QList< int >, QList< int > >();
        }
        auto shape = df.shape();
        for (const QModelIndex& index : selindexs) {
            if (index.row() < (int)shape.first && index.column() < (int)shape.second) {
                rows.append(index.row());
                cols.append(index.column());
            }
        }
    } else {
        //不确保返回的列数都在dataframe里
        for (const QModelIndex& index : selindexs) {
            rows.append(index.row());
            cols.append(index.column());
        }
    }
    return qMakePair(rows, cols);
}

/**
 * @brief 表格点击
 * @param index
 */
void DADataOperateOfDataFrameWidget::onTableViewClicked(const QModelIndex& index)
{
    if (!_data.isDataFrame()) {
        emit selectTypeChanged({}, DAPyDType());
        return;
    }
    DAPyDataFrame df = _data.toDataFrame();
    if (index.column() >= (int)df.shape().first) {
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

QUndoStack* DADataOperateOfDataFrameWidget::getUndoStack() const
{
    return const_cast< QUndoStack* >(&_undoStack);
}
