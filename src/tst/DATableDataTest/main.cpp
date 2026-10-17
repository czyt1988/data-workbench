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

private:
    // 生成/复用sqlite测试库（百万行），失败返回false并置mSqliteErr
    bool ensureSqliteReady();
    // 创建绑定测试库的惰性表数据（SELECT带ORDER BY保证分页稳定）
    DA::DAAbstractData::Pointer makeSqliteLazyData();

private:
    static constexpr qlonglong c_sqliteRows = 1000000;
    bool mPandasOk                          = false;
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

    // ---- 表体菜单：剪切/复制/粘贴；无有效选区（无模型）→ 全部置灰 ----
    DA::DADataTableView tv;
    QMenu* body = tv.buildContextMenu(QModelIndex());
    QVERIFY(body != nullptr);
    QStringList bodySeq;
    const auto bodyActions = body->actions();
    for (QAction* a : bodyActions) {
        bodySeq << a->text();
        QVERIFY2(!a->isEnabled(), qPrintable(QStringLiteral("action should be disabled without selection: %1").arg(a->text())));
    }
    const QStringList expectedBody { QStringLiteral("Cut"), QStringLiteral("Copy"), QStringLiteral("Paste") };
    QCOMPARE(bodySeq, expectedBody);
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

    // 图表「选择序列」拾取链路回归：左键点击列头 → sectionClicked → columnHeaderClicked
    QSignalSpy clickSpy(&w2, &DA::DADataOperateOfDataFrameWidget::columnHeaderClicked);
    w2.resize(800, 400);
    w2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w2));
    const int xPos = hh->sectionViewportPosition(colF) + qMax(1, hh->sectionSize(colF) / 2);
    const int yPos = qMax(1, hh->viewport()->height() / 2);
    QTest::mouseClick(hh->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(xPos, yPos));
    QVERIFY(clickSpy.count() >= 1);
    QCOMPARE(clickSpy.takeFirst().at(0).toInt(), colF);
    w2.hide();
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
