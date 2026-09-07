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

#include "Models/DADataTableModel.h"
#include <QUndoStack>

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

private:
    bool mPandasOk = false;
    QUndoStack mUndoStack;
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
