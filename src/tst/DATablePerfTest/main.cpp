// DATablePerfTest —— 表格性能与防劣化用例（2026-11 从 DATableDataTest 拆出）
//
// 拆出原因：这些用例使用 10 万 / 100 万行夹具，单跑数十秒到数分钟，
// testSelectAllLargeSelectionPerf 更会撞穿 Qt Test 的 5 分钟看门狗，
// 导致整个默认测试套件必然为红、并拖慢每一次"跑表格测试"。
// 现在由 ctest label `bench` 标记，默认套件不跑；需要时：
//     .\scripts\test.ps1 -Suite table -Bench
// 或   ctest -L bench
//
// 夹具与本套件同处一文件（少量重复），避免默认套件为共享代码引入耦合。

#include <QtTest/QtTest>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <cstdlib>
#include <QDataStream>
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QVariantList>
#include <QMetaType>

#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#include <pybind11/eval.h>

// DAPyScripts 算子封装（002：da_sort 多键置换 / da_reorder_rows / 查找替换）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "Commands/DACommandsDataFrame.h"
#include "Dialog/DADialogFindReplace.h"
#include <QElapsedTimer>
#include <QSet>
#include <QDate>
#include <QBrush>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMenu>
#include <QItemSelectionModel>
#include <QAbstractButton>

#include "DAData.h"
#include "DAAbstractData.h"
#include "DATableDataSource.h"
#include "DATableDataBlock.h"
#include "DATableScan.h"
#include "DADataFactory.h"

// 表格元数据基础设施（001 计划 §4.1）
#include "DATableHighlightOverlay.h"
#include "DATableKeyRemap.h"
#include "DATableMetadata.h"
#include "DATableMetadataRegistry.h"
#include "DATableStyleManager.h"
#include "DATableViewStateStore.h"
#include "DATablePaneContainer.h"
#include "DATableStatsWidget.h"
#include "Commands/DACommandsTableColumnView.h"
#include "Dialog/DADialogTableColumnView.h"
#include "Dialog/DADialogGotoCell.h"
#include <QWindow>
#include <QScrollBar>
#include <QSpinBox>
#include "DADataHeaderView.h"
#include "DADataTableView.h"
#include "DADataOperateOfDataFrameWidget.h"
#include <QSignalSpy>
#include <QMenu>
#include <QHeaderView>
#include <QTest>

#include "Models/DADataTableModel.h"
#include <QUndoStack>

// 条件格式（003 计划 §4.1）
#include "DATableConditionalFormatRule.h"
#include "DATableConditionalFormatStore.h"
#include "Commands/DACommandsConditionalFormat.h"
#include "Dialog/DADialogConditionalFormat.h"
#include <QDomDocument>
#include <QStandardItemModel>
#include <QElapsedTimer>
#include <QLabel>
#include <QImage>

// 计算列（004 计划 §3，T1.23）
#include "DAComputedColumn.h"
#include "DATableComputedColumnStore.h"
#include "Dialog/DADialogComputedColumn.h"
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QListWidget>
#include <QListView>
#include <QTableWidget>

// 分列/合并列（004 计划 §4，T1.9）
#include "Dialog/DADialogSplitColumn.h"
#include "Dialog/DADialogMergeColumns.h"
#include "Dialog/DADialogGroupBy.h"
#include "DADataManager.h"
#include <QRadioButton>
#include <QSpinBox>
#include <QItemSelectionModel>
#include <QItemSelection>

// 转置/melt 形状变换（004 计划 §6，T1.27）
#include "Dialog/DADialogReshape.h"
#include "DADataUndoCommand.h"
#include <QCheckBox>

// 填充（005 计划 §3，T1.24）
#include "Dialog/DADialogFillSeries.h"

// 数据校验（005 计划 §4，T1.25）
#include "DATableValidationRule.h"
#include "DATableValidationStore.h"
#include "Commands/DACommandsTableValidation.h"
#include "Dialog/DADialogValidationRules.h"
#include "Dialog/DADialogValidationReport.h"
#include "DATableStyleItemDelegate.h"
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <DATextEditWidget.h>

// 单元格批注（005 计划 §5，T1.26）
#include "DATableNoteStore.h"
#include "DATableNoteEditDialog.h"
#include "Commands/DACommandsTableStyle.h"
#include <QToolTip>
#include <QHelpEvent>
#include <QFile>

// 撤销历史面板（005 计划 §6，T1.21）
#include "DAUndoHistoryWidget.h"
#include <QUndoGroup>
#include <QUndoView>
#include <QSpinBox>

#include <QTemporaryDir>

#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 构造混合类型测试DataFrame（int64/float64含NaN/str/datetime64/bool，10行5列）
static pybind11::object makeTestDataFrame(){
    try {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({
    "i": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    "f": [0.5, float("nan"), 2.5, 3.5, float("nan"), 5.5, 6.5, 7.5, 8.5, 9.5],
    "s": ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"],
    "d": pd.to_datetime(["2020-01-01","2020-01-02","2020-01-03","2020-01-04","2020-01-05",
                          "2020-01-06","2020-01-07","2020-01-08","2020-01-09","2020-01-10"]),
    "b": [True, False, True, False, True, False, True, False, True, False],
})
)",
                       pybind11::globals(),
                       scope);
        return scope[ "df" ];
    } catch (const std::exception& e) {
        qWarning() << "makeTestDataFrame failed:" << e.what();
    }
    return pybind11::none();
}


// 条件格式相关的类型别名与夹具（与 DATableDataTest 中同名 anonymous namespace 保持一致）
namespace
{
using DA::DATableConditionalFormatRule;
using DA::DATableConditionalFormatStore;
using CfCond = DATableConditionalFormatRule::Condition;
using CfType = DATableConditionalFormatRule::Type;

DATableConditionalFormatRule makeBgRule(CfCond cond, const QVariant& v1, const QColor& bg, const QVariant& v2 = QVariant())
{
    DATableConditionalFormatRule r;
    r.type      = CfType::CellRule;
    r.condition = cond;
    r.value1    = v1;
    r.value2    = v2;
    r.style.setBackground(QBrush(bg));
    return r;
}
}  // namespace

//===================================================
// 测试类
//===================================================
class DATablePerfTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSortPerf100k();
    void testSortSelectionRemapPerf();
    void testConditionalScrollPerf1k();
    void testSplitColumnPerf100k();
    void testValidationPerf1M();
    void testBatch3FullChain100k();
    void testSelectAllLargeSelectionPerf();

private:
    bool mPandasOk    = false;
    bool mPyScriptsOk = false;
};

void DATablePerfTest::initTestCase()
{
    // 与APP main.cpp的初始化逻辑一致：解析解释器路径并设置Python Home，
    // 否则缺少PyConfig home时sys.path不含site-packages，pandas无法导入。
    // 候选中优先取存在Lib/site-packages的真实安装，排除Microsoft Store存根
    // （WindowsApps\python.exe是0字节重解析点，会导致sys.path损坏）
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
    // DAPyScripts 算子模块（002）：sys.path 指向本仓库 src/PyScripts（DAWorkbench 包源码位置），
    // 再初始化模块封装。DAWorkbench.DAPyBase 的 io/app_wrapper 依赖 app 注册的 da_app 绑定模块，
    // 测试环境缺失时按 _safe_import_submodule 降级为 None，不影响 dataframe 算子子模块
    if (mPandasOk) {
        const QString tstDir   = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath();
        const QString repoRoot = QDir::cleanPath(tstDir + QStringLiteral("/../../.."));
        DA::DAPyInterpreter::appendSysPath(repoRoot + QStringLiteral("/src/PyScripts"));
        // 测试环境没有 app 注册的 da_app/da_interface/da_data pybind 模块，
        // DAWorkbench.DAPyBase.io / app_wrapper 会导入失败 → DAWorkbench 顶层缺 io 属性 →
        // DAPyWorkBench::import 的 attr("io") 抛错连坐 dataframe。用空模块桩顶掉这三个
        // 绑定模块（io.py 只在函数体内使用 da_app.*，模块级 import 桩即可通过），
        // 让 dataframe 算子模块可用——测试只用 da_sort/da_search_data 等纯 pandas 算子
        try {
            pybind11::exec(R"(
import sys, types
for _m in ("da_app", "da_interface", "da_data"):
    if _m not in sys.modules:
        sys.modules[_m] = types.ModuleType(_m)
)");
        } catch (const std::exception& e) {
            qWarning() << "stub da_* modules failed:" << e.what();
        }
        mPyScriptsOk = DA::DAPyScripts::initScripts();
        if (!mPyScriptsOk) {
            qWarning() << "DAPyScripts::initScripts failed";
        }
    }
}


void DATablePerfTest::testSortPerf100k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
np.random.seed(123)
big = pd.DataFrame({"v": np.random.rand(100000), "w": np.random.randint(0, 100, 100000)})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "big" ]);
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    QVector< int > perm;
    QElapsedTimer timer;
    timer.start();
    QVERIFY(py.sort(df, { qMakePair(0, true) }, &perm));
    const qint64 elapsed = timer.elapsed();
    qDebug().noquote() << QStringLiteral("testSortPerf100k: 100k single-key sort elapsed_ms=%1").arg(elapsed);
    QCOMPARE(perm.size(), 100000);
    QVERIFY2(elapsed <= 300, qPrintable(QStringLiteral("100k sort took %1 ms (> 300 ms)").arg(elapsed)));
    // 抽查升序（首、中、尾）
    const QList< int > probes { 1, 1000, 50000, 99999 };
    for (int r : probes) {
        QVERIFY(df.iat(r - 1, 0).toDouble() <= df.iat(r, 0).toDouble());
    }
    // 双键排序也在预算内
    timer.restart();
    QVERIFY(py.sort(df, { qMakePair(1, true), qMakePair(0, false) }, &perm));
    const qint64 elapsed2 = timer.elapsed();
    qDebug().noquote() << QStringLiteral("testSortPerf100k: 100k two-key sort elapsed_ms=%1").arg(elapsed2);
    QVERIFY2(elapsed2 <= 300, qPrintable(QStringLiteral("100k two-key sort took %1 ms (> 300 ms)").arg(elapsed2)));
}

void DATablePerfTest::testSortSelectionRemapPerf()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    // ---- 正确性（小表 10 行）----
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    DA::DADataHeaderView* hh = qobject_cast< DA::DADataHeaderView* >(tv->horizontalHeader());
    QVERIFY(hh != nullptr);
    int colI = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model() && hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("i")) {
            colI = c;
            break;
        }
    }
    QVERIFY(colI >= 0);
    QAbstractItemModel* m  = tv->model();
    QItemSelectionModel* sm = tv->selectionModel();
    // 散落选中 i 列行 0/2/5（值 1/3/6）→ 降序（10..1）后旧行 r 的新位置 = 9-r
    QItemSelection sel;
    const QList< int > pickRows { 0, 2, 5 };
    for (int r : pickRows) {
        const QModelIndex ix = m->index(r, colI);
        sel.select(ix, ix);
    }
    sm->select(sel, QItemSelectionModel::ClearAndSelect);
    w.sortColumn(colI, Qt::DescendingOrder);
    QSet< int > mappedRows;
    const QModelIndexList after = sm->selectedIndexes();
    for (const QModelIndex& ix : after) {
        if (ix.column() == colI) {
            mappedRows.insert(tv->toActualRow(ix.row()));
        }
    }
    QCOMPARE(mappedRows.size(), 3);
    QVERIFY(mappedRows.contains(9));  // 值1 → 行9
    QVERIFY(mappedRows.contains(7));  // 值3 → 行7
    QVERIFY(mappedRows.contains(4));  // 值6 → 行4
    // 连续行段：当前为降序（10..1），选中视图行 2..5（值 8/7/6/5）→ 升序后落位行 4..7，连续 → 应归并为 1 个 range
    QItemSelection sel2;
    sel2.select(m->index(2, colI), m->index(5, colI));
    sm->select(sel2, QItemSelectionModel::ClearAndSelect);
    w.sortColumn(colI, Qt::AscendingOrder);
    QCOMPARE(sm->selection().size(), 1);
    const QItemSelectionRange only = sm->selection().at(0);
    QCOMPARE(only.top(), 4);
    QCOMPARE(only.bottom(), 7);
    QCOMPARE(only.left(), colI);
    QCOMPARE(only.right(), colI);
    w.hide();

    // ---- 性能（32560 行，复现用户现场规模）----
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
rng = np.random.default_rng(7)
N = 32560
big = pd.DataFrame({"v": rng.normal(size=N), "w": rng.integers(0, 1000, size=N)})
)",
                   scope);
    DA::DAPyDataFrame bdf(scope[ "big" ]);
    DA::DAData bd(bdf);
    DA::DATableMetadataRegistry reg2(nullptr);
    DA::DADataOperateOfDataFrameWidget wb(bd, &reg2);
    wb.resize(800, 400);
    wb.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wb));
    DA::DADataTableView* tvb = wb.getDataTableView();
    QVERIFY(tvb != nullptr);
    // 整列选择（每行一个 range，2 万格）——旧实现的卡死触发器
    tvb->selectColumn(0);
    QVERIFY(tvb->selectionModel()->selectedIndexes().size() > 10000);
    QElapsedTimer t;
    t.start();
    wb.sortColumn(0, Qt::DescendingOrder);
    const qint64 msSort = t.elapsed();
    qDebug().noquote() << QStringLiteral("testSortSelectionRemapPerf: 32560-row sort with full-column selection elapsed_ms=%1").arg(msSort);
    // 旧实现实测 985~7736ms（用户现场 6~19s）；修复后应为几十 ms 级，放宽到 1500ms 防抖
    QVERIFY2(msSort < 1500, qPrintable(QStringLiteral("sort with full-column selection too slow: %1 ms").arg(msSort)));
    // 整窗快速路径语义：排序后列仍整体选中
    QVERIFY(tvb->selectionModel()->hasSelection());
    // undo 走同一重映射路径，同样不许卡
    t.restart();
    wb.getUndoStack()->undo();
    const qint64 msUndo = t.elapsed();
    qDebug().noquote() << QStringLiteral("testSortSelectionRemapPerf: undo elapsed_ms=%1").arg(msUndo);
    QVERIFY2(msUndo < 1500, qPrintable(QStringLiteral("sort undo too slow: %1 ms").arg(msUndo)));
    wb.hide();
}

void DATablePerfTest::testConditionalScrollPerf1k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import numpy as np
import pandas as pd
pdf5k = pd.DataFrame({
    "x": np.arange(5000, dtype=float),
    "y": np.random.rand(5000) * 100.0,
    "z": np.arange(5000, dtype=float) % 7,
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "pdf5k" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    DATableConditionalFormatStore* store = w.conditionalFormatStore();
    QVERIFY(store != nullptr);
    // 三列各一条规则：色阶 + 数据条 + 单元格规则
    DATableConditionalFormatRule scaleRule;
    scaleRule.type = CfType::ColorScale;
    store->addRule(0, scaleRule);
    DATableConditionalFormatRule barRule;
    barRule.type = CfType::DataBar;
    store->addRule(1, barRule);
    store->addRule(2, makeBgRule(CfCond::Greater, QVariant(3), QColor(0xE6, 0xC2, 0x7C)));
    // 用计数提供者替换 widget 安装的 Python 提供者：钉住统计计算次数
    int statsCalls = 0;
    store->setStatsProvider([ &statsCalls ](int column, DATableConditionalFormatStore::ColumnStats& out) {
        ++statsCalls;
        out.minValue    = 0.0;
        out.maxValue    = 5000.0;
        out.medianValue = 2500.0;
        Q_UNUSED(column);
        return true;
    });
    // 首绘：色阶列与数据条列各计算一次统计（单元格规则列不算）
    tv->viewport()->repaint();
    QApplication::processEvents();
    const int callsAfterFirstPaint = statsCalls;
    QVERIFY2(callsAfterFirstPaint >= 2, qPrintable(QStringLiteral("stats computed lazily on first paint, calls=%1").arg(callsAfterFirstPaint)));
    // 50 次跳滚（覆盖 5000 行）+ 全视口重绘计时
    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i <= 4900; i += 100) {
        tv->showActualRow(i);
        tv->viewport()->repaint();
        if ((i % 1000) == 0) {
            QApplication::processEvents();
        }
    }
    const qint64 elapsed = timer.elapsed();
    // 滚动不重算统计（缓存 + 精确失效挂点：滚动不发 cellValueChanged）
    QCOMPARE(statsCalls, callsAfterFirstPaint);
    // 离屏软件渲染的宽松上界（主要防"每格重算统计/每格遍历全部规则"级回归；
    // 主观流畅度仍属 §4.2 手工验收）
    QVERIFY2(elapsed < 20000, qPrintable(QStringLiteral("50 scroll steps + repaints took %1 ms").arg(elapsed)));
    qDebug().noquote() << QStringLiteral("testConditionalScrollPerf1k: 5000 rows, 50 scroll steps + repaints elapsed_ms=%1, statsCalls=%2")
                              .arg(elapsed)
                              .arg(statsCalls);
    w.hide();
}

void DATablePerfTest::testSplitColumnPerf100k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S": [",".join(str((i * j) % 97) for j in range(5)) for i in range(100000)]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QCOMPARE(df.shape().first, static_cast< std::size_t >(100000));
    QElapsedTimer timer;
    timer.start();
    const QStringList names =
        py.splitColumn(df, 0, QStringLiteral("char"), QStringLiteral(","), QString(), QList< int >(), 0, QStringLiteral("discard"), true, QStringList());
    const qint64 elapsed = timer.elapsed();
    QCOMPARE(names.size(), 5);
    QCOMPARE(df.shape().second, static_cast< std::size_t >(6));
    // 验收口径：无界面冻结超过数秒——算子级 10 万行 × 5 段应在 3s 内（实测亚秒级）
    QVERIFY2(elapsed < 3000, qPrintable(QStringLiteral("100k split took %1 ms (>= 3000 ms)").arg(elapsed)));
    qDebug().noquote() << QStringLiteral("DATablePerfTest::testSplitColumnPerf100k: rows=100000 parts=5 elapsed_ms=%1").arg(elapsed);
}

void DATablePerfTest::testValidationPerf1M()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
n = 1000000
rng = np.random.default_rng(42)
df = pd.DataFrame({
    "v": rng.uniform(-10, 110, n),
    "cat": rng.choice(["a", "b", "c", "zzz"], n),
    "dup": rng.integers(0, 500000, n),
    "v2": rng.uniform(0, 1, n),
    "v3": rng.uniform(0, 1, n),
})
df.loc[df.index[::100000], "v"] = np.nan
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QCOMPARE(df.shape().first, static_cast< std::size_t >(1000000));
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("vperf"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    DA::DATableValidationStore* store = w.validationStore();
    QVERIFY(store != nullptr);
    // 规则：数值区间 + 枚举 + 唯一 + 非空（四类混合，含 O(n) 的唯一性）
    DA::DATableValidationRule rNum;
    rNum.columnName = QStringLiteral("v");
    rNum.type = DA::DATableValidationRule::Type::NumericRange;
    rNum.hasNumericMin = true;
    rNum.numericMin = 0.0;
    rNum.hasNumericMax = true;
    rNum.numericMax = 100.0;
    store->addRule(rNum);
    DA::DATableValidationRule rEnum;
    rEnum.columnName = QStringLiteral("cat");
    rEnum.type = DA::DATableValidationRule::Type::EnumSet;
    rEnum.enumValues = QStringList { QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c") };
    store->addRule(rEnum);
    DA::DATableValidationRule rUniq;
    rUniq.columnName = QStringLiteral("dup");
    rUniq.type = DA::DATableValidationRule::Type::Unique;
    store->addRule(rUniq);
    DA::DATableValidationRule rNe;
    rNe.columnName = QStringLiteral("v");
    rNe.type = DA::DATableValidationRule::Type::NotEmpty;
    store->addRule(rNe);
    // 首次校验（含唯一性 O(n)）
    int elapsed1 = -1;
    QElapsedTimer wall;
    wall.start();
    const auto out1 = w.evaluateValidationRules(&elapsed1);
    const qint64 wall1 = wall.elapsed();
    QCOMPARE(out1.size(), 4);
    QVERIFY(!out1[ 2 ].rows.isEmpty());  // dup 必有重复
    QCOMPARE(store->uniquePythonEvals(), 1);
    // 二次校验：唯一性命中缓存不重算（打点计数不变）
    int elapsed2 = -1;
    wall.restart();
    const auto out2 = w.evaluateValidationRules(&elapsed2);
    const qint64 wall2 = wall.elapsed();
    QCOMPARE(store->uniquePythonEvals(), 1);
    QCOMPARE(out2[ 2 ].rows, out1[ 2 ].rows);
    // 耗时记录（开发诊断，英文）+ 宽松上限断言（100 万行混合四类规则含装载高亮 ≤ 30s，
    // 二次校验因唯一性缓存命中应不慢于首次）
    qDebug().noquote() << QStringLiteral("DATablePerfTest::testValidationPerf1M: first_eval_ms=%1 first_wall_ms=%2 second_eval_ms=%3 second_wall_ms=%4 violations=%5")
                              .arg(elapsed1)
                              .arg(wall1)
                              .arg(elapsed2)
                              .arg(wall2)
                              .arg(out1[ 0 ].rows.size() + out1[ 1 ].rows.size() + out1[ 2 ].rows.size() + out1[ 3 ].rows.size());
    QVERIFY2(wall1 < 30000, qPrintable(QStringLiteral("first validation too slow: %1 ms").arg(wall1)));
    w.hide();
}

void DATablePerfTest::testBatch3FullChain100k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    QElapsedTimer total;
    total.start();
    pybind11::exec(R"(
import pandas as pd, numpy as np
n = 100000
rng = np.random.default_rng(11)
cat_pool = ["alpha", "beta", "x777", "gamma"]
sp = np.arange(n, dtype="float64") * 0.5
sp[::10] = np.nan
df = pd.DataFrame({
    "id": rng.integers(0, 10**9, n),
    "val": rng.uniform(-20, 120, n),
    "cat": rng.choice(cat_pool, n),
    "d": pd.to_datetime("2020-01-01") + pd.to_timedelta(rng.integers(0, 1000, n), unit="D"),
    "sp": sp,
    "flag": rng.integers(0, 2, n).astype(bool),
})
first_val = float(df.iat[0, 1])
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    const double firstVal = scope[ "first_val" ].cast< double >();
    QCOMPARE(df.shape().first, static_cast< std::size_t >(100000));
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("chain100k"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 500);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    QUndoStack& pageStack = *w.getUndoStack();
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    QElapsedTimer step;
    qint64 msNote = 0, msSort = 0, msReplace = 0, msCf = 0, msFill = 0, msValidate = 0;

    // —— ① 批注（先放批注，验证它扛过后续排序）——
    DA::DATableNoteStore* notes = w.noteStore();
    QVERIFY(notes != nullptr);
    step.start();
    {
        DA::DATableNoteStore::Note n1;
        n1.text      = QStringLiteral("chain-note");
        n1.createdAt = QDateTime::currentDateTime();
        n1.updatedAt = n1.createdAt;
        w.push(new DA::DACommandTableNote_set(notes, 50000, 0, DA::DATableNoteStore::Note(), n1));
    }
    msNote = step.elapsed();
    QCOMPARE(notes->noteText(50000, 0), QStringLiteral("chain-note"));

    // —— ② 排序（列头右键菜单入口的页面执行点，破坏式重排 + 置换撤销；
    //    2026-10-18 交互修订：点击表头不再排序）——
    step.start();
    {
        DA::DADataHeaderView* hh = qobject_cast< DA::DADataHeaderView* >(tv->horizontalHeader());
        QVERIFY(hh != nullptr);
        int colVal = -1;
        for (int c = 0; c < hh->count(); ++c) {
            if (hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("val")) {
                colVal = c;
                break;
            }
        }
        QVERIFY(colVal >= 0);
        w.sortColumn(colVal, Qt::AscendingOrder);
        QCOMPARE(w.sortSpec().size(), 1);
    }
    msSort = step.elapsed();
    // 升序：首行是列最小值附近（<= 0 的负端），且批注仍在（跟着某一行移动，总数不变）
    QVERIFY(df.iat(0, 1).toDouble() < 0.0);
    QCOMPARE(notes->count(), 1);
    QCOMPARE(pageStack.count(), 2);  // 批注 + 排序

    // —— ③ 查找替换（变更清单，一个撤销单元）——
    QVector< int > repRows, repCols;
    pybind11::list oldVals, newVals;
    step.start();
    {
        QVERIFY(py.findReplace(df,
                               QStringLiteral("x777"),
                               QStringLiteral("x777z"),
                               false,
                               false,
                               false,
                               qMakePair(0, 99999),
                               qMakePair(2, 2),
                               &repRows,
                               &repCols,
                               &oldVals,
                               &newVals));
        QVERIFY(repRows.size() > 10000);  // 1/4 池命中率
        auto* cmd = new DA::DACommandDataFrame_replace(df, repRows, repCols, oldVals, newVals);
        cmd->setText(QStringLiteral("replace all (chain)"));
        QVERIFY(cmd->exec());
        w.push(cmd);
    }
    msReplace = step.elapsed();
    const int firstRepRow = repRows.isEmpty() ? -1 : repRows.first();
    if (firstRepRow >= 0) {
        QCOMPARE(df.iat(static_cast< std::size_t >(firstRepRow), 2).toString(), QStringLiteral("x777z"));
    }
    QCOMPARE(pageStack.count(), 3);

    // —— ④ 条件格式（元数据 + delegate 绘制，不进撤销栈）——
    step.start();
    {
        DA::DATableConditionalFormatStore* cf = w.conditionalFormatStore();
        QVERIFY(cf != nullptr);
        DA::DATableConditionalFormatRule rule;
        rule.type      = DA::DATableConditionalFormatRule::Type::CellRule;
        rule.condition = DA::DATableConditionalFormatRule::Condition::Greater;
        rule.value1    = QVariant(100.0);
        DA::DATableCellStyle st;
        st.setBackground(QBrush(QColor(0xCE, 0x60, 0x43)));
        rule.style = st;
        cf->addRule(1, rule);
        // 全视口重绘无卡顿（滚动到中部再渲染一次）
        tv->showActualRow(50000);
        QApplication::processEvents();
        QImage img(tv->viewport()->size(), QImage::Format_ARGB32);
        tv->viewport()->render(&img);
        QVERIFY(!img.isNull());
    }
    msCf = step.elapsed();
    const int stackAfterCf = pageStack.count();

    // —— ⑤ 填充（sp 列 ffill，选 100 行）——
    step.start();
    {
        int colSp = -1;
        const QList< QString > cols = df.columns();
        for (int c = 0; c < cols.size(); ++c) {
            if (cols[ c ] == QLatin1String("sp")) {
                colSp = c;
                break;
            }
        }
        QVERIFY(colSp >= 0);
        QItemSelection sel(tv->model()->index(0, colSp), tv->model()->index(99, colSp));
        tv->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
        const int written = w.fillSelection(DA::DADataOperateOfDataFrameWidget::FillMode::Forward);
        QVERIFY(written > 0);  // sp 每 10 行一个 NaN，100 行内必有填充
    }
    msFill = step.elapsed();
    QCOMPARE(pageStack.count(), stackAfterCf + 1);  // 条件格式不入栈、填充入栈

    // —— ⑥ 校验（只读，违规高亮 + 耗时记录）——
    step.start();
    {
        DA::DATableValidationStore* vs = w.validationStore();
        QVERIFY(vs != nullptr);
        DA::DATableValidationRule rNum;
        rNum.columnName = QStringLiteral("val");
        rNum.type       = DA::DATableValidationRule::Type::NumericRange;
        rNum.hasNumericMin = true;
        rNum.numericMin = 0.0;
        rNum.hasNumericMax = true;
        rNum.numericMax = 100.0;
        vs->addRule(rNum);
        DA::DATableValidationRule rNe;
        rNe.columnName = QStringLiteral("cat");
        rNe.type       = DA::DATableValidationRule::Type::NotEmpty;
        rNe.level      = DA::DATableValidationRule::Level::Warning;
        vs->addRule(rNe);
        int evalMs = -1;
        const auto outcomes = w.evaluateValidationRules(&evalMs);
        QCOMPARE(outcomes.size(), 2);
        QVERIFY(outcomes[ 0 ].rows.size() > 10000);  // uniform(-20,120) 约 2/7 越界
        QVERIFY(evalMs >= 0);
        // 违规高亮已装载（任取一条违规行）
        DA::DATableHighlightOverlay* ov = w.highlightOverlay();
        QCOMPARE(ov->levelAt(outcomes[ 0 ].rows.first(), outcomes[ 0 ].columnPosition),
                 DA::DATableHighlightOverlay::Level::ViolationError);
    }
    msValidate = step.elapsed();
    QCOMPARE(pageStack.count(), stackAfterCf + 1);  // 校验只读不入栈（红线2）

    // —— 全部撤销：数据精确还原 ——
    while (pageStack.index() > 0) {
        pageStack.undo();
    }
    QCOMPARE(pageStack.index(), 0);
    QCOMPARE(df.iat(0, 1).toDouble(), firstVal);          // 排序还原
    QCOMPARE(notes->count(), 0);                          // 批注命令还原
    QVERIFY(qIsNaN(df.iat(10, 4).toDouble()));            // 填充还原（sp[10] 回到 NaN）
    if (firstRepRow >= 0) {
        // 排序已还原 → firstRepRow 的绝对行含义回到替换时（替换发生在排序后、排序已先还原？）
        // 撤销顺序：填充→替换→排序→批注；替换还原发生在排序还原之前，行号在替换坐标系内一致
        QVERIFY(df.iat(static_cast< std::size_t >(firstRepRow), 2).toString() != QStringLiteral("x777z"));
    }
    // 校验高亮是瞬态（不在撤销栈），清除入口只清自己来源
    w.clearValidationHighlights();
    QVERIFY(w.highlightOverlay()->isEmpty());

    const qint64 msTotal = total.elapsed();
    qDebug().noquote() << QStringLiteral("DATablePerfTest::testBatch3FullChain100k: note=%1ms sort=%2ms replace=%3ms cf=%4ms fill=%5ms validate=%6ms total=%7ms")
                              .arg(msNote).arg(msSort).arg(msReplace).arg(msCf).arg(msFill).arg(msValidate).arg(msTotal);
    // 宽松上限（防劣化）：全链路 < 60s；排序单步 < 10s（002 实测 10 万行 ~15ms）
    QVERIFY2(msTotal < 60000, qPrintable(QStringLiteral("full chain too slow: %1 ms").arg(msTotal)));
    QVERIFY2(msSort < 10000, qPrintable(QStringLiteral("sort too slow: %1 ms").arg(msSort)));
    w.hide();
}

void DATablePerfTest::testSelectAllLargeSelectionPerf()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
df = pd.DataFrame(np.arange(20000 * 200, dtype="int64").reshape(20000, 200),
                  columns=["c%03d" % i for i in range(200)])
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QVERIFY(!df.isNone());
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    // 构造耗时守卫（2026-11 修复回归）：列宽自适应曾对每列遍历全模型（20000 行 × 200 列
    // = 400 万次 index()+visualRect()），构造函数要跑数十秒到数分钟，直接把本用例拖过
    // Qt Test 的 300s 看门狗。现在只采样视口可见行，构造应在百毫秒级；上限放到 5s 以
    // 容忍慢机器，但只要退回全模型遍历就必然触线。
    QElapsedTimer ctorTimer;
    ctorTimer.start();
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    const qint64 msCtor = ctorTimer.elapsed();
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    const int modelRows = tv->model()->rowCount();
    const int modelCols = tv->model()->columnCount();
    QVERIFY2(modelRows * modelCols > 1000000,
             qPrintable(QStringLiteral("fixture too small: %1x%2").arg(modelRows).arg(modelCols)));

    // 角格点击的等价入口：selectAll 同步触发 selectionChanged → 代表样式/
    // 显示格式计算，计时覆盖整条选区变化处理链
    QElapsedTimer t;
    t.start();
    tv->selectAll();
    const qint64 msSelectAll = t.elapsed();

    // 选区规模自检（确认守卫打在真的大选区上），并验证几何计数不物化
    QCOMPARE(w.selectedCellCount(), modelRows * modelCols);
    QCOMPARE(w.selectionSegmentCount(), 1);

    t.restart();
    const QList< int > cols = w.getSelectedDataframeCoumns(false);
    const QList< int > rows = w.getSelectedDataframeRows(false);
    const qint64 msGeometry = t.elapsed();
    QCOMPARE(cols.size(), modelCols);
    QCOMPARE(rows.size(), modelRows);

    // 整列/整行判定：旧实现经 Qt 的 selectedColumns/selectedRows 是 O(列×行)
    t.restart();
    const QList< int > fullCols = w.getFullySelectedDataframeColumns(false);
    const QList< int > fullRows = w.getFullySelectedDataframeRows(false);
    const qint64 msFullSpan     = t.elapsed();
    QCOMPARE(fullCols.size(), modelCols);
    QCOMPARE(fullRows.size(), modelRows);

    // 显示格式（点击链路里的第二个消费者，经 collectSelectedColumns 走整列判定）
    t.restart();
    w.getCurrentColumnDisplayFormat();
    const qint64 msFormat = t.elapsed();

    // 代表样式（点击链路里的主要消费者，走列级快路径）
    t.restart();
    QVERIFY(w.getCurrentCellStyle().isNull());
    const qint64 msStyle = t.elapsed();

    qDebug().noquote() << QStringLiteral("DATablePerfTest::testSelectAllLargeSelectionPerf: cells=%1 "
                                         "ctor=%2ms selectAll=%3ms geometry=%4ms fullSpan=%5ms format=%6ms style=%7ms")
                              .arg(w.selectedCellCount())
                              .arg(msCtor)
                              .arg(msSelectAll)
                              .arg(msGeometry)
                              .arg(msFullSpan)
                              .arg(msFormat)
                              .arg(msStyle);

    // 防劣化上限：单步 < 200ms（新实现实测均为毫秒级；旧实现在同夹具下
    // selectAll 1079ms / geometry 524ms / style 935ms，会全部触线）
    QVERIFY2(msCtor < 5000, qPrintable(QStringLiteral("widget construction too slow: %1 ms").arg(msCtor)));
    QVERIFY2(msSelectAll < 200, qPrintable(QStringLiteral("selectAll too slow: %1 ms").arg(msSelectAll)));
    QVERIFY2(msGeometry < 200, qPrintable(QStringLiteral("selection geometry too slow: %1 ms").arg(msGeometry)));
    QVERIFY2(msFullSpan < 200, qPrintable(QStringLiteral("full row/column span too slow: %1 ms").arg(msFullSpan)));
    QVERIFY2(msFormat < 200, qPrintable(QStringLiteral("display format too slow: %1 ms").arg(msFormat)));
    QVERIFY2(msStyle < 200, qPrintable(QStringLiteral("representative style too slow: %1 ms").arg(msStyle)));
    w.hide();
}

// 不用QTEST_MAIN：嵌入式python的静态对象（DAPyBindQt.dll内的解释器句柄、QVariant caster
// 类型缓存等）在进程teardown阶段析构顺序不可控，会在测试结果完整输出后引发访问冲突。
// qExec返回时断言结果与-o文件均已写完，直接终止进程跳过teardown，保证退出码可信
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DATablePerfTest tc;
    int ret = QTest::qExec(&tc, argc, argv);
    std::fflush(nullptr);
#ifdef Q_OS_WIN
    // Windows下_exit仍会走LdrShutdownProcess→DLL_PROCESS_DETACH触发各DLL静态析构，
    // 必须TerminateProcess才能完全跳过
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(ret));
#else
    ::_exit(ret);
#endif
    return ret;
}
#include "main.moc"
