#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <cstdio>
#include <cstdlib>

#ifdef Q_OS_WIN
#    include <windows.h>
#endif

#include "DAPyInterpreter.h"
#include "DAPyModulePandas.h"
#include "DAPyDataFrame.h"
#include <pybind11/eval.h>

class DADataFrameTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testReadCsv();
    void testColumns();
    void testEmptyAndShape();
    void testIlocAndColumnAccess();
    // 回归：按列号取列必须走位置索引（iloc[整数] 取的是行），重名列也不能歧义
    void testColumnAtPositional();

private:
    bool loadCsv(DA::DAPyDataFrame& out, QString* err = nullptr);
};

void DADataFrameTest::initTestCase()
{
    // 通过 DAPyInterpreter 自动检测并初始化嵌入式 Python 解释器
    // DAPyModulePandas 内部依赖此解释器，手动调用 Py_SetPythonHome 会覆盖正确的路径
    DA::DAPyInterpreter::initializePythonInterpreter();
}

bool DADataFrameTest::loadCsv(DA::DAPyDataFrame& out, QString* err)
{
    if (!QFile::exists(QStringLiteral("IBM.csv"))) {
        if (err) {
            *err = QStringLiteral("IBM.csv not available");
        }
        return false;
    }
    out = DA::DAPyModulePandas::getInstance().read_csv(QStringLiteral("IBM.csv"));
    if (!out) {
        if (err) {
            *err = DA::DAPyModulePandas::getInstance().getLastErrorString();
        }
        return false;
    }
    return true;
}

void DADataFrameTest::testReadCsv()
{
    DA::DAPyDataFrame df;
    QString err;
    if (!loadCsv(df, &err)) {
        QSKIP(err.toUtf8().constData());
        return;
    }
    QVERIFY(df);
    QVERIFY(df.size() > 0);
}

void DADataFrameTest::testColumns()
{
    DA::DAPyDataFrame df;
    QString err;
    if (!loadCsv(df, &err)) {
        QSKIP(err.toUtf8().constData());
        return;
    }
    QVERIFY(df);

    const QList< QString > cols = df.columns();  // const 避免 COW 深拷贝
    QVERIFY(!cols.isEmpty());
    QCOMPARE(static_cast< std::size_t >(cols.size()), df.shape().second);
}

void DADataFrameTest::testEmptyAndShape()
{
    DA::DAPyDataFrame df;
    QString err;
    if (!loadCsv(df, &err)) {
        QSKIP(err.toUtf8().constData());
        return;
    }
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
    DA::DAPyDataFrame df;
    QString err;
    if (!loadCsv(df, &err)) {
        QSKIP(err.toUtf8().constData());
        return;
    }
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

/**
 * @brief 回归：按列号取列必须走位置索引
 *
 * 背景：pandas 的 DataFrame.iloc[整数] 取的是「行」，一行的长度等于列数；
 * 若把它当成「按列号取列」用，大表按列取数就只会返回列数个值（10 万行 × 2 列 → 2 个点）。
 * 本用例同时覆盖重名列：两列都叫 a 时，按位置取仍要拿到各自的值。
 */
void DADataFrameTest::testColumnAtPositional()
{
    // 嵌入式解释器可能选不到带 pandas 的 Python（例如本机 PATH 里的 WindowsApps 存根）：
    // 本用例直接用 pandas 造数据，无法导入时跳过，而不是抛 pybind11 异常让整用例失败
    // （与同文件其它用例「IBM.csv/pandas 不可用则 QSKIP」的口径一致）。
    try {
        pybind11::exec("import pandas");
    } catch (const pybind11::error_already_set&) {
        QSKIP("pandas is not available in the embedded Python interpreter");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame([[1, 10, 100], [2, 20, 200], [3, 30, 300]], columns=["a", "a", "b"])
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QVERIFY(df);
    QCOMPARE(df.shape().first, std::size_t(3));
    QCOMPARE(df.shape().second, std::size_t(3));

    // iloc(整数) 是行：一行长度等于列数（这正是误用时会「丢点」的原因）
    const DA::DAPySeries row0 = df.iloc(0);
    QVERIFY(row0);
    QCOMPARE(row0.size(), std::size_t(3));

    // columnAt 按位置取列，重名列也各取各的
    const DA::DAPySeries c0 = df.columnAt(0);
    const DA::DAPySeries c1 = df.columnAt(1);
    const DA::DAPySeries c2 = df.columnAt(2);
    QVERIFY(c0);
    QVERIFY(c1);
    QVERIFY(c2);
    QCOMPARE(c0.size(), std::size_t(3));

    const QVariantList v0 = c0.valuesToVariantList(0, 3);
    const QVariantList v1 = c1.valuesToVariantList(0, 3);
    const QVariantList v2 = c2.valuesToVariantList(0, 3);
    QCOMPARE(v0.size(), 3);
    QCOMPARE(v1.size(), 3);
    QCOMPARE(v2.size(), 3);
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(v0.at(i).toInt(), i + 1);
        QCOMPARE(v1.at(i).toInt(), (i + 1) * 10);
        QCOMPARE(v2.at(i).toInt(), (i + 1) * 100);
    }

    // 带行偏移的区间切片
    const QVariantList part = c1.valuesToVariantList(1, 2);
    QCOMPARE(part.size(), 2);
    QCOMPARE(part.at(0).toInt(), 20);
    QCOMPARE(part.at(1).toInt(), 30);

    // 越界列返回无效 Series
    QVERIFY(!df.columnAt(3));
}

// 不用 QTEST_MAIN：嵌入式 Python 的静态对象（解释器句柄、pybind11 类型缓存）在进程
// teardown 阶段析构顺序不可控，会在结果输出后被判定为 SEGFAULT（测试全 PASS 但退出码
// 非零，被 ctest 记为失败）。这里沿用 DATableDataTest / DATableUndoBenchmark 的做法：
// qExec 返回后结果已落盘，直接结束进程跳过 teardown，保证退出码可信。
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DADataFrameTest tc;
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
