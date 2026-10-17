#include "DADataOperateOfDataFrameWidget.h"
#include "ui_DADataOperateOfDataFrameWidget.h"
#include "Models/DADataTableModel.h"
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
#include "DAWaitCursorScoped.h"
#include "DALogCategory.h"
#include "DATableDataSource.h"
#include "DATableKeyRemap.h"
// stl
#include <memory>
// qt
#include <QTableView>
#include <QHeaderView>
#include <QSet>
#include <QHash>
#include <QMessageBox>
#include <QPointer>
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QElapsedTimer>
// cmd
#include "Commands/DACommandsDataFrame.h"
#include "Commands/DACommandsTableStyle.h"
#include "Commands/DACommandsTableColumnFormat.h"
#include "DADataManager.h"
// table metadata / style
#include "DATableStyleManager.h"
#include "DATableStyleItemDelegate.h"
#include "DATableMetadataRegistry.h"
#include "DATableMetadata.h"
#include "DATableViewStateStore.h"
#include "DATableHighlightOverlay.h"
#include "DADataHeaderView.h"
// Dialog
#include "Dialog/DARenameColumnsNameDialog.h"
#include "Dialog/DADialogDataframeColumnCastToNumeric.h"
#include "Dialog/DADialogDataframeColumnCastToDatetime.h"
#include "Dialog/DADialogInsertNewColumn.h"
#include "Dialog/DADialogDataframeColumnDescribe.h"
#include "Dialog/DADialogFindReplace.h"
// python operator wrapper（查找/替换）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include <climits>

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

namespace
{
// 替换撤销阈值（格）：变更格数超过此值降级为「不可撤销 + 确认提示」（设计稿 §4.11 策略表，默认 100 万格）
constexpr int c_replaceUndoCellLimit = 1000000;
}

//===================================================
// DADataOperateOfDataFrameWidget
//===================================================
int DADataOperateOfDataFrameWidget::getDataOperatePageType() const
{
    return DADataOperatePageWidget::DataOperateOfDataFrame;
}

DADataOperateOfDataFrameWidget::DADataOperateOfDataFrameWidget(const DAData& d, DATableMetadataRegistry* registry, QWidget* parent)
    : DADataOperatePageWidget(parent)
    , ui(new Ui::DADataOperateOfDataFrameWidget)
    , mDialogCastNumArgs(nullptr)
    , mDialogCastDatetimeArgs(nullptr)
{
    ui->setupUi(this);

    // 用 DADataHeaderView 替换默认表头（列头右键菜单/带修饰键点击信号/行头宽拖拽）。
    // 必须在 setModel 之前替换：QTableView::setModel 会把模型接到当前表头上。
    // 行高/Interactive 设置在 DADataHeaderView(Vertical) 构造内重建
    // （参照 DACacheWindowTableView 构造对默认行头的同款设置）。
    ui->tableView->setHorizontalHeader(new DADataHeaderView(Qt::Horizontal, ui->tableView));
    ui->tableView->setVerticalHeader(new DADataHeaderView(Qt::Vertical, ui->tableView));

    mModel = new DADataTableModel(getUndoStack(), this);

    ui->tableView->setModel(mModel);
    // 表格元数据束与各子存储、delegate
    // 元数据束借自会话级 DATableMetadataRegistry（随数据存在，widget 关闭后保留，重开可见）。
    // 行列增删的样式键偏移同步在 §7 的回调中接入（onRowsInserted 等系列）。
    DATableMetadata* metadata = registry ? registry->getOrCreate(d) : nullptr;
    mStyleManager             = metadata ? metadata->styleManager() : nullptr;
    mViewStateStore           = metadata ? metadata->viewState() : nullptr;
    // 瞬态高亮层：页级持有（查找/校验两个来源共用），不序列化、不进撤销栈；
    // 注入 delegate 绘制管线③（瞬态装饰）
    mHighlightOverlay = new DATableHighlightOverlay(this);
    mStyleDelegate    = new DATableStyleItemDelegate(mStyleManager, mHighlightOverlay, this);
    ui->tableView->setItemDelegate(mStyleDelegate);
    // 高亮层变更时重绘视口（瞬态装饰不经模型 dataChanged 链路）
    connect(mHighlightOverlay, &DATableHighlightOverlay::changed, this, [ this ]() {
        ui->tableView->viewport()->update();
    });
    // 模型读取列显示格式（actualData 的 DisplayRole 分支）
    mModel->setStyleManager(mStyleManager);
    // 样式变更时触发 view 刷新（actualRow 转回 logical row 通知 model）
    connect(mStyleManager, &DATableStyleManager::styleChanged, this, [ this ](int actualRow, int actualCol) {
        if (!mModel) {
            return;
        }
        mModel->notifyDataChanged(actualRow, actualCol);
    });
    connect(mStyleManager, &DATableStyleManager::styleRangeChanged, this, [ this ](int rowStart, int colStart, int rowEnd, int colEnd) {
        if (!mModel) {
            return;
        }
        // -1 表示整个维度；行号统一用数据源绝对行，由 model 换算到窗口内
        const int rStart = (rowStart < 0) ? 0 : rowStart;
        const int rEnd   = (rowEnd < 0) ? qMax(0, mModel->actualRowCount() - 1) : rowEnd;
        const int cStart = (colStart < 0) ? 0 : colStart;
        const int cEnd   = (colEnd < 0) ? mModel->columnCount() - 1 : colEnd;
        mModel->notifyDataChanged(rStart, cStart, rEnd, cEnd);
    });
    connect(mStyleManager, &DATableStyleManager::styleReset, this, [ this ]() {
        if (mModel) {
            // 全表刷新
            mModel->notifyDataChanged(0, 0, qMax(0, mModel->actualRowCount() - 1), mModel->columnCount() - 1);
        }
    });
    // 视图状态（列宽按列名 + 行头宽，R6）：
    // 结构变化（模型重置/插删列）后按列名重新应用，保证列宽不随位置错位；
    // 采集只认用户拖拽（sectionResized + 左键按住在表头上），程序化改动不写 store
    connect(mModel, &QAbstractItemModel::modelReset, this, [ this ]() {
        applyViewStateFromStore();
    });
    connect(mModel, &QAbstractItemModel::columnsInserted, this, [ this ]() {
        applyViewStateFromStore();
        // 插列后排序规格里的列位置失效，清除规格（数据顺序不变，只清指示器）
        mSortSpec.clear();
        if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(ui->tableView->horizontalHeader())) {
            hh->setSortSpec(mSortSpec);
        }
    });
    connect(mModel, &QAbstractItemModel::columnsRemoved, this, [ this ]() {
        applyViewStateFromStore();
        // 删列后排序规格里的列位置失效，清除规格
        mSortSpec.clear();
        if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(ui->tableView->horizontalHeader())) {
            hh->setSortSpec(mSortSpec);
        }
    });
    connect(ui->tableView->horizontalHeader(), &QHeaderView::sectionResized, this, [ this ](int logicalIndex, int oldSize, int newSize) {
        Q_UNUSED(oldSize);
        onHeaderSectionResized(logicalIndex, newSize);
    });
    // 行头宽拖拽（DADataHeaderView vertical 实例）→ 写视图状态
    if (DADataHeaderView* vh = qobject_cast< DADataHeaderView* >(ui->tableView->verticalHeader())) {
        connect(vh, &DADataHeaderView::rowHeaderWidthChanged, this, [ this ](int width) {
            if (mViewStateStore) {
                mViewStateStore->setRowHeaderWidth(width);
            }
        });
    }
    // 关闭不必要的绘制特性
    setDAData(d);
    // 构造完成后应用一次视图状态（工程加载的宽度回填在注册表里，此时已可借用）
    applyViewStateFromStore();
    connect(ui->tableView, &QTableView::clicked, this, &DADataOperateOfDataFrameWidget::onTableViewClicked);
    // 表头点击（002：切换到 DADataHeaderView::clickedWithModifiers，排序交互与拾取模式在页面内分流；
    // 拾取模式下仍发 columnHeaderClicked，图表「选择序列」拾取链路保持不变）
    if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(ui->tableView->horizontalHeader())) {
        connect(hh, &DADataHeaderView::clickedWithModifiers, this, &DADataOperateOfDataFrameWidget::onHeaderClickedWithModifiers);
    }
    // 查找/替换入口（002 T1.4）：视图快捷键 Ctrl+F/Ctrl+H 与表体右键菜单共用表意信号
    connect(ui->tableView, &DADataTableView::findRequested, this, [ this ]() {
        showFindReplace(false);
    });
    connect(ui->tableView, &DADataTableView::replaceRequested, this, [ this ]() {
        showFindReplace(true);
    });
    connect(ui->tableView, &DADataTableView::findBarCloseRequested, this, &DADataOperateOfDataFrameWidget::closeFindReplace);
    // 选中变化时反向同步 ribbon 控件
    if (auto sm = ui->tableView->selectionModel()) {
        connect(sm, &QItemSelectionModel::selectionChanged, this, [ this ]() {
            Q_EMIT currentStyleChanged(getCurrentCellStyle());
            Q_EMIT currentDisplayFormatChanged(getCurrentColumnDisplayFormat());
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
    // dataframe与通用表格数据（如数据库惰性表）都视为有数据；series维持原语义不开数据页
    return mData.isDataFrame() || (mData.isTable() && !mData.isSeries());
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
    // 数据替换后旧排序规格（列位置+方向）失去意义，清除（排序规格不持久化，R13）
    mSortSpec.clear();
    if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(ui->tableView->horizontalHeader())) {
        hh->setSortSpec(mSortSpec);
    }
    // 通用表格数据（DATableDataSource，如数据库惰性表）也可显示；series维持原语义
    if (d.isDataFrame() || (d.isTable() && !d.isSeries())) {
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
    DAPyDataFrame df = mData.toDataFrame();
    if (df.isNone()) {
        // 非dataframe数据（如只读的数据库惰性表）不支持插入行
        return;
    }
    std::unique_ptr< DACommandDataFrame_insertNanRow > cmd(new DACommandDataFrame_insertNanRow(df, row));
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
    push(cmd.release());
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
    if (mData.toDataFrame().isNone()) {
        // 非dataframe数据（如只读的数据库惰性表）不支持插入列，弹窗前先拦截
        return;
    }
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
    push(cmd.release());
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
    push(cmd.release());
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
    push(cmd.release());
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
    push(cmd.release());
    return cells.size();
}

/**
 * @brief 粘贴剪贴板内容到当前选区左上角
 *
 * 解析剪贴板 TSV 文本（Tab 分列、\n 分行，与 copySelectionToClipboard 格式对称），
 * 从当前选中区左上角起填充；超出数据表边界的部分丢弃并 daWarning 提示丢弃规模（D9）。
 * 数值列遇非数值文本时按 iat 现有转换链路处理（写入失败则整条命令回滚）。
 * 撤销命令走 DADataOperatePageWidget::push 统一入口（push 即激活栈）。
 * @return 成功写入的单元格数（0 表示未写入）
 */
int DADataOperateOfDataFrameWidget::pasteFromClipboard()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    QClipboard* cb = QGuiApplication::clipboard();
    if (!cb) {
        return 0;
    }
    QString text = cb->text();
    if (text.isEmpty()) {
        daWarning << tr("Clipboard is empty");  // cn:剪贴板为空
        return 0;
    }
    // 定位选区左上角；无选区时用当前光标格
    QModelIndex anchor;
    if (QItemSelectionModel* sm = ui->tableView->selectionModel()) {
        const QModelIndexList idxs = sm->selectedIndexes();
        if (!idxs.isEmpty()) {
            anchor = idxs.first();
        } else {
            anchor = sm->currentIndex();
        }
    }
    if (!anchor.isValid()) {
        daWarning << tr("Please select a cell to paste into");  // cn:请先选中要粘贴的起始单元格
        return 0;
    }
    auto shape  = df.shape();
    const int dfRows = static_cast< int >(shape.first);
    const int dfCols = static_cast< int >(shape.second);
    // 视图行号是滑动窗内逻辑行，写 dataframe 前必须换成绝对行
    const int startRow = toActualRow(anchor.row());
    const int startCol = anchor.column();
    if (startRow >= dfRows || startCol >= dfCols) {
        return 0;
    }
    // 解析 TSV（Tab 分列、\n 分行、\r 行尾剔除）
    const QStringList lines = text.split(QLatin1Char('\n'));
    QList< int > rows, cols;
    QList< QVariant > values;
    int droppedRows = 0, droppedCols = 0;
    int contentLineCount = 0;
    for (int r = 0; r < lines.size(); ++r) {
        QString line = lines[ r ];
        // 结尾换行产生的末尾空行跳过；中间空行保留占位（与复制格式对称）
        if (r == lines.size() - 1 && line.isEmpty()) {
            break;
        }
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
        const QStringList fields = line.split(QLatin1Char('\t'));
        const bool rowOverflow = (startRow + contentLineCount >= dfRows);
        int lineDroppedCols = 0;
        for (int c = 0; c < fields.size(); ++c) {
            if (startCol + c >= dfCols) {
                ++lineDroppedCols;
                continue;
            }
            if (rowOverflow) {
                continue;
            }
            rows.append(startRow + contentLineCount);
            cols.append(startCol + c);
            values.append(fields[ c ]);
        }
        if (rowOverflow) {
            ++droppedRows;
        } else {
            droppedCols += lineDroppedCols;
        }
        ++contentLineCount;
    }
    if (rows.isEmpty()) {
        daWarning << tr("Nothing to paste: the clipboard content exceeds the table boundary");  // cn:无可粘贴内容：剪贴板内容超出表格边界
        return 0;
    }
    if (droppedRows > 0 || droppedCols > 0) {
        daWarning << tr("Paste partially dropped: %1 row(s) and %2 cell(s) outside the table were ignored")
                        .arg(droppedRows)
                        .arg(droppedCols);  // cn:粘贴部分丢弃：%1 行与 %2 个单元格超出表格范围被忽略
    }
    std::unique_ptr< DACommandDataFrame_paste > cmd(new DACommandDataFrame_paste(df, rows, cols, values));
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
        // 数值列写入非数值等场景：整条命令回滚
        daWarning << tr("Paste failed: the content does not match the column data type");  // cn:粘贴失败：内容与列数据类型不匹配
        return 0;
    }
    push(cmd.release());
    return rows.size();
}

/**
 * @brief 剪切选中区
 *
 * 先按复制格式导出到剪贴板，再把选中区置 nan（复用 DACommandDataFrame_setnan，可撤销）
 * @return 受影响单元格数
 */
int DADataOperateOfDataFrameWidget::cutSelection()
{
    // 先复制（失败说明无选中）
    if (!ui->tableView->copySelectionToClipboard()) {
        daWarning << tr("No cells selected to cut");  // cn:没有选中可剪切的单元格
        return 0;
    }
    // 复制成功说明有选中，置 nan（同 removeSelectCell 语义）
    return removeSelectCell();
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
    // 表头刷新走模型通知入口（命令不再持有 QHeaderView*）
    DACommandDataFrame_renameColumns* cmd = new DACommandDataFrame_renameColumns(df, cols, oldcols, mModel);
    if (!cmd->exec()) {
        return;
    }
    // 列改名迁移：视图状态按列名引用（R20），宽度记录随改名同步迁移
    if (mViewStateStore) {
        const int n = qMin(cols.size(), oldcols.size());
        for (int i = 0; i < n; ++i) {
            if (cols[ i ] != oldcols[ i ]) {
                mViewStateStore->renameColumn(oldcols[ i ], cols[ i ]);
            }
        }
    }
    //! 通知datamanager
    if (DADataManager* mgr = mData.getDataManager()) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
    }

    push(cmd);
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
    std::unique_ptr< DACommandDataFrame_renameColumns > cmd(
        new DACommandDataFrame_renameColumns(df, newcols, oldcols, mModel));
    if (!cmd->exec()) {
        return false;
    }
    // 列改名迁移：视图状态按列名引用（R20），宽度记录随改名同步迁移；
    // 放在 refreshData 之前，保证 modelReset 触发 applyViewStateFromStore 时按新列名命中
    if (mViewStateStore) {
        mViewStateStore->renameColumn(oldcols[ col ], newName);
    }
    if (DADataManager* mgr = mData.getDataManager()) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
    }
    push(cmd.release());
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
    push(cmd.release());  // 成功，push会执行redo但会跳过
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
    push(cmd.release());  // 推入后不会执行redo逻辑部分
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
    push(cmd.release());
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
    push(cmd.release());  // 推入后不会执行redo逻辑部分
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
    const int rowCount              = dataRowCount();
    for (const QModelIndex& i : selindexs) {
        // 视图行号是滑动窗内的逻辑行，数据操作一律用数据源绝对行
        const int actualRow = toActualRow(i.row());
        if (ensureInDataframe && actualRow >= rowCount) {
            continue;
        }
        res.insert(actualRow);
    }
    return res.values();
}

/**
 * @brief 视图行号转换为数据源绝对行号
 *
 * 表格视图带滑动窗，选中项的行号是窗口内行号；凡是拿行号去读写 dataframe、
 * 样式表、undo 命令的地方都必须先经过这里，否则滚动之后就会操作到别的行。
 * @param viewRow 视图行号（滑动窗内逻辑行）
 * @return 数据源绝对行号
 */
int DADataOperateOfDataFrameWidget::toActualRow(int viewRow) const
{
    if (!ui->tableView) {
        return viewRow;
    }
    return ui->tableView->toActualRow(viewRow);
}

/**
 * @brief 数据源真实数据行数（不含尾部预留的空白行）
 * @return 行数
 */
int DADataOperateOfDataFrameWidget::dataRowCount() const
{
    if (mModel) {
        return qMax(0, mModel->actualRowCount() - mModel->getExtraRowCount());
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    return static_cast< int >(df.shape().first);
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
    const int rowCount              = dataRowCount();
    for (const QModelIndex& i : selindexs) {
        const int actualRow = toActualRow(i.row());
        if (ensureInDataframe && actualRow >= rowCount) {
            continue;
        }
        res.insert(actualRow);
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
    const int rowCount = dataRowCount();
    for (const QModelIndex& i : selindexs) {
        const int actualRow = toActualRow(i.row());
        if (!ensureInDataframe || actualRow < rowCount) {
            return actualRow;
        }
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
    const int rowCount    = dataRowCount();
    const int columnCount = dataColumnCount();
    const QModelIndexList selindexs = selModel->selectedIndexes();
    for (const QModelIndex& index : selindexs) {
        const int actualRow = toActualRow(index.row());
        if (ensureInDataframe && (actualRow >= rowCount || index.column() >= columnCount)) {
            continue;
        }
        res.append(QPoint(actualRow, index.column()));
    }
    return res;
}

/**
 * @brief 数据源真实数据列数（不含尾部预留的空白列）
 * @return 列数
 */
int DADataOperateOfDataFrameWidget::dataColumnCount() const
{
    if (mModel) {
        return qMax(0, mModel->columnCount() - mModel->getExtraColumnCount());
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    return static_cast< int >(df.shape().second);
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
 * @brief 获取视图状态子存储
 * @return 视图状态子存储指针（借自元数据束，registry 为空时为 nullptr）
 */
DATableViewStateStore* DADataOperateOfDataFrameWidget::viewStateStore() const
{
    return mViewStateStore;
}

/**
 * @brief 获取瞬态高亮层
 * @return 高亮层指针（页级持有，非空）
 */
DATableHighlightOverlay* DADataOperateOfDataFrameWidget::highlightOverlay() const
{
    return mHighlightOverlay;
}

/**
 * @brief 从视图状态子存储应用列宽与行头宽
 *
 * 列宽按「列名 + 第几次出现（occurrence）」匹配逻辑列号后 resizeSection——
 * 按列名匹配保证插列/删列后宽度不错位（R6/R20）。应用期间置守卫标志，
 * 屏蔽 sectionResized 采集，避免把「应用」当「用户改动」写回 store。
 * 行头宽用 setFixedWidth 落地（Qt 纵向表头宽度=clamp(qMax(min,sizeHint),max)，
 * min=max=w 恒为 w，不会被视图布局覆盖）。
 */
void DADataOperateOfDataFrameWidget::applyViewStateFromStore()
{
    if (!mViewStateStore || !mModel) {
        return;
    }
    mApplyingViewState = true;
    QHeaderView* hh = ui->tableView->horizontalHeader();
    if (hh) {
        const int totalCols = mModel->columnCount();
        QHash< QString, int > seen;  // 列名 -> 已出现次数（即下一列的 occurrence）
        for (int c = 0; c < totalCols; ++c) {
            const QString name = mModel->headerData(c, Qt::Horizontal).toString();
            const int occ      = seen.value(name, 0);
            seen[ name ]       = occ + 1;
            if (name.isEmpty()) {
                continue;  // 尾部预留空白列无列名，不应用
            }
            const int w = mViewStateStore->columnWidth(name, occ);
            if (w > 0) {
                hh->resizeSection(c, w);
            }
        }
    }
    const int rhw = mViewStateStore->rowHeaderWidth();
    if (rhw > 0) {
        if (QHeaderView* vh = ui->tableView->verticalHeader()) {
            vh->setFixedWidth(rhw);
        }
    }
    mApplyingViewState = false;
}

/**
 * @brief 采集列宽改动写入视图状态子存储
 *
 * 只记录用户拖拽：左键按住且鼠标在表头上；应用守卫期间、程序化/布局触发的
 * sectionResized（模型重置、窗口缩放拉伸末列等）一律不记录。尾部预留空白列
 * 与空列名不记录。occurrence = 该列之前同名列的个数（重名列消歧，R20）。
 * @param logicalIndex 逻辑列号
 * @param newSize 新宽度（像素）
 */
void DADataOperateOfDataFrameWidget::onHeaderSectionResized(int logicalIndex, int newSize)
{
    if (mApplyingViewState || !mViewStateStore || !mModel) {
        return;
    }
    QHeaderView* hh = ui->tableView->horizontalHeader();
    if (!hh || !(QGuiApplication::mouseButtons() & Qt::LeftButton) || !hh->underMouse()) {
        return;  // 非用户拖拽
    }
    if (logicalIndex < 0 || logicalIndex >= dataColumnCount()) {
        return;  // 尾部预留空白列不记录
    }
    const QString name = mModel->headerData(logicalIndex, Qt::Horizontal).toString();
    if (name.isEmpty()) {
        return;
    }
    int occ = 0;
    for (int c = 0; c < logicalIndex; ++c) {
        if (mModel->headerData(c, Qt::Horizontal).toString() == name) {
            ++occ;
        }
    }
    mViewStateStore->setColumnWidth(name, occ, newSize);
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
            const int actualRow       = row;
            DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, newStyle, true);
        }
    } else {
        // 单元格级：合并 fragment 到已有单元格样式
        for (const QPoint& p : cells) {
            // QPoint(actualRow, column)
            const int actualRow       = p.x();
            const int col             = p.y();
            DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, newStyle, true);
        }
    }

    push(cmd.release());
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

    if (!fullCols.isEmpty()) {
        // 列级：直接替换
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, style, false);
        }
    } else if (!fullRows.isEmpty()) {
        // 行级：直接替换
        for (int row : std::as_const(fullRows)) {
            const int actualRow       = row;
            DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, style, false);
        }
    } else {
        // 单元格级：直接替换
        for (const QPoint& p : cells) {
            const int actualRow       = p.x();
            const int col             = p.y();
            DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, style, false);
        }
    }

    push(cmd.release());
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

    if (!fullCols.isEmpty()) {
        for (int col : std::as_const(fullCols)) {
            if (mStyleManager->hasColumnStyle(col)) {
                DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
                cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else if (!fullRows.isEmpty()) {
        for (int row : std::as_const(fullRows)) {
            const int actualRow = row;
            if (mStyleManager->hasRowStyle(actualRow)) {
                DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
                cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else {
        for (const QPoint& p : cells) {
            const int actualRow = p.x();
            const int col       = p.y();
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
    push(cmd.release());
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
    push(cmd.release());
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

    if (!fullCols.isEmpty()) {
        // 整列选中：取列级 resolveCellStyle（用 actualRow=0 采样）
        // 但要检查所有选中列的一致性，且需考虑行级/单元格级叠加
        // 简化：用所有选中单元格的 resolveCellStyle 判断一致性
        for (const QPoint& p : cells) {
            styles.append(mStyleManager->resolveCellStyle(p.x(), p.y()));
        }
        if (styles.isEmpty()) {
            // cells 为空但 fullCols 非空（极端情况），用列级样式
            for (int col : std::as_const(fullCols)) {
                styles.append(mStyleManager->resolveCellStyle(0, col));
            }
        }
    } else if (!fullRows.isEmpty()) {
        for (const QPoint& p : cells) {
            styles.append(mStyleManager->resolveCellStyle(p.x(), p.y()));
        }
        if (styles.isEmpty()) {
            for (int row : std::as_const(fullRows)) {
                styles.append(mStyleManager->resolveCellStyle(row, 0));
            }
        }
    } else {
        for (const QPoint& p : cells) {
            styles.append(mStyleManager->resolveCellStyle(p.x(), p.y()));
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

/**
 * @brief 收集当前选中区涉及的列号（用于列级显示格式操作）
 *
 * 优先用整列选中（getFullySelectedDataframeColumns），否则取选中单元格涉及的列
 * （getSelectedDataframeCoumns，去重）。两者都空返回空列表。
 * @param ensureInDataframe 是否限定在 dataframe 范围内
 * @return 列号列表
 */
QList< int > collectSelectedColumns(const DADataOperateOfDataFrameWidget* w, bool ensureInDataframe)
{
    QList< int > cols = w->getFullySelectedDataframeColumns(ensureInDataframe);
    if (cols.isEmpty()) {
        cols = w->getSelectedDataframeCoumns(ensureInDataframe);
    }
    return cols;
}

/**
 * @brief 设置列显示格式到选中列
 *
 * 列级操作：对选中区涉及的所有列设置格式（可撤销）。
 * @param fmt 显示格式
 */
void DADataOperateOfDataFrameWidget::setDisplayFormatToSelection(const DATableDisplayFormat& fmt)
{
    if (!mStyleManager) {
        return;
    }
    QList< int > cols = collectSelectedColumns(this, false);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    std::unique_ptr< DACommandTableColumnFormat > cmd(new DACommandTableColumnFormat(mStyleManager));
    for (int col : std::as_const(cols)) {
        DATableDisplayFormat oldFmt =
            mStyleManager->hasColumnFormat(col) ? mStyleManager->getColumnFormat(col) : DATableDisplayFormat();
        cmd->addChange(col, oldFmt, fmt);
    }
    push(cmd.release());
}

/**
 * @brief 清除选中列的显示格式
 *
 * 仅清除显示格式，保留 bg/fg/font 样式。仅对有格式的列记录变更。
 */
void DADataOperateOfDataFrameWidget::clearDisplayFormatFromSelection()
{
    if (!mStyleManager) {
        return;
    }
    QList< int > cols = collectSelectedColumns(this, false);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    std::unique_ptr< DACommandTableColumnFormat > cmd(new DACommandTableColumnFormat(mStyleManager));
    for (int col : std::as_const(cols)) {
        if (mStyleManager->hasColumnFormat(col)) {
            DATableDisplayFormat oldFmt = mStyleManager->getColumnFormat(col);
            cmd->addChange(col, oldFmt, DATableDisplayFormat());  // invalid → 清除
        }
    }
    if (cmd->isEmpty()) {
        return;
    }
    push(cmd.release());
}

/**
 * @brief 获取选中列代表的显示格式
 *
 * 收集选中列各自的列级格式，若全部一致（含全 invalid）返回该格式，
 * 否则返回 invalid（ribbon 格式下拉框回退到 General）。
 * @return 代表格式
 */
DATableDisplayFormat DADataOperateOfDataFrameWidget::getCurrentColumnDisplayFormat() const
{
    DATableDisplayFormat result;  // invalid
    if (!mStyleManager) {
        return result;
    }
    QList< int > cols = collectSelectedColumns(this, false);
    if (cols.isEmpty()) {
        return result;
    }
    QList< DATableDisplayFormat > fmts;
    for (int col : std::as_const(cols)) {
        fmts.append(mStyleManager->hasColumnFormat(col) ? mStyleManager->getColumnFormat(col)
                                                        : DATableDisplayFormat());
    }
    const DATableDisplayFormat& first = fmts.first();
    for (int i = 1; i < fmts.size(); ++i) {
        if (!(fmts[ i ] == first)) {
            return result;  // 不一致 → invalid
        }
    }
    return first;
}

/**
 * @brief 表头拾取模式（图表「选择序列」会话期间置位）
 *
 * 拾取模式下表头点击只转发 columnHeaderClicked 信号（拾取链路保持不变）、不排序；
 * 由 DADataOperateWidget::setHeaderPickMode 转发（拾取器 connect/disconnectDataWidgets 切换）。
 * @param on true=进入拾取模式
 */
void DADataOperateOfDataFrameWidget::setHeaderPickMode(bool on)
{
    mHeaderPickMode = on;
}

/**
 * @brief 是否处于表头拾取模式
 * @return 拾取模式返回 true
 */
bool DADataOperateOfDataFrameWidget::isHeaderPickMode() const
{
    return mHeaderPickMode;
}

/**
 * @brief 当前多列排序规格
 * @return 列逻辑索引+方向列表（顺序即优先级；不持久化 R13）
 */
QVector< QPair< int, Qt::SortOrder > > DADataOperateOfDataFrameWidget::sortSpec() const
{
    return mSortSpec;
}

/**
 * @brief 数据源是否可编辑（只读/惰性源上排序入口不生效，D1）
 *
 * 判定路径同 DADataTableModel::actualFlags 的只读源分支：tableSource 非空且
 * isTableEditable() 为假（如数据库惰性表）→ 只读。
 * @return 可编辑返回 true
 */
bool DADataOperateOfDataFrameWidget::isTableEditable() const
{
    if (mData.isNull()) {
        return false;
    }
    DAData d                 = mData;
    const DATableDataSource* ts = d.tableSource();
    return !(ts && !ts->isTableEditable());
}

/**
 * @brief 表头带修饰键点击（002 排序交互入口）
 *
 * 分流规则（§4.1）：
 * - 拾取模式：只转发 columnHeaderClicked（图表「选择序列」拾取链路零回归），不排序；
 * - 无修饰单击：该列已是规格中唯一一列 → 切方向（升↔降）；否则重置为「仅该列，升序」；
 * - Shift/Ctrl+单击：该列不在规格中 → 追加（升序）；已在 → 切换其方向；
 * - 只读/惰性源：不排序，daWarning 提示一次；
 * - 每次变更 = 一个撤销单元（一次点击一个命令）。
 * @param logicalIndex 列逻辑索引
 * @param modifiers 修饰键
 */
void DADataOperateOfDataFrameWidget::onHeaderClickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers)
{
    if (mHeaderPickMode) {
        // 拾取模式：只转发信号（修饰键不吞，拾取侧按列填充表达式）
        Q_EMIT columnHeaderClicked(logicalIndex);
        return;
    }
    if (logicalIndex < 0 || logicalIndex >= dataColumnCount()) {
        return;  // 尾部预留空白列不排序
    }
    if (!isTableEditable()) {
        daWarning << tr("This data source is read-only and cannot be sorted.");  // cn:该数据源为只读，无法排序
        return;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || dataRowCount() <= 1) {
        return;  // 非 dataframe 或无可排序的行
    }
    // 更新排序规格（列 logical index + 方向，顺序即优先级）
    const bool addKey = modifiers.testFlag(Qt::ShiftModifier) || modifiers.testFlag(Qt::ControlModifier);
    QVector< QPair< int, Qt::SortOrder > > newSpec = mSortSpec;
    if (addKey) {
        int idx = -1;
        for (int i = 0; i < newSpec.size(); ++i) {
            if (newSpec.at(i).first == logicalIndex) {
                idx = i;
                break;
            }
        }
        if (idx >= 0) {
            newSpec[ idx ].second = (newSpec.at(idx).second == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            newSpec.append(qMakePair(logicalIndex, Qt::AscendingOrder));
        }
    } else {
        if (newSpec.size() == 1 && newSpec.at(0).first == logicalIndex) {
            // 该列已是规格中唯一一列 → 升↔降循环（Excel 语义：点击只切换方向，不取消排序）
            newSpec[ 0 ].second = (newSpec.at(0).second == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            newSpec.clear();
            newSpec.append(qMakePair(logicalIndex, Qt::AscendingOrder));
        }
    }
    applySortSpec(newSpec);
}

/**
 * @brief 应用排序规格（构造命令 + 回调接线 + push）
 *
 * 命令生效后按设计稿 §4.1 顺序联动：先样式键重映射（directionalCallback 里
 * styleManager()->onRowsReordered(最近应用置换)）→ 再模型刷新（notifyDataChanged
 * 清块缓存 + 行头 index 变更通知）→ 最后按置换修正选区、视口回顶。
 * 排序路径打点计时（qDebug 开发诊断，验收口径：10 万行 ≤300 ms）。
 * @param newSpec 新排序规格
 * @return 成功 push 返回 true；算子失败（如混合类型列）提示并保持原规格返回 false
 */
bool DADataOperateOfDataFrameWidget::applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& newSpec)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return false;
    }
    QVector< QPair< int, bool > > keys;
    keys.reserve(newSpec.size());
    for (const auto& k : newSpec) {
        keys.append(qMakePair(k.first, k.second == Qt::AscendingOrder));
    }
    std::unique_ptr< DACommandDataFrame_sortRows > cmd(new DACommandDataFrame_sortRows(df, keys));
    QPointer< DADataTableModel > modle          = mModel;
    QPointer< DADataTableView > view            = ui->tableView;
    DACommandDataFrame_sortRows* cmdRaw         = cmd.get();
    DATableStyleManager* styleMgr               = mStyleManager;
    DADataOperateOfDataFrameWidget* self        = this;
    // ① 样式键重映射（先于模型刷新；exec/undo 均传最近应用的置换，undo 传逆置换即等价按逆置换重映射）
    cmd->setDirectionalCallBack([ styleMgr, cmdRaw ](bool isUndo) {
        Q_UNUSED(isUndo);
        if (styleMgr) {
            styleMgr->onRowsReordered(cmdRaw->lastAppliedPermutation());
        }
    });
    // ② 模型刷新 → ③ 选区跟随 → 视口回顶（回调范式照 paste 的逐格 notifyDataChanged 写法）
    cmd->setCallBack([ modle, view, cmdRaw, self ]() {
        if (modle) {
            const int rows = modle->actualRowCount();
            const int cols = modle->columnCount();
            modle->notifyDataChanged(0, 0, qMax(0, rows - 1), qMax(0, cols - 1));
            // 行头显示 index 值，排序后 index 随行移动，需通知垂直表头重取
            modle->notifyHeaderDataChanged(Qt::Vertical, 0, qMax(0, rows - 1));
        }
        if (view) {
            self->remapSelectionByPermutation(cmdRaw->lastAppliedPermutation());
            view->showActualRow(0);  // 视口回顶（行位置整体变了，含 undo/redo）
        }
    });
    QElapsedTimer timer;
    timer.start();
    if (!cmd->exec()) {
        // 算子报错（如 object 列混合类型不可比较）：排序规格回滚到变更前
        daWarning << tr("Cannot sort: the column contains values of incomparable mixed types.");  // cn:无法排序：该列包含不可比较的混合类型
        return false;
    }
    push(cmd.release());
    mSortSpec = newSpec;
    if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(ui->tableView->horizontalHeader())) {
        hh->setSortSpec(mSortSpec);
    }
    // 排序路径计时打点（开发诊断，英文；验收：10 万行 ≤300 ms）
    qDebug().noquote() << QStringLiteral("DADataOperateOfDataFrameWidget::applySortSpec: rows=%1 keys=%2 elapsed_ms=%3")
                              .arg(dataRowCount())
                              .arg(keys.size())
                              .arg(timer.elapsed());
    return true;
}

/**
 * @brief 按最近应用的置换修正选区（选中行随排序跟随）
 *
 * appliedPerm 为新→旧置换（new[i]=old[appliedPerm[i]]），旧行号 old 的新位置 =
 * inv[old]（逆置换）。行数不变、滑动窗未平移，回调时视图选中的位置号即重排前的
 * 数据行号，直接按逆置换映射到新位置重建选区；映射出窗口的行跳过。
 * @param appliedPerm 最近应用的置换（exec=正置换 / undo=逆置换）
 */
void DADataOperateOfDataFrameWidget::remapSelectionByPermutation(const QVector< int >& appliedPerm)
{
    DADataTableView* view = ui->tableView;
    QItemSelectionModel* sm = view ? view->selectionModel() : nullptr;
    QAbstractItemModel* m   = view ? view->model() : nullptr;
    if (!sm || !m || appliedPerm.isEmpty() || !sm->hasSelection()) {
        return;
    }
    const QVector< int > inv = DATableKeyRemap::inversePermutation(appliedPerm);
    auto mapRow              = [ this, view, &inv ](int viewRow) -> int {
        const int oldActual = toActualRow(viewRow);
        if (oldActual < 0 || oldActual >= inv.size() || inv.at(oldActual) < 0) {
            return -1;
        }
        return view->toLogicalRow(inv.at(oldActual));  // 目标行不在当前滑动窗内时返回 -1
    };
    QItemSelection newSel;
    const QModelIndexList idxs = sm->selectedIndexes();
    for (const QModelIndex& i : idxs) {
        const int newLogical = mapRow(i.row());
        if (newLogical < 0) {
            continue;
        }
        const QModelIndex ni = m->index(newLogical, i.column());
        if (ni.isValid()) {
            newSel.select(ni, ni);
        }
    }
    QModelIndex newCur;
    const QModelIndex cur = sm->currentIndex();
    if (cur.isValid()) {
        const int newLogical = mapRow(cur.row());
        if (newLogical >= 0) {
            newCur = m->index(newLogical, cur.column());
        }
    }
    sm->select(newSel, QItemSelectionModel::ClearAndSelect);
    if (newCur.isValid()) {
        sm->setCurrentIndex(newCur, QItemSelectionModel::NoUpdate);
    }
}

/**
 * @brief 打开查找/替换浮动条（懒建，非模态，页级持有）
 * @param expandReplace true 展开替换行（Ctrl+H），false 仅查找（Ctrl+F）
 */
void DADataOperateOfDataFrameWidget::showFindReplace(bool expandReplace)
{
    if (!mFindReplaceDialog) {
        mFindReplaceDialog = new DADialogFindReplace(this);
        connect(mFindReplaceDialog, &DADialogFindReplace::searchRequested, this, &DADataOperateOfDataFrameWidget::onFindSearchRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::nextRequested, this, &DADataOperateOfDataFrameWidget::onFindNextRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::previousRequested, this, &DADataOperateOfDataFrameWidget::onFindPrevRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::replaceRequested, this, &DADataOperateOfDataFrameWidget::onFindReplaceRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::replaceAllRequested, this, &DADataOperateOfDataFrameWidget::onFindReplaceAllRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::closed, this, &DADataOperateOfDataFrameWidget::onFindDialogClosed);
    }
    mFindReplaceDialog->activate(expandReplace);
    updateFindCountLabel();
}

/**
 * @brief 关闭查找/替换浮动条（清高亮经 closed → onFindDialogClosed）
 */
void DADataOperateOfDataFrameWidget::closeFindReplace()
{
    if (mFindReplaceDialog && mFindReplaceDialog->isVisible()) {
        mFindReplaceDialog->close();
    }
}

int DADataOperateOfDataFrameWidget::findHitCount() const
{
    return mFindHits.size();
}

int DADataOperateOfDataFrameWidget::currentFindHitIndex() const
{
    return mFindHitIndex;
}

void DADataOperateOfDataFrameWidget::onFindSearchRequested()
{
    runFindScan();
}

void DADataOperateOfDataFrameWidget::onFindPrevRequested()
{
    if (mFindHits.isEmpty()) {
        // 无命中（如刚打开未扫描）：先扫一次再导航
        runFindScan();
        if (mFindHits.isEmpty()) {
            return;
        }
        gotoFindHit(mFindHits.size() - 1);
        return;
    }
    gotoFindHit(mFindHitIndex - 1);
}

void DADataOperateOfDataFrameWidget::onFindNextRequested()
{
    if (mFindHits.isEmpty()) {
        runFindScan();
        if (mFindHits.isEmpty()) {
            return;
        }
        gotoFindHit(0);
        return;
    }
    gotoFindHit(mFindHitIndex + 1);
}

void DADataOperateOfDataFrameWidget::onFindReplaceRequested()
{
    applyFindReplace(false);
}

void DADataOperateOfDataFrameWidget::onFindReplaceAllRequested()
{
    applyFindReplace(true);
}

void DADataOperateOfDataFrameWidget::onFindDialogClosed()
{
    // 清查找高亮（按来源清 Find，不影响 Validation 层）与命中状态；
    // 序号自增使任何在途扫描结果作废（关闭即弃）
    ++mFindRequestSerial;
    mFindHits.clear();
    mFindHitIndex  = -1;
    mFindTotalHits = 0;
    if (mHighlightOverlay) {
        mHighlightOverlay->clear(DATableHighlightOverlay::Source::Find);
    }
}

/**
 * @brief 执行一次查找扫描（硬约束①②③的页面侧落点）
 *
 * ①去抖在对话框侧（QTimer 350ms）；②请求序号：进入即自增，结果返回时序号非最新即丢弃
 * （不排队、过期即弃）；③上限截断在算子侧（max_hits 默认 10 万），totalHits 带真实总数。
 * 扫描同步执行 + DAWaitCursorScoped 忙等待光标（RAII 保证异常/提前返回也恢复）。
 * 命中灌入 overlay(Source::Find, Level::Hit)，当前命中单独 Level::CurrentHit。
 */
void DADataOperateOfDataFrameWidget::runFindScan()
{
    if (!mFindReplaceDialog) {
        return;
    }
    const int serial                             = ++mFindRequestSerial;  // 硬约束②：本次请求序号
    mLastFindOptions                             = mFindReplaceDialog->options();
    const DADialogFindReplace::FindOptions& opt  = mLastFindOptions;
    // 清旧命中与高亮
    mFindHits.clear();
    mFindHitIndex  = -1;
    mFindTotalHits = 0;
    mHighlightOverlay->clear(DATableHighlightOverlay::Source::Find);
    if (opt.keyword.isEmpty()) {
        updateFindCountLabel();
        return;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        updateFindCountLabel();
        return;
    }
    DAWaitCursorScoped waitCursor;  // 忙等待光标（§4.2 进度反馈），RAII 恢复
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    const DAPyScriptsDataFrame::SearchDataResult res =
        py.searchData(df,
                      opt.keyword,
                      opt.caseSensitive,
                      opt.useRegex,
                      opt.wholeCell,
                      findScanRowRange(opt.selectionOnly),
                      findScanColRange(opt.selectionOnly));
    if (serial != mFindRequestSerial) {
        return;  // 过期结果丢弃（期间又发起了新请求/关闭）
    }
    if (!res.ok) {
        mFindReplaceDialog->setSearchFailed(true);  // 无效正则等
        updateFindCountLabel();
        return;
    }
    mFindTotalHits = res.totalHits;
    const int n    = qMin(res.rows.size(), res.cols.size());
    mFindHits.reserve(n);
    for (int i = 0; i < n; ++i) {
        mFindHits.append(qMakePair(res.rows.at(i), res.cols.at(i)));
    }
    mHighlightOverlay->set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, mFindHits);
    if (!mFindHits.isEmpty()) {
        gotoFindHit(0);  // 首条设为当前命中并定位（内部会更新计数标签）
    } else {
        updateFindCountLabel();
    }
}

/**
 * @brief 导航到第 idx 条命中（循环），更新当前命中高亮并滚动定位
 * @param idx 目标序号（越界自动回绕）
 */
void DADataOperateOfDataFrameWidget::gotoFindHit(int idx)
{
    const int n = mFindHits.size();
    if (n == 0) {
        mFindHitIndex = -1;
        mHighlightOverlay->clear(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit);
        updateFindCountLabel();
        return;
    }
    mFindHitIndex              = ((idx % n) + n) % n;  // 循环导航
    const QPair< int, int > hit = mFindHits.at(mFindHitIndex);
    // 当前命中单独一条 Level::CurrentHit（覆盖该格的 Hit 级别）
    mHighlightOverlay->set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit, { hit });
    // 滚动定位 + 选中（命中来自全表扫描，可能落在滑动窗外，selectActualCell 内部先 showActualRow）
    ui->tableView->selectActualCell(hit.first, hit.second);
    updateFindCountLabel();
}

/**
 * @brief 扫描行范围：全表 → nullopt；选区 → 选中行包围盒（数据源绝对行号）
 * @param selectionOnly 范围选项是否为「当前选区」
 * @return 行闭区间或 nullopt
 */
std::optional< QPair< int, int > > DADataOperateOfDataFrameWidget::findScanRowRange(bool selectionOnly) const
{
    if (!selectionOnly) {
        return std::nullopt;
    }
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (!sm || !sm->hasSelection()) {
        return std::nullopt;  // 无选区按全表处理
    }
    int minR = INT_MAX;
    int maxR = INT_MIN;
    const QModelIndexList idxs = sm->selectedIndexes();
    for (const QModelIndex& i : idxs) {
        const int ar = toActualRow(i.row());
        minR         = qMin(minR, ar);
        maxR         = qMax(maxR, ar);
    }
    if (maxR < minR) {
        return std::nullopt;
    }
    return qMakePair(minR, maxR);
}

/**
 * @brief 扫描列范围：全表 → nullopt；选区 → 选中列包围盒
 * @param selectionOnly 范围选项是否为「当前选区」
 * @return 列闭区间或 nullopt
 */
std::optional< QPair< int, int > > DADataOperateOfDataFrameWidget::findScanColRange(bool selectionOnly) const
{
    if (!selectionOnly) {
        return std::nullopt;
    }
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (!sm || !sm->hasSelection()) {
        return std::nullopt;
    }
    int minC = INT_MAX;
    int maxC = INT_MIN;
    const QModelIndexList idxs = sm->selectedIndexes();
    for (const QModelIndex& i : idxs) {
        minC = qMin(minC, i.column());
        maxC = qMax(maxC, i.column());
    }
    if (maxC < minC) {
        return std::nullopt;
    }
    return qMakePair(minC, maxC);
}

/**
 * @brief 更新计数标签（含上限截断提示，硬约束③）
 */
void DADataOperateOfDataFrameWidget::updateFindCountLabel()
{
    if (!mFindReplaceDialog) {
        return;
    }
    mFindReplaceDialog->setHitCount(mFindHitIndex + 1, mFindHits.size(), mFindTotalHits, mFindTotalHits > mFindHits.size());
}

/**
 * @brief 应用替换（all=false 替换当前命中这一格 / true 替换扫描范围全部）
 *
 * 流程：先用同一匹配语义预扫拿变更格数（真实命中总数=将变更格数）→ 超阈值
 * （c_replaceUndoCellLimit）弹「不可撤销」确认，确认后执行但不入栈（§4.11 降级路径，
 * 不违反 D4）→ 否则 findReplace 就地替换 + 构造 DACommandDataFrame_replace push
 * （一次全部替换 = 一个撤销单元）→ 刷新模型 → 重新扫描刷新命中。
 * @param all 是否全部替换
 */
void DADataOperateOfDataFrameWidget::applyFindReplace(bool all)
{
    if (!mFindReplaceDialog) {
        return;
    }
    const DADialogFindReplace::FindOptions opt = mFindReplaceDialog->options();
    if (opt.keyword.isEmpty()) {
        return;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    if (!isTableEditable()) {
        daWarning << tr("This data source is read-only and cannot be modified.");  // cn:该数据源为只读，无法修改
        return;
    }
    // 解析替换范围：全部替换 → 扫描范围；替换当前 → 当前命中单格
    std::optional< QPair< int, int > > rowRange;
    std::optional< QPair< int, int > > colRange;
    if (all) {
        rowRange = findScanRowRange(opt.selectionOnly);
        colRange = findScanColRange(opt.selectionOnly);
    } else {
        if (mFindHitIndex < 0 || mFindHitIndex >= mFindHits.size()) {
            // 无当前命中：先扫一次（如刚打开直接点替换）
            runFindScan();
            if (mFindHits.isEmpty()) {
                daInfo << tr("No matches found");  // cn:未找到匹配项
                return;
            }
        }
        const QPair< int, int > hit = mFindHits.at(mFindHitIndex);
        rowRange                    = qMakePair(hit.first, hit.first);
        colRange                    = qMakePair(hit.second, hit.second);
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    // 预扫变更格数（与替换同一匹配语义，命中总数=将变更格数）
    int pendingCount = 0;
    {
        DAWaitCursorScoped waitCursor;
        const DAPyScriptsDataFrame::SearchDataResult probe =
            py.searchData(df, opt.keyword, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange);
        if (!probe.ok) {
            mFindReplaceDialog->setSearchFailed(true);  // 无效正则等
            return;
        }
        pendingCount = probe.totalHits;
    }
    if (pendingCount <= 0) {
        daInfo << tr("No matches found");  // cn:未找到匹配项
        return;
    }
    bool undoable = true;
    if (pendingCount > c_replaceUndoCellLimit) {
        const auto btn = QMessageBox::question(this,
                                               tr("Replace All"),  // cn:全部替换
                                               tr("Too many cells will be changed (%1). This operation cannot be undone. Continue?")
                                                   .arg(pendingCount),  // cn:改动量过大（%1 格），本次替换将不可撤销，是否继续？
                                               QMessageBox::Yes | QMessageBox::No,
                                               QMessageBox::No);
        if (btn != QMessageBox::Yes) {
            return;
        }
        undoable = false;
    }
    QVector< int > rows;
    QVector< int > cols;
    pybind11::list oldValues;
    pybind11::list newValues;
    bool okReplace = false;
    {
        DAWaitCursorScoped waitCursor;
        if (undoable) {
            okReplace = py.findReplace(df, opt.keyword, opt.replaceText, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange, &rows, &cols, &oldValues, &newValues);
        } else {
            // 降级路径：只执行替换，不记录清单（不可撤销、不入栈）
            okReplace = py.findReplace(df, opt.keyword, opt.replaceText, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange);
        }
    }
    if (!okReplace) {
        mFindReplaceDialog->setSearchFailed(true);
        return;
    }
    if (!undoable) {
        notifyReplaceChanged(QVector< int >(), QVector< int >());  // 未记录清单 → 全表刷新
        Q_EMIT replaceApplied(pendingCount);
        return;
    }
    if (rows.isEmpty()) {
        daInfo << tr("No matches found");  // cn:未找到匹配项
        return;
    }
    // 正常路径：数据已被 findReplace 就地替换，构造命令持有变更清单后 push
    //（DACommandWithRedoCount 首次 redo 跳过，不重复替换；undo/redo 经 da_write_cells 批量写回）
    std::unique_ptr< DACommandDataFrame_replace > cmd(new DACommandDataFrame_replace(df, rows, cols, oldValues, newValues));
    QPointer< DADataTableModel > modle    = mModel;
    DACommandDataFrame_replace* cmdRaw    = cmd.get();
    cmd->setCallBack([ modle, cmdRaw ]() {
        if (!modle) {
            return;
        }
        const QVector< int >& rs = cmdRaw->changedRows();
        const QVector< int >& cs = cmdRaw->changedCols();
        if (rs.isEmpty()) {
            return;
        }
        int r0 = rs.at(0), r1 = rs.at(0), c0 = cs.at(0), c1 = cs.at(0);
        for (int r : rs) {
            r0 = qMin(r0, r);
            r1 = qMax(r1, r);
        }
        for (int c : cs) {
            c0 = qMin(c0, c);
            c1 = qMax(c1, c);
        }
        modle->notifyDataChanged(r0, c0, r1, c1);
    });
    push(cmd.release());
    notifyReplaceChanged(rows, cols);
    Q_EMIT replaceApplied(rows.size());
}

/**
 * @brief 替换生效后刷新模型（按变更范围一次性通知，避免逐格）
 * @param rows 变更行（空 → 全表刷新，降级路径未记录清单时用）
 * @param cols 变更列
 */
void DADataOperateOfDataFrameWidget::notifyReplaceChanged(const QVector< int >& rows, const QVector< int >& cols)
{
    if (!mModel) {
        return;
    }
    if (rows.isEmpty()) {
        mModel->notifyDataChanged(0, 0, qMax(0, mModel->actualRowCount() - 1), qMax(0, mModel->columnCount() - 1));
    } else {
        int r0 = rows.at(0), r1 = rows.at(0), c0 = cols.at(0), c1 = cols.at(0);
        for (int r : rows) {
            r0 = qMin(r0, r);
            r1 = qMax(r1, r);
        }
        for (int c : cols) {
            c0 = qMin(c0, c);
            c1 = qMax(c1, c);
        }
        mModel->notifyDataChanged(r0, c0, r1, c1);
    }
    runFindScan();  // 替换后重新扫描刷新命中（§WP2 步骤 6）
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
