#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

#include "DAPyInterpreter.h"
#include "DAPyModulePandas.h"
#include "DAPyDataFrame.h"

class DADataFrameTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testReadCsv();
    void testColumns();
    void testEmptyAndShape();
    void testIlocAndColumnAccess();

private:
    DA::DAPyDataFrame loadCsvOrSkip();
};

void DADataFrameTest::initTestCase()
{
    // 通过 DAPyInterpreter 自动检测并初始化嵌入式 Python 解释器
    // DAPyModulePandas 内部依赖此解释器，手动调用 Py_SetPythonHome 会覆盖正确的路径
    DA::DAPyInterpreter::initializePythonInterpreter();
}

DA::DAPyDataFrame DADataFrameTest::loadCsvOrSkip()
{
    if (!QFile::exists(QStringLiteral("IBM.csv"))) {
        QSKIP("IBM.csv not available, skipping DataFrame test");
    }
    DA::DAPyDataFrame df = DA::DAPyModulePandas::getInstance().read_csv(QStringLiteral("IBM.csv"));
    if (!df) {
        QFAIL(qPrintable(DA::DAPyModulePandas::getInstance().getLastErrorString()));
    }
    return df;
}

void DADataFrameTest::testReadCsv()
{
    DA::DAPyDataFrame df = loadCsvOrSkip();
    QVERIFY(df);
    QVERIFY(df.size() > 0);
}

void DADataFrameTest::testColumns()
{
    DA::DAPyDataFrame df = loadCsvOrSkip();
    QVERIFY(df);

    const QList< QString > cols = df.columns();  // const 避免 COW 深拷贝
    QVERIFY(!cols.isEmpty());
    QCOMPARE(static_cast< std::size_t >(cols.size()), df.shape().second);
}

void DADataFrameTest::testEmptyAndShape()
{
    DA::DAPyDataFrame df = loadCsvOrSkip();
    QVERIFY(df);

    QVERIFY(!df.empty());

    std::size_t row = df.shape().first;
    std::size_t col = df.shape().second;
    QVERIFY(row > 0);
    QVERIFY(col > 0);
    QCOMPARE(df.size(), row * col);
}

void DADataFrameTest::testIlocAndColumnAccess()
{
    DA::DAPyDataFrame df = loadCsvOrSkip();
    QVERIFY(df);

    std::size_t row = df.shape().first;
    QVERIFY(row > 0);

    // 按行打印（iloc 返回 DAPySeries）
    qDebug() << "print row by iloc:";
    for (std::size_t i = 0; i < row; ++i) {
        qDebug() << df.iloc(i);
    }

    // 按列打印（operator[] 返回 DAPySeries）
    // 使用 const 声明避免对非 const Qt 容器范围迭代触发 COW 深拷贝
    qDebug() << "print column by []:";
    const QList< QString > cols = df.columns();
    for (const QString& c : cols) {
        qDebug() << df[ c ];
    }
}

QTEST_MAIN(DADataFrameTest)
#include "main.moc"
