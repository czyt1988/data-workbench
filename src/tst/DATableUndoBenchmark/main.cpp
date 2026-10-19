#include <QtTest/QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <QUndoStack>
#include <QTableView>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include "pandas/DAPyDataFrame.h"
#include <pybind11/eval.h>

#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "Commands/DACommandsDataFrame.h"

#include "DAData.h"
#include "DAAbstractData.h"
#include "DATableDataSource.h"
#include "DATableDataBlock.h"
#include "Models/DADataTableModel.h"

#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

//===================================================
// 005 T1.21 撤销成本基准（量化先行，设计稿 §4.11 B / D7）
//
// 三部分：
// ① 100 万行 × 20 列可写 DataFrame 上四类撤销命令的耗时与内存驻留：
//    iat 值记录 / DACommandWithTemporaryData 快照（对照基线）/ sortRows 置换 /
//    replace 变更清单（1 万格）；
// ② 1 亿行合成惰性源 + 滑动窗（默认 20000）：模型启动时间与常驻内存——
//    「滑动窗下 1 亿行可开」的实测依据；
// ③ D7 拆窗对照实验：同一 100 万行惰性源，默认窗口 vs 窗口=全量行数（等价"拆窗"
//    全量驻留，含视图/表头按行记账成本），差值折算每行常驻成本并线性外推 1 亿行
//    （1 亿行全量物化在本机必然 OOM，外推口径写入报告）。窗口大小只在测试进程内
//    setCacheWindowSize，不改主干默认值（红线6）。
//
// 测量口径：QElapsedTimer 手工计时（全仓无 QBENCHMARK 先例，不引入新框架）；
// 内存取进程 WorkingSet（GetProcessMemoryInfo），差值受分配器缓存影响为近似值，
// 报告注明。断言全部为宽松上限（防劣化），数值以 qDebug "BENCH:" 行输出供报告誊录。
//===================================================

namespace
{
// 当前进程工作集字节数（非 Windows 返回 0）
qint64 workingSetBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast< qint64 >(pmc.WorkingSetSize);
    }
#endif
    return 0;
}

// 当前进程峰值工作集字节数
qint64 peakWorkingSetBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast< qint64 >(pmc.PeakWorkingSetSize);
    }
#endif
    return 0;
}
}

//===================================================
// 合成惰性数据源（行数可配，5 列，cell 实时生成不驻留内存；同 DATableDataTest 先例）
//===================================================
class DABenchLazyTable : public DA::DAAbstractData, public DA::DATableDataSource
{
public:
    explicit DABenchLazyTable(std::size_t totalRows) : mTotalRows(totalRows)
    {
    }
    std::size_t mTotalRows;
    mutable int mFetchCount = 0;
    mutable std::size_t mFetchedRows = 0;  ///< 累计实际生成的行数（全量驻留成本对照用）

public:
    DA::DAAbstractData::DataType getDataType() const override
    {
        return DA::DAAbstractData::TypeInnerData;
    }
    QVariant toVariant(std::size_t dim1, std::size_t dim2) const override
    {
        return makeCell(dim1, dim2);
    }
    bool setValue(std::size_t, std::size_t, const QVariant&) override
    {
        return false;
    }
    DA::DATableDataSource* tableSource() override
    {
        return this;
    }
    const DA::DATableDataSource* tableSource() const override
    {
        return this;
    }
    bool isReferenceData() const override
    {
        return true;
    }
    bool supportsUndoSnapshot() const override
    {
        return false;
    }
    QString typeIdentifier() const override
    {
        return QStringLiteral("DATableUndoBenchmark.LazyTable");
    }
    void write(QDataStream&) override
    {
    }
    bool read(QDataStream&) override
    {
        return false;
    }

public:  // DATableDataSource
    std::size_t tableRowCount() const override
    {
        return mTotalRows;
    }
    std::size_t tableColumnCount() const override
    {
        return 5;
    }
    QString tableColumnName(std::size_t column) const override
    {
        return QStringLiteral("col%1").arg(column);
    }
    int tableColumnType(std::size_t column) const override
    {
        switch (column) {
        case 0:
            return QMetaType::LongLong;
        case 1:
            return QMetaType::Double;
        case 2:
            return QMetaType::QString;
        case 3:
            return QMetaType::Bool;
        default:
            return QMetaType::Int;
        }
    }
    DA::DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override
    {
        ++mFetchCount;
        if (startRow >= mTotalRows) {
            return DA::DATableDataBlock();
        }
        std::size_t n = std::min(rowCount, mTotalRows - startRow);
        DA::DATableDataBlock block(startRow, 5);
        QVariantList headers;
        headers.reserve(static_cast< int >(n));
        for (std::size_t r = 0; r < n; ++r) {
            QVariantList row;
            row.reserve(5);
            for (std::size_t c = 0; c < 5; ++c) {
                row.append(makeCell(startRow + r, c));
            }
            block.appendRow(row);
            headers.append(static_cast< qlonglong >(startRow + r));
        }
        block.setRowHeaders(headers);
        mFetchedRows += n;
        return block;
    }
    bool isLazyLoaded() const override
    {
        return true;
    }
    bool isTableEditable() const override
    {
        return false;
    }

public:
    static QVariant makeCell(std::size_t row, std::size_t col)
    {
        switch (col) {
        case 0:
            return QVariant(static_cast< qlonglong >(row));
        case 1:
            return QVariant(static_cast< double >(row) * 0.5);
        case 2:
            return QVariant(QStringLiteral("r%1c%2").arg(row).arg(col));
        case 3:
            return QVariant(row % 2 == 0);
        default:
            return QVariant(static_cast< int >(row + col));
        }
    }
};

//===================================================
// 基准测试类
//===================================================
class DATableUndoBenchmark : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    // ① 100 万行 × 20 列：四类撤销命令耗时与内存驻留
    void benchCommands1Mx20();
    // ② 1 亿行惰性源 + 默认滑动窗：启动时间与常驻内存
    void benchLazy1e8Startup();
    // ③ D7 拆窗对照：100 万行惰性源，默认窗口 vs 全量窗口
    void benchWindowControlD7();
    void cleanupTestCase();

private:
    bool mPandasOk    = false;
    bool mPyScriptsOk = false;
};

void DATableUndoBenchmark::initTestCase()
{
    // 与 DATableDataTest 同款 Python 初始化（解释器路径 + sys.path 指向仓库 PyScripts）
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
    }
    if (mPandasOk) {
        const QString tstDir   = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath();
        const QString repoRoot = QDir::cleanPath(tstDir + QStringLiteral("/../../.."));
        DA::DAPyInterpreter::appendSysPath(repoRoot + QStringLiteral("/src/PyScripts"));
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
    }
    qDebug().noquote() << QStringLiteral("BENCH: process_peak_ws_mb_at_start=%1")
                              .arg(peakWorkingSetBytes() / (1024.0 * 1024.0), 0, 'f', 1);
}

void DATableUndoBenchmark::cleanupTestCase()
{
}

/**
 * @brief ① 100 万行 × 20 列可写表：四类撤销命令的耗时与内存驻留
 *
 * iat（值记录，现状单元格编辑）/ dropIRow（DACommandWithTemporaryData 整表快照，
 * 对照基线：构造即 pickle 全表到临时文件，undo 读回）/ sortRows（置换，O(n) 索引）/
 * replace（1 万格变更清单，da_write_cells 批量写回）。
 */
void DATableUndoBenchmark::benchCommands1Mx20()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    try {
    pybind11::dict scope;
    QElapsedTimer t;
    t.start();
    pybind11::exec(R"(
import pandas as pd, numpy as np
n = 1000000
rng = np.random.default_rng(7)
data = {}
for i in range(20):
    data["c%d" % i] = rng.uniform(0, 1000, n)
df = pd.DataFrame(data)
old_vals = df.iloc[0:10000, 5].tolist()
new_vals = [v + 1.0 for v in old_vals]
)",
                   pybind11::globals(),
                   scope);
    const qint64 buildMs = t.elapsed();
    DA::DAPyDataFrame df(scope[ "df" ]);
    QCOMPARE(df.shape().first, static_cast< std::size_t >(1000000));
    QCOMPARE(df.shape().second, static_cast< std::size_t >(20));
    const qint64 wsTable = workingSetBytes();
    qDebug().noquote() << QStringLiteral("BENCH: table_build_ms=%1 table_ws_mb=%2")
                              .arg(buildMs)
                              .arg(wsTable / (1024.0 * 1024.0), 0, 'f', 1);

    // —— 1) iat 值记录（单元格编辑现状路径）——
    qint64 ws0 = workingSetBytes();
    const QVariant oldv = df.iat(500000, 3);
    t.restart();
    DA::DACommandDataFrame_iat cmdIat(df, 500000, 3, oldv, QVariant(oldv.toDouble() + 1.0));
    QVERIFY(cmdIat.exec());
    const qint64 iatApplyMs = t.elapsed();
    const qint64 iatRetain  = workingSetBytes() - ws0;
    t.restart();
    cmdIat.undo();
    const qint64 iatUndoMs = t.elapsed();
    qDebug().noquote() << QStringLiteral("BENCH: iat_apply_ms=%1 iat_undo_ms=%2 iat_retain_kb=%3")
                              .arg(iatApplyMs)
                              .arg(iatUndoMs)
                              .arg(iatRetain / 1024.0, 0, 'f', 1);
    QVERIFY2(iatApplyMs < 5000, "single iat edit on 1M rows must stay < 5s (T1.21 acceptance: <100ms typical)");

    // —— 2) 整表快照对照（DACommandWithTemporaryData：构造 pickle 全表 → 临时文件）——
    ws0 = workingSetBytes();
    t.restart();
    DA::DACommandDataFrame_dropIRow cmdSnap(df, { 999999 });
    const qint64 snapCtorMs  = t.elapsed();
    const qint64 snapRetain  = workingSetBytes() - ws0;
    t.restart();
    QVERIFY(cmdSnap.exec());
    const qint64 snapExecMs = t.elapsed();
    t.restart();
    cmdSnap.undo();  // 从临时文件读回整表
    const qint64 snapUndoMs = t.elapsed();
    qDebug().noquote() << QStringLiteral("BENCH: snapshot_ctor_ms=%1 snapshot_exec_ms=%2 snapshot_undo_ms=%3 snapshot_retain_mb=%4 (pickle 在磁盘)")
                              .arg(snapCtorMs)
                              .arg(snapExecMs)
                              .arg(snapUndoMs)
                              .arg(snapRetain / (1024.0 * 1024.0), 0, 'f', 1);

    // —— 3) 置换排序（002 sortRows：O(n) 索引，无数据复制）——
    ws0 = workingSetBytes();
    t.restart();
    DA::DACommandDataFrame_sortRows cmdSort(df, QVector< QPair< int, bool > >({ qMakePair(0, true) }));
    QVERIFY(cmdSort.exec());  // 首次 exec 含置换计算
    const qint64 sortApplyMs = t.elapsed();
    const qint64 sortRetain  = workingSetBytes() - ws0;
    t.restart();
    cmdSort.undo();  // 逆置换
    const qint64 sortUndoMs = t.elapsed();
    qDebug().noquote() << QStringLiteral("BENCH: sort_apply_ms=%1 sort_undo_ms=%2 sort_retain_mb=%3 (perm+inv 两个 1M int 数组)")
                              .arg(sortApplyMs)
                              .arg(sortUndoMs)
                              .arg(sortRetain / (1024.0 * 1024.0), 0, 'f', 1);
    QVERIFY2(sortApplyMs < 60000, "1M-row permutation sort must stay < 60s");

    // —— 4) 变更清单替换（002 replace：1 万格，da_write_cells 批量写回）——
    pybind11::list oldList = scope[ "old_vals" ].cast< pybind11::list >();
    pybind11::list newList = scope[ "new_vals" ].cast< pybind11::list >();
    QVector< int > repRows, repCols;
    repRows.reserve(10000);
    repCols.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        repRows.append(i);
        repCols.append(5);
    }
    ws0 = workingSetBytes();
    t.restart();
    DA::DACommandDataFrame_replace cmdRep(df, repRows, repCols, oldList, newList);
    const qint64 repCtorMs = t.elapsed();
    QCOMPARE(cmdRep.changeCount(), 10000);
    const qint64 repRetain = workingSetBytes() - ws0;
    t.restart();
    QVERIFY(cmdRep.exec());  // 写新值（按列分组批量赋值）
    const qint64 repExecMs = t.elapsed();
    t.restart();
    cmdRep.undo();  // 写回旧值
    const qint64 repUndoMs = t.elapsed();
    qDebug().noquote() << QStringLiteral("BENCH: replace10k_ctor_ms=%1 replace10k_exec_ms=%2 replace10k_undo_ms=%3 replace10k_retain_mb=%4")
                              .arg(repCtorMs)
                              .arg(repExecMs)
                              .arg(repUndoMs)
                              .arg(repRetain / (1024.0 * 1024.0), 0, 'f', 1);
    QVERIFY2(repExecMs < 30000, "10k-cell change-list write must stay < 30s");

    qDebug().noquote() << QStringLiteral("BENCH: commands_section_peak_ws_mb=%1")
                              .arg(peakWorkingSetBytes() / (1024.0 * 1024.0), 0, 'f', 1);
    } catch (const std::exception& e) {
        qDebug().noquote() << "BENCH-EXC:" << e.what();
        throw;
    }
}

/**
 * @brief ② 1 亿行合成惰性源 + 默认滑动窗：模型启动时间与常驻内存
 *
 * 滑动窗（默认 20000 行）把一切按行的 Qt 行为压进窗口内：1 亿行只驻留窗口块，
 * 启动成本与总行数无关（schema O(1)）。本项是「滑动窗下 1 亿行可开」的实测依据。
 */
void DATableUndoBenchmark::benchLazy1e8Startup()
{
    QUndoStack stack;
    const qint64 ws0 = workingSetBytes();
    QElapsedTimer t;
    t.start();
    DA::DAAbstractData::Pointer lazy(new DABenchLazyTable(static_cast< std::size_t >(100000000)));  // 1 亿行
    DA::DAData d(lazy);
    DA::DADataTableModel model(&stack);
    model.setData(d);
    // 触摸首窗口数据（触发首块 fetch）
    const QVariant v0 = model.data(model.index(0, 0));
    QVERIFY(v0.isValid());
    const qint64 startupMs = t.elapsed();
    const qint64 wsDelta   = workingSetBytes() - ws0;
    QCOMPARE(model.actualRowCount(), 100000000);
    QVERIFY(model.rowCount() <= model.getCacheWindowSize());
    qDebug().noquote() << QStringLiteral("BENCH: lazy1e8_startup_ms=%1 lazy1e8_resident_mb=%2 window_rows=%3")
                              .arg(startupMs)
                              .arg(wsDelta / (1024.0 * 1024.0), 0, 'f', 1)
                              .arg(model.rowCount());
    QVERIFY2(startupMs < 20000, "1e8-row lazy startup with sliding window must stay < 20s");
}

/**
 * @brief ③ D7 拆窗对照实验：同一惰性源，滑动窗 vs 窗口=全量（等价"拆窗"）
 *
 * 三段测量：
 * A) 100 万行 + 默认窗口（20000）：模型+视图启动、只物化窗口内数据；
 * B) 100 万行 + 窗口=全量：强制触底遍历全部块（512 行/块）→ 数据全量物化 +
 *    Qt 按行记账（QHeaderView 段数=行数）；B-A 差值 = 拆窗的实测代价；
 * C) 1000 万行 + 窗口=全量（不触数据，只 setModel）：隔离 Qt 每行记账成本，
 *    线性外推 1 亿行（设计稿 §1.2 事实 10 的 800MB 量级实测核对）。
 * 1 亿行全量物化在本机必然 OOM，故用 1e6/1e7 实测 + 线性外推，口径写入报告。
 * 窗口大小只在测试进程内 setCacheWindowSize，不改主干默认值（红线6）。
 */
void DATableUndoBenchmark::benchWindowControlD7()
{
    QUndoStack stack;

    // —— A) 100 万行，默认滑动窗（20000 行）——
    qint64 msA = 0, wsA = 0;
    int fetchA = 0;
    {
        const qint64 ws0 = workingSetBytes();
        DABenchLazyTable* lazy = new DABenchLazyTable(1000000);
        DA::DAAbstractData::Pointer p(lazy);
        DA::DAData d(p);
        QElapsedTimer t;
        t.start();
        DA::DADataTableModel model(&stack);
        model.setData(d);
        QTableView view;  // 离屏不 show：setModel 即触发表头段记账
        view.setModel(&model);
        QVERIFY(model.data(model.index(0, 2)).isValid());
        QVERIFY(model.data(model.index(model.rowCount() - 1, 2)).isValid());  // 窗口尾部
        msA = t.elapsed();
        wsA = workingSetBytes() - ws0;
        fetchA = lazy->mFetchCount;
        QCOMPARE(model.rowCount(), qMin(model.getCacheWindowSize(), 1000000));
    }

    // —— B) 100 万行，窗口=全量：遍历全部块强制物化（512 行/块）——
    qint64 msB = 0, wsB = 0;
    int fetchB = 0;
    {
        const qint64 ws0 = workingSetBytes();
        DABenchLazyTable* lazy = new DABenchLazyTable(1000000);
        DA::DAAbstractData::Pointer p(lazy);
        DA::DAData d(p);
        QElapsedTimer t;
        t.start();
        DA::DADataTableModel model(&stack);
        model.setCacheWindowSize(1000000);  // 仅测试进程内，红线6
        model.setData(d);
        QTableView view;
        view.setModel(&model);
        // 触底遍历：每 512 行取一次字符串列（最贵单元格），强制块缓存全量物化
        for (int r = 0; r < 1000000; r += 512) {
            QVERIFY(model.data(model.index(r, 2)).isValid());
        }
        QVERIFY(model.data(model.index(999999, 2)).isValid());
        msB = t.elapsed();
        wsB = workingSetBytes() - ws0;
        fetchB = lazy->mFetchCount;
        QCOMPARE(model.rowCount(), 1000000);
        QVERIFY(fetchB > 1000);  // 确实全量取数（对照 A 的少量块）
    }

    // —— C) 1000 万行，窗口=全量（不触数据）：隔离 Qt 每行记账成本 ——
    qint64 msC = 0, wsC = 0;
    {
        const qint64 ws0 = workingSetBytes();
        DABenchLazyTable* lazy = new DABenchLazyTable(10000000);
        DA::DAAbstractData::Pointer p(lazy);
        DA::DAData d(p);
        QElapsedTimer t;
        t.start();
        DA::DADataTableModel model(&stack);
        model.setCacheWindowSize(10000000);  // 仅测试进程内，红线6
        model.setData(d);
        QTableView view;
        view.setModel(&model);  // 表头 1e7 段记账
        QVERIFY(model.data(model.index(0, 2)).isValid());
        msC = t.elapsed();
        wsC = workingSetBytes() - ws0;
        QCOMPARE(model.rowCount(), 10000000);
    }

    // 折算与外推
    const double perRowDataBytes = (wsB > wsA) ? static_cast< double >(wsB - wsA) / (1000000.0 - 20000.0) : 0.0;
    const double perRowQtBytes   = wsC > 0 ? static_cast< double >(wsC) / 10000000.0 : 0.0;
    const double est1e8Gb        = (perRowDataBytes + perRowQtBytes) * 1.0e8 / (1024.0 * 1024.0 * 1024.0);
    qDebug().noquote() << QStringLiteral("BENCH: windowA_default_ms=%1 windowA_ws_mb=%2 windowA_fetch=%3 | windowB_full_ms=%4 windowB_ws_mb=%5 windowB_fetch=%6 | windowC_1e7_bookkeeping_ms=%7 windowC_1e7_ws_mb=%8")
                              .arg(msA).arg(wsA / (1024.0 * 1024.0), 0, 'f', 1).arg(fetchA)
                              .arg(msB).arg(wsB / (1024.0 * 1024.0), 0, 'f', 1).arg(fetchB)
                              .arg(msC).arg(wsC / (1024.0 * 1024.0), 0, 'f', 1);
    qDebug().noquote() << QStringLiteral("BENCH: d7_per_row_data_bytes=%1 d7_per_row_qt_bytes=%2 d7_extrapolated_1e8_gb=%3 (1e6/1e7 实测线性外推；1e8 全量物化必 OOM)")
                              .arg(perRowDataBytes, 0, 'f', 1)
                              .arg(perRowQtBytes, 0, 'f', 1)
                              .arg(est1e8Gb, 0, 'f', 2);
    // 方向性断言：拆窗（全量物化）显著贵于滑动窗
    QVERIFY2(msB > msA && wsB > wsA, "full-window control must cost more than sliding window (time and memory)");
    qDebug().noquote() << QStringLiteral("BENCH: d7_section_peak_ws_mb=%1")
                              .arg(peakWorkingSetBytes() / (1024.0 * 1024.0), 0, 'f', 1);
}

// 不用QTEST_MAIN：嵌入式python静态对象 teardown 顺序不可控（同 DATableDataTest 口径），
// qExec 返回后直接终止进程，保证退出码可信
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DATableUndoBenchmark tc;
    int ret = QTest::qExec(&tc, argc, argv);
    std::fflush(nullptr);
#ifdef Q_OS_WIN
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(ret));
#else
    ::_exit(ret);
#endif
    return ret;
}
#include "main.moc"
