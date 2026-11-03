#include "Table/Controllers/DATableNavigationService.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Core/DATableSelectionService.h"
#include "Table/Controllers/DATableViewStateController.h"
#include "Table/Controllers/DATableColumnViewController.h"
// qt
#include <QDialog>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QStringList>
#include <QTableView>
#include <QWidget>
// DAGui
#include "Table/Models/DADataTableModel.h"
#include "Table/Views/DADataTableView.h"
#include "Table/Views/DATablePaneContainer.h"
#include "Table/Metadata/DATableViewStateStore.h"
#include "Table/Dialogs/DADialogGotoCell.h"
// DAData
#include "DADataPyDataFrame.h"
#include "numpy/DAPyDType.h"
#include "pandas/DAPyIndex.h"
// python 索引标签查询（findLabelLocations）
#include "DAPybind11QtCaster.hpp"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

namespace DA
{
//===================================================
// DATableNavigationService
//===================================================

DATableNavigationService::DATableNavigationService(const DATablePageContext& ctx,
                                                   const DATableSelectionService& sel,
                                                   DATableViewStateController* viewState,
                                                   DATableColumnViewController* columnView,
                                                   QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mViewState(viewState), mColumnView(columnView)
{
}

/**
 * @brief 打开定位对话框（007 T1.7）
 *
 * 懒建复用；setup 重置初值（不回显残留）；exec 模态，确定后解析请求。
 * 无数据页静默返回。
 */
void DATableNavigationService::showGotoDialog()
{
    DADataTableModel* model = mCtx.model();
    if (!model || mCtx.dataRowCount() <= 0 || mCtx.dataColumnCount() <= 0) {
        return;
    }
    if (!mGotoDialog) {
        mGotoDialog = new DADialogGotoCell(mCtx.hostWidget());
    }
    QStringList columnNames;  // 重名列保留重复项（不做去重）
    for (int c = 0; c < mCtx.dataColumnCount(); ++c) {
        columnNames.append(model->headerData(c, Qt::Horizontal).toString());
    }
    const QModelIndex cur        = mSel.currentOrSelectionTopLeftIndex();
    const int currentRowPosition = cur.isValid() ? mCtx.toActualRow(cur.row()) + 1 : 1;
    const QString currentColName = cur.isValid() ? model->headerData(cur.column(), Qt::Horizontal).toString() : QString();
    const bool hasHiddenColumns  = (mCtx.viewStateStore() && !mCtx.viewStateStore()->hiddenColumns().isEmpty());
    mGotoDialog->setup(mCtx.dataRowCount(), columnNames, currentRowPosition, currentColName, hasHiddenColumns);
    if (mGotoDialog->exec() != QDialog::Accepted) {
        return;
    }
    resolveGotoRequest(mGotoDialog->request());
}

/**
 * @brief 解析定位请求并调定位单入口（007 T1.7）
 *
 * 行解析 → 列解析 → 只调定位宿主桥（不自己写滚动/定位）。返回 false =
 * 未解析成功（已给用户可见提示、未改选区）。行列口径只用 dataRowCount()/
 * dataColumnCount()；currentIndex() 的行是滑动窗逻辑行，经 toActualRow 换算。
 * @param req 定位请求
 * @return 是否定位成功
 */
bool DATableNavigationService::resolveGotoRequest(const DADialogGotoCell::Request& req)
{
    DADataTableModel* model = mCtx.model();
    if (!model || mCtx.dataRowCount() <= 0 || mCtx.dataColumnCount() <= 0) {
        return false;
    }
    // ① 行解析
    int actualRow = 0;
    switch (req.rowKind) {
    case DADialogGotoCell::Request::RowKind::FirstRow:
        actualRow = 0;
        break;
    case DADialogGotoCell::Request::RowKind::LastRow:
        actualRow = mCtx.dataRowCount() - 1;
        break;
    case DADialogGotoCell::Request::RowKind::Position:
        if (req.rowPosition < 1 || req.rowPosition > mCtx.dataRowCount()) {
            daWarning << tr("Row number is out of range");  // cn:行号超出范围
            return false;
        }
        actualRow = req.rowPosition - 1;
        break;
    case DADialogGotoCell::Request::RowKind::None: {
        // 保持当前行（当前项无效 → 第 0 行）
        const QModelIndex cur = mSel.currentOrSelectionTopLeftIndex();
        actualRow = cur.isValid() ? mCtx.toActualRow(cur.row()) : 0;
        break;
    }
    case DADialogGotoCell::Request::RowKind::IndexLabel: {
        DAPyIndex index                    = mCtx.df().index();
        DAPyIndex::IndexLabelLocations loc = index.findLabelLocations(DA::PY::toPyObject(req.indexLabel));
        if (!loc.found) {  // 数值索引：字符串→整数/浮点再试一次（同一个查询路径）
            bool ok = false;
            const long long asInt = req.indexLabel.toLongLong(&ok);
            if (ok) {
                loc = index.findLabelLocations(pybind11::int_(asInt));
            } else {
                const double asDouble = req.indexLabel.toDouble(&ok);
                if (ok) {
                    loc = index.findLabelLocations(pybind11::float_(asDouble));
                }
            }
        }
        if (!loc.found) {
            daWarning << tr("Index label not found: %1").arg(req.indexLabel);  // cn:未找到索引标签：%1
            return false;
        }
        if (loc.count > 1) {
            daInfo << tr("There are %1 rows with this index label; jumped to the first one").arg(loc.count);  // cn:有 %1 行使用该索引标签，已跳到第一个
        }
        actualRow = static_cast< int >(loc.firstPos);
        break;
    }
    }
    // ② 列解析
    int col = 0;
    switch (req.columnKind) {
    case DADialogGotoCell::Request::ColumnKind::FirstColumn:
        col = 0;
        break;
    case DADialogGotoCell::Request::ColumnKind::LastColumn:
        col = mCtx.dataColumnCount() - 1;
        break;
    case DADialogGotoCell::Request::ColumnKind::Name: {
        int found    = -1;  // 重名取第一个（数据列序）
        int nameHits = 0;
        for (int c = 0; c < mCtx.dataColumnCount(); ++c) {
            if (model->headerData(c, Qt::Horizontal).toString() == req.columnName) {
                if (found < 0) {
                    found = c;
                }
                ++nameHits;
            }
        }
        if (found < 0) {
            daWarning << tr("Column not found: %1").arg(req.columnName);  // cn:未找到列名：%1
            return false;
        }
        if (nameHits > 1) {
            daInfo << tr("There are %1 columns with this name; used the first one").arg(nameHits);  // cn:有 %1 个同名列，已取第一个
        }
        col = found;
        break;
    }
    case DADialogGotoCell::Request::ColumnKind::None: {
        // 保持当前列（当前项无效 → 第 0 列）
        const QModelIndex cur = mSel.currentOrSelectionTopLeftIndex();
        col = cur.isValid() ? cur.column() : 0;
        break;
    }
    }
    // ③ 收尾：定位单入口（keepHidden 透传）
    locateActualCell(actualRow, col, /*select=*/true, /*keepHidden=*/req.keepHidden);
    return true;
}

/**
 * @brief 确保整列可见并滚动到位
 *
 * 隐藏列收口（007）：图表拾取/表头右键过来的"确保整列可见"若落在隐藏列上，
 * 先走列视图的不入栈入口取消隐藏（occ = 0，第一同名）+ 提示，再滚动/整列选中。
 * @param colName 列名
 * @param selectCol true = 整列选中
 */
void DATableNavigationService::ensureColumnVisible(const QString& colName, bool selectCol)
{
    if (colName.isEmpty()) {
        return;
    }
    QTableView* tv = mCtx.mainView();
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

    if (mCtx.viewStateStore() && !colName.isEmpty()) {
        // 隐藏列收口（007）：不处理会静默失效——先取消隐藏再整列选中
        if (mCtx.viewStateStore()->isColumnHidden(colName, 0)) {
            if (mColumnView) {
                mColumnView->setColumnHiddenDirectly(colName, 0, false);
            }
            daInfo << tr("The column \"%1\" was hidden and has been made visible").arg(colName);  // cn:列"%1"已隐藏，现已显示（与 locateActualCell 复用同一句串，不造第二份）
        }
    }

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

/**
 * @brief 定位单入口（007）：跳到「数据行 actualRow + 列 col」并选中
 *
 * 四步语义（顺序不可颠倒）：①越界夹取（无数据直接返回）→ ②隐藏列处置
 * （只有「要选中且不要求保持隐藏」才取消隐藏 + 提示，不入撤销栈）→ ③纵向一律
 * 主窗格 showActualRow（冻结窗格绝不调——placeViewportAt 会改模型窗口起始行，
 * 与主窗格抢窗口）→ ④横向按冻结区分流。
 * 只读：不写数据、不入撤销栈、不写工程；唯一例外是取消隐藏。
 * @param actualRow 数据源绝对行号（越界自动夹到数据范围）
 * @param col 列号
 * @param select false = 只摆位 + 移当前项（选区范围导航不收缩选区、不夺焦点）
 * @param keepHidden true = 保留目标列的隐藏状态（对话框勾选项）
 */
void DATableNavigationService::locateActualCell(int actualRow, int col, bool select, bool keepHidden)
{
    // ① 越界夹取：无数据直接返回（用户可见提示走对话框侧，见 007b）
    const int rows = mCtx.dataRowCount();
    const int cols = mCtx.dataColumnCount();
    if (rows <= 0 || cols <= 0) {
        return;
    }
    actualRow = qBound(0, actualRow, rows - 1);
    col       = qBound(0, col, cols - 1);

    DADataTableView* mainView = mCtx.mainView();  // 006 约定 = 主窗格（模型窗口与滚动位置的唯一驱动者）
    if (!mainView || !mainView->model()) {
        return;
    }

    // ② 隐藏列处置：只有「要选中、且不要求保持隐藏」的定位才取消隐藏
    //（select=false 的查找选区导航、keepHidden=true 的对话框勾选项都保持原可见性）
    if (select && !keepHidden && mCtx.viewStateStore()) {
        const QString name = mainView->actualColumnName(col);
        if (!name.isEmpty()) {  // 空列名（尾部预留空白列）无身份，先判掉再查 store
            int occ = 0;        // occurrence = 该列之前同名列个数（数据列序中计，
            for (int c = 0; c < col; ++c) {  // 与列宽采集同法，不要按视图顺序算）
                if (mainView->model()->headerData(c, Qt::Horizontal).toString() == name) {
                    ++occ;
                }
            }
            if (mCtx.viewStateStore()->isColumnHidden(name, occ)) {
                if (mColumnView) {
                    mColumnView->setColumnHiddenDirectly(name, occ, false);  // 列视图的不入栈入口：写真源 + 统一重投影
                }
                daInfo << tr("The column \"%1\" was hidden and has been made visible").arg(name);  // cn:列"%1"已隐藏，现已显示
            }
        }
    }

    // ③ 纵向一律走主窗格：showActualRow = placeViewportAt（目标行落到视口首行，贴数据两端时收紧）
    mainView->showActualRow(actualRow);

    // ④ 横向：冻结区判定用视图状态控制器（真源唯一，不要自己重算）；
    //    判定放在 ② 之后——取消隐藏会改变"视图顺序前 m 个可见列"，冻结集合随之变
    const bool frozen = mViewState && mViewState->isColumnInFrozenArea(col);
    if (frozen) {
        // 冻结区：不调 showColumn。QTableView::showColumn = horizontalHeader()->showSection()，
        // 会静默解除该列在主窗格的隐藏（绕开真源与投影）→ 手动三步：摆位（已做）→
        // 共享 selectionModel 的 setCurrentIndex（flags 随 select）→ 焦点给包含该列的窗格
        //（设计稿 §3.4.4：焦点决定 Ctrl+C/编辑在哪个窗格生效）
        mainView->selectionModel()->setCurrentIndex(
            mainView->model()->index(mainView->toLogicalRow(actualRow), col),
            select ? QItemSelectionModel::ClearAndSelect : QItemSelectionModel::NoUpdate);
        if (select) {
            if (DATablePaneContainer* c = mCtx.paneContainer()) {
                if (DADataTableView* fv = c->frozenView()) {
                    fv->setFocus();
                }
            }
        }
    } else if (select) {
        // 主区：复用既有语义（内部 showActualRow + showColumn + setCurrentIndex(ClearAndSelect)）；
        // 此分支成立的前提是 ② 已把列解除隐藏。
        // ⚠️ 已知边界：keepHidden=true 且列既隐藏又不在冻结区时，这里的 showColumn 会把
        // 该列重新显示——勾选项的语义是"别主动解隐藏"，而主区定位本身要求列可见；
        // v1 接受这个结果，不要为此在主区分支手工改写可见性（那会变成第二条真源路径）
        mainView->selectActualCell(actualRow, col);
    } else {
        // select=false（查找的"选区范围"导航）：不调 showColumn、不收缩选区，只移当前项；
        // 焦点不动（与既有查找选区范围分支一致，查找条继续响应上下导航键）
        mainView->selectionModel()->setCurrentIndex(
            mainView->model()->index(mainView->toLogicalRow(actualRow), col),
            QItemSelectionModel::NoUpdate);
    }
}

/**
 * @brief 定位单入口的列名重载：重名取第一个同名列（数据列序）
 *
 * 找不到列名时静默返回——用户可见提示归对话框路径（007b 的 resolveGotoRequest）。
 * @param actualRow 数据源绝对行号
 * @param columnName 列名
 * @param select 同列号重载
 * @param keepHidden 同列号重载
 */
void DATableNavigationService::locateActualCell(int actualRow, const QString& columnName, bool select, bool keepHidden)
{
    DADataTableModel* model = mCtx.model();
    if (columnName.isEmpty() || !model) {
        return;
    }
    int col = -1;  // 重名取第一个同名列（数据列序）
    for (int c = 0; c < mCtx.dataColumnCount(); ++c) {
        if (model->headerData(c, Qt::Horizontal).toString() == columnName) {
            col = c;
            break;
        }
    }
    if (col < 0) {
        return;
    }
    locateActualCell(actualRow, col, select, keepHidden);
}

/**
 * @brief 表体点击分发（类型上报）
 *
 * 非 dataframe / 越界列上报空类型；正常列查 dtypeObject 上报
 * （页面转发 selectTypeChanged → APP 同步 ribbon 类型显示）。
 * @param index 点击的模型索引
 */
void DATableNavigationService::onTableViewClicked(const QModelIndex& index)
{
    if (!mCtx.data().isDataFrame()) {
        Q_EMIT selectTypeChanged({ index.column() }, DAPyDType());
        return;
    }
    DAPyDataFrame df = mCtx.df();
    if (index.column() >= (int)df.shape().second) {
        Q_EMIT selectTypeChanged({ index.column() }, DAPyDType());
        return;
    }
    DAPyDType t;
    try {
        t = df.dtypeObject(index.column());
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }

    Q_EMIT selectTypeChanged({ index.column() }, t);
}

/**
 * @brief 表头带修饰键点击（2026-10-18 交互修订：点击表头不排序）
 *
 * 误触即整表重排、大表卡顿数秒，排序入口收敛到列头右键菜单。本槽仅保留
 * 拾取模式转发：拾取模式 → columnHeaderClicked（图表「选择序列」拾取链路
 * 零回归）；非拾取模式：不做任何事（Qt 默认的列选中/高亮行为保留）。
 * @param logicalIndex 列逻辑索引
 * @param modifiers 修饰键（不吞，拾取侧按列填充表达式）
 */
void DATableNavigationService::onHeaderClickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers);
    if (mCtx.isHeaderPickMode && mCtx.isHeaderPickMode()) {
        // 拾取模式：只转发信号
        Q_EMIT columnHeaderClicked(logicalIndex);
    }
}
}  // end of namespace DA
