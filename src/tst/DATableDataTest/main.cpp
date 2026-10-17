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
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>

#include "DAData.h"
#include "DAAbstractData.h"
#include "DATableDataSource.h"
#include "DATableDataBlock.h"
#include "DADataFactory.h"

// 表格元数据基础设施（001 计划 §4.1）
#include "DATableHighlightOverlay.h"
#include "DATableKeyRemap.h"
#include "DATableMetadata.h"
#include "DATableMetadataRegistry.h"
#include "DATableStyleManager.h"
#include "DATableViewStateStore.h"
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

#include "DASqliteLazyTable.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>

#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

//===================================================
// 合成惰性数据源：千万行×5列，cell 按行列号实时生成，不驻留内存
// 模拟数据库惰性表的行为（fetchBlock 计数用于验证取数次数）
//===================================================
class DATestLazyTable : public DA::DAAbstractData, public DA::DATableDataSource
{
public:
    static constexpr std::size_t c_totalRows = 10000000;
    static constexpr std::size_t c_totalCols = 5;
    mutable int mFetchCount                  = 0;
    QString mRefName;  ///< 引用式序列化内容（模拟连接串+表名）

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
        return false;  // 只读
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
        return QStringLiteral("DATableDataTest.LazyTable");
    }
    void write(QDataStream& out) override
    {
        out << mRefName;
    }
    bool read(QDataStream& in) override
    {
        in >> mRefName;
        return in.status() == QDataStream::Ok;
    }

public:  // DATableDataSource
    std::size_t tableRowCount() const override
    {
        return c_totalRows;
    }
    std::size_t tableColumnCount() const override
    {
        return c_totalCols;
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
        if (startRow >= c_totalRows) {
            return DA::DATableDataBlock();
        }
        std::size_t n = std::min(rowCount, c_totalRows - startRow);
        DA::DATableDataBlock block(startRow, c_totalCols);
        QVariantList headers;
        headers.reserve(static_cast< int >(n));
        for (std::size_t r = 0; r < n; ++r) {
            QVariantList row;
            row.reserve(static_cast< int >(c_totalCols));
            for (std::size_t c = 0; c < c_totalCols; ++c) {
                row.append(makeCell(startRow + r, c));
            }
            block.appendRow(row);
            headers.append(static_cast< qlonglong >(startRow + r));
        }
        block.setRowHeaders(headers);
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
            return QVariant(static_cast< double >(row) * 0.5 + static_cast< double >(col));
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
// 测试辅助
//===================================================

// NaN感知的QVariant相等比较（QVariant(double NaN) == QVariant(double NaN) 为 false）
static bool variantEquals(const QVariant& a, const QVariant& b)
{
    if (a.userType() == QMetaType::Double || b.userType() == QMetaType::Double) {
        bool aNan = (a.userType() == QMetaType::Double && qIsNaN(a.toDouble()));
        bool bNan = (b.userType() == QMetaType::Double && qIsNaN(b.toDouble()));
        if (aNan || bNan) {
            return aNan && bNan;
        }
    }
    return a == b;
}

// 构造混合类型测试DataFrame（int64/float64含NaN/str/datetime64/bool，10行5列）
static pybind11::object makeTestDataFrame()
{
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

// 构造测试Series（float64含NaN，带名字，6行）
static pybind11::object makeTestSeries()
{
    try {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
ser = pd.Series([1.5, float("nan"), 2.5, 3.5, float("nan"), 5.5], name="ser")
)",
                       pybind11::globals(),
                       scope);
        return scope[ "ser" ];
    } catch (const std::exception& e) {
        qWarning() << "makeTestSeries failed:" << e.what();
    }
    return pybind11::none();
}

// 构造int64测试Series（验证批量取数保持numpy标量类型，与iat一致为LongLong）
static pybind11::object makeTestSeriesInt()
{
    try {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
ser = pd.Series([10, 20, 30, 40], dtype="int64", name="ser_int")
)",
                       pybind11::globals(),
                       scope);
        return scope[ "ser" ];
    } catch (const std::exception& e) {
        qWarning() << "makeTestSeriesInt failed:" << e.what();
    }
    return pybind11::none();
}

//===================================================
// 测试类
//===================================================
class DATableDataTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testLazyTableSchema();
    void testLazyTableFetchBlock();
    void testBlockBounds();
    void testPandasDataFrameEquivalence();
    void testPandasSeriesEquivalence();
    void testEmptyDataFrame();
    void testFactoryRoundTrip();
    void testModelLazyDisplay();
    void testModelBlockCacheAndScroll();
    void testModelPandasEditRegression();
    void testSqliteLazyTable();
    void testSqliteModelIntegration();
    void testSqliteReferenceRoundTrip();
    // 表格元数据基础设施（001 计划 §4.1）：瞬态高亮层
    void testHighlightOverlayBasics();
    void testHighlightOverlayPriority();
    void testHighlightOverlayKeyPacking();
    // 位置键重映射工具与 DATableStyleManager::onRowsReordered
    void testKeyRemapUtility();
    void testStyleManagerRowsReordered();
    // 视图状态子存储与元数据注册表
    void testViewStateStore();
    void testMetadataRegistry();
    // §4.2 手工验收的程序化验证：旧工程 XML 兼容、菜单构成、视图状态重开应用、表头点击链路
    void testLegacyTableStylesXmlCompat();
    void testContextMenuComposition();
    void testViewStateApplyAndHeaderClickChain();
    // 002 表头排序（T1.3 §4.1）：算子多键+置换、命令 undo/redo+样式重映射、表头交互/拾取互斥/只读源、10万行计时
    void testSortOperatorMultiKeyPerm();
    void testSortRowsCommandUndoRedoAndStyleRemap();
    void testHeaderSortInteraction();
    void testSortPerf100k();
    // 002 查找/替换（T1.4 §4.1）：搜索扩展、替换算子、替换命令 undo/redo、浮动条去抖、页面查找替换全链路
    void testSearchOperatorExtended();
    void testFindReplaceOperator();
    void testReplaceCommandUndoRedo();
    void testFindReplaceDialogDebounce();
    void testFindReplaceWidgetFlow();
    // 003 条件格式（T1.5 §4.1）：规则求值、叠加顺序、色阶插值、数据条归一化、
    // 存储 XML round-trip/重映射/统计缓存、命令 undo/redo、统计算子、绘制像素级验证、
    // 对话框惰性源限制、千行滚动计时
    void testConditionalRuleEvaluation();
    void testConditionalEvaluateOrder();
    void testColorScaleInterpolation();
    void testDataRowNormalization();
    void testConditionalStoreXmlAndRemap();
    void testConditionalStoreStatsCache();
    void testConditionalCommands();
    void testColumnStatsOperator();
    void testConditionalPaintWidgetFlow();
    void testConditionalDialogSourceRestriction();
    void testConditionalScrollPerf1k();

private:
    // 生成/复用sqlite测试库（百万行），失败返回false并置mSqliteErr
    bool ensureSqliteReady();
    // 创建绑定测试库的惰性表数据（SELECT带ORDER BY保证分页稳定）
    DA::DAAbstractData::Pointer makeSqliteLazyData();

private:
    static constexpr qlonglong c_sqliteRows = 1000000;
    bool mPandasOk                          = false;
    bool mPyScriptsOk                       = false;  ///< DAPyScripts 算子模块可用（da_sort 等）
    QUndoStack mUndoStack;
    QTemporaryDir mTmpDir;
    QString mSqliteConnName = QStringLiteral("databledata_sqlite_conn");
    bool mSqliteReady       = false;
    QString mSqliteErr;
};

void DATableDataTest::initTestCase()
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

void DATableDataTest::cleanupTestCase()
{
    if (QSqlDatabase::contains(mSqliteConnName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(mSqliteConnName, false);
            if (db.isOpen()) {
                db.close();
            }
        }  // 局部db副本先析构，避免removeDatabase告警
        QSqlDatabase::removeDatabase(mSqliteConnName);
    }
}

// 生成/复用sqlite测试库：bigtable(id INTEGER PRIMARY KEY, val REAL, label TEXT)
bool DATableDataTest::ensureSqliteReady()
{
    if (mSqliteReady) {
        return true;
    }
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        mSqliteErr = QStringLiteral("QSQLITE driver not available");
        return false;
    }
    QString dbPath = mTmpDir.isValid() ? mTmpDir.filePath(QStringLiteral("lazy.sqlite"))
                                       : QDir::temp().filePath(QStringLiteral("databledata_lazy.sqlite"));
    {
        QSqlDatabase db = QSqlDatabase::contains(mSqliteConnName)
                              ? QSqlDatabase::database(mSqliteConnName, false)
                              : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), mSqliteConnName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            mSqliteErr = db.lastError().text();
            return false;
        }
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS bigtable(id INTEGER PRIMARY KEY, val REAL, label TEXT)"))) {
            mSqliteErr = q.lastError().text();
            return false;
        }
        // 已生成过则直接复用
        QSqlQuery cnt(db);
        if (cnt.exec(QStringLiteral("SELECT COUNT(*) FROM bigtable")) && cnt.next()
            && cnt.value(0).toLongLong() >= c_sqliteRows) {
            mSqliteReady = true;
            return true;
        }
        q.exec(QStringLiteral("DELETE FROM bigtable"));
        db.transaction();
        const qlonglong batch = 5000;
        for (qlonglong start = 0; start < c_sqliteRows; start += batch) {
            QStringList values;
            values.reserve(static_cast< int >(batch));
            for (qlonglong r = start; r < qMin(start + batch, c_sqliteRows); ++r) {
                values << QStringLiteral("(%1,%2,'row%1')").arg(r).arg(r * 0.5);
            }
            QString sql = QStringLiteral("INSERT INTO bigtable(id,val,label) VALUES ") + values.join(QLatin1Char(','));
            if (!q.exec(sql)) {
                mSqliteErr = q.lastError().text();
                db.rollback();
                return false;
            }
        }
        db.commit();
    }
    mSqliteReady = true;
    return true;
}

DA::DAAbstractData::Pointer DATableDataTest::makeSqliteLazyData()
{
    std::shared_ptr< DASqliteLazyTable > p = std::make_shared< DASqliteLazyTable >();
    p->setConnectionName(mSqliteConnName);
    p->setSelectSql(QStringLiteral("SELECT id, val, label FROM bigtable ORDER BY id"));
    return p;
}

// 合成惰性源经DAData句柄的schema与能力位
void DATableDataTest::testLazyTableSchema()
{
    DA::DAAbstractData::Pointer p(new DATestLazyTable());
    DA::DAData d(p);
    QVERIFY(!d.isNull());
    QVERIFY(d.isTable());
    QVERIFY(!d.isDataFrame());
    QVERIFY(!d.isSeries());
    QVERIFY(d.isReferenceData());
    QVERIFY(!d.supportsUndoSnapshot());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("DATableDataTest.LazyTable"));

    // shape()走tableSource路由，不再返回(0,0)
    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, DATestLazyTable::c_totalRows);
    QCOMPARE(sp.second, DATestLazyTable::c_totalCols);

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QCOMPARE(ts->tableColumnName(2), QStringLiteral("col2"));
    QCOMPARE(ts->tableColumnType(0), static_cast< int >(QMetaType::LongLong));
    QVERIFY(ts->isLazyLoaded());
    QVERIFY(!ts->isTableEditable());

    // const路径
    const DA::DAData& cd               = d;
    const DA::DATableDataSource* cts   = cd.tableSource();
    QVERIFY(cts != nullptr);
    QCOMPARE(cts->tableRowCount(), DATestLazyTable::c_totalRows);
}

// 合成惰性源的块级取数：越界、截断、行头、取数计数
void DATableDataTest::testLazyTableFetchBlock()
{
    DATestLazyTable* lazy = new DATestLazyTable();
    DA::DAAbstractData::Pointer p(lazy);
    DA::DAData d(p);
    DA::DATableDataSource* ts = d.tableSource();

    const int before                 = lazy->mFetchCount;
    DA::DATableDataBlock block       = ts->fetchBlock(5000000, 100);
    QCOMPARE(lazy->mFetchCount, before + 1);  // 一次块取数=一次fetch
    QVERIFY(block.isValid());
    QCOMPARE(block.startRow(), static_cast< std::size_t >(5000000));
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(100));
    QCOMPARE(block.columnCount(), DATestLazyTable::c_totalCols);
    QVERIFY(block.containsRow(5000000));
    QVERIFY(block.containsRow(5000099));
    QVERIFY(!block.containsRow(5000100));
    QVERIFY(!block.containsRow(4999999));

    for (std::size_t r = 5000000; r < 5000100; r += 7) {
        for (std::size_t c = 0; c < DATestLazyTable::c_totalCols; ++c) {
            QVERIFY(block.cell(r, c) == DATestLazyTable::makeCell(r, c));
            // 与cell级接口toVariant一致
            QVERIFY(d.value(r, c) == block.cell(r, c));
        }
        QVERIFY(block.rowHeader(r) == QVariant(static_cast< qlonglong >(r)));
    }
    // 越界cell返回无效QVariant
    QVERIFY(!block.cell(5000100, 0).isValid());
    QVERIFY(!block.cell(5000000, 99).isValid());

    // 尾部截断
    DA::DATableDataBlock tail = ts->fetchBlock(DATestLazyTable::c_totalRows - 10, 100);
    QVERIFY(tail.isValid());
    QCOMPARE(tail.rowCount(), static_cast< std::size_t >(10));

    // 完全越界返回无效块
    QVERIFY(!ts->fetchBlock(DATestLazyTable::c_totalRows, 1).isValid());
}

// DATableDataBlock自身的边界行为
void DATableDataTest::testBlockBounds()
{
    DA::DATableDataBlock invalid;
    QVERIFY(!invalid.isValid());
    QVERIFY(invalid.isEmpty());
    QVERIFY(!invalid.containsRow(0));
    QVERIFY(!invalid.cell(0, 0).isValid());

    DA::DATableDataBlock block(10, 2);
    QVERIFY(block.isValid());
    QVERIFY(block.isEmpty());  // 有效但还没有行
    QCOMPARE(block.startRow(), static_cast< std::size_t >(10));
    QCOMPARE(block.columnCount(), static_cast< std::size_t >(2));

    QVariantList row;
    row << QVariant(1) << QVariant(QStringLiteral("a"));
    block.appendRow(row);
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(1));
    QVERIFY(block.containsRow(10));
    QVERIFY(!block.containsRow(9));
    QVERIFY(!block.containsRow(11));
    QCOMPARE(block.cell(10, 0), QVariant(1));
    QCOMPARE(block.cell(10, 1).toString(), QStringLiteral("a"));

    block.clear();
    QVERIFY(!block.isValid());
}

// pandas DataFrame：fetchBlock与逐cell iat的等价性（值+类型）
void DATableDataTest::testPandasDataFrameEquivalence()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    QVERIFY(d.isDataFrame());
    QVERIFY(d.isTable());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("DataFrame"));
    QVERIFY(!d.isReferenceData());
    QVERIFY(d.supportsUndoSnapshot());

    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(10));
    QCOMPARE(sp.second, static_cast< std::size_t >(5));

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QVERIFY(!ts->isLazyLoaded());      // pandas数据全量在内存
    QVERIFY(ts->isTableEditable());    // 可编辑

    // 列名与现有columns()一致
    const QList< QString > cols = df.columns();
    QCOMPARE(static_cast< std::size_t >(cols.size()), ts->tableColumnCount());
    for (std::size_t c = 0; c < ts->tableColumnCount(); ++c) {
        QCOMPARE(ts->tableColumnName(c), cols[ static_cast< int >(c) ]);
    }

    // 列类型与caster转换语义对齐
    QCOMPARE(ts->tableColumnType(0), static_cast< int >(QMetaType::LongLong));   // int64
    QCOMPARE(ts->tableColumnType(1), static_cast< int >(QMetaType::Double));     // float64
    QCOMPARE(ts->tableColumnType(2), static_cast< int >(QMetaType::UnknownType));  // object(str)
    QCOMPARE(ts->tableColumnType(3), static_cast< int >(QMetaType::QDateTime));   // datetime64
    QCOMPARE(ts->tableColumnType(4), static_cast< int >(QMetaType::Bool));       // bool

    // 全表块取数与逐cell iat等价（值+QVariant类型）
    DA::DATableDataBlock block = ts->fetchBlock(0, 10);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(10));
    for (std::size_t r = 0; r < 10; ++r) {
        for (std::size_t c = 0; c < 5; ++c) {
            QVariant expect = df.iat(r, c);
            QVariant got    = block.cell(r, c);
            QVERIFY2(variantEquals(expect, got),
                     qPrintable(QStringLiteral("cell(%1,%2) mismatch: iat=%3 block=%4")
                                    .arg(r)
                                    .arg(c)
                                    .arg(expect.toString())
                                    .arg(got.toString())));
            QCOMPARE(got.userType(), expect.userType());
        }
        // 行头与index().value(i)等价
        QVERIFY(block.rowHeader(r) == df.index().value(r));
    }

    // 部分块：fetchBlock(4,3)覆盖绝对行4~6
    DA::DATableDataBlock part = ts->fetchBlock(4, 3);
    QVERIFY(part.isValid());
    QCOMPARE(part.rowCount(), static_cast< std::size_t >(3));
    QVERIFY(part.containsRow(4));
    QVERIFY(part.containsRow(6));
    QVERIFY(!part.containsRow(7));
    QVERIFY(variantEquals(part.cell(5, 1), df.iat(5, 1)));

    // 尾部截断与越界
    DA::DATableDataBlock tail = ts->fetchBlock(9, 5);
    QVERIFY(tail.isValid());
    QCOMPARE(tail.rowCount(), static_cast< std::size_t >(1));
    QVERIFY(!ts->fetchBlock(10, 5).isValid());
}

// pandas Series：单列表格语义与逐元素value()等价
void DATableDataTest::testPandasSeriesEquivalence()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestSeries();
    QVERIFY(!obj.is_none());
    DA::DAPySeries ser(obj);
    DA::DAData d(ser);
    QVERIFY(d.isSeries());
    QVERIFY(d.isTable());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("Series"));

    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(6));
    QCOMPARE(sp.second, static_cast< std::size_t >(1));

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QCOMPARE(ts->tableColumnName(0), QStringLiteral("ser"));
    QCOMPARE(ts->tableColumnType(0), static_cast< int >(QMetaType::Double));

    DA::DATableDataBlock block = ts->fetchBlock(1, 3);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(3));
    QCOMPARE(block.columnCount(), static_cast< std::size_t >(1));
    for (std::size_t r = 1; r < 4; ++r) {
        QVariant expect = ser.value(r);
        QVariant got    = block.cell(r, 0);
        QVERIFY2(variantEquals(expect, got),
                 qPrintable(QStringLiteral("series cell(%1) mismatch").arg(r)));
        QCOMPARE(got.userType(), expect.userType());
        QVERIFY(block.rowHeader(r) == ser.index().value(r));
    }
    // 与cell级接口toVariant一致
    QVERIFY(variantEquals(d.value(2, 0), block.cell(2, 0)));

    // int64 series：批量取数须保持numpy标量语义（LongLong，与iat一致），
    // 防止pandas 3.0把元素转成python int导致类型退化为Int
    pybind11::object objInt = makeTestSeriesInt();
    QVERIFY(!objInt.is_none());
    DA::DAPySeries serInt(objInt);
    DA::DAData di(serInt);
    DA::DATableDataSource* tsi = di.tableSource();
    QVERIFY(tsi != nullptr);
    QCOMPARE(tsi->tableColumnType(0), static_cast< int >(QMetaType::LongLong));
    DA::DATableDataBlock blockInt = tsi->fetchBlock(0, 4);
    QVERIFY(blockInt.isValid());
    for (std::size_t r = 0; r < 4; ++r) {
        QVariant expect = serInt.value(r);
        QVariant got    = blockInt.cell(r, 0);
        QCOMPARE(got, expect);
        QCOMPARE(got.userType(), expect.userType());
        QCOMPARE(got.userType(), static_cast< int >(QMetaType::LongLong));
    }
}

// 空DataFrame的退化行为
void DATableDataTest::testEmptyDataFrame()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    DA::DAPyDataFrame df;  // 默认构造空DataFrame
    DA::DAData d(df);
    QVERIFY(d.isDataFrame());
    QVERIFY(d.isTable());
    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(0));
    QCOMPARE(sp.second, static_cast< std::size_t >(0));
    QVERIFY(!d.tableSource()->fetchBlock(0, 10).isValid());
}

// 工厂注册表与引用式序列化往返
void DATableDataTest::testFactoryRoundTrip()
{
    const QString tid = QStringLiteral("DATableDataTest.LazyTable");

    // 未注册时create返回空
    QVERIFY(!DA::DADataFactory::contains(tid));
    QVERIFY(DA::DADataFactory::create(tid) == nullptr);

    // 注册
    QVERIFY(DA::DADataFactory::registerCreator(tid, []() {
        return DA::DAAbstractData::Pointer(new DATestLazyTable());
    }));
    QVERIFY(DA::DADataFactory::contains(tid));
    QVERIFY(DA::DADataFactory::registeredTypeIdentifiers().contains(tid));
    // 空参数注册失败
    QVERIFY(!DA::DADataFactory::registerCreator(QString(), []() { return DA::DAAbstractData::Pointer(); }));
    QVERIFY(!DA::DADataFactory::registerCreator(tid, DA::DADataFactory::Creator()));

    // 引用式序列化：write写payload
    QByteArray payload;
    {
        DATestLazyTable src;
        src.mRefName = QStringLiteral("db://test/table1");
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDataStream out(&buf);
        src.write(out);
    }
    QVERIFY(!payload.isEmpty());

    // 工厂重建 + read恢复引用
    DA::DAAbstractData::Pointer p = DA::DADataFactory::create(tid);
    QVERIFY(p != nullptr);
    QCOMPARE(p->typeIdentifier(), tid);
    QVERIFY(p->isReferenceData());
    QVERIFY(p->tableSource() != nullptr);
    {
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::ReadOnly));
        QDataStream in(&buf);
        QVERIFY(p->read(in));
    }
    DATestLazyTable* lazy = dynamic_cast< DATestLazyTable* >(p.get());
    QVERIFY(lazy != nullptr);
    QCOMPARE(lazy->mRefName, QStringLiteral("db://test/table1"));
    // 重建后立即可用
    DA::DATableDataBlock block = lazy->fetchBlock(0, 5);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(5));

    // 注销
    DA::DADataFactory::unregisterCreator(tid);
    QVERIFY(!DA::DADataFactory::contains(tid));
    QVERIFY(DA::DADataFactory::create(tid) == nullptr);
}

// DADataTableModel接入惰性表：窗口映射、schema缓存、只读门禁
void DATableDataTest::testModelLazyDisplay()
{
    DATestLazyTable* lazy = new DATestLazyTable();
    DA::DAAbstractData::Pointer p(lazy);
    DA::DAData d(p);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);

    // actualRowCount走schema缓存，千万行不被滑动窗截断
    QCOMPARE(model.actualRowCount(), static_cast< int >(DATestLazyTable::c_totalRows));
    // 视图行数=滑动窗上限，规避QTableView超大行数问题
    QVERIFY(model.rowCount() < model.actualRowCount());
    QCOMPARE(model.rowCount(), qMin(model.getCacheWindowSize(), model.actualRowCount()));
    // 列数=5数据列+扩展列
    QCOMPARE(model.columnCount(), static_cast< int >(DATestLazyTable::c_totalCols) + model.getExtraColumnCount());
    // 水平表头来自tableColumnName
    QCOMPARE(model.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("col2"));
    // 垂直表头来自块rowHeaders
    QVERIFY(model.headerData(3, Qt::Vertical, Qt::DisplayRole) == QVariant(static_cast< qlonglong >(3)));
    // 只读：无编辑标志，setData被拒绝
    // （经基类指针调用，DADataTableModel::setData(DAData)会隐藏基类三参重载）
    QVERIFY(!(model.flags(model.index(0, 0)) & Qt::ItemIsEditable));
    QAbstractItemModel* baseModel = &model;
    QVERIFY(!baseModel->setData(baseModel->index(0, 0), QVariant(123), Qt::EditRole));
    // cell值经块缓存正确映射
    QVERIFY(model.data(model.index(3, 0), Qt::DisplayRole) == DATestLazyTable::makeCell(3, 0));
    QVERIFY(model.data(model.index(39, 4), Qt::DisplayRole) == DATestLazyTable::makeCell(39, 4));
    // 超出数据范围的扩展列/行返回无效值
    QVERIFY(!model.data(model.index(0, static_cast< int >(DATestLazyTable::c_totalCols)), Qt::DisplayRole).isValid());
}

// 块缓存命中与滚动窗口：扫描可见区不重复取数，滚动一次只预取一块
void DATableDataTest::testModelBlockCacheAndScroll()
{
    DATestLazyTable* lazy = new DATestLazyTable();
    DA::DAAbstractData::Pointer p(lazy);
    DA::DAData d(p);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);  // refreshData会预取窗口起始块
    const int fetchAfterSet = lazy->mFetchCount;
    QVERIFY(fetchAfterSet >= 1);

    // 扫描可见区40行×5列+行头：全部命中同一缓存块，零额外取数
    for (int r = 0; r < 40; ++r) {
        for (int c = 0; c < 5; ++c) {
            QVERIFY(model.data(model.index(r, c), Qt::DisplayRole) == DATestLazyTable::makeCell(r, c));
            model.headerData(r, Qt::Vertical, Qt::DisplayRole);
        }
    }
    QCOMPARE(lazy->mFetchCount, fetchAfterSet);

    // 滚动窗口到表中段：预取一次
    model.setCacheWindowStartRow(5000000);
    const int fetchAfterScroll = lazy->mFetchCount;
    QVERIFY(fetchAfterScroll > fetchAfterSet);
    // 视图行r映射到绝对行5000000+r
    for (int r = 0; r < 40; ++r) {
        QVERIFY(model.data(model.index(r, 0), Qt::DisplayRole) == DATestLazyTable::makeCell(5000000 + r, 0));
    }
    // 预取块覆盖可见区，扫描不再触发取数
    QCOMPARE(lazy->mFetchCount, fetchAfterScroll);
}

// pandas经统一块路径后的显示与编辑回归：cell编辑、undo、缓存失效
void DATableDataTest::testModelPandasEditRegression()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);

    // pandas表可编辑
    QVERIFY(model.flags(model.index(0, 0)) & Qt::ItemIsEditable);
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 1LL);
    QCOMPARE(model.data(model.index(9, 0), Qt::EditRole).toLongLong(), 10LL);

    // 经undo栈的cell编辑（经基类指针调用三参setData）
    QAbstractItemModel* baseModel = &model;
    QVERIFY(baseModel->setData(baseModel->index(0, 0), QVariant(42), Qt::EditRole));
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 42LL);
    // DisplayRole同步更新（块缓存已被notify回调失效）
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toLongLong(), 42LL);

    // undo还原
    mUndoStack.undo();
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 1LL);
    // redo再应用
    mUndoStack.redo();
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 42LL);
    mUndoStack.clear();
}

// SQLite惰性表：schema/COUNT/LIMIT-OFFSET分页/能力位——数据库插件数据层样板验证
void DATableDataTest::testSqliteLazyTable()
{
    if (!ensureSqliteReady()) {
        QSKIP(qPrintable(mSqliteErr));
    }
    DA::DAAbstractData::Pointer p = makeSqliteLazyData();
    DASqliteLazyTable* tbl        = static_cast< DASqliteLazyTable* >(p.get());
    DA::DAData d(p);
    QVERIFY(d.isTable());
    QVERIFY(d.isReferenceData());
    QVERIFY(!d.supportsUndoSnapshot());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("DATableDataTest.SqliteLazyTable"));

    // 百万行schema经COUNT(*)与LIMIT 0探针，全表不进内存
    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(c_sqliteRows));
    QCOMPARE(sp.second, static_cast< std::size_t >(3));

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QVERIFY(ts->isLazyLoaded());
    QVERIFY(!ts->isTableEditable());
    QCOMPARE(ts->tableColumnName(0), QStringLiteral("id"));
    QCOMPARE(ts->tableColumnName(1), QStringLiteral("val"));
    QCOMPARE(ts->tableColumnName(2), QStringLiteral("label"));
    // REAL→Double、TEXT→QString；INTEGER经LIMIT 0探针在Qt5驱动下报Int（有值时为LongLong），
    // 列类型是提示性信息，整数族即可
    const int idType = ts->tableColumnType(0);
    QVERIFY(idType == QMetaType::Int || idType == QMetaType::LongLong);
    QCOMPARE(ts->tableColumnType(1), static_cast< int >(QMetaType::Double));
    QCOMPARE(ts->tableColumnType(2), static_cast< int >(QMetaType::QString));

    // 分页块取数：表中部一次查询取100行
    const int fetchBefore          = tbl->fetchCount();
    DA::DATableDataBlock block     = ts->fetchBlock(123456, 100);
    QCOMPARE(tbl->fetchCount(), fetchBefore + 1);
    QVERIFY(block.isValid());
    QCOMPARE(block.startRow(), static_cast< std::size_t >(123456));
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(100));
    QCOMPARE(block.cell(123456, 0).toLongLong(), 123456LL);
    QCOMPARE(block.cell(123456, 1).toDouble(), 123456 * 0.5);
    QCOMPARE(block.cell(123555, 2).toString(), QStringLiteral("row123555"));
    // 未提供行头（模型将回退显示序号）
    QVERIFY(!block.rowHeader(123456).isValid());

    // 尾部截断与越界
    DA::DATableDataBlock tail = ts->fetchBlock(static_cast< std::size_t >(c_sqliteRows) - 5, 100);
    QVERIFY(tail.isValid());
    QCOMPARE(tail.rowCount(), static_cast< std::size_t >(5));
    QVERIFY(!ts->fetchBlock(static_cast< std::size_t >(c_sqliteRows), 1).isValid());

    // cell级接口与只读语义
    QCOMPARE(d.value(7, 2).toString(), QStringLiteral("row7"));
    QVERIFY(!d.setValue(7, 2, QVariant(QStringLiteral("x"))));
}

// SQLite惰性表经DADataTableModel的窗口显示与滚动取数
void DATableDataTest::testSqliteModelIntegration()
{
    if (!ensureSqliteReady()) {
        QSKIP(qPrintable(mSqliteErr));
    }
    DA::DAAbstractData::Pointer p = makeSqliteLazyData();
    DASqliteLazyTable* tbl        = static_cast< DASqliteLazyTable* >(p.get());
    DA::DAData d(p);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);
    QCOMPARE(model.actualRowCount(), static_cast< int >(c_sqliteRows));
    QVERIFY(model.rowCount() < model.actualRowCount());  // 滑动窗截断，视图不遍历百万行
    QCOMPARE(model.columnCount(), 3 + model.getExtraColumnCount());
    QCOMPARE(model.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("label"));

    // 只读门禁
    QVERIFY(!(model.flags(model.index(0, 0)) & Qt::ItemIsEditable));
    QAbstractItemModel* baseModel = &model;
    QVERIFY(!baseModel->setData(baseModel->index(0, 0), QVariant(1), Qt::EditRole));

    // 表头区数据与行头序号回退
    QCOMPARE(model.data(model.index(7, 0), Qt::DisplayRole).toLongLong(), 7LL);
    QCOMPARE(model.data(model.index(7, 2), Qt::DisplayRole).toString(), QStringLiteral("row7"));
    QCOMPARE(model.headerData(7, Qt::Vertical, Qt::DisplayRole).toLongLong(), 7LL);

    // 滚动到表尾区域：窗口起始行会被clamp到 总行数-窗口大小+扩展行，
    // 取clamp后的最大起始行验证表尾显示
    const int tailStart = model.actualRowCount() - model.rowCount() + model.getExtraRowCount();
    model.setCacheWindowStartRow(tailStart);
    QCOMPARE(model.getCacheWindowStartRow(), tailStart);
    const int fetchAfterJump = tbl->fetchCount();
    QVERIFY(fetchAfterJump >= 1);
    // 预取一次后扫描可见区零额外查询
    for (int r = 0; r < 40; ++r) {
        QCOMPARE(model.data(model.index(r, 0), Qt::DisplayRole).toLongLong(), static_cast< qlonglong >(tailStart + r));
        model.data(model.index(r, 1), Qt::DisplayRole);
        model.data(model.index(r, 2), Qt::DisplayRole);
        model.headerData(r, Qt::Vertical, Qt::DisplayRole);
    }
    QCOMPARE(tbl->fetchCount(), fetchAfterJump);
    // 窗口最远数据行（表尾最后一行）触发一次新块查询后正确映射
    const int lastViewRow = model.rowCount() - 1 - model.getExtraRowCount();
    QCOMPARE(model.data(model.index(lastViewRow, 2), Qt::DisplayRole).toString(),
             QStringLiteral("row%1").arg(c_sqliteRows - 1));
    QVERIFY(tbl->fetchCount() <= fetchAfterJump + 2);
}

// SQLite惰性表引用式序列化：工厂注册→write→create→read→立即可取数（模拟工程保存/加载）
void DATableDataTest::testSqliteReferenceRoundTrip()
{
    if (!ensureSqliteReady()) {
        QSKIP(qPrintable(mSqliteErr));
    }
    const QString tid = QStringLiteral("DATableDataTest.SqliteLazyTable");
    QVERIFY(DA::DADataFactory::registerCreator(tid, []() {
        return DA::DAAbstractData::Pointer(new DASqliteLazyTable());
    }));

    // 原对象写出引用payload（连接名+SELECT）
    DA::DAAbstractData::Pointer src = makeSqliteLazyData();
    QByteArray payload;
    {
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDataStream out(&buf);
        src->write(out);
    }
    QVERIFY(!payload.isEmpty());
    QVERIFY(payload.size() < 1024);  // 引用payload是常量级，与表规模无关

    // 工厂重建+read恢复（工程加载路径）
    DA::DAAbstractData::Pointer rebuilt = DA::DADataFactory::create(tid);
    QVERIFY(rebuilt != nullptr);
    {
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::ReadOnly));
        QDataStream in(&buf);
        QVERIFY(rebuilt->read(in));
    }
    DASqliteLazyTable* tbl = dynamic_cast< DASqliteLazyTable* >(rebuilt.get());
    QVERIFY(tbl != nullptr);
    QCOMPARE(tbl->connectionName(), mSqliteConnName);
    QCOMPARE(tbl->selectSql(), QStringLiteral("SELECT id, val, label FROM bigtable ORDER BY id"));

    // 重建后立即可取数（连接池仍在；真实插件场景由read/首次访问时重连）
    QCOMPARE(tbl->tableRowCount(), static_cast< std::size_t >(c_sqliteRows));
    DA::DATableDataBlock block = tbl->fetchBlock(0, 3);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(3));
    QCOMPARE(block.cell(2, 2).toString(), QStringLiteral("row2"));

    DA::DADataFactory::unregisterCreator(tid);
}

//===================================================
// 表格元数据基础设施（001 计划 §4.1 单测）
//===================================================

// 高亮层：set/levelAt/clear(Source)/clear(Source,Level)/clearAll、来源隔离、级别隔离、changed 信号
void DATableDataTest::testHighlightOverlayBasics()
{
    using DA::DATableHighlightOverlay;
    DATableHighlightOverlay overlay;
    QVERIFY(overlay.isEmpty());
    QCOMPARE(overlay.levelAt(0, 0), DATableHighlightOverlay::Level::None);

    // Find 命中装载
    QSignalSpy spy(&overlay, &DATableHighlightOverlay::changed);
    overlay.set(DATableHighlightOverlay::Source::Find,
                DATableHighlightOverlay::Level::Hit,
                { qMakePair(1, 2), qMakePair(3, 4) });
    QCOMPARE(spy.count(), 1);
    QVERIFY(!overlay.isEmpty());
    QCOMPARE(overlay.levelAt(1, 2), DATableHighlightOverlay::Level::Hit);
    QCOMPARE(overlay.levelAt(3, 4), DATableHighlightOverlay::Level::Hit);
    QCOMPARE(overlay.levelAt(9, 9), DATableHighlightOverlay::Level::None);

    // 级别隔离：同来源设 Hit 后再设 CurrentHit，Hit 集合仍在
    overlay.set(DATableHighlightOverlay::Source::Find,
                DATableHighlightOverlay::Level::CurrentHit,
                { qMakePair(1, 2) });
    QCOMPARE(overlay.levelAt(1, 2), DATableHighlightOverlay::Level::CurrentHit);
    QCOMPARE(overlay.levelAt(3, 4), DATableHighlightOverlay::Level::Hit);

    // Validation 来源与 Find 并存，Error/Warning 两级并存
    overlay.set(DATableHighlightOverlay::Source::Validation,
                DATableHighlightOverlay::Level::ViolationError,
                { qMakePair(5, 5) });
    overlay.set(DATableHighlightOverlay::Source::Validation,
                DATableHighlightOverlay::Level::ViolationWarning,
                { qMakePair(6, 6) });
    QCOMPARE(overlay.levelAt(5, 5), DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::ViolationWarning);

    // 来源隔离：清 Find 不动 Validation
    overlay.clear(DATableHighlightOverlay::Source::Find);
    QCOMPARE(overlay.levelAt(1, 2), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(3, 4), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(5, 5), DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::ViolationWarning);

    // clear(Source, Level) 只清该级别（查找导航单独重设 CurrentHit 的场景）
    overlay.clear(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay.levelAt(5, 5), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::ViolationWarning);

    // clearAll
    overlay.clearAll();
    QVERIFY(overlay.isEmpty());
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::None);
}

// 高亮层：levelAt 固定优先级 CurrentHit > Hit > ViolationError > ViolationWarning；
// 同格既是查找命中又是校验违规时以 Find 为准（有意决策，本用例钉死，不许改成校验优先）
void DATableDataTest::testHighlightOverlayPriority()
{
    using DA::DATableHighlightOverlay;
    DATableHighlightOverlay overlay;

    // (0,0)：四级别同格 → CurrentHit 胜出
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, { qMakePair(0, 0) });
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit, { qMakePair(0, 0) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, { qMakePair(0, 0) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, { qMakePair(0, 0) });
    QCOMPARE(overlay.levelAt(0, 0), DATableHighlightOverlay::Level::CurrentHit);

    // (1,1)：Find Hit 与 Validation Error 同格 → 以 Find 为准
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, { qMakePair(1, 1) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, { qMakePair(1, 1) });
    QCOMPARE(overlay.levelAt(1, 1), DATableHighlightOverlay::Level::Hit);

    // (2,2)：Error 压过 Warning
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, { qMakePair(2, 2) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, { qMakePair(2, 2) });
    QCOMPARE(overlay.levelAt(2, 2), DATableHighlightOverlay::Level::ViolationError);

    // (3,3)：仅 Warning
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, { qMakePair(3, 3) });
    QCOMPARE(overlay.levelAt(3, 3), DATableHighlightOverlay::Level::ViolationWarning);

    // 语义色映射：命中/警告=金黄、当前命中=主蓝、错误=橙红（官方色板）、None 无效
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::Hit), QColor(0xE6, 0xC2, 0x7C));
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::CurrentHit), QColor(0x52, 0x80, 0xC1));
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::ViolationError), QColor(0xCE, 0x60, 0x43));
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::ViolationWarning), QColor(0xE6, 0xC2, 0x7C));
    QVERIFY(!DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::None).isValid());
}

// 高亮层：打包键 (row<<32)|col 的边界——col=0 与 col=2^31-1 不串行、行维度不串行
void DATableDataTest::testHighlightOverlayKeyPacking()
{
    using DA::DATableHighlightOverlay;
    DATableHighlightOverlay overlay;
    const int maxCol = 2147483647;  // 2^31 - 1（键打包前提：行/列号 < 2^31）

    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, { qMakePair(7, 0) });
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit, { qMakePair(7, maxCol) });
    QCOMPARE(overlay.levelAt(7, 0), DATableHighlightOverlay::Level::Hit);
    QCOMPARE(overlay.levelAt(7, maxCol), DATableHighlightOverlay::Level::CurrentHit);
    // 行维度不串：相邻行的同列、同行的相邻列都无命中
    QCOMPARE(overlay.levelAt(6, 0), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(8, 0), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(6, maxCol), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(7, 1), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(7, maxCol - 1), DATableHighlightOverlay::Level::None);
}

// 重映射工具：置换/逆置换、行键与单元格键（按行/按列）、置换+逆置换还原、空置换、单元素置换
void DATableDataTest::testKeyRemapUtility()
{
    using DA::DATableKeyRemap;
    // 置换语义钉死：perm[i] = 新表第 i 行在旧表中的行号（与 df.iloc[perm] 一致）
    const QVector< int > perm { 2, 0, 1 };  // 新行0←旧行2，新行1←旧行0，新行2←旧行1
    const QVector< int > inv = DATableKeyRemap::inversePermutation(perm);
    const QVector< int > invExpected { 1, 2, 0 };  // inv[旧行号]=新位置
    QCOMPARE(inv, invExpected);

    // 行键重映射
    QHash< int, QString > rowKeys;
    rowKeys.insert(0, QStringLiteral("a"));
    rowKeys.insert(1, QStringLiteral("b"));
    rowKeys.insert(2, QStringLiteral("c"));
    DATableKeyRemap::remapKeys(rowKeys, perm);
    QCOMPARE(rowKeys.value(0), QStringLiteral("c"));  // 旧行2 → 新行0
    QCOMPARE(rowKeys.value(1), QStringLiteral("a"));  // 旧行0 → 新行1
    QCOMPARE(rowKeys.value(2), QStringLiteral("b"));  // 旧行1 → 新行2

    // 置换 + 逆置换 = 还原
    DATableKeyRemap::remapKeys(rowKeys, inv);
    QCOMPARE(rowKeys.value(0), QStringLiteral("a"));
    QCOMPARE(rowKeys.value(1), QStringLiteral("b"));
    QCOMPARE(rowKeys.value(2), QStringLiteral("c"));

    // 单元格键按行维重映射（列维不变）
    QHash< QPair< int, int >, QString > cells;
    cells.insert(qMakePair(0, 5), QStringLiteral("x"));
    cells.insert(qMakePair(2, 7), QStringLiteral("y"));
    DATableKeyRemap::remapKeys(cells, perm, DATableKeyRemap::Dimension::Row);
    QCOMPARE(cells.value(qMakePair(1, 5)), QStringLiteral("x"));  // 旧行0 → 新行1
    QCOMPARE(cells.value(qMakePair(0, 7)), QStringLiteral("y"));  // 旧行2 → 新行0
    QVERIFY(!cells.contains(qMakePair(0, 5)));
    QCOMPARE(cells.size(), 2);

    // 单元格键按列维重映射（行维不变）：列置换交换 5↔7
    const QVector< int > colPerm { 0, 1, 2, 3, 4, 7, 6, 5 };  // 新列5←旧列7，新列7←旧列5
    DATableKeyRemap::remapKeys(cells, colPerm, DATableKeyRemap::Dimension::Column);
    QCOMPARE(cells.value(qMakePair(1, 7)), QStringLiteral("x"));
    QCOMPARE(cells.value(qMakePair(0, 5)), QStringLiteral("y"));

    // 空置换：新表 0 行，全部键失效
    QHash< int, QString > h0;
    h0.insert(0, QStringLiteral("a"));
    DATableKeyRemap::remapKeys(h0, QVector< int >());
    QVERIFY(h0.isEmpty());

    // 单元素置换
    QHash< int, QString > h1;
    h1.insert(0, QStringLiteral("only"));
    DATableKeyRemap::remapKeys(h1, QVector< int > { 0 });
    QCOMPARE(h1.size(), 1);
    QCOMPARE(h1.value(0), QStringLiteral("only"));

    // 未被置换覆盖的旧键丢弃（新表行数少于旧表）
    QHash< int, QString > h2;
    h2.insert(0, QStringLiteral("a"));
    h2.insert(5, QStringLiteral("far"));
    DATableKeyRemap::remapKeys(h2, QVector< int > { 0 });
    QCOMPARE(h2.size(), 1);
    QCOMPARE(h2.value(0), QStringLiteral("a"));
}

// DATableStyleManager::onRowsReordered：单元格/行样式键位移正确、styleReset 发出、
// 列级样式不受影响、与 onRowsInserted/onRowsRemoved 不互相干扰、空置换为无操作
void DATableDataTest::testStyleManagerRowsReordered()
{
    DA::DATableStyleManager mgr;
    auto makeStyle = [](const QColor& c) {
        DA::DATableCellStyle s;
        s.setBackground(QBrush(c));
        return s;
    };
    const QColor red(255, 0, 0), green(0, 255, 0), blue(0, 0, 255), yellow(255, 255, 0);
    mgr.setCellStyle(0, 1, makeStyle(red));
    mgr.setCellStyle(2, 1, makeStyle(green));
    mgr.setRowStyle(1, makeStyle(blue));
    mgr.setColumnStyle(3, makeStyle(yellow));  // 列级：行重排不应影响

    QSignalSpy resetSpy(&mgr, &DA::DATableStyleManager::styleReset);
    const QVector< int > perm { 2, 0, 1 };  // 新表 = 旧表 iloc[[2,0,1]]
    mgr.onRowsReordered(perm);
    QCOMPARE(resetSpy.count(), 1);
    // 旧行2(green)→新行0；旧行0(red)→新行1；旧行1的单元格无、行样式 blue→新行2
    QCOMPARE(mgr.getCellStyle(0, 1).background().color(), green);
    QCOMPARE(mgr.getCellStyle(1, 1).background().color(), red);
    QVERIFY(!mgr.hasCellStyle(2, 1));
    QCOMPARE(mgr.getRowStyle(2).background().color(), blue);
    QVERIFY(!mgr.hasRowStyle(0));
    QVERIFY(!mgr.hasRowStyle(1));
    // 列级样式不受行重排影响
    QCOMPARE(mgr.getColumnStyle(3).background().color(), yellow);

    // 与 onRowsInserted/onRowsRemoved 不互相干扰：头部插一行键 +1，删回还原
    mgr.onRowsInserted({ 0 });
    QCOMPARE(mgr.getCellStyle(1, 1).background().color(), green);
    QCOMPARE(mgr.getCellStyle(2, 1).background().color(), red);
    mgr.onRowsRemoved({ 0 });
    QCOMPARE(mgr.getCellStyle(0, 1).background().color(), green);
    QCOMPARE(mgr.getCellStyle(1, 1).background().color(), red);

    // 空置换 = 无操作：不发信号、不清存储（与 on*Inserted/Removed 的空表行为一致）
    const int resetCount = resetSpy.count();
    mgr.onRowsReordered(QVector< int >());
    QCOMPARE(resetSpy.count(), resetCount);
    QCOMPARE(mgr.getCellStyle(0, 1).background().color(), green);
}

// DATableViewStateStore：XML round-trip（列宽按名/行头宽）、renameColumn 迁移、
// removeColumn 后重新插入同名不限、重名列 occurrence 消歧互不影响
void DATableDataTest::testViewStateStore()
{
    DA::DATableViewStateStore store;
    QVERIFY(store.isEmpty());
    QCOMPARE(store.columnWidth(QStringLiteral("value")), -1);
    QCOMPARE(store.rowHeaderWidth(), -1);

    store.setColumnWidth(QStringLiteral("id"), 0, 80);
    store.setColumnWidth(QStringLiteral("value"), 0, 120);
    store.setColumnWidth(QStringLiteral("value"), 1, 200);  // 两个同名 value 列
    store.setRowHeaderWidth(60);
    QVERIFY(!store.isEmpty());
    // 重名列 occurrence 消歧：各自宽度互不影响
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 0), 120);
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 1), 200);
    store.setColumnWidth(QStringLiteral("value"), 0, 130);
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 0), 130);
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 1), 200);

    // XML round-trip
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("table"));
    doc.appendChild(root);
    store.toXml(doc, root);
    QDomElement vsEle = root.firstChildElement(QStringLiteral("view-state"));
    QVERIFY(!vsEle.isNull());
    DA::DATableViewStateStore store2;
    QVERIFY(store2.fromXml(vsEle));
    QCOMPARE(store2.columnWidth(QStringLiteral("id"), 0), 80);
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 0), 130);
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 1), 200);
    QCOMPARE(store2.rowHeaderWidth(), 60);

    // renameColumn 迁移（occurrence 保持）
    store2.renameColumn(QStringLiteral("value"), QStringLiteral("val2"));
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 0), -1);
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 1), -1);
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 0), 130);
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 1), 200);

    // removeColumn 后重新插入同名不限
    store2.removeColumn(QStringLiteral("val2"));
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 0), -1);
    store2.setColumnWidth(QStringLiteral("val2"), 0, 99);
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 0), 99);

    // 空 store 不写出 <view-state> 段；clear 复位
    DA::DATableViewStateStore emptyStore;
    QDomDocument doc2;
    QDomElement root2 = doc2.createElement(QStringLiteral("table"));
    doc2.appendChild(root2);
    emptyStore.toXml(doc2, root2);
    QVERIFY(root2.firstChildElement(QStringLiteral("view-state")).isNull());
    store2.clear();
    QVERIFY(store2.isEmpty());
}

// DATableMetadataRegistry：getOrCreate/get/has/remove/clear/nonEmptyEntries 语义与旧注册表一致；
// 束内子存储随束创建；空束不进保存清单
void DATableDataTest::testMetadataRegistry()
{
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DAAbstractData::Pointer p1(new DATestLazyTable());
    DA::DAAbstractData::Pointer p2(new DATestLazyTable());
    DA::DAData d1(p1);
    DA::DAData d2(p2);

    QVERIFY(!registry.has(d1));
    QVERIFY(registry.get(d1) == nullptr);
    QVERIFY(registry.nonEmptyEntries().isEmpty());

    // getOrCreate：幂等，束内子存储随束创建
    DA::DATableMetadata* m1 = registry.getOrCreate(d1);
    QVERIFY(m1 != nullptr);
    QCOMPARE(registry.get(d1), m1);
    QVERIFY(registry.has(d1));
    QCOMPARE(registry.getOrCreate(d1), m1);
    QVERIFY(m1->styleManager() != nullptr);
    QVERIFY(m1->viewState() != nullptr);
    QVERIFY(m1->isEmpty());
    // 空束不进保存清单
    QVERIFY(registry.nonEmptyEntries().isEmpty());

    // 非空束进 nonEmptyEntries（保存遍历依据）
    m1->viewState()->setRowHeaderWidth(50);
    QCOMPARE(registry.nonEmptyEntries().size(), 1);
    QCOMPARE(registry.nonEmptyEntries().first().first, d1);
    QCOMPARE(registry.nonEmptyEntries().first().second, m1);

    // 不同数据键互不干扰
    DA::DATableMetadata* m2 = registry.getOrCreate(d2);
    QVERIFY(m2 != nullptr);
    QVERIFY(m2 != m1);
    QCOMPARE(registry.get(d2), m2);

    // remove：显式移除并销毁
    registry.remove(d2);
    QVERIFY(!registry.has(d2));
    QVERIFY(registry.get(d2) == nullptr);
    QCOMPARE(registry.getOrCreate(d2) != m2, true);  // 重新创建是新对象
    registry.remove(d2);

    // clear：清空全部
    registry.clear();
    QVERIFY(!registry.has(d1));
    QVERIFY(registry.get(d1) == nullptr);
    QVERIFY(registry.nonEmptyEntries().isEmpty());
}

// §4.2 验收（程序化）：旧工程 table-styles.xml 的 <table> 内层结构可被元数据束兼容读取
// （内层样式标签与旧文件一致、无 <view-state> 段），新格式 round-trip 样式+视图状态都在
void DATableDataTest::testLegacyTableStylesXmlCompat()
{
    auto makeStyle = [](const QColor& c) {
        DA::DATableCellStyle s;
        s.setBackground(QBrush(c));
        return s;
    };
    const QColor red(255, 0, 0), blue(0, 0, 255), green(0, 255, 0);

    // 构造旧工程 table-styles.xml 的 <table> 内层（内层标签由 DATableStyleManager::toXml
    // 写出，与旧文件逐字节同构），并做一次「文件文本」round-trip 模拟真实解析路径
    DA::DATableStyleManager src;
    src.setColumnStyle(1, makeStyle(red));
    src.setRowStyle(2, makeStyle(blue));
    src.setCellStyle(3, 0, makeStyle(green));
    QDomDocument doc;
    QDomElement tableEle = doc.createElement(QStringLiteral("table"));
    tableEle.setAttribute(QStringLiteral("data-id"), QStringLiteral("42"));
    tableEle.setAttribute(QStringLiteral("data-name"), QStringLiteral("legacy"));
    doc.appendChild(tableEle);
    src.toXml(doc, tableEle);
    const QByteArray legacyText = doc.toByteArray();

    QDomDocument parsed;
    QVERIFY(parsed.setContent(legacyText));
    QDomElement parsedTable = parsed.documentElement();
    QVERIFY(parsedTable.firstChildElement(QStringLiteral("view-state")).isNull());  // 旧文件无 view-state 段

    DA::DATableMetadata meta;
    QVERIFY(meta.fromXml(parsedTable));
    QCOMPARE(meta.styleManager()->getColumnStyle(1).background().color(), red);
    QCOMPARE(meta.styleManager()->getRowStyle(2).background().color(), blue);
    QCOMPARE(meta.styleManager()->getCellStyle(3, 0).background().color(), green);
    QVERIFY(meta.viewState()->isEmpty());  // 旧文件没有视图状态：为空而非报错
    QVERIFY(!meta.isEmpty());              // 但有样式：进保存清单（另存即升级为新文件内容）

    // 新格式 round-trip：样式 + <view-state> 都恢复
    meta.viewState()->setColumnWidth(QStringLiteral("f"), 0, 111);
    meta.viewState()->setRowHeaderWidth(55);
    QDomDocument doc2;
    QDomElement t2 = doc2.createElement(QStringLiteral("table"));
    doc2.appendChild(t2);
    meta.toXml(doc2, t2);
    QDomDocument parsed2;
    QVERIFY(parsed2.setContent(doc2.toByteArray()));
    DA::DATableMetadata meta2;
    QVERIFY(meta2.fromXml(parsed2.documentElement()));
    QCOMPARE(meta2.styleManager()->getColumnStyle(1).background().color(), red);
    QCOMPARE(meta2.styleManager()->getRowStyle(2).background().color(), blue);
    QCOMPARE(meta2.styleManager()->getCellStyle(3, 0).background().color(), green);
    QCOMPARE(meta2.viewState()->columnWidth(QStringLiteral("f"), 0), 111);
    QCOMPARE(meta2.viewState()->rowHeaderWidth(), 55);
}

// §4.2 验收（程序化）：列头菜单 6 项 4 组（顺序/分隔符与迁移前逐项一致、图标取 DAGui 资源）；
// 表体菜单 v1 三项；无选区全部置灰。离屏构建 QMenu 断言 action 文本序列，不弹出。
void DATableDataTest::testContextMenuComposition()
{
    // ---- 列头菜单：重命名列/删除列 ‖ 复制列名 ‖ 定位最大/定位最小 ‖ 描述统计 ----
    DA::DADataHeaderView hv(Qt::Horizontal, nullptr);
    QMenu* menu = hv.buildColumnContextMenu(0);
    QVERIFY(menu != nullptr);
    QStringList seq;
    const auto headerActions = menu->actions();
    for (QAction* a : headerActions) {
        seq << (a->isSeparator() ? QStringLiteral("<SEP>") : a->text());
        if (!a->isSeparator()) {
            QVERIFY2(!a->icon().isNull(), qPrintable(QStringLiteral("icon empty for %1").arg(a->text())));
        }
    }
    const QStringList expectedHeader { QStringLiteral("Rename Column"),
                                       QStringLiteral("Remove Column"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Copy Column Name"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Goto Max"),
                                       QStringLiteral("Goto Min"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Show Statistics") };
    QCOMPARE(seq, expectedHeader);
    delete menu;

    // ---- 表体菜单：剪切/复制/粘贴 ‖ 查找/替换（002）；无有效选区（无模型）----
    // 启用规则：剪切/复制/粘贴无选区置灰；查找恒可用；替换在只读源置灰（无模型=只读）
    DA::DADataTableView tv;
    QMenu* body = tv.buildContextMenu(QModelIndex());
    QVERIFY(body != nullptr);
    QStringList bodySeq;
    const auto bodyActions = body->actions();
    QHash< QString, QAction* > byText;
    for (QAction* a : bodyActions) {
        bodySeq << (a->isSeparator() ? QStringLiteral("<SEP>") : a->text());
        if (!a->isSeparator()) {
            byText.insert(a->text(), a);
        }
    }
    const QStringList expectedBody { QStringLiteral("Cut"),
                                     QStringLiteral("Copy"),
                                     QStringLiteral("Paste"),
                                     QStringLiteral("<SEP>"),
                                     QStringLiteral("Find..."),
                                     QStringLiteral("Replace...") };
    QCOMPARE(bodySeq, expectedBody);
    // 剪贴板组无选区置灰
    QVERIFY(!byText.value(QStringLiteral("Cut"))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Copy"))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Paste"))->isEnabled());
    // 查找恒可用；替换在无模型（只读）时置灰
    QVERIFY(byText.value(QStringLiteral("Find..."))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Replace..."))->isEnabled());
    delete body;
}

// §4.2 验收（程序化）：列宽/行头宽「拖拽后重开数据页保留」——store 写入后新建页面
// 构造时按列名应用；图表「选择序列」拾取链路回归——点击表头仍发 columnHeaderClicked
void DATableDataTest::testViewStateApplyAndHeaderClickChain()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);

    // 第一次打开：模拟用户拖拽后写入 store（列 f 宽 137、行头宽 66）
    {
        DA::DADataOperateOfDataFrameWidget w(d, &registry);
        DA::DATableViewStateStore* store = w.viewStateStore();
        QVERIFY(store != nullptr);
        store->setColumnWidth(QStringLiteral("f"), 0, 137);
        store->setRowHeaderWidth(66);
    }

    // 「重开」数据页（同一 registry 即同一会话；工程保存/加载由 XML round-trip 用例覆盖）：
    // 构造时按列名应用列宽，行头宽以 setFixedWidth（min=max=w）落地
    DA::DADataOperateOfDataFrameWidget w2(d, &registry);
    DA::DADataTableView* tv = w2.getDataTableView();
    QVERIFY(tv != nullptr);
    QHeaderView* hh = tv->horizontalHeader();
    QVERIFY(hh != nullptr);
    int colF = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model() && hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("f")) {
            colF = c;
            break;
        }
    }
    QVERIFY(colF >= 0);
    QCOMPARE(hh->sectionSize(colF), 137);
    QHeaderView* vh = tv->verticalHeader();
    QVERIFY(vh != nullptr);
    QCOMPARE(vh->minimumWidth(), 66);
    QCOMPARE(vh->maximumWidth(), 66);

    // 图表「选择序列」拾取链路回归（002：链路切换为 DADataHeaderView::clickedWithModifiers，
    // 拾取模式下页面只转发 columnHeaderClicked 不排序）：拾取模式 → 左键点击列头 → columnHeaderClicked
    w2.setHeaderPickMode(true);
    QVERIFY(w2.isHeaderPickMode());
    QSignalSpy clickSpy(&w2, &DA::DADataOperateOfDataFrameWidget::columnHeaderClicked);
    w2.resize(800, 400);
    w2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w2));
    const int xPos = hh->sectionViewportPosition(colF) + qMax(1, hh->sectionSize(colF) / 2);
    const int yPos = qMax(1, hh->viewport()->height() / 2);
    QTest::mouseClick(hh->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(xPos, yPos));
    QVERIFY(clickSpy.count() >= 1);
    QCOMPARE(clickSpy.takeFirst().at(0).toInt(), colF);
    // 拾取模式下带 Ctrl 修饰点击同样只转发（修饰键不被吞、不排序）
    if (mPyScriptsOk) {
        QTest::mouseClick(hh->viewport(), Qt::LeftButton, Qt::ControlModifier, QPoint(xPos, yPos));
        QVERIFY(clickSpy.count() >= 1);
        QCOMPARE(clickSpy.takeFirst().at(0).toInt(), colF);
        QVERIFY(w2.sortSpec().isEmpty());  // 拾取模式不产生排序规格
    }
    // 退出拾取模式后点击列头走排序，不再发 columnHeaderClicked
    if (mPyScriptsOk) {
        w2.setHeaderPickMode(false);
        QTest::mouseClick(hh->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(xPos, yPos));
        QCOMPARE(clickSpy.count(), 0);
        QCOMPARE(w2.sortSpec().size(), 1);
        QCOMPARE(w2.sortSpec().at(0).first, colF);
    }
    w2.hide();
}

//===================================================
// 002 表头排序（T1.3，计划 §4.1 用例）
//===================================================

// 002 §4.1：da_sort 多键 + 置换——单键升/降序结果正确、稳定排序（同值保序）、
// perm 语义（new[i]==old[perm[i]]）、置换往返（逆置换还原）、混合类型抛错
void DATableDataTest::testSortOperatorMultiKeyPerm()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();

    // 单键升序 + perm 语义（new[i] == old[perm[i]]）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"a":[3,1,2],"b":["x","y","z"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QList< QVariant > oldB { df.iat(0, 1), df.iat(1, 1), df.iat(2, 1) };
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, true) }, &perm));
        QCOMPARE(perm, QVector< int >({ 1, 2, 0 }));
        QCOMPARE(df.iat(0, 0).toInt(), 1);
        QCOMPARE(df.iat(1, 0).toInt(), 2);
        QCOMPARE(df.iat(2, 0).toInt(), 3);
        for (int i = 0; i < 3; ++i) {
            QCOMPARE(df.iat(i, 1), oldB.at(perm.at(i)));
        }
    }
    // 单键降序稳定（同值保序：c, b(0), b(2), a(1), a(3)）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"k":["b","a","b","a","c"],"ord":[0,1,2,3,4]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, false) }, &perm));
        const QList< int > expect { 4, 0, 2, 1, 3 };
        for (int i = 0; i < 5; ++i) {
            QCOMPARE(df.iat(i, 1).toInt(), expect.at(i));
        }
    }
    // 多键：A 升序主键，B 降序次键；NaN 恒排最后（组内也一样）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,1,2,2,1],"B":[1.0,2.0,3.0,4.0,None]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, true), qMakePair(1, false) }, &perm));
        QCOMPARE(df.iat(0, 0).toInt(), 1);
        QCOMPARE(df.iat(1, 0).toInt(), 1);
        QCOMPARE(df.iat(2, 0).toInt(), 1);
        QCOMPARE(df.iat(3, 0).toInt(), 2);
        QCOMPARE(df.iat(4, 0).toInt(), 2);
        QCOMPARE(df.iat(0, 1).toDouble(), 2.0);
        QCOMPARE(df.iat(1, 1).toDouble(), 1.0);
        QVERIFY(qIsNaN(df.iat(2, 1).toDouble()));
        QCOMPARE(df.iat(3, 1).toDouble(), 4.0);
        QCOMPARE(df.iat(4, 1).toDouble(), 3.0);
    }
    // 置换往返：随机数据 sort → 逆置换还原 → 正置换再排序（undo/redo 的算子基础）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd, numpy as np
np.random.seed(7)
df = pd.DataFrame({"x":np.random.rand(50),"y":np.random.randint(0,5,50)})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< QVariant > oldX, oldY;
        oldX.reserve(50);
        oldY.reserve(50);
        for (int r = 0; r < 50; ++r) {
            oldX.append(df.iat(r, 0));
            oldY.append(df.iat(r, 1));
        }
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, true) }, &perm));
        QCOMPARE(perm.size(), 50);
        for (int r = 1; r < 50; ++r) {
            QVERIFY(df.iat(r - 1, 0).toDouble() <= df.iat(r, 0).toDouble());
        }
        const QVector< int > inv = DA::DATableKeyRemap::inversePermutation(perm);
        QVERIFY(py.reorderRows(df, inv));
        for (int r = 0; r < 50; ++r) {
            QCOMPARE(df.iat(r, 0), oldX.at(r));
            QCOMPARE(df.iat(r, 1), oldY.at(r));
        }
        QVERIFY(py.reorderRows(df, perm));
        for (int r = 1; r < 50; ++r) {
            QVERIFY(df.iat(r - 1, 0).toDouble() <= df.iat(r, 0).toDouble());
        }
    }
    // 混合类型 object 列：算子抛 ValueError（not comparable）→ wrapper 返回 false，数据不变
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"m":pd.Series([1,"a",2.5], dtype=object)})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< int > perm;
        QVERIFY(!py.sort(df, { qMakePair(0, true) }, &perm));
        QCOMPARE(df.iat(0, 0).toInt(), 1);
        QCOMPARE(df.iat(1, 0).toString(), QLatin1String("a"));
    }
}

// 002 §4.1：DACommandDataFrame_sortRows undo/redo 语义 + onRowsReordered 样式键随排序联动
void DATableDataTest::testSortRowsCommandUndoRedoAndStyleRemap()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"a":[3,1,2],"b":["x","y","z"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    // 行 0（a=3）设单元格样式，验证排序后样式键随行移动
    DA::DATableStyleManager mgr;
    DA::DATableCellStyle red;
    red.setBackground(QColor(Qt::red));
    mgr.setCellStyle(0, 0, red, false);
    QVERIFY(mgr.hasCellStyle(0, 0));

    QUndoStack stack;
    DA::DACommandDataFrame_sortRows* cmd = new DA::DACommandDataFrame_sortRows(df, { qMakePair(0, true) });
    DA::DACommandDataFrame_sortRows* cmdRaw = cmd;
    cmd->setDirectionalCallBack([ &mgr, cmdRaw ](bool isUndo) {
        Q_UNUSED(isUndo);
        mgr.onRowsReordered(cmdRaw->lastAppliedPermutation());
    });
    QVERIFY(cmd->exec());
    // 升序 [1,2,3]；原行 0（a=3,b=x）移动到行 2，样式跟随
    QCOMPARE(df.iat(0, 0).toInt(), 1);
    QCOMPARE(df.iat(2, 0).toInt(), 3);
    QCOMPARE(df.iat(2, 1).toString(), QLatin1String("x"));
    QVERIFY(!mgr.hasCellStyle(0, 0));
    QVERIFY(mgr.hasCellStyle(2, 0));
    // 命令文本（撤销面板显示用）
    QCOMPARE(cmd->text(), QStringLiteral("Sort rows"));

    stack.push(cmd);  // 首次 redo 跳过（DACommandWithRedoCount）
    QCOMPARE(df.iat(0, 0).toInt(), 1);
    // Ctrl+Z 一次 → 顺序完全还原，样式回原行
    stack.undo();
    QCOMPARE(df.iat(0, 0).toInt(), 3);
    QCOMPARE(df.iat(1, 0).toInt(), 1);
    QCOMPARE(df.iat(2, 0).toInt(), 2);
    QCOMPARE(df.iat(0, 1).toString(), QLatin1String("x"));
    QCOMPARE(df.iat(1, 1).toString(), QLatin1String("y"));
    QVERIFY(mgr.hasCellStyle(0, 0));
    QVERIFY(!mgr.hasCellStyle(2, 0));
    // redo → 再次排序，样式再跟随
    stack.redo();
    QCOMPARE(df.iat(0, 0).toInt(), 1);
    QCOMPARE(df.iat(2, 0).toInt(), 3);
    QVERIFY(mgr.hasCellStyle(2, 0));
    QVERIFY(!mgr.hasCellStyle(0, 0));
}

// 002 §4.2（程序化）：表头点击排序交互——单击升/降循环、点另一列重置、Shift/Ctrl 追加与切换次排序列、
// 一次点击=一个撤销单元、选中行随置换跟随、拾取模式互斥、只读源不排序
void DATableDataTest::testHeaderSortInteraction()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
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
    auto colOf = [ hh ](const char* name) -> int {
        for (int c = 0; c < hh->count(); ++c) {
            if (hh->model() && hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String(name)) {
                return c;
            }
        }
        return -1;
    };
    const int colI = colOf("i");
    const int colF = colOf("f");
    const int colS = colOf("s");
    QVERIFY(colI >= 0 && colF >= 0 && colS >= 0);
    auto clickHeader = [ hh ](int col, Qt::KeyboardModifiers mods) {
        const int x = hh->sectionViewportPosition(col) + qMax(1, hh->sectionSize(col) / 2);
        const int y = qMax(1, hh->viewport()->height() / 2);
        QTest::mouseClick(hh->viewport(), Qt::LeftButton, mods, QPoint(x, y));
    };

    // 单击 → 升序（i 列本就 1..10，数据不变但规格生效）
    clickHeader(colI, Qt::NoModifier);
    QCOMPARE(w.sortSpec().size(), 1);
    QCOMPARE(w.sortSpec().at(0).first, colI);
    QCOMPARE(w.sortSpec().at(0).second, Qt::AscendingOrder);
    QCOMPARE(hh->sortSpec(), w.sortSpec());  // 表头角标数据源同步
    QCOMPARE(df.iat(0, colI).toInt(), 1);
    // 再点 → 降序
    clickHeader(colI, Qt::NoModifier);
    QCOMPARE(w.sortSpec().at(0).second, Qt::DescendingOrder);
    QCOMPARE(df.iat(0, colI).toInt(), 10);
    QCOMPARE(df.iat(9, colI).toInt(), 1);
    // 点另一列 → 只按新列排序（旧列指示消失：规格仅剩一项）
    clickHeader(colF, Qt::NoModifier);
    QCOMPARE(w.sortSpec().size(), 1);
    QCOMPARE(w.sortSpec().at(0).first, colF);
    QCOMPARE(w.sortSpec().at(0).second, Qt::AscendingOrder);
    // f 升序：前 8 行单调不减，2 个 NaN 排最后
    for (int r = 1; r < 8; ++r) {
        QVERIFY(df.iat(r - 1, colF).toDouble() <= df.iat(r, colF).toDouble());
    }
    QVERIFY(qIsNaN(df.iat(8, colF).toDouble()));
    QVERIFY(qIsNaN(df.iat(9, colF).toDouble()));
    // Shift 点第二列 → 次排序列（两列规格，多列序号角标的数据源）
    clickHeader(colI, Qt::ShiftModifier);
    QCOMPARE(w.sortSpec().size(), 2);
    QCOMPARE(w.sortSpec().at(1).first, colI);
    QCOMPARE(w.sortSpec().at(1).second, Qt::AscendingOrder);
    QCOMPARE(hh->sortSpec().size(), 2);
    // 角标绘制冒烟（§4.2「两列都显示序号角标」的程序化验证）：渲染表头，
    // 断言角标主蓝 #5280C1 与箭头深蓝灰 #5D6D7E 的纯色像素出现（抗锯齿边缘不参与计数）
    {
        const QSize sz = hh->viewport()->size();
        QVERIFY(sz.width() > 0 && sz.height() > 0);
        QImage img(sz, QImage::Format_ARGB32);
        img.fill(static_cast< QRgb >(0x00FFFFFF));
        hh->viewport()->render(&img);
        auto countColor = [ &img ](QRgb c) -> int {
            int n = 0;
            for (int y = 0; y < img.height(); ++y) {
                for (int x = 0; x < img.width(); ++x) {
                    if (img.pixel(x, y) == c) {
                        ++n;
                    }
                }
            }
            return n;
        };
        const int badgePx = countColor(qRgb(0x52, 0x80, 0xC1));  // 序号角标圆底（两列各一枚）
        const int arrowPx = countColor(qRgb(0x5D, 0x6D, 0x7E));  // 右端方向箭头（两列各一枚）
        QVERIFY2(badgePx >= 10, qPrintable(QStringLiteral("badge pixels=%1").arg(badgePx)));
        QVERIFY2(arrowPx >= 4, qPrintable(QStringLiteral("arrow pixels=%1").arg(arrowPx)));
    }
    // Ctrl 点同列 → 切换其方向（规格列数不变）
    QVector< QVariant > beforeUndoF, beforeUndoI;
    beforeUndoF.reserve(10);
    beforeUndoI.reserve(10);
    for (int r = 0; r < 10; ++r) {
        beforeUndoF.append(df.iat(r, colF));
        beforeUndoI.append(df.iat(r, colI));
    }
    clickHeader(colI, Qt::ControlModifier);
    QCOMPARE(w.sortSpec().size(), 2);
    QCOMPARE(w.sortSpec().at(1).second, Qt::DescendingOrder);
    bool changed = false;
    for (int r = 0; r < 10 && !changed; ++r) {
        changed = (df.iat(r, colI) != beforeUndoI.at(r));
    }
    QVERIFY(changed);  // 方向切换确实重排了数据
    // Ctrl+Z 一次 → 顺序完全还原（一次点击 = 一个撤销单元）
    // f 列含 NaN，QVariant 相等比较对 NaN!=NaN，用 NaN 感知比较
    auto sameValue = [](const QVariant& a, const QVariant& b) -> bool {
        const double da = a.toDouble();
        const double db = b.toDouble();
        if (qIsNaN(da) && qIsNaN(db)) {
            return true;
        }
        return a == b;
    };
    w.getUndoStack()->undo();
    for (int r = 0; r < 10; ++r) {
        QVERIFY2(sameValue(df.iat(r, colF), beforeUndoF.at(r)), qPrintable(QStringLiteral("undo mismatch at row %1 col f").arg(r)));
        QVERIFY2(sameValue(df.iat(r, colI), beforeUndoI.at(r)), qPrintable(QStringLiteral("undo mismatch at row %1 col i").arg(r)));
    }
    // 选中行随置换跟随：选中行 0 → 排序 → 选区映射到该行数据的新位置
    tv->selectionModel()->select(tv->model()->index(0, colS), QItemSelectionModel::ClearAndSelect);
    const QString marked = df.iat(0, colS).toString();
    clickHeader(colI, Qt::NoModifier);  // 无修饰 → 重置为仅 i 升序
    QCOMPARE(w.sortSpec().size(), 1);
    int newRow = -1;
    for (int r = 0; r < 10; ++r) {
        if (df.iat(r, colS).toString() == marked) {
            newRow = r;
            break;
        }
    }
    QVERIFY(newRow >= 0);
    bool selFollowed = false;
    const QModelIndexList sel = tv->selectionModel()->selectedIndexes();
    for (const QModelIndex& si : sel) {
        if (tv->toActualRow(si.row()) == newRow) {
            selFollowed = true;
        }
    }
    QVERIFY(selFollowed);
    w.hide();

    // 只读/惰性源：点击表头不排序、规格保持空（daWarning 提示路径）
    if (ensureSqliteReady()) {
        DA::DAAbstractData::Pointer lazy = makeSqliteLazyData();
        DA::DAData ld(lazy);
        DA::DATableMetadataRegistry reg2(nullptr);
        DA::DADataOperateOfDataFrameWidget wl(ld, &reg2);
        wl.resize(600, 300);
        wl.show();
        QVERIFY(QTest::qWaitForWindowExposed(&wl));
        DA::DADataHeaderView* hl = qobject_cast< DA::DADataHeaderView* >(wl.getDataTableView()->horizontalHeader());
        QVERIFY(hl != nullptr);
        const int x = hl->sectionViewportPosition(0) + qMax(1, hl->sectionSize(0) / 2);
        const int y = qMax(1, hl->viewport()->height() / 2);
        QTest::mouseClick(hl->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(x, y));
        QVERIFY(wl.sortSpec().isEmpty());
        wl.hide();
    } else {
        qWarning() << "sqlite lazy table not ready, read-only sort guard skipped:" << mSqliteErr;
    }
}

// 002 §4.2（程序化）：10 万行单列排序 ≤300 ms（算子+封装路径计时）
void DATableDataTest::testSortPerf100k()
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

//===================================================
// 002 查找/替换（T1.4，计划 §4.1 用例）
//===================================================

// 002 §4.1：da_search_data 扩展——大小写、正则、整格匹配、行/列范围命中集合、上限截断与总数、
// 数值列等值（文本查找不退化数值命中）、文本列能搜到（toDouble 修复的验收）
void DATableDataTest::testSearchOperatorExtended()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
sdf = pd.DataFrame({
    "t": ["Abc", "abc", "xabcx", None, "def"],
    "n": [1.0, 2.0, 32.0, 4.0, np.nan],
    "i": [10, 20, 30, 32, 50],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "sdf" ]);

    // 文本列子串、大小写不敏感（默认）：命中 t 列 3 格
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 3);
        QCOMPARE(res.rows, QVector< int >({ 0, 1, 2 }));
        QCOMPARE(res.cols, QVector< int >({ 0, 0, 0 }));
    }
    // 区分大小写：只剩小写 abc 两格
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), true, false, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 1, 2 }));
    }
    // 正则：^a 匹配 Abc/abc（不区分大小写）
    {
        const auto res = py.searchData(df, QStringLiteral("^a"), false, true, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 0, 1 }));
    }
    // 整格匹配：只有纯 "abc" 的格（行1），Abc 大小写不敏感整格也算（行0）
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, true);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 0, 1 }));
    }
    // 数值列等值命中（文本查找不退化数值命中）："32" → n 列行2、i 列行3
    {
        const auto res = py.searchData(df, QStringLiteral("32"), false, false, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        // 行主序：(2,1) 先于 (3,2)
        QCOMPARE(res.rows, QVector< int >({ 2, 3 }));
        QCOMPARE(res.cols, QVector< int >({ 1, 2 }));
    }
    // 行范围 [1,2]：t 列 abc 只剩行1、行2
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false, qMakePair(1, 2));
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 1, 2 }));
    }
    // 列范围 [1,2]（排除 t 列）：搜 abc 命中 0
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false, std::nullopt, qMakePair(1, 2));
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 0);
        QVERIFY(res.rows.isEmpty());
    }
    // 上限截断：max_hits=2 → 只回 2 条，totalHits=3（truncated 为真）
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false, std::nullopt, std::nullopt, 2);
        QVERIFY(res.ok);
        QCOMPARE(res.rows.size(), 2);
        QCOMPARE(res.totalHits, 3);
        QVERIFY(res.truncated());
    }
    // 无效正则：ok=false（不崩、不静默命中）
    {
        const auto res = py.searchData(df, QStringLiteral("(["), false, true, false);
        QVERIFY(!res.ok);
    }
}

// 002 §4.1：da_find_and_replace——变更清单与替换结果一致、数值列整格替换保持 dtype、
// 文本列正则替换、按清单写回还原（撤销）
void DATableDataTest::testFindReplaceOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();

    // 文本子串替换 + 变更清单 + 写回还原
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"t": ["Abc", "abc", "xabcx"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVector< int > rows, cols;
        pybind11::list olds, news;
        QVERIFY(py.findReplace(df, QStringLiteral("abc"), QStringLiteral("Q"), false, false, false, std::nullopt, std::nullopt, &rows, &cols, &olds, &news));
        QCOMPARE(rows, QVector< int >({ 0, 1, 2 }));
        QCOMPARE(cols, QVector< int >({ 0, 0, 0 }));
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("Q"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("xQx"));
        // 写回旧值 = 撤销
        QVERIFY(py.writeCells(df, rows, cols, olds));
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("Abc"));
        QCOMPARE(df.iat(1, 0).toString(), QLatin1String("abc"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("xabcx"));
    }
    // 正则替换（捕获组）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"t": ["a1b", "c2d", "ef"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVERIFY(py.findReplace(df, QStringLiteral("([a-z])(\\d)"), QStringLiteral("\\2\\1"), false, true, false));
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("1ab"));
        QCOMPARE(df.iat(1, 0).toString(), QLatin1String("2cd"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("ef"));
    }
    // 数值列整格替换保持 dtype（int64 保持整型）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"i": [10, 32, 50]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVector< int > rows, cols;
        QVERIFY(py.findReplace(df, QStringLiteral("32"), QStringLiteral("7"), false, false, false, std::nullopt, std::nullopt, &rows, &cols));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(df.iat(1, 0).toInt(), 7);
        // dtype 仍为整型（Python 侧校验；用 exec 执行含 import 的语句，eval 只接受表达式）
        pybind11::dict chk;
        chk[ "df" ] = df.object();
        pybind11::exec("import numpy as np\nres = bool(df['i'].dtype == np.int64)", pybind11::globals(), chk);
        const bool isInt = chk[ "res" ].cast< bool >();
        QVERIFY(isInt);
    }
    // 数值列 replace 不可解析 → 该列不替换（保持原值）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"i": [10, 32, 50]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVector< int > rows, cols;
        QVERIFY(py.findReplace(df, QStringLiteral("32"), QStringLiteral("abc"), false, false, false, std::nullopt, std::nullopt, &rows, &cols));
        QVERIFY(rows.isEmpty());
        QCOMPARE(df.iat(1, 0).toInt(), 32);
    }
}

// 002 §4.1：DACommandDataFrame_replace undo/redo 语义（小表 + 变更清单断言）
void DATableDataTest::testReplaceCommandUndoRedo()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"t": ["aa", "ab", "ba", "bb"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "rdf" ]);
    // 先就地替换 "a"→"X" 拿变更清单（模拟页面前置 findReplace），再据此构造命令
    QVector< int > rows, cols;
    pybind11::list olds, news;
    QVERIFY(py.findReplace(df, QStringLiteral("a"), QStringLiteral("X"), false, false, false, std::nullopt, std::nullopt, &rows, &cols, &olds, &news));
    QCOMPARE(rows.size(), 3);  // aa(0), ab(1), ba(2)
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("XX"));

    QUndoStack stack;
    DA::DACommandDataFrame_replace* cmd = new DA::DACommandDataFrame_replace(df, rows, cols, olds, news);
    QCOMPARE(cmd->changeCount(), 3);
    QCOMPARE(cmd->text(), QStringLiteral("Replace all"));
    stack.push(cmd);  // 首次 redo 跳过（数据已是替换后状态）
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("XX"));
    // undo → 写回旧值
    stack.undo();
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("aa"));
    QCOMPARE(df.iat(1, 0).toString(), QLatin1String("ab"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("ba"));
    QCOMPARE(df.iat(3, 0).toString(), QLatin1String("bb"));
    // redo → 再写新值
    stack.redo();
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("XX"));
    QCOMPARE(df.iat(1, 0).toString(), QLatin1String("Xb"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("bX"));
}

// 002 §4.2（程序化）：浮动查找条去抖——连续输入只触发一次 searchRequested（硬约束①）
void DATableDataTest::testFindReplaceDialogDebounce()
{
    DA::DADialogFindReplace dlg;
    QSignalSpy spy(&dlg, &DA::DADialogFindReplace::searchRequested);
    QVERIFY(spy.isValid());
    // 模拟连续敲击 "abc"：三次快速改文本（未经去抖间隔）
    dlg.activate(false);
    QLineEdit* findEdit = dlg.findChild< QLineEdit* >(QStringLiteral("lineEditFind"));
    QVERIFY(findEdit != nullptr);
    findEdit->setText(QStringLiteral("a"));
    findEdit->setText(QStringLiteral("ab"));
    findEdit->setText(QStringLiteral("abc"));
    // 去抖窗口内不应触发
    QTest::qWait(DA::DADialogFindReplace::debounceIntervalMs() / 2);
    QCOMPARE(spy.count(), 0);
    // 越过去抖窗口后恰好一次
    QTest::qWait(DA::DADialogFindReplace::debounceIntervalMs() + 100);
    QCOMPARE(spy.count(), 1);
    // 再输入一轮 → 再恰好一次（restart 语义）
    findEdit->setText(QStringLiteral("abcd"));
    findEdit->setText(QStringLiteral("abcde"));
    QTest::qWait(DA::DADialogFindReplace::debounceIntervalMs() + 100);
    QCOMPARE(spy.count(), 2);
    // 选项快照
    findEdit->setText(QStringLiteral("kw"));
    dlg.findChild< QCheckBox* >(QStringLiteral("checkBoxRegex"))->setChecked(true);
    dlg.findChild< QComboBox* >(QStringLiteral("comboBoxScope"))->setCurrentIndex(1);
    const auto opt = dlg.options();
    QCOMPARE(opt.keyword, QStringLiteral("kw"));
    QVERIFY(opt.useRegex);
    QVERIFY(opt.selectionOnly);
    // Esc 关闭发 closed
    QSignalSpy closeSpy(&dlg, &DA::DADialogFindReplace::closed);
    QTest::keyClick(&dlg, Qt::Key_Escape);
    QCOMPARE(closeSpy.count(), 1);
}

// 002 §4.2（程序化）：页面查找/替换全链路——扫描装载命中、导航、全部替换、撤销还原、Esc 清高亮
void DATableDataTest::testFindReplaceWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
wdf = pd.DataFrame({"t": ["abc", "def", "abcx", "xyz", "abc"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "wdf" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 360);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // 打开查找条并扫描 "abc"（3 命中）
    w.showFindReplace(false);
    DA::DADialogFindReplace* dlg = w.findChild< DA::DADialogFindReplace* >();
    QVERIFY(dlg != nullptr);
    QLineEdit* findEdit = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditFind"));
    QVERIFY(findEdit != nullptr);
    findEdit->setText(QStringLiteral("abc"));
    dlg->requestSearchNow();  // 跳过去抖立即扫描
    QCOMPARE(w.findHitCount(), 3);
    QCOMPARE(w.currentFindHitIndex(), 0);
    // 高亮层：Hit 3 格 + CurrentHit 1 格
    DA::DATableHighlightOverlay* ov = w.highlightOverlay();
    QVERIFY(ov != nullptr);
    QCOMPARE(ov->levelAt(0, 0), DA::DATableHighlightOverlay::Level::CurrentHit);
    QCOMPARE(ov->levelAt(2, 0), DA::DATableHighlightOverlay::Level::Hit);
    QCOMPARE(ov->levelAt(1, 0), DA::DATableHighlightOverlay::Level::None);
    // 导航到下一条
    QPushButton* nextBtn = dlg->findChild< QPushButton* >(QStringLiteral("pushButtonNext"));
    QVERIFY(nextBtn != nullptr);
    QTest::mouseClick(nextBtn, Qt::LeftButton);
    QCOMPARE(w.currentFindHitIndex(), 1);
    QCOMPARE(ov->levelAt(2, 0), DA::DATableHighlightOverlay::Level::CurrentHit);
    // 全部替换 abc→Q（展开替换行）
    w.showFindReplace(true);
    QLineEdit* replEdit = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditReplace"));
    QVERIFY(replEdit != nullptr);
    replEdit->setText(QStringLiteral("Q"));
    QSignalSpy replaceSpy(&w, &DA::DADataOperateOfDataFrameWidget::replaceApplied);
    QPushButton* replAllBtn = dlg->findChild< QPushButton* >(QStringLiteral("pushButtonReplaceAll"));
    QVERIFY(replAllBtn != nullptr);
    QTest::mouseClick(replAllBtn, Qt::LeftButton);
    QCOMPARE(replaceSpy.count(), 1);
    QCOMPARE(replaceSpy.takeFirst().at(0).toInt(), 3);
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("Q"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("Qx"));
    QCOMPARE(df.iat(4, 0).toString(), QLatin1String("Q"));
    // 替换后自动重扫：关键字 abc 已无命中
    QCOMPARE(w.findHitCount(), 0);
    // Ctrl+Z 一次 → 完全还原（全部替换 = 一个撤销单元）
    w.getUndoStack()->undo();
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("abc"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("abcx"));
    QCOMPARE(df.iat(4, 0).toString(), QLatin1String("abc"));
    // 选区内替换（§4.2）：范围=当前选区，只改选区包围盒内的命中
    {
        DA::DADataTableView* tv = w.getDataTableView();
        QVERIFY(tv != nullptr);
        QItemSelectionModel* sm = tv->selectionModel();
        QVERIFY(sm != nullptr);
        // 选中行 0..2（列 0）：命中 rows {0,2,4} 中只有 0、2 在选区内
        sm->select(QItemSelection(tv->model()->index(0, 0), tv->model()->index(2, 0)), QItemSelectionModel::ClearAndSelect);
        dlg->findChild< QComboBox* >(QStringLiteral("comboBoxScope"))->setCurrentIndex(1);  // 当前选区
        findEdit->setText(QStringLiteral("abc"));
        replEdit->setText(QStringLiteral("S"));
        dlg->requestSearchNow();
        QCOMPARE(w.findHitCount(), 2);  // 扫描范围同样受选区约束
        QTest::mouseClick(replAllBtn, Qt::LeftButton);
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("S"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("Sx"));
        QCOMPARE(df.iat(4, 0).toString(), QLatin1String("abc"));  // 选区外不变
        w.getUndoStack()->undo();
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("abc"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("abcx"));
        dlg->findChild< QComboBox* >(QStringLiteral("comboBoxScope"))->setCurrentIndex(0);  // 复位全表
    }
    // 关闭查找条 → Find 高亮清空
    w.showFindReplace(false);
    findEdit->setText(QStringLiteral("abc"));
    dlg->requestSearchNow();
    QCOMPARE(w.findHitCount(), 3);
    w.closeFindReplace();
    QCOMPARE(w.findHitCount(), 0);
    QCOMPARE(ov->levelAt(0, 0), DA::DATableHighlightOverlay::Level::None);
    QVERIFY(ov->isEmpty());
    w.hide();
}

//===================================================
// 003 条件格式（T1.5 §4.1）
//===================================================
namespace
{
using DA::DATableConditionalFormatRule;
using DA::DATableConditionalFormatStore;
using CfCond = DATableConditionalFormatRule::Condition;
using CfType = DATableConditionalFormatRule::Type;

// 官方色板端点（色阶默认色）
const QColor c_cfScaleMin(0xBD, 0xCD, 0xE8);  // 浅蓝
const QColor c_cfScaleMax(0x52, 0x80, 0xC1);  // 主蓝
const QColor c_cfBar(0xCE, 0x60, 0x43);       // 橙红（测试数据条用，与背景区分明显）

// 构造带背景色的单元格规则
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

// 统计图像中某矩形内与 target 完全相等的像素数（文字/网格线像素自然排除）
int countExactPixels(const QImage& img, const QRect& rect, QRgb target)
{
    int n = 0;
    const QRect r = rect.intersected(img.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) {
        for (int x = r.left(); x <= r.right(); ++x) {
            if (img.pixel(x, y) == target) {
                ++n;
            }
        }
    }
    return n;
}

// 统计矩形内"深色文字"像素数（数据条之上文字未被遮挡的证据：条色为亮色，文字为深色）
int countDarkPixels(const QImage& img, const QRect& rect)
{
    int n = 0;
    const QRect r = rect.intersected(img.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) {
        for (int x = r.left(); x <= r.right(); ++x) {
            const QRgb p = img.pixel(x, y);
            if (qRed(p) < 90 && qGreen(p) < 90 && qBlue(p) < 90) {
                ++n;
            }
        }
    }
    return n;
}

// 规则列表逐元素相等（避免依赖 QTest::toString 流式输出）
bool cfRulesEqual(const QVector< DATableConditionalFormatRule >& a, const QVector< DATableConditionalFormatRule >& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (int i = 0; i < a.size(); ++i) {
        if (!(a.at(i) == b.at(i))) {
            return false;
        }
    }
    return true;
}
}  // namespace

/**
 * @brief 规则求值：每种操作符边界（语义钉死见 DATableConditionalFormatRule 头注释）、
 * 数值列/文本列分别比较、null/NaN 不命中
 */
void DATableDataTest::testConditionalRuleEvaluation()
{
    // Greater：严格比较，等于不命中（钉死）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(101), QVariant("100"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(100), QVariant("100"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(99), QVariant("100"), QVariant()));
    // 数值单元格 vs 字符串参数："100" 可严格转数值 → 数值比较（跨类型数值等价）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(100.5), QVariant(100), QVariant()));
    // 文本列：双方非数值 → 字符串比较
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(QStringLiteral("b")), QVariant(QStringLiteral("a")), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(QStringLiteral("a")), QVariant(QStringLiteral("b")), QVariant()));
    // 混合（"12abc" 无法整串转数值）→ 退化字符串比较
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(QStringLiteral("12abc")), QVariant(QStringLiteral("12")), QVariant()));
    // Less：镜像语义
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Less, QVariant(99), QVariant("100"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Less, QVariant(100), QVariant("100"), QVariant()));
    // Between：闭区间 [v1, v2]（含端点，钉死）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(5), QVariant(1), QVariant(10)));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(1), QVariant(1), QVariant(10)));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(10), QVariant(1), QVariant(10)));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(0), QVariant(1), QVariant(10)));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(11), QVariant(1), QVariant(10)));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(QStringLiteral("b")), QVariant(QStringLiteral("a")), QVariant(QStringLiteral("c"))));
    // Equal：数值等价跨类型、字符串精确
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(100), QVariant("100"), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(100.0), QVariant(100), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(QStringLiteral("abc")), QVariant(QStringLiteral("abc")), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(QStringLiteral("abc")), QVariant(QStringLiteral("abd")), QVariant()));
    // Contains：字符串子串，区分大小写（钉死）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Contains, QVariant(QStringLiteral("hello world")), QVariant(QStringLiteral("world")), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Contains, QVariant(QStringLiteral("Hello")), QVariant(QStringLiteral("hello")), QVariant()));
    // IsEmpty / IsNotEmpty：invalid/null/NaN/空串为空，数值 0 非空
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(), QVariant(), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(QString()), QVariant(), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(qQNaN()), QVariant(), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(0.0), QVariant(), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsNotEmpty, QVariant(0.0), QVariant(), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::IsNotEmpty, QVariant(), QVariant(), QVariant()));
    // 空值/NaN 对其余操作符一律不命中（钉死）
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(), QVariant("5"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(qQNaN()), QVariant("5"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(QString()), QVariant(QStringLiteral("x")), QVariant()));
    // 参数缺失 → 不命中
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(5), QVariant(), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(5), QVariant(1), QVariant()));
    // toNumeric 严格性
    double d = 0.0;
    QVERIFY(DATableConditionalFormatRule::toNumeric(QVariant(QStringLiteral("123")), &d) && d == 123.0);
    QVERIFY(!DATableConditionalFormatRule::toNumeric(QVariant(QStringLiteral("123abc")), &d));
    QVERIFY(!DATableConditionalFormatRule::toNumeric(QVariant(qQNaN()), &d));
    QVERIFY(!DATableConditionalFormatRule::toNumeric(QVariant(QDateTime::currentDateTime()), &d));
}

/**
 * @brief 叠加顺序：同列两条规则都命中时后者覆盖前者；静态样式打底 + 条件覆盖；
 * 停用规则跳过；色阶/数据条与统计有效性
 */
void DATableDataTest::testConditionalEvaluateOrder()
{
    // 两条都命中：后者覆盖前者的背景，未设置属性保留 merge 语义
    QVector< DATableConditionalFormatRule > rules;
    DATableConditionalFormatRule r1 = makeBgRule(CfCond::Greater, QVariant(0), QColor(0xCE, 0x60, 0x43));
    DATableConditionalFormatRule r2;
    r2.type      = CfType::CellRule;
    r2.condition = CfCond::Greater;
    r2.value1    = QVariant(5);
    r2.style.setForeground(QColor(0xFF, 0xFF, 0xFF));  // 只设前景
    rules << r1 << r2;
    DATableConditionalFormatStore::ColumnStats stats;
    auto res = DATableConditionalFormatStore::evaluate(rules, QVariant(10), false, stats);
    QVERIFY(res.style.backgroundValid());
    QCOMPARE(res.style.background().color(), QColor(0xCE, 0x60, 0x43));  // r1 的背景保留（r2 未设背景）
    QVERIFY(res.style.foregroundValid());
    QCOMPARE(res.style.foreground(), QColor(0xFF, 0xFF, 0xFF));  // r2 前景叠加
    // 后者覆盖前者：r2 也设背景 → 最终为 r2 的背景
    r2.style.setBackground(QBrush(QColor(0x66, 0x9E, 0x8B)));
    rules[ 1 ] = r2;
    res        = DATableConditionalFormatStore::evaluate(rules, QVariant(10), false, stats);
    QCOMPARE(res.style.background().color(), QColor(0x66, 0x9E, 0x8B));
    // 只命中第一条：值为 3（>0 但 <5）→ r1 背景 + 无前景
    res = DATableConditionalFormatStore::evaluate(rules, QVariant(3), false, stats);
    QCOMPARE(res.style.background().color(), QColor(0xCE, 0x60, 0x43));
    QVERIFY(!res.style.foregroundValid());
    // 停用规则跳过
    rules[ 0 ].enabled = false;
    res                = DATableConditionalFormatStore::evaluate(rules, QVariant(3), false, stats);
    QVERIFY(res.style.isNull());
    rules[ 0 ].enabled = true;
    // 静态打底 + 条件覆盖（delegate 的叠加语义：base.mergeFrom(cf)）
    DA::DATableCellStyle base;
    base.setBackground(QBrush(QColor(0xBD, 0xCD, 0xE8)));  // 静态列样式底色
    res = DATableConditionalFormatStore::evaluate(rules, QVariant(10), false, stats);
    base.mergeFrom(res.style);
    QCOMPARE(base.background().color(), QColor(0x66, 0x9E, 0x8B));  // 条件覆盖静态
    // 色阶规则：统计无效 → 不产生背景；统计有效 → 插值背景
    QVector< DATableConditionalFormatRule > scaleRules;
    DATableConditionalFormatRule sr;
    sr.type = CfType::ColorScale;
    scaleRules << sr;
    res = DATableConditionalFormatStore::evaluate(scaleRules, QVariant(5.0), false, stats);
    QVERIFY(res.style.isNull());
    stats.minValue = 0.0;
    stats.maxValue = 10.0;
    res            = DATableConditionalFormatStore::evaluate(scaleRules, QVariant(5.0), true, stats);
    QVERIFY(res.style.backgroundValid());
    QCOMPARE(res.style.background().color(), DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 0.5));
    QVERIFY(!res.style.foregroundValid());  // 前景不因色阶变化（可读性）
    // 非数值单元格：色阶跳过
    res = DATableConditionalFormatStore::evaluate(scaleRules, QVariant(QStringLiteral("abc")), true, stats);
    QVERIFY(res.style.isNull());
    // 数据条规则：统计有效 + 数值 → 条长比例与颜色；统计无效 → 无条
    QVector< DATableConditionalFormatRule > barRules;
    DATableConditionalFormatRule br;
    br.type     = CfType::DataBar;
    br.barColor = c_cfBar;
    barRules << br;
    res = DATableConditionalFormatStore::evaluate(barRules, QVariant(2.5), true, stats);
    QVERIFY(res.hasDataBar);
    QCOMPARE(res.barRatio, 0.25);
    QCOMPARE(res.barColor, c_cfBar);
    res = DATableConditionalFormatStore::evaluate(barRules, QVariant(2.5), false, stats);
    QVERIFY(!res.hasDataBar);
}

/**
 * @brief 色阶插值：端点、中点（三色）、双色；min==max 退化；越界截断
 */
void DATableDataTest::testColorScaleInterpolation()
{
    const QColor cmin(0xBD, 0xCD, 0xE8);
    const QColor cmid(0xFF, 0xFF, 0xFF);
    const QColor cmax(0x52, 0x80, 0xC1);
    // 双色：端点精确
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(0.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmin);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(10.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmax);
    // 双色中点 = lerp(0.5)（与纯函数自洽）
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(5.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmin, cmax, 0.5));
    // 三色：value<=median 段 cmin→cmid，> median 段 cmid→cmax
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(5.0, 0.0, 10.0, 5.0, true, cmin, cmid, cmax), cmid);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(0.0, 0.0, 10.0, 5.0, true, cmin, cmid, cmax), cmin);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(10.0, 0.0, 10.0, 5.0, true, cmin, cmid, cmax), cmax);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(2.5, 0.0, 10.0, 5.0, true, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmin, cmid, 0.5));
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(7.5, 0.0, 10.0, 5.0, true, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmid, cmax, 0.5));
    // min==max 退化：normalizedPosition → 0.5（钉死）
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(5.0, 5.0, 5.0, 5.0, false, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmin, cmax, 0.5));
    // 越界截断到端点色
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(-100.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmin);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(100.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmax);
    // lerpColor 端点与中值分量
    QCOMPARE(DATableConditionalFormatRule::lerpColor(cmin, cmax, 0.0), cmin);
    QCOMPARE(DATableConditionalFormatRule::lerpColor(cmin, cmax, 1.0), cmax);
    const QColor half = DATableConditionalFormatRule::lerpColor(QColor(0, 0, 0), QColor(255, 255, 255), 0.5);
    QVERIFY(qAbs(half.red() - 128) <= 1 && qAbs(half.green() - 128) <= 1 && qAbs(half.blue() - 128) <= 1);
}

/**
 * @brief 数据条归一化：(v-min)/(max-min)；min==max 退化取半格；越界截断 [0,1]
 */
void DATableDataTest::testDataRowNormalization()
{
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(5.0, 0.0, 10.0), 0.5);
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(0.0, 0.0, 10.0), 0.0);
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(10.0, 0.0, 10.0), 1.0);
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(-5.0, 0.0, 10.0), 0.0);   // 越下界截断
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(15.0, 0.0, 10.0), 1.0);   // 越上界截断
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(5.0, 5.0, 5.0), 0.5);     // min==max 退化半格（§1.4-1）
    QCOMPARE(DATableConditionalFormatRule::normalizedPosition(2.5, 0.0, 10.0), 0.25);
}

/**
 * @brief 存储：XML round-trip（含 DATableMetadata 束聚合）、列插删重映射、
 * moveRule/clearColumn/clearAll 语义与信号
 */
void DATableDataTest::testConditionalStoreXmlAndRemap()
{
    DATableConditionalFormatStore store;
    // 列 1：单元格规则（背景+前景+字体、Between 双参数、停用规则）+ 三色色阶 + 数据条
    DATableConditionalFormatRule cellRule;
    cellRule.type      = CfType::CellRule;
    cellRule.condition = CfCond::Between;
    cellRule.value1    = QVariant(QStringLiteral("10"));
    cellRule.value2    = QVariant(QStringLiteral("20"));
    cellRule.style.setBackground(QBrush(QColor(0xCE, 0x60, 0x43)));
    cellRule.style.setForeground(QColor(0xFF, 0xFF, 0xFF));
    QFont f(QStringLiteral("Arial"), 12);
    f.setBold(true);
    cellRule.style.setFont(f);
    DATableConditionalFormatRule offRule = makeBgRule(CfCond::Greater, QVariant(100), QColor(0x66, 0x9E, 0x8B));
    offRule.enabled                      = false;
    DATableConditionalFormatRule scaleRule;
    scaleRule.type           = CfType::ColorScale;
    scaleRule.threeColorScale = true;
    DATableConditionalFormatRule barRule;
    barRule.type     = CfType::DataBar;
    barRule.barColor = QColor(0x49, 0x7C, 0xAD);
    store.addRule(1, cellRule);
    store.addRule(1, offRule);
    store.addRule(1, scaleRule);
    store.addRule(1, barRule);
    store.addRule(3, makeBgRule(CfCond::Contains, QVariant(QStringLiteral("x")), QColor(0xE6, 0xC2, 0x7C)));
    QCOMPARE(store.ruleCount(), 5);
    QCOMPARE(store.columnsWithRules(), QList< int >() << 1 << 3);
    // XML round-trip（store 级）
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("table"));
    doc.appendChild(root);
    store.toXml(doc, root);
    const QString xml = doc.toString();
    QVERIFY(xml.contains(QLatin1String("<conditional-format>")));
    QVERIFY(xml.contains(QLatin1String("col=\"1\"")));
    QVERIFY(xml.contains(QLatin1String("type=\"color-scale\"")));
    QVERIFY(xml.contains(QLatin1String("cond=\"between\"")));
    QDomDocument doc2;
    QVERIFY(doc2.setContent(xml));
    DATableConditionalFormatStore store2;
    QVERIFY(store2.fromXml(doc2.documentElement().firstChildElement(QStringLiteral("conditional-format"))));
    QCOMPARE(store2.ruleCount(), 5);
    QCOMPARE(store2.columnsWithRules(), QList< int >() << 1 << 3);
    // operator== 逐字段（含字体/停用/双参数/色阶端点/条色）
    QVERIFY(cfRulesEqual(store2.columnRules(1), store.columnRules(1)));
    QVERIFY(cfRulesEqual(store2.columnRules(3), store.columnRules(3)));
    // DATableMetadata 束聚合 round-trip（<conditional-format> 段随束序列化）
    {
        DA::DATableMetadata meta;
        meta.conditionalFormat()->addRule(2, cellRule);
        meta.styleManager()->setColumnStyle(0, DA::DATableCellStyle());
        QDomDocument mdoc;
        QDomElement mroot = mdoc.createElement(QStringLiteral("table"));
        mdoc.appendChild(mroot);
        meta.toXml(mdoc, mroot);
        QDomDocument mdoc2;
        QVERIFY(mdoc2.setContent(mdoc.toString()));
        DA::DATableMetadata meta2;
        QVERIFY(meta2.fromXml(mdoc2.documentElement()));
        QCOMPARE(meta2.conditionalFormat()->ruleCount(), 1);
        QVERIFY(cfRulesEqual(meta2.conditionalFormat()->columnRules(2), QVector< DATableConditionalFormatRule >() << cellRule));
        QVERIFY(!meta2.isEmpty());
    }
    // 信号：rulesChanged(column)
    QSignalSpy changedSpy(&store, &DATableConditionalFormatStore::rulesChanged);
    QSignalSpy resetSpy(&store, &DATableConditionalFormatStore::ruleReset);
    store.moveRule(1, 0, 2);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(store.columnRules(1).at(2) == cellRule);  // [off,scale,cell,bar]
    store.moveRule(1, 2, 0);
    QVERIFY(store.columnRules(1).at(0) == cellRule);  // 还原
    QCOMPARE(changedSpy.count(), 1);                  // 第二次 move 也发 rulesChanged(1)
    QCOMPARE(changedSpy.takeFirst().at(0).toInt(), 1);
    // clearColumn：桶删除 + 信号
    store.clearColumn(3);
    QVERIFY(!store.hasRules(3));
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toInt(), 3);
    store.clearColumn(3);  // 已空列再清不发信号（无变化）
    QCOMPARE(changedSpy.count(), 0);
    // 列插删重映射（签名 const QList<int>&，语义同 DATableStyleManager）
    store.addRule(0, makeBgRule(CfCond::Greater, QVariant(1), QColor(0x52, 0x80, 0xC1)));
    // 现有：列 0、列 1；插入列 1 → 原列 1 的规则移到列 2，列 0 不动
    store.onColumnsInserted({ 1 });
    QCOMPARE(resetSpy.count(), 1);
    QVERIFY(store.hasRules(0));
    QVERIFY(!store.hasRules(1));
    QCOMPARE(store.columnRules(2).size(), 4);
    // 删除列 2 → 该列规则丢弃，> 2 的键 -1（此处无）
    store.onColumnsRemoved({ 2 });
    QCOMPARE(resetSpy.count(), 2);
    QVERIFY(!store.hasRules(2));
    QVERIFY(store.hasRules(0));
    // 批量插入多个索引（降序处理语义与样式管理器一致）：{0,5} 插入 {1,3} → {0,7}
    store.addRule(5, makeBgRule(CfCond::Greater, QVariant(1), QColor(0x52, 0x80, 0xC1)));
    store.onColumnsInserted({ 1, 3 });
    QVERIFY(store.hasRules(0));   // < 1 不动
    QVERIFY(!store.hasRules(1));  // 插入列本身无规则
    QVERIFY(!store.hasRules(5));  // 5 已位移
    QVERIFY(store.hasRules(7));   // 5 → +2（1 与 3 都 <= 5）
    // 批量删除：删掉 0 与 7 → 全部规则清空
    store.onColumnsRemoved({ 0, 7 });
    QVERIFY(!store.hasRules(0));
    QVERIFY(!store.hasRules(6));
    QCOMPARE(store.ruleCount(), 0);
    // clearAll：全部清空 + ruleReset
    const int resetsBefore = resetSpy.count();
    store.clearAll();
    QVERIFY(store.isEmpty());
    QCOMPARE(store.ruleCount(), 0);
    QCOMPARE(resetSpy.count(), resetsBefore + 1);
    // 空 store toXml 不写段
    QDomDocument edoc;
    QDomElement eroot = edoc.createElement(QStringLiteral("table"));
    edoc.appendChild(eroot);
    store.toXml(edoc, eroot);
    QVERIFY(edoc.firstChildElement(QStringLiteral("conditional-format")).isNull());
}

/**
 * @brief 统计缓存：惰性计算一次 + 缓存命中不再调提供者；失效触发重算；
 * 无规则列不计算；失败结果也缓存（不每格重试）
 */
void DATableDataTest::testConditionalStoreStatsCache()
{
    DATableConditionalFormatStore store;
    int providerCalls = 0;
    store.setStatsProvider([ &providerCalls ](int column, DATableConditionalFormatStore::ColumnStats& out) {
        ++providerCalls;
        if (column == 9) {
            return false;  // 模拟非数值列/失败
        }
        out.minValue    = 0.0;
        out.maxValue    = 100.0;
        out.medianValue = 50.0;
        return true;
    });
    DATableConditionalFormatStore::ColumnStats st;
    // 无规则的列：不计算（红线2）
    QVERIFY(!store.columnStats(0, st));
    QCOMPARE(providerCalls, 0);
    // 加色阶规则后：首次计算，二次命中缓存
    DATableConditionalFormatRule sr;
    sr.type = CfType::ColorScale;
    store.addRule(0, sr);
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 1);
    QCOMPARE(st.minValue, 0.0);
    QCOMPARE(st.maxValue, 100.0);
    QCOMPARE(st.medianValue, 50.0);
    QVERIFY(store.columnStats(0, st));
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 1);  // 缓存生效，不重算
    // 失效单列 → 重算一次
    store.invalidateColumnStats(0);
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 2);
    // 失败列：失败也缓存，不每格重试
    DATableConditionalFormatRule sr9;
    sr9.type = CfType::DataBar;
    store.addRule(9, sr9);
    QVERIFY(!store.columnStats(9, st));
    QVERIFY(!store.columnStats(9, st));
    QCOMPARE(providerCalls, 3);  // 只调用一次
    // invalidateAllStats → 全部重算
    store.invalidateAllStats();
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 4);
    // needsColumnStats：纯单元格规则列不需要统计
    store.addRule(1, makeBgRule(CfCond::Greater, QVariant(1), QColor(0xCE, 0x60, 0x43)));
    QVERIFY(!store.needsColumnStats(1));
    QVERIFY(store.needsColumnStats(0));
    // 提供者置空 → 统计不可用
    store.setStatsProvider(nullptr);
    QVERIFY(!store.columnStats(0, st));
}

/**
 * @brief 命令：增/删/改/移动/清列的 undo/redo 结果对象一致（旧新对象记录范式）
 */
void DATableDataTest::testConditionalCommands()
{
    DATableConditionalFormatStore store;
    QUndoStack stack;
    const DATableConditionalFormatRule ruleA = makeBgRule(CfCond::Greater, QVariant(10), QColor(0xCE, 0x60, 0x43));
    const DATableConditionalFormatRule ruleB = makeBgRule(CfCond::Less, QVariant(5), QColor(0x66, 0x9E, 0x8B));
    const QVector< DATableConditionalFormatRule > onlyA  { ruleA };
    const QVector< DATableConditionalFormatRule > bThenA { ruleB, ruleA };
    const QVector< DATableConditionalFormatRule > aThenB { ruleA, ruleB };
    // 增：push 即 redo
    stack.push(new DA::DACommandConditionalFormat_addRule(&store, 0, ruleA));
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    stack.undo();
    QVERIFY(!store.hasRules(0));
    stack.redo();
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    // 第二条 + 指定位置插入
    stack.push(new DA::DACommandConditionalFormat_addRule(&store, 0, ruleB, 0));
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    stack.redo();
    // 改：旧新对象记录
    DATableConditionalFormatRule ruleA2 = ruleA;
    ruleA2.value1                       = QVariant(20);
    stack.push(new DA::DACommandConditionalFormat_modifyRule(&store, 0, 1, ruleA, ruleA2));
    QVERIFY(store.columnRules(0).at(1) == ruleA2);
    stack.undo();
    QVERIFY(store.columnRules(0).at(1) == ruleA);
    stack.redo();
    QVERIFY(store.columnRules(0).at(1) == ruleA2);
    stack.undo();  // 回到 [ruleB, ruleA]
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    // 移动：记录前后索引
    stack.push(new DA::DACommandConditionalFormat_moveRule(&store, 0, 0, 1));
    QVERIFY(cfRulesEqual(store.columnRules(0), aThenB));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.redo();
    QVERIFY(cfRulesEqual(store.columnRules(0), aThenB));
    stack.undo();  // 回到 [ruleB, ruleA]
    // 删：undo 原位插回（对象一致）
    stack.push(new DA::DACommandConditionalFormat_removeRule(&store, 0, 0));
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.redo();
    stack.undo();  // 回到 [ruleB, ruleA]
    // 清列：undo 整体还原
    stack.push(new DA::DACommandConditionalFormat_clearColumn(&store, 0));
    QVERIFY(!store.hasRules(0));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.redo();
    QVERIFY(!store.hasRules(0));
}

/**
 * @brief Python 算子 da_column_stats：min/max/median 全表归约、NaN 排除、
 * 非数值列/越界 → 不可用（C++ wrapper ok=false）
 */
void DATableDataTest::testColumnStatsOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import numpy as np
import pandas as pd
sdf = pd.DataFrame({
    "v": [1.0, 2.0, 3.0, 4.0, 100.0],
    "n": [1.0, np.nan, 3.0, np.nan, 5.0],
    "t": ["a", "b", "c", "d", "e"],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "sdf" ]);
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    auto r0                      = py.columnStats(df, 0);
    QVERIFY(r0.ok);
    QCOMPARE(r0.minValue, 1.0);
    QCOMPARE(r0.maxValue, 100.0);
    QCOMPARE(r0.medianValue, 3.0);
    // NaN 排除后归约（有效值 [1,3,5]）
    auto r1 = py.columnStats(df, 1);
    QVERIFY(r1.ok);
    QCOMPARE(r1.minValue, 1.0);
    QCOMPARE(r1.maxValue, 5.0);
    QCOMPARE(r1.medianValue, 3.0);
    // 纯文本列：pd.to_numeric coerce 全 NaN → 不可用
    auto r2 = py.columnStats(df, 2);
    QVERIFY(!r2.ok);
    // 越界 → 不可用（IndexError 经 dealException 吞掉）
    auto r3 = py.columnStats(df, 99);
    QVERIFY(!r3.ok);
    auto r4 = py.columnStats(df, -1);
    QVERIFY(!r4.ok);
}

/**
 * @brief 绘制管线②④的像素级验证（§4.2 程序化）：色阶分布、编辑极值后端点更新
 *（统计失效）、数据条几何与文字不被遮挡、查找高亮③覆盖条件格式②、
 * 规则命令 undo/redo、预览叠加/还原
 */
void DATableDataTest::testConditionalPaintWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import numpy as np
import pandas as pd
cdf = pd.DataFrame({
    "v": np.arange(10, dtype=float),
    "b": np.arange(10, dtype=float),
    "t": ["s%d" % i for i in range(10)],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "cdf" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    tv->clearSelection();
    DATableConditionalFormatStore* store = w.conditionalFormatStore();
    QVERIFY(store != nullptr);
    // 色阶（列 0，双色默认浅蓝→主蓝）+ 数据条（列 1，橙红）
    DATableConditionalFormatRule scaleRule;
    scaleRule.type = CfType::ColorScale;
    store->addRule(0, scaleRule);
    DATableConditionalFormatRule barRule;
    barRule.type     = CfType::DataBar;
    barRule.barColor = c_cfBar;
    store->addRule(1, barRule);
    auto renderViewport = [ &w, tv ]() {
        // 清除当前格（模型装载后默认 (0,0)）：windows 风格会给当前格叠渐变着色，
        // 干扰纯色像素断言（选区已在测试开头 clearSelection）
        if (tv->selectionModel() && tv->selectionModel()->currentIndex().isValid()) {
            tv->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
        }
        QApplication::processEvents();
        tv->viewport()->repaint();
        QImage img(tv->viewport()->size(), QImage::Format_ARGB32);
        tv->viewport()->render(&img);
        return img;
    };
    auto cellRect = [ tv ](int row, int col) {
        return tv->visualRect(tv->model()->index(row, col));
    };
    // —— 色阶分布：min 行（值0）= 浅蓝端点，max 行（值9）= 主蓝端点，中间行介于两者 ——
    QImage img = renderViewport();
    const QRgb lowRgb  = c_cfScaleMin.rgb();
    const QRgb highRgb = c_cfScaleMax.rgb();
    QVERIFY2(countExactPixels(img, cellRect(0, 0), lowRgb) >= 20, "color scale min cell should be filled with low-end color");
    QVERIFY2(countExactPixels(img, cellRect(9, 0), highRgb) >= 20, "color scale max cell should be filled with high-end color");
    // 中间行（值5，t=5/9）：与两端点色都不同（分布插值的证据）
    const QRect midRect = cellRect(5, 0);
    QCOMPARE(countExactPixels(img, midRect, lowRgb), 0);
    QCOMPARE(countExactPixels(img, midRect, highRgb), 0);
    const QColor midExpect = DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 5.0 / 9.0);
    QVERIFY2(countExactPixels(img, midRect, midExpect.rgb()) >= 20, "middle cell should be the interpolated color");
    // —— 数据条：max 行满格条（右侧区域为条色），min 行比例为 0 无条 ——
    const QRect barMaxRect = cellRect(9, 1);
    QVERIFY2(countExactPixels(img, barMaxRect, c_cfBar.rgb()) >= 20, "data bar of max cell should be drawn");
    // 文字不被遮挡：条色为亮色（橙红），文字为深色 → 条区域内存在深色文字像素
    QVERIFY2(countDarkPixels(img, barMaxRect) > 0, "text pixels must be visible above the data bar");
    const QRect barMinRect = cellRect(0, 1);
    QCOMPARE(countExactPixels(img, barMinRect, c_cfBar.rgb()), 0);  // ratio=0 无条
    // 2px 内边距：单元格最右侧 1px 列不应有条色（条不满到边缘）——值9 比例=1 时条右缘 = 内容区右缘
    QVERIFY(countExactPixels(img, QRect(barMaxRect.right(), barMaxRect.top(), 1, barMaxRect.height()), c_cfBar.rgb()) == 0);
    // —— 编辑极值 → 统计失效 → 色阶端点更新（§4.2 第 2 条）——
    // 行 8（值 8）编辑前 t=8/9；把行 9 的值 9 改成 90 后 max=90 → 行 8 的 t=8/90，
    // 颜色随之变化即证明统计缓存被编辑失效并重算
    const QColor before8 = DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 8.0 / 9.0);
    QVERIFY2(countExactPixels(img, cellRect(8, 0), before8.rgb()) >= 20, "row8 paints with old max=9 interpolation");
    QVERIFY(tv->model()->setData(tv->model()->index(9, 0), QVariant(90.0), Qt::EditRole));
    img = renderViewport();
    const QColor after8 = DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 8.0 / 90.0);
    QVERIFY2(countExactPixels(img, cellRect(8, 0), after8.rgb()) >= 20,
             "stats must be invalidated after editing the extreme value (row8 re-interpolated against max=90)");
    QCOMPARE(countExactPixels(img, cellRect(8, 0), before8.rgb()), 0);
    // 被编辑格（值 90）= 新 max → 高端色
    QVERIFY2(countExactPixels(img, cellRect(9, 0), highRgb) >= 20, "new max cell paints high-end color");
    // 撤销编辑 → 统计再失效 → 恢复原端点
    w.getUndoStack()->undo();
    img = renderViewport();
    QVERIFY2(countExactPixels(img, cellRect(8, 0), before8.rgb()) >= 20, "undo edit restores original scale endpoints");
    QCOMPARE(countExactPixels(img, cellRect(8, 0), after8.rgb()), 0);
    // —— ③瞬态覆盖②：查找高亮（金黄 alpha160）叠加在色阶背景之上 ——
    {
        DA::DATableHighlightOverlay* ov = w.highlightOverlay();
        ov->set(DA::DATableHighlightOverlay::Source::Find, DA::DATableHighlightOverlay::Level::Hit, { qMakePair(0, 0) });
        img = renderViewport();
        // 期望混合色：dst = (hl*160 + bg*(255-160))/255（bg = 色阶低端浅蓝）
        const QColor hl(0xE6, 0xC2, 0x7C);
        auto blend = [ & ](int cBg, int cHl) {
            return (cHl * 160 + cBg * (255 - 160)) / 255;
        };
        const QColor expect(blend(c_cfScaleMin.red(), hl.red()), blend(c_cfScaleMin.green(), hl.green()), blend(c_cfScaleMin.blue(), hl.blue()));
        const QRect r00 = cellRect(0, 0);
        int blended     = 0;
        for (int y = r00.top(); y <= r00.bottom(); ++y) {
            for (int x = r00.left(); x <= r00.right(); ++x) {
                const QRgb p = img.pixel(x, y);
                if (qAbs(qRed(p) - expect.red()) <= 2 && qAbs(qGreen(p) - expect.green()) <= 2 && qAbs(qBlue(p) - expect.blue()) <= 2) {
                    ++blended;
                }
            }
        }
        QVERIFY2(blended >= 20, qPrintable(QStringLiteral("overlay(3) must cover conditional format(2), blended px=%1").arg(blended)));
        // 纯②色不再出现于该格（被③覆盖）
        QCOMPARE(countExactPixels(img, r00, lowRgb), 0);
        ov->clear(DA::DATableHighlightOverlay::Source::Find);
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(0, 0), lowRgb) >= 20, "clearing overlay restores 2-layer paint");
    }
    // —— 规则命令 undo/redo（加规则/删规则）——
    {
        QSignalSpy ruleSpy(store, &DATableConditionalFormatStore::rulesChanged);
        QUndoStack* us = w.getUndoStack();
        QVERIFY(us != nullptr);
        DATableConditionalFormatRule greenRule = makeBgRule(CfCond::Greater, QVariant(5), QColor(0x66, 0x9E, 0x8B));
        us->push(new DA::DACommandConditionalFormat_addRule(store, 0, greenRule));
        QCOMPARE(store->columnRules(0).size(), 2);
        QVERIFY(ruleSpy.count() >= 1);
        img = renderViewport();
        // 值 6..9 命中绿规则（列表靠后覆盖色阶）
        QVERIFY2(countExactPixels(img, cellRect(9, 0), QColor(0x66, 0x9E, 0x8B).rgb()) >= 20, "later rule overrides color scale");
        us->undo();
        QCOMPARE(store->columnRules(0).size(), 1);
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(9, 0), highRgb) >= 20, "undo add-rule restores scale paint");
        // push 时栈顶位于已撤销的编辑命令之下（undo 后 index 在其前）→ 编辑命令被丢弃，
        // 栈中只剩本 addRule 命令（QUndoStack::push 标准语义）
        QCOMPARE(us->count(), 1);
        us->redo();
        QCOMPARE(store->columnRules(0).size(), 2);
        us->undo();  // 复原，避免影响后续断言
    }
    // —— 预览规则集：叠加显示、清除还原（对话框实时预览机制）——
    {
        QHash< int, QVector< DATableConditionalFormatRule > > preview;
        preview.insert(0, QVector< DATableConditionalFormatRule >() << makeBgRule(CfCond::IsNotEmpty, QVariant(), QColor(0xE6, 0xC2, 0x7C)));
        store->setPreviewRules(preview);
        QVERIFY(store->hasPreview());
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(3, 0), QColor(0xE6, 0xC2, 0x7C).rgb()) >= 20, "preview rules paint without committing");
        QCOMPARE(store->ruleCount(), 2);  // 预览不落库（列0 一条色阶 + 列1 一条数据条）
        store->clearPreview();
        QVERIFY(!store->hasPreview());
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(3, 0), QColor(0xE6, 0xC2, 0x7C).rgb()) == 0, "clearPreview restores committed rules");
    }
    w.hide();
}

/**
 * @brief 对话框：惰性/只读源上色阶与数据条置灰 + 说明文案；普通源全部可用；
 * 编辑不落库（取消后 store 不变）、预览通道工作
 */
void DATableDataTest::testConditionalDialogSourceRestriction()
{
    DATableConditionalFormatStore store;
    const QStringList names { QStringLiteral("a"), QStringLiteral("b") };
    {
        DA::DADialogConditionalFormat dlg(&store, nullptr);
        dlg.setup(names, { 0, 1 }, false);  // 惰性/只读源
        QComboBox* comboType = dlg.findChild< QComboBox* >(QStringLiteral("comboType"));
        QVERIFY(comboType != nullptr);
        QStandardItemModel* m = qobject_cast< QStandardItemModel* >(comboType->model());
        QVERIFY(m != nullptr);
        QVERIFY(m->item(0)->isEnabled());   // 单元格规则可用
        QVERIFY(!m->item(1)->isEnabled());  // 色阶置灰
        QVERIFY(!m->item(2)->isEnabled());  // 数据条置灰
        QLabel* hint = dlg.findChild< QLabel* >(QStringLiteral("labelDisabledHint"));
        QVERIFY(hint != nullptr);
        QVERIFY(!hint->text().isEmpty());       // 说明文案（tr）
        QVERIFY(!hint->isHidden());             // 且可见
        // 预览通道：setup 后 store 挂上预览（初始 = 现状）
        QVERIFY(store.hasPreview());
        // 新增规则只进工作集，不落库
        dlg.show();  // 离屏显示，保证鼠标事件投递
        QPushButton* addBtn = dlg.findChild< QPushButton* >(QStringLiteral("buttonAdd"));
        QVERIFY(addBtn != nullptr);
        QTest::mouseClick(addBtn, Qt::LeftButton);
        QCOMPARE(dlg.resultRules().value(0).size() + dlg.resultRules().value(1).size(), 1);
        QCOMPARE(store.ruleCount(), 0);  // 未提交
        dlg.reject();                    // 取消 → 预览还原
        QVERIFY(!store.hasPreview());
        QVERIFY(store.isEmpty());
    }
    {
        DA::DADialogConditionalFormat dlg(&store, nullptr);
        dlg.setup(names, { 0 }, true);  // 普通源
        QComboBox* comboType = dlg.findChild< QComboBox* >(QStringLiteral("comboType"));
        QStandardItemModel* m = qobject_cast< QStandardItemModel* >(comboType->model());
        QVERIFY(m->item(1)->isEnabled());
        QVERIFY(m->item(2)->isEnabled());
        dlg.accept();  // 确定：预览清除，结果交调用方（此处无差异 → 不提交）
        QVERIFY(!store.hasPreview());
    }
}

/**
 * @brief 滚动性能（§4.2「滚动千行无卡顿」的程序化近似）：5000 行 × 3 规则列，
 * 50 次跳滚 + 全视口重绘计时；并用计数提供者钉住「统计只算一次、滚动不重算」
 *（红线2 的缓存语义），逐格遍历规则由按列分桶结构保证（rulesForColumn O(1)）
 */
void DATableDataTest::testConditionalScrollPerf1k()
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

// 不用QTEST_MAIN：嵌入式python的静态对象（DAPyBindQt.dll内的解释器句柄、QVariant caster
// 类型缓存等）在进程teardown阶段析构顺序不可控，会在测试结果完整输出后引发访问冲突。
// qExec返回时断言结果与-o文件均已写完，直接终止进程跳过teardown，保证退出码可信
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DATableDataTest tc;
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
