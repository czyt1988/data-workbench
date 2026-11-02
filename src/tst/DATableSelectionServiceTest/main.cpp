#include <QtTest/QtTest>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QSet>
#include <algorithm>
#include <cstdlib>

#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include <pybind11/eval.h>

#include "DAData.h"
#include "Models/DADataTableModel.h"
#include "DADataTableView.h"
#include "DATablePaneContainer.h"
#include "DATableMetadataRegistry.h"
#include "DADataOperateOfDataFrameWidget.h"
#include <QUndoStack>
#include <QItemSelection>
#include <QItemSelectionModel>

// 表格页核心（008 期1）
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

//===================================================
// 测试辅助
//===================================================

// 构造一个 pandas 背书的 DAData（6 行 × 3 列：cat/val/val2）
static DA::DAData makePandasData()
{
    pybind11::exec(R"(
import pandas as pd
_da_selection_df = pd.DataFrame({
    'cat': ['a', 'b', 'a', 'c', 'b', 'a'],
    'val': [1.0, 2.0, 3.0, 4.0, 5.0, 6.0],
    'val2': [2.0, 4.0, 6.0, 8.0, 10.0, 12.0],
})
)");
    pybind11::object obj = pybind11::globals()["_da_selection_df"];
    return DA::DAData(obj);
}

// 大表（rows 行 × 3 列）：验证滑动窗行号换算用
static DA::DAData makePandasDataFrame(int rows)
{
    pybind11::exec(qPrintable(QStringLiteral(R"(
import pandas as pd
_rows = %1
_da_selection_big_df = pd.DataFrame({
    'a': range(_rows),
    'b': range(_rows),
    'c': [x % 3 for x in range(_rows)],
})
)").arg(rows)));
    pybind11::object obj = pybind11::globals()["_da_selection_big_df"];
    return DA::DAData(obj);
}

// QList<int> → QSet<int>（选区方法走 QSet::values() 顺序不保证，比对用集合）
static QSet< int > toSet(const QList< int >& l)
{
    return QSet< int >(l.begin(), l.end());
}

// 独立上下文装配（008 期1 的验收点：只构造模型 + 视图 + 上下文，不建整页）
struct StandaloneTable
{
    QUndoStack stack;
    DA::DADataTableModel model;
    DA::DADataTableView view;
    DA::DAData data;
    DA::DATablePageContext ctx;
    DA::DATableSelectionService sel;
    explicit StandaloneTable(const DA::DAData& d) : model(&stack), data(d), sel(ctx)
    {
        model.setData(data);
        view.setModel(&model);
        ctx.setModel(&model);
        ctx.setMainView(&view);
        ctx.setData(data);
    }
};

// 整页路径的程序化选区（照 DATableQuickPlotTest 的 selectColumnRange）
static bool selectColumnRange(DA::DADataOperateOfDataFrameWidget& page, int firstCol, int lastCol)
{
    DA::DADataTableView* view = page.getDataTableView();
    if (!view || !view->model()) {
        return false;
    }
    QAbstractItemModel* model = view->model();
    if (model->rowCount() <= 0 || lastCol >= model->columnCount()) {
        return false;
    }
    const QItemSelection sel(model->index(0, firstCol), model->index(model->rowCount() - 1, lastCol));
    page.tableSelectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
    return page.isDataframeTableHaveSelection();
}

//===================================================
// DATableSelectionServiceTest
//===================================================

/**
 * @brief 表格页选区服务测试（008 期1）
 *
 * 覆盖五类边界口径：尾部预留列 / 隐藏列 / 冻结窗格 / 多段选区 / 重叠选区；
 * 同时覆盖两条构造路径：独立上下文（模型+视图，不建整页）与整页 Facade 转发。
 */
class DATableSelectionServiceTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    // 独立上下文
    void testStandaloneContextBasics();
    void testTrailingReservedColumns();
    void testSlidingWindowRowMapping();
    void testMultiSegmentSelection();
    void testOverlappingSelection();
    // 整页（Facade 转发）
    void testHiddenColumnsViewOrder();
    void testFrozenPaneSharedSelection();

private:
    bool mPandasOk = false;
};

void DATableSelectionServiceTest::initTestCase()
{
    // 与 APP main.cpp 的初始化逻辑一致：解析解释器路径并设置 Python Home
    QString pythonHomePath;
    const QList< QFileInfo > candidates = DA::DAPyInterpreter::wherePython();
    for (const QFileInfo& fi : candidates) {
        QDir homeDir = fi.absoluteDir();
        if (homeDir.exists(QStringLiteral("Lib/site-packages"))) {
            pythonHomePath = homeDir.absolutePath();
            break;
        }
    }
    if (pythonHomePath.isEmpty()) {
        QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
        if (!pypath.isEmpty()) {
            pythonHomePath = QFileInfo(pypath).absolutePath();
        }
    }
    DA::DAPyInterpreter::initializePythonInterpreter(pythonHomePath);
    try {
        pybind11::module::import("pandas");
        mPandasOk = true;
    } catch (const std::exception& e) {
        qWarning() << "pandas not available:" << e.what();
        mPandasOk = false;
    }
}

/**
 * @brief 独立上下文：数据行/列计数、无选区基线、基本选区读取
 */
void DATableSelectionServiceTest::testStandaloneContextBasics()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    StandaloneTable t(makePandasData());
    QCOMPARE(t.ctx.dataRowCount(), 6);
    QCOMPARE(t.ctx.dataColumnCount(), 3);
    QCOMPARE(t.ctx.toActualRow(3), 3);  // 未滚动：视图行 = 绝对行

    // 无选区基线
    QVERIFY(!t.sel.haveSelection());
    QCOMPARE(t.sel.selectionSegmentCount(), 0);
    int rb = -1, re = -1, cb = -1, ce = -1;
    QVERIFY(!t.sel.selectionBoundingBox(&rb, &re, &cb, &ce));
    QCOMPARE(t.sel.selectedCellCount(), 0);
    QVERIFY(!t.sel.currentActualCell(&rb, &cb));
    QVERIFY(t.sel.selectedDataframeColumns().isEmpty());
    QVERIFY(t.sel.selectedDataframeRows().isEmpty());
    QVERIFY(t.sel.selectedDataframeCells().isEmpty());
    QVERIFY(!t.sel.currentOrSelectionTopLeftIndex().isValid());
    QVERIFY(t.sel.currentSingleCellDisplayText().isEmpty());

    // 选中 3 行 × 2 列
    QItemSelectionModel* sm = t.sel.selectionModel();
    QVERIFY(sm);
    const QModelIndex tl = t.model.index(0, 0);
    const QModelIndex br = t.model.index(2, 1);
    const QModelIndex cur = t.model.index(1, 1);
    sm->select(QItemSelection(tl, br), QItemSelectionModel::ClearAndSelect);
    sm->setCurrentIndex(cur, QItemSelectionModel::NoUpdate);

    QVERIFY(t.sel.haveSelection());
    QCOMPARE(toSet(t.sel.selectedDataframeColumns()), QSet< int >({ 0, 1 }));
    QCOMPARE(toSet(t.sel.selectedDataframeRows()), QSet< int >({ 0, 1, 2 }));
    QCOMPARE(t.sel.selectedCellCount(), 6);
    QCOMPARE(t.sel.selectionSegmentCount(), 1);
    QVERIFY(t.sel.selectionBoundingBox(&rb, &re, &cb, &ce));
    QCOMPARE(rb, 0);
    QCOMPARE(re, 2);
    QCOMPARE(cb, 0);
    QCOMPARE(ce, 1);
    QVERIFY(t.sel.currentActualCell(&rb, &cb));
    QCOMPARE(rb, 1);
    QCOMPARE(cb, 1);
    QVERIFY(t.sel.currentOrSelectionTopLeftIndex() == cur);  // currentIndex 有效时优先返回当前格

    // 身份表（(列名, occurrence) 口径）
    const QList< QPair< QString, int > > idents = t.sel.actualColumnIdentities();
    QCOMPARE(idents.size(), 3);
    QVERIFY(idents.at(0) == qMakePair(QStringLiteral("cat"), 0));
    QVERIFY(idents.at(1) == qMakePair(QStringLiteral("val"), 0));
    QVERIFY(idents.at(2) == qMakePair(QStringLiteral("val2"), 0));

    // 选区按列分组的连续行区间
    const QHash< int, QVector< QPair< int, int > > > runs = t.sel.selectedColumnRowRuns();
    QCOMPARE(runs.size(), 2);
    const QVector< QPair< int, int > > expectedColRun = { qMakePair(0, 2) };
    QVERIFY(runs.value(0) == expectedColRun);
    QVERIFY(runs.value(1) == expectedColRun);
}

/**
 * @brief 尾部预留列：ensureInDataframe=true 剔除超出数据列范围的列
 */
void DATableSelectionServiceTest::testTrailingReservedColumns()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    StandaloneTable t(makePandasData());
    t.model.setExtraColumnCount(2);
    QCOMPARE(t.model.columnCount(), 5);       // 数据列 3 + 预留列 2
    QCOMPARE(t.ctx.dataColumnCount(), 3);     // 真实数据列不含预留列

    QItemSelectionModel* sm = t.sel.selectionModel();
    QVERIFY(sm);
    // 全模型选区（含预留列 3、4）
    const QItemSelection sel(t.model.index(0, 0), t.model.index(t.model.rowCount() - 1, 4));
    sm->select(sel, QItemSelectionModel::ClearAndSelect);

    QCOMPARE(toSet(t.sel.selectedDataframeColumns(true)), QSet< int >({ 0, 1, 2 }));
    QCOMPARE(toSet(t.sel.selectedDataframeColumns(false)), QSet< int >({ 0, 1, 2, 3, 4 }));
    QCOMPARE(t.sel.oneSelectedDataframeColumn(true), 0);
    // 完全选中列：数据列 3 个（预留列按 ensure 剔除/保留）
    QCOMPARE(toSet(t.sel.fullySelectedDataframeColumns(true)), QSet< int >({ 0, 1, 2 }));
    QCOMPARE(toSet(t.sel.fullySelectedDataframeColumns(false)), QSet< int >({ 0, 1, 2, 3, 4 }));
    // 单元格按数据列范围过滤
    const QList< QPoint > cells = t.sel.selectedDataframeCells(true);
    QVERIFY(!cells.isEmpty());
    for (const QPoint& p : cells) {
        QVERIFY(p.y() < 3);
    }
}

/**
 * @brief 滑动窗行号换算：视图行 → 数据源绝对行（唯一换算点）
 */
void DATableSelectionServiceTest::testSlidingWindowRowMapping()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    const int rows = 1200;
    StandaloneTable t(makePandasDataFrame(rows));
    QCOMPARE(t.ctx.dataRowCount(), rows);
    t.model.setCacheWindowSize(200);          // 默认窗口 2 万行会覆盖全表，显式缩小验证滑动窗
    QVERIFY(t.model.rowCount() < rows);       // 滑动窗截断

    // 滚动窗口到中部
    t.model.setCacheWindowStartRow(600);
    QCOMPARE(t.ctx.toActualRow(0), 600);
    QCOMPARE(t.ctx.toActualRow(7), 607);

    QItemSelectionModel* sm = t.sel.selectionModel();
    QVERIFY(sm);
    // 选中窗口内视图行 0..4（→ 绝对行 600..604），列 0
    const QItemSelection sel(t.model.index(0, 0), t.model.index(4, 0));
    sm->select(sel, QItemSelectionModel::ClearAndSelect);

    QCOMPARE(toSet(t.sel.selectedDataframeRows(true)), QSet< int >({ 600, 601, 602, 603, 604 }));
    QCOMPARE(t.sel.oneSelectedDataframeRow(true), 600);
    int rb = -1, re = -1, cb = -1, ce = -1;
    QVERIFY(t.sel.selectionBoundingBox(&rb, &re, &cb, &ce));
    QCOMPARE(rb, 600);
    QCOMPARE(re, 604);
}

/**
 * @brief 多段选区：段数/几何计数/包围盒/行列去重
 */
void DATableSelectionServiceTest::testMultiSegmentSelection()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    StandaloneTable t(makePandasData());
    QItemSelectionModel* sm = t.sel.selectionModel();
    QVERIFY(sm);

    // 两段：A(0,0)-(1,1) 2×2；B(4,2)-(5,2) 2×1
    QItemSelection sel;
    sel.select(t.model.index(0, 0), t.model.index(1, 1));
    sel.select(t.model.index(4, 2), t.model.index(5, 2));
    sm->select(sel, QItemSelectionModel::ClearAndSelect);

    QCOMPARE(t.sel.selectionSegmentCount(), 2);
    QCOMPARE(t.sel.selectedCellCount(), 2 * 2 + 2 * 1);  // 6，几何计数不重复
    QCOMPARE(toSet(t.sel.selectedDataframeColumns()), QSet< int >({ 0, 1, 2 }));
    QCOMPARE(toSet(t.sel.selectedDataframeRows()), QSet< int >({ 0, 1, 4, 5 }));
    QCOMPARE(t.sel.selectedDataframeCells().size(), 6);
    int rb = -1, re = -1, cb = -1, ce = -1;
    QVERIFY(t.sel.selectionBoundingBox(&rb, &re, &cb, &ce));
    QCOMPARE(rb, 0);
    QCOMPARE(re, 5);
    QCOMPARE(cb, 0);
    QCOMPARE(ce, 2);

    // 同列两段不连续行 → 两个 run
    QItemSelection selRuns;
    selRuns.select(t.model.index(0, 0), t.model.index(1, 0));
    selRuns.select(t.model.index(4, 0), t.model.index(5, 0));
    sm->select(selRuns, QItemSelectionModel::ClearAndSelect);
    const QHash< int, QVector< QPair< int, int > > > runs = t.sel.selectedColumnRowRuns();
    QCOMPARE(runs.size(), 1);
    const QVector< QPair< int, int > > expectedRuns = { qMakePair(0, 1), qMakePair(4, 5) };
    QVERIFY(runs.value(0) == expectedRuns);
}

/**
 * @brief 重叠选区：合并覆盖后不重复计数
 */
void DATableSelectionServiceTest::testOverlappingSelection()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    StandaloneTable t(makePandasData());
    QItemSelectionModel* sm = t.sel.selectionModel();
    QVERIFY(sm);

    // 两个重叠区间的累加选择（第二次 Select 与当前选区合并为互不重叠区间）：
    // A(0,0)-(3,1) 4×2=8；B(2,0)-(5,2) 4×3=12；重叠 (2..3)×(0..1)=4
    // → 合并后并集 8 + 12 - 4 = 16 格（行 0..1 只覆盖列 0..1，行 2..5 覆盖全 3 列）
    QItemSelection selA;
    selA.select(t.model.index(0, 0), t.model.index(3, 1));
    sm->select(selA, QItemSelectionModel::ClearAndSelect);
    QItemSelection selB;
    selB.select(t.model.index(2, 0), t.model.index(5, 2));
    sm->select(selB, QItemSelectionModel::Select);

    QCOMPARE(t.sel.selectedCellCount(), 16);  // 并集不重复计数
    QCOMPARE(toSet(t.sel.selectedDataframeRows()), QSet< int >({ 0, 1, 2, 3, 4, 5 }));
    QCOMPARE(toSet(t.sel.selectedDataframeColumns()), QSet< int >({ 0, 1, 2 }));
    QCOMPARE(t.sel.selectedDataframeCells().size(), 16);  // selectedIndexes 本身去重

    // 重叠选区下的整行/整列判定：独立模型 columnCount = max(数据列 3 + 预留 1, 最小显示 4) = 4，
    // 选区只覆盖列 0..2 → 整行均不成立；列 0/1 覆盖全部 6 行 → 整列成立，列 2 只覆盖 4 行
    QCOMPARE(toSet(t.sel.fullySelectedDataframeRows()), QSet< int >());
    QCOMPARE(toSet(t.sel.fullySelectedDataframeColumns()), QSet< int >({ 0, 1 }));
}

/**
 * @brief 隐藏列：有序选区按视图状态跳过隐藏列（整页 Facade 转发路径）
 */
void DATableSelectionServiceTest::testHiddenColumnsViewOrder()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    DA::DATableMetadataRegistry registry(nullptr);
    const DA::DAData data = makePandasData();
    DA::DADataOperateOfDataFrameWidget page(data, &registry);
    page.resize(900, 520);
    page.show();
    QTest::qWait(30);

    // 隐藏首列 cat（006 不入栈入口：写 store + 重投影）
    page.setColumnHiddenDirectly(QStringLiteral("cat"), 0, true);

    // 程序化全选三列（选区含隐藏列 0，视图顺序应为 [1, 2]）
    QVERIFY(selectColumnRange(page, 0, 2));
    const QList< int > inOrder = page.getSelectedDataColumnsInViewOrder();
    QCOMPARE(inOrder, QList< int >({ 1, 2 }));  // 隐藏列跳过 + 视图顺序稳定

    // 无序口径仍含隐藏列（不过滤可见性）
    QCOMPARE(toSet(page.getSelectedDataframeCoumns(true)), QSet< int >({ 0, 1, 2 }));
}

/**
 * @brief 冻结窗格：共享 selectionModel，选区经服务读取与主窗格一致
 */
void DATableSelectionServiceTest::testFrozenPaneSharedSelection()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    DA::DATableMetadataRegistry registry(nullptr);
    const DA::DAData data = makePandasData();
    DA::DADataOperateOfDataFrameWidget page(data, &registry);
    page.resize(900, 520);
    page.show();
    QTest::qWait(30);

    // 冻结首列 → 冻结窗格懒建
    page.setColumnFrozen(0, true);
    DA::DATablePaneContainer* c = page.paneContainer();
    QVERIFY(c);
    DA::DADataTableView* frozen = c->frozenView();
    QVERIFY(frozen);
    // 两窗格共享同一个 selectionModel（服务只读主窗格的前提）
    QCOMPARE(frozen->selectionModel(), page.tableSelectionModel());

    // 经冻结窗格的选择（模拟用户在冻结区点选 0..2 行）
    QAbstractItemModel* m = frozen->model();
    QVERIFY(m);
    const QItemSelection sel(m->index(0, 0), m->index(2, 0));
    page.tableSelectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);

    QVERIFY(page.isDataframeTableHaveSelection());
    QCOMPARE(page.selectionSegmentCount(), 1);
    QCOMPARE(toSet(page.getSelectedDataframeCoumns(true)), QSet< int >({ 0 }));
    QCOMPARE(toSet(page.getSelectedDataframeRows(true)), QSet< int >({ 0, 1, 2 }));
    QCOMPARE(page.selectedCellCount(), 3);
    // 只选中列 0 的部分行 → 整列/整行均不成立
    QVERIFY(page.getFullySelectedDataframeColumns(true).isEmpty());
    QVERIFY(page.getFullySelectedDataframeRows(true).isEmpty());
}

/**
 * @brief 测试入口
 *
 * 本测试会初始化内嵌 Python 并创建 pandas DataFrame。进程退出时解释器析构在
 * 测试环境下会挂住（与 DATableQuickPlotTest 的退出阶段问题同源），因此在 QtTest
 * 报告输出完成后直接结束进程，跳过静态析构，保证 ctest 正常结束。
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DATableSelectionServiceTest tc;
    const int rc = QTest::qExec(&tc, argc, argv);
    std::fflush(stdout);
    std::fflush(stderr);
#ifdef Q_OS_WIN
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(rc));
#else
    std::_Exit(rc);
#endif
    return rc;  // 不可达
}

#include "main.moc"
