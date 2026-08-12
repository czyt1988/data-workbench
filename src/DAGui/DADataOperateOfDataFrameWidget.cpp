#include "DADataOperateOfDataFrameWidget.h"
#include "ui_DADataOperateOfDataFrameWidget.h"
#include "Models/DADataTableModel.h"
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
#include "DAWaitCursorScoped.h"
#include "DALogCategory.h"
// stl
#include <memory>
// qt
#include <QTableView>
#include <QHeaderView>
#include <QSet>
#include <QMessageBox>
#include <QPointer>
// cmd
#include "Commands/DACommandsDataFrame.h"
#include "Commands/DACommandsTableStyle.h"
#include "DADataManager.h"
// table style
#include "DATableStyleManager.h"
#include "DATableStyleItemDelegate.h"
#include "DATableStyleRegistry.h"
// Dialog
#include "Dialog/DARenameColumnsNameDialog.h"
#include "Dialog/DADialogDataframeColumnCastToNumeric.h"
#include "Dialog/DADialogDataframeColumnCastToDatetime.h"
#include "Dialog/DADialogInsertNewColumn.h"
#include "Dialog/DADialogDataframeColumnDescribe.h"

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

DADataOperateOfDataFrameWidget::DADataOperateOfDataFrameWidget(const DAData& d, DATableStyleRegistry* registry, QWidget* parent)
    : DADataOperatePageWidget(parent)
    , ui(new Ui::DADataOperateOfDataFrameWidget)
    , mDialogCastNumArgs(nullptr)
    , mDialogCastDatetimeArgs(nullptr)
{
    ui->setupUi(this);

    mModel = new DADataTableModel(getUndoStack(), this);

    ui->tableView->setModel(mModel);
    // 表格样式管理器与 delegate
    // 样式管理器借自会话级 DATableStyleRegistry（随数据存在，widget 关闭后保留，重开可见）。
    // 行列增删的样式键偏移同步在 §7 的回调中接入（onRowsInserted 等系列）。
    mStyleManager  = registry ? registry->getOrCreate(d) : nullptr;
    mStyleDelegate = new DATableStyleItemDelegate(mStyleManager, this);
    ui->tableView->setItemDelegate(mStyleDelegate);
    // 样式变更时触发 view 刷新（actualRow 转回 logical row 通知 model）
    connect(mStyleManager, &DATableStyleManager::styleChanged, this, [ this ](int actualRow, int actualCol) {
        if (!mModel) {
            return;
        }
        int logicalRow = actualRow - mModel->getCacheWindowStartRow();
        if (logicalRow >= 0 && logicalRow < mModel->rowCount()) {
            mModel->notifyDataChanged(logicalRow, actualCol);
        }
    });
    connect(mStyleManager, &DATableStyleManager::styleRangeChanged, this, [ this ](int rowStart, int colStart, int rowEnd, int colEnd) {
        if (!mModel) {
            return;
        }
        int offset = mModel->getCacheWindowStartRow();
        // -1 表示整个维度
        int lrStart = (rowStart < 0) ? 0 : rowStart - offset;
        int lrEnd   = (rowEnd < 0) ? mModel->rowCount() - 1 : rowEnd - offset;
        int lcStart = (colStart < 0) ? 0 : colStart;
        int lcEnd   = (colEnd < 0) ? mModel->columnCount() - 1 : colEnd;
        if (lrStart >= 0 && lrStart < mModel->rowCount() && lcStart >= 0 && lcStart < mModel->columnCount()) {
            lrEnd   = qMin(lrEnd, mModel->rowCount() - 1);
            lcEnd   = qMin(lcEnd, mModel->columnCount() - 1);
            mModel->notifyDataChanged(lrStart, lcStart, lrEnd, lcEnd);
        }
    });
    connect(mStyleManager, &DATableStyleManager::styleReset, this, [ this ]() {
        if (mModel) {
            // 全表刷新
            mModel->notifyDataChanged(0, 0, mModel->rowCount() - 1, mModel->columnCount() - 1);
        }
    });
    // 关闭不必要的绘制特性
    setDAData(d);
    connect(ui->tableView, &QTableView::clicked, this, &DADataOperateOfDataFrameWidget::onTableViewClicked);
    // 表头点击信号转发，用于"选择序列"窗口拾取列
    connect(ui->tableView->horizontalHeader(), &QHeaderView::sectionClicked, this, &DADataOperateOfDataFrameWidget::columnHeaderClicked);
    // 选中变化时反向同步 ribbon 控件
    if (auto sm = ui->tableView->selectionModel()) {
        connect(sm, &QItemSelectionModel::selectionChanged, this, [ this ]() {
            Q_EMIT currentStyleChanged(getCurrentCellStyle());
        });
    }
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
        ui->tableView->setData(d);
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
    std::unique_ptr< DACommandDataFrame_insertNanRow > cmd(new DACommandDataFrame_insertNanRow(mData.toDataFrame(), row));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, row ]() {
        if (modle) {
            modle->notifyRowsInserted({ row });
        }
    });
    // 行列增删样式键同步：插入行时 >=row 的键 +1；撤销时按删除处理（>=row 的键 -1）
    DATableStyleManager* styleMgr = mStyleManager;
    cmd->setDirectionalCallBack([ styleMgr, row ](bool isUndo) {
        if (!styleMgr) {
            return;
        }
        if (isUndo) {
            styleMgr->onRowsRemoved({ row });
        } else {
            styleMgr->onRowsInserted({ row });
        }
    });
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
                             tr("Warning"),                                                     // cn:警告
                             tr("The name of the new column to be inserted must be specified")  // cn:必须指定列的名字
        );
        return;
    }
    DAPyDType dt = dlg.getDType();
    if (dlg.isRangeMode()) {
        cmd.reset(
            new DACommandDataFrame_insertColumn(mData.toDataFrame(), col, name, dlg.getStartValue(), dlg.getStopValue()));
    } else {
        cmd.reset(new DACommandDataFrame_insertColumn(mData.toDataFrame(), col, name, dlg.getDefaultValue()));
    }
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, col ]() {
        if (modle) {
            // 此操作会删除一列，添加一列，整个modelreflash
            modle->notifyColumnsRemoved({ col });
        }
    });
    // 行列增删样式键同步：插入列时 >=col 的键 +1；撤销时按删除处理
    DATableStyleManager* styleMgr = mStyleManager;
    cmd->setDirectionalCallBack([ styleMgr, col ](bool isUndo) {
        if (!styleMgr) {
            return;
        }
        if (isUndo) {
            styleMgr->onColumnsRemoved({ col });
        } else {
            styleMgr->onColumnsInserted({ col });
        }
    });
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
        daWarning << tr("Please select valid data cells");  // cn:请选择正确的行
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_dropIRow > cmd(new DACommandDataFrame_dropIRow(mData.toDataFrame(), rows));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, rows ]() {
        if (modle) {
            modle->notifyRowsInserted(rows);
        }
    });
    // 行列增删样式键同步：删除行时 ==row 的键删除，>row 的键 -1；撤销时按插入处理
    // 注意：被删除行的样式在删除时丢失，撤销(load恢复dataframe)无法恢复样式（已知限制，见 spec §7.3）
    DATableStyleManager* styleMgr = mStyleManager;
    cmd->setDirectionalCallBack([ styleMgr, rows ](bool isUndo) {
        if (!styleMgr) {
            return;
        }
        if (isUndo) {
            styleMgr->onRowsInserted(rows);
        } else {
            styleMgr->onRowsRemoved(rows);
        }
    });
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
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_dropIColumn > cmd(new DACommandDataFrame_dropIColumn(mData.toDataFrame(), columns));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, columns ]() {
        if (modle) {
            modle->notifyColumnsRemoved(columns);
        }
    });
    // 行列增删样式键同步：删除列时 ==col 的键删除，>col 的键 -1；撤销时按插入处理
    // 注意：被删除列的样式在删除时丢失，撤销(load恢复dataframe)无法恢复样式（已知限制，见 spec §7.3）
    DATableStyleManager* styleMgr = mStyleManager;
    cmd->setDirectionalCallBack([ styleMgr, columns ](bool isUndo) {
        if (!styleMgr) {
            return;
        }
        if (isUndo) {
            styleMgr->onColumnsInserted(columns);
        } else {
            styleMgr->onColumnsRemoved(columns);
        }
    });
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
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return 0;
    }
    QList< int > rows, cols;
    rows.reserve(cells.size());
    cols.reserve(cells.size());
    for (const auto p : cells) {
        rows.append(p.x());
        cols.append(p.y());
    }
    std::unique_ptr< DACommandDataFrame_setnan > cmd(new DACommandDataFrame_setnan(df, rows, cols));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, rows, cols ]() {
        if (modle) {
            const auto size = qMin(rows.size(), cols.size());
            for (int i = 0; i < size; ++i) {
                modle->notifyDataChanged(rows[ i ], cols[ i ]);
            }
        }
    });
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
        daWarning << tr("Table has no columns");  // cn:表格没有列
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
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
    }

    getUndoStack()->push(cmd);
}

/**
 * @brief 重命名单列
 *
 * 构造"全表列名列表，仅修改目标列"传给 DACommandDataFrame_renameColumns，
 * 复用现有命令的 undo/redo 与 headerDataChanged 通知机制。
 * @param col 列位置索引
 * @param newName 新列名
 * @return 成功返回 true；列为空/重名/越界/exec 失败返回 false
 */
bool DADataOperateOfDataFrameWidget::renameColumn(int col, const QString& newName)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return false;
    }
    QList< QString > oldcols = df.columns();
    if (col < 0 || col >= oldcols.size()) {
        return false;
    }
    if (newName.isEmpty()) {
        daWarning << tr("Column name cannot be empty");  // cn:列名不能为空
        return false;
    }
    if (oldcols.contains(newName)) {
        daWarning << tr("Column name \"%1\" already exists, please use another name").arg(newName);  // cn:列名"%1"已存在，请使用其他名称
        return false;
    }
    QList< QString > newcols = oldcols;
    newcols[ col ]           = newName;
    QHeaderView* hv          = ui->tableView->horizontalHeader();
    std::unique_ptr< DACommandDataFrame_renameColumns > cmd(
        new DACommandDataFrame_renameColumns(df, newcols, oldcols, hv));
    if (!cmd->exec()) {
        return false;
    }
    if (DADataManager* mgr = mData.getDataManager()) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
    }
    getUndoStack()->push(cmd.release());
    // DADataTableModel 在缓存模式下缓存了列名，headerDataChanged 信号无法刷新缓存，
    // 需显式调用 refreshData() 重置模型以刷新表头显示
    if (DADataTableModel* m = ui->tableView->getDataModel()) {
        m->refreshData();
    }
    return true;
}

/**
 * @brief 设置选择列的数据类型
 * @param dtypeName
 * @return 成功改变类型返回true
 */
bool DADataOperateOfDataFrameWidget::changeSelectColumnType(const DAPyDType& dt)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        emit selectTypeChanged({ }, DAPyDType());
        return false;
    }
    QList< int > selColumns = getSelectedDataframeCoumns();
    if (selColumns.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        emit selectTypeChanged({ }, DAPyDType());
        return false;
    }
    std::unique_ptr< DACommandDataFrame_astype > cmd(new DACommandDataFrame_astype(df, selColumns, dt));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, selColumns ]() {
        if (modle) {
            for (int c : std::as_const(selColumns)) {
                modle->notifyColumnChanged(c);
            }
        }
    });
    if (!cmd->exec()) {
        // 说明没有设置成功
        emit selectTypeChanged({ }, DAPyDType());
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
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    if (mDialogCastNumArgs == nullptr) {
        mDialogCastNumArgs = new DADialogDataframeColumnCastToNumeric(this);
    }
    if (QDialog::Accepted != mDialogCastNumArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypeObject(colsIndex.first());
    pybind11::dict args = mDialogCastNumArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castNum > cmd(new DACommandDataFrame_castNum(df, colsIndex, args));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, colsIndex ]() {
        if (modle) {
            for (int c : std::as_const(colsIndex)) {
                modle->notifyColumnChanged(c);
            }
        }
    });
    if (!cmd->exec()) {
        return;
    }
    getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
    DAPyDType dt2 = df.dtypeObject(colsIndex.first());
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
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    if (mDialogCastDatetimeArgs == nullptr) {
        mDialogCastDatetimeArgs = new DADialogDataframeColumnCastToDatetime(this);
    }
    if (QDialog::Accepted != mDialogCastDatetimeArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypeObject(colsIndex.first());
    pybind11::dict args = mDialogCastDatetimeArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castDatetime > cmd(new DACommandDataFrame_castDatetime(df, colsIndex, args));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, colsIndex ]() {
        if (modle) {
            for (int c : std::as_const(colsIndex)) {
                modle->notifyColumnChanged(c);
            }
        }
    });
    if (!cmd->exec()) {
        return;
    }
    getUndoStack()->push(cmd.release());
    DAPyDType dt2 = df.dtypeObject(colsIndex.first());
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
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return false;
    }
    std::unique_ptr< DACommandDataFrame_setIndex > cmd = std::make_unique< DACommandDataFrame_setIndex >(df, colsIndex);
    DADataTableModel* modle                            = mModel;
    cmd->setCallBack([ modle ]() {
        if (modle) {
            modle->refreshData();
        }
    });
    if (!cmd->exec()) {
        return false;
    }
    getUndoStack()->push(cmd.release());  // 推入后不会执行redo逻辑部分
    return true;
}

/**
 * @brief 显示列统计信息
 *
 * 通过 pandas Series.describe() 获取指定列的统计信息（count、mean、std、min、25%、50%、75%、max 等），
 * 在弹出的对话框中展示。
 * @param col 列位置索引
 */
void DADataOperateOfDataFrameWidget::showColumnDescribe(int col)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    auto shape = df.shape();
    if (col < 0 || col >= (int)shape.second) {
        return;
    }
    QString colName = df.columnName(col);
    QString dtypeStr;
    QList<QPair<QString, QString>> stats;
    pybind11::gil_scoped_acquire gil;
    try {
        DAPySeries s      = df[ colName ];
        dtypeStr          = s.dtypeString();
        DAPySeries desc   = s.describe();
        QStringList statNames = desc.indexAsStringList();
        for (int i = 0; i < statNames.size(); ++i) {
            stats.append({ statNames[ i ], desc.valueAsString(i) });
        }
    } catch (const std::exception& e) {
        qCritical() << e.what();
        daWarning << tr("Unable to get statistics for this column");  // cn:无法获取此列的统计信息
        return;
    }
    if (stats.isEmpty()) {
        daWarning << tr("Unable to get statistics for this column");  // cn:无法获取此列的统计信息
        return;
    }
    if (!mDialogColumnDescribe) {
        mDialogColumnDescribe = new DADialogDataframeColumnDescribe(this);
    }
    mDialogColumnDescribe->setColumnName(colName);
    mDialogColumnDescribe->setColumnDType(dtypeStr);
    mDialogColumnDescribe->setStatistics(stats);
    mDialogColumnDescribe->exec();
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
 * @brief 获取dataframe表格的tableview
 *
 * @return DADataTableView*
 */
DADataTableView* DADataOperateOfDataFrameWidget::getDataTableView() const
{
    return ui->tableView;
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
    const QModelIndexList selindexs = selModel->selectedIndexes();
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
    const QModelIndexList selindexs = selModel->selectedIndexes();
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
    const QModelIndexList selindexs = selModel->selectedColumns();
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
    const QModelIndexList selindexs = selModel->selectedRows();
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
    const QModelIndexList selindexs = selModel->selectedIndexes();
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
    const QModelIndexList selindexs = selModel->selectedIndexes();
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

void DADataOperateOfDataFrameWidget::refreshTable()
{
    if (mModel) {
        mModel->refreshData();
    }
}

/**
 * @brief 确保列可见
 * @param colName
 */
void DADataOperateOfDataFrameWidget::ensureColumnVisible(const QString& colName, bool selectCol)
{
    if (colName.isEmpty()) {
        return;
    }
    QTableView* tv = ui->tableView;
    if (!tv || !tv->model()) {
        return;
    }

    // 直接找列号
    int col       = -1;
    const auto* m = tv->model();
    for (int c = 0, total = m->columnCount(); c < total; ++c) {
        if (m->headerData(c, Qt::Horizontal).toString() == colName) {
            col = c;
            break;
        }
    }
    if (col < 0)
        return;

    tv->scrollTo(m->index(0, col), QAbstractItemView::EnsureVisible);
    if (selectCol) {
        QItemSelectionModel* sel = tv->selectionModel();
        if (!sel) {
            return;
        }
        QModelIndex topLeft     = m->index(0, col);
        QModelIndex bottomRight = m->index(m->rowCount() - 1, col);
        QItemSelection selection(topLeft, bottomRight);

        sel->clearSelection();  // 去掉旧选区
        sel->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Columns);
    }
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
    const QModelIndexList selindexs = selModel->selectedIndexes();
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
        t = df.dtypeObject(index.column());
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }

    emit selectTypeChanged({ index.column() }, t);
}

/**
 * @brief 获取样式管理器
 * @return 样式管理器指针
 */
DATableStyleManager* DADataOperateOfDataFrameWidget::styleManager() const
{
    return mStyleManager;
}

/**
 * @brief 合并样式片段到选中区
 *
 * 自动判定层级：选中整列→列级，选中整行→行级，否则单元格级。
 * fragment 只设部分属性（如只设 background），merge 到目标已有样式，未设属性保留。
 * @param fragment 样式片段
 */
void DADataOperateOfDataFrameWidget::mergeStyleToSelection(const DATableCellStyle& fragment)
{
    if (!mStyleManager) {
        return;
    }
    // 判定应用层级
    QList< int > fullCols = getFullySelectedDataframeColumns(false);
    QList< int > fullRows = getFullySelectedDataframeRows(false);
    const QList< QPoint > cells = getSelectedDataframeCells(false);

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));

    // getSelectedDataframeCells 返回 QPoint(index.row(), index.column())，row 是 logical row
    // 需转换为 actualRow（加 getCacheWindowStartRow）
    int cacheOffset = mModel ? mModel->getCacheWindowStartRow() : 0;

    if (!fullCols.isEmpty()) {
        // 列级：合并 fragment 到已有列样式
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, newStyle, true);
        }
    } else if (!fullRows.isEmpty()) {
        // 行级：合并 fragment 到已有行样式
        for (int row : std::as_const(fullRows)) {
            int actualRow             = row + cacheOffset;
            DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, newStyle, true);
        }
    } else {
        // 单元格级：合并 fragment 到已有单元格样式
        for (const QPoint& p : cells) {
            // QPoint(index.row(), index.column()) → p.x()=row(logical), p.y()=col
            int actualRow             = p.x() + cacheOffset;
            int col                   = p.y();
            DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, newStyle, true);
        }
    }

    getUndoStack()->push(cmd.release());
}

/**
 * @brief 设置完整样式到选中区（整体替换，为格式刷准备）
 *
 * 自动判定层级：选中整列→列级，选中整行→行级，否则单元格级。
 * 传入什么样式就设置什么样式，完全替换目标已有样式（不做 merge）。
 * style 中 valid=false 的属性会让目标对应属性变为"未设置"。
 * @param style 完整样式
 */
void DADataOperateOfDataFrameWidget::applyStyleToSelection(const DATableCellStyle& style)
{
    if (!mStyleManager) {
        return;
    }
    QList< int > fullCols = getFullySelectedDataframeColumns(false);
    QList< int > fullRows = getFullySelectedDataframeRows(false);
    const QList< QPoint > cells = getSelectedDataframeCells(false);

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));
    int cacheOffset = mModel ? mModel->getCacheWindowStartRow() : 0;

    if (!fullCols.isEmpty()) {
        // 列级：直接替换
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, style, false);
        }
    } else if (!fullRows.isEmpty()) {
        // 行级：直接替换
        for (int row : std::as_const(fullRows)) {
            int actualRow             = row + cacheOffset;
            DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, style, false);
        }
    } else {
        // 单元格级：直接替换
        for (const QPoint& p : cells) {
            int actualRow             = p.x() + cacheOffset;
            int col                   = p.y();
            DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, style, false);
        }
    }

    getUndoStack()->push(cmd.release());
}

/**
 * @brief 清除选中区样式
 *
 * 自动判定层级，仅清除有样式的目标的样式。
 */
void DADataOperateOfDataFrameWidget::clearStyleSelection()
{
    if (!mStyleManager) {
        return;
    }
    QList< int > fullCols = getFullySelectedDataframeColumns(false);
    QList< int > fullRows = getFullySelectedDataframeRows(false);
    const QList< QPoint > cells = getSelectedDataframeCells(false);

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));
    int cacheOffset = mModel ? mModel->getCacheWindowStartRow() : 0;

    if (!fullCols.isEmpty()) {
        for (int col : std::as_const(fullCols)) {
            if (mStyleManager->hasColumnStyle(col)) {
                DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
                cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else if (!fullRows.isEmpty()) {
        for (int row : std::as_const(fullRows)) {
            int actualRow = row + cacheOffset;
            if (mStyleManager->hasRowStyle(actualRow)) {
                DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
                cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else {
        for (const QPoint& p : cells) {
            int actualRow = p.x() + cacheOffset;
            int col       = p.y();
            if (mStyleManager->hasCellStyle(actualRow, col)) {
                DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
                cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, DATableCellStyle(), false);
            }
        }
    }

    // 避免推送空命令到 undo 栈（选中区无样式时不产生 undo 步骤）
    if (cmd->isEmpty()) {
        return;
    }
    getUndoStack()->push(cmd.release());
}

/**
 * @brief 清除整表所有样式（可撤销）
 *
 * 记录所有现有样式的 oldStyle，构造撤销命令。
 */
void DADataOperateOfDataFrameWidget::clearStyleAll()
{
    if (!mStyleManager || mStyleManager->isEmpty()) {
        return;
    }
    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));
    // 收集列级样式
    const auto& cs = mStyleManager->styledColumns();
    for (int col : cs) {
        DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
        cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, DATableCellStyle(), false);
    }

    // 收集行级样式
    const auto& rs = mStyleManager->styledRows();
    for (int row : rs) {
        DATableCellStyle oldStyle = mStyleManager->getRowStyle(row);
        cmd->addChange(DACommandTableStyle::Row, row, 0, oldStyle, DATableCellStyle(), false);
    }
    // 收集单元格级样式
    const auto& cells = mStyleManager->styledCells();
    for (const QPair< int, int >& key : cells) {
        DATableCellStyle oldStyle = mStyleManager->getCellStyle(key.first, key.second);
        cmd->addChange(DACommandTableStyle::Cell, key.first, key.second, oldStyle, DATableCellStyle(), false);
    }
    getUndoStack()->push(cmd.release());
}

/**
 * @brief 获取选中区代表的样式
 *
 * 对每个属性（background/foreground/font）独立判断选中区所有单元格的一致性：
 * - 所有单元格该属性值相同 → 该属性 valid，返回具体值
 * - 该属性在不同单元格有不同值 → 该属性 invalid（ribbon 不调整该控件）
 * 无选中时返回空样式。
 * @return 代表样式
 */
DATableCellStyle DADataOperateOfDataFrameWidget::getCurrentCellStyle() const
{
    DATableCellStyle result;
    if (!mStyleManager || !mModel) {
        return result;
    }
    // 收集选中区所有单元格的 resolveCellStyle
    QList< DATableCellStyle > styles;
    QList< int > fullCols = getFullySelectedDataframeColumns(false);
    QList< int > fullRows = getFullySelectedDataframeRows(false);
    const QList< QPoint > cells = getSelectedDataframeCells(false);
    int cacheOffset = mModel->getCacheWindowStartRow();

    if (!fullCols.isEmpty()) {
        // 整列选中：取列级 resolveCellStyle（用 actualRow=0 采样）
        // 但要检查所有选中列的一致性，且需考虑行级/单元格级叠加
        // 简化：用所有选中单元格的 resolveCellStyle 判断一致性
        for (const QPoint& p : cells) {
            styles.append(mStyleManager->resolveCellStyle(p.x() + cacheOffset, p.y()));
        }
        if (styles.isEmpty()) {
            // cells 为空但 fullCols 非空（极端情况），用列级样式
            for (int col : std::as_const(fullCols)) {
                styles.append(mStyleManager->resolveCellStyle(0, col));
            }
        }
    } else if (!fullRows.isEmpty()) {
        for (const QPoint& p : cells) {
            styles.append(mStyleManager->resolveCellStyle(p.x() + cacheOffset, p.y()));
        }
        if (styles.isEmpty()) {
            for (int row : std::as_const(fullRows)) {
                styles.append(mStyleManager->resolveCellStyle(row + cacheOffset, 0));
            }
        }
    } else {
        for (const QPoint& p : cells) {
            styles.append(mStyleManager->resolveCellStyle(p.x() + cacheOffset, p.y()));
        }
    }

    if (styles.isEmpty()) {
        return result;
    }

    // 对每个属性独立判断一致性
    const DATableCellStyle& first = styles.first();

    // background
    bool bgConsistent = true;
    for (int i = 1; i < styles.size(); ++i) {
        if (styles[ i ].backgroundValid() != first.backgroundValid()
            || (first.backgroundValid() && styles[ i ].background() != first.background())) {
            bgConsistent = false;
            break;
        }
    }
    if (bgConsistent && first.backgroundValid()) {
        result.setBackground(first.background());
    }

    // foreground
    bool fgConsistent = true;
    for (int i = 1; i < styles.size(); ++i) {
        if (styles[ i ].foregroundValid() != first.foregroundValid()
            || (first.foregroundValid() && styles[ i ].foreground() != first.foreground())) {
            fgConsistent = false;
            break;
        }
    }
    if (fgConsistent && first.foregroundValid()) {
        result.setForeground(first.foreground());
    }

    // font
    bool fontConsistent = true;
    for (int i = 1; i < styles.size(); ++i) {
        if (styles[ i ].fontValid() != first.fontValid()
            || (first.fontValid() && styles[ i ].font() != first.font())) {
            fontConsistent = false;
            break;
        }
    }
    if (fontConsistent && first.fontValid()) {
        result.setFont(first.font());
    }

    return result;
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
