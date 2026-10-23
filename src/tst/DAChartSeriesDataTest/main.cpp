#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QScopedPointer>
#include <cstdio>
// DA
#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include "pandas/DAPyDataFrame.h"
#include "DAData.h"
#include "ChartAddItem/DAChartSeriesDataResult.h"
#include "ChartAddItem/DAChartAddItemWidgetFactory.h"
#include "ChartAddItem/DAChartAddCurveWidget.h"
#include "Dialog/DADialogChartGuide.h"
// qwt
#include "qwt_plot_curve.h"
// pybind11
#include <pybind11/eval.h>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace
{
// 与 APP 初始化逻辑一致：优先取带 Lib/site-packages 的真实安装（排除 Microsoft Store 存根）
QString findPythonHome()
{
    const QList< QFileInfo > candidates = DA::DAPyInterpreter::wherePython();
    for (const QFileInfo& fi : candidates) {
        QDir homeDir = fi.absoluteDir();
        if (homeDir.exists(QStringLiteral("Lib/site-packages"))) {
            return homeDir.absolutePath();
        }
    }
    const QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
    return pypath.isEmpty() ? QString() : QFileInfo(pypath).absolutePath();
}

// 构造测试数据：a/b 三行；另取一个两行的数据表用于长度不一致场景
// （pandas 不允许同一个 dataframe 内各列长度不同）
DA::DAData makeTestData(const char* expr)
{
    pybind11::object df = pybind11::eval(expr);
    return DA::DAData(DA::DAPyDataFrame(df));
}
}  // namespace

/**
 * @brief 图表取数层测试
 *
 * 这层不依赖任何界面元素，因此可以脱离对话框直接断言：
 * 取数结果、自增序列、以及各类失败原因码。
 */
class DAChartSeriesDataTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSingleSeries();
    void testXAutoincrement();
    void testYAutoincrement();
    void testBothAutoincrement();
    void testMissingX();
    void testMultiSeries();
    void testMultiSeriesLengthMismatch();
    void testMultiSeriesNoY();
    void testRegistryFamilies();
    void testGuideSharedPage();
    void testGuideCreatesItems();

private:
    DA::DAData mData;
    DA::DAData mShortData;
    bool mPandasOk = false;
};

void DAChartSeriesDataTest::initTestCase()
{
    DA::DAPyInterpreter::initializePythonInterpreter(findPythonHome());
    try {
        pybind11::module::import("pandas");
        mPandasOk = true;
    } catch (const std::exception& e) {
        qWarning() << "pandas not available:" << e.what();
    }
    if (!mPandasOk) {
        QSKIP("pandas not available, skip chart series extraction tests");
    }
    try {
        mData = makeTestData(
            "__import__('pandas').DataFrame({'a': [0.0, 1.0, 2.0], "
            "'b': [10.0, 20.0, 30.0]})");
        mShortData = makeTestData(
            "__import__('pandas').DataFrame({'d': [1.0, 2.0]})");
    } catch (const std::exception& e) {
        QFAIL(e.what());
    } catch (...) {
        QFAIL("unknown exception while building test dataframe");
    }
    QVERIFY(mData.isDataFrame());
    QVERIFY(mShortData.isDataFrame());
}

void DAChartSeriesDataTest::testSingleSeries()
{
    DA::DAChartXYSeriesSpec spec;
    spec.xData   = mData;
    spec.xColumn = QStringLiteral("a");
    spec.ySeries.append(qMakePair(mData, QStringLiteral("b")));

    const DA::DAChartSeriesResult res = DA::extractXYSeries(spec);
    QVERIFY2(res.isOk(), "single series extraction should succeed");
    QCOMPARE(res.points.size(), 3);
    QCOMPARE(res.points[ 0 ].x(), 0.0);
    QCOMPARE(res.points[ 0 ].y(), 10.0);
    QCOMPARE(res.points[ 2 ].x(), 2.0);
    QCOMPARE(res.points[ 2 ].y(), 30.0);
}

void DAChartSeriesDataTest::testXAutoincrement()
{
    DA::DAChartXYSeriesSpec spec;
    spec.xAutoincrement = true;
    spec.xBase          = 1.0;
    spec.xStep          = 2.0;
    spec.ySeries.append(qMakePair(mData, QStringLiteral("b")));

    const DA::DAChartSeriesResult res = DA::extractXYSeries(spec);
    QVERIFY2(res.isOk(), "x autoincrement extraction should succeed");
    QCOMPARE(res.points.size(), 3);
    QCOMPARE(res.points[ 0 ].x(), 1.0);
    QCOMPARE(res.points[ 1 ].x(), 3.0);
    QCOMPARE(res.points[ 2 ].x(), 5.0);
    QCOMPARE(res.points[ 2 ].y(), 30.0);
}

void DAChartSeriesDataTest::testYAutoincrement()
{
    DA::DAChartXYSeriesSpec spec;
    spec.xData          = mData;
    spec.xColumn        = QStringLiteral("a");
    spec.yAutoincrement = true;
    spec.yBase          = 0.0;
    spec.yStep          = 1.0;

    const DA::DAChartSeriesResult res = DA::extractXYSeries(spec);
    QVERIFY2(res.isOk(), "y autoincrement extraction should succeed");
    QCOMPARE(res.points.size(), 3);
    QCOMPARE(res.points[ 0 ].y(), 0.0);
    QCOMPARE(res.points[ 2 ].y(), 2.0);
}

void DAChartSeriesDataTest::testBothAutoincrement()
{
    DA::DAChartXYSeriesSpec spec;
    spec.xAutoincrement = true;
    spec.yAutoincrement = true;

    const DA::DAChartSeriesResult res = DA::extractXYSeries(spec);
    QVERIFY(!res.isOk());
    QCOMPARE(res.error.code, DA::DAChartSeriesDataError::BothAutoincrement);
    QVERIFY(res.points.isEmpty());
}

void DAChartSeriesDataTest::testMissingX()
{
    DA::DAChartXYSeriesSpec spec;
    spec.ySeries.append(qMakePair(mData, QStringLiteral("b")));

    const DA::DAChartSeriesResult res = DA::extractXYSeries(spec);
    QVERIFY(!res.isOk());
    QCOMPARE(res.error.code, DA::DAChartSeriesDataError::NoXSeries);
}

void DAChartSeriesDataTest::testMultiSeries()
{
    DA::DAChartXYSeriesSpec spec;
    spec.xData   = mData;
    spec.xColumn = QStringLiteral("a");
    spec.ySeries.append(qMakePair(mData, QStringLiteral("b")));
    spec.ySeries.append(qMakePair(mData, QStringLiteral("a")));

    const DA::DAChartMultiSeriesResult res = DA::extractMultiXYSeries(spec);
    QVERIFY2(res.isOk(), "multi series extraction should succeed");
    QCOMPARE(res.series.size(), 2);
    QCOMPARE(res.names, QStringList({ QStringLiteral("b"), QStringLiteral("a") }));
    QCOMPARE(res.series[ 0 ].size(), 3);
    QCOMPARE(res.series[ 1 ][ 2 ].y(), 2.0);
}

void DAChartSeriesDataTest::testMultiSeriesLengthMismatch()
{
    DA::DAChartXYSeriesSpec spec;
    spec.xData   = mData;
    spec.xColumn = QStringLiteral("a");
    spec.ySeries.append(qMakePair(mShortData, QStringLiteral("d")));

    const DA::DAChartMultiSeriesResult res = DA::extractMultiXYSeries(spec);
    QVERIFY(!res.isOk());
    QCOMPARE(res.error.code, DA::DAChartSeriesDataError::LengthMismatch);
    QCOMPARE(res.error.seriesName, QStringLiteral("d"));
    QCOMPARE(res.error.seriesSize, qulonglong(2));
    QCOMPARE(res.error.xSeriesName, QStringLiteral("a"));
    QCOMPARE(res.error.xSize, qulonglong(3));
}

void DAChartSeriesDataTest::testMultiSeriesNoY()
{
    DA::DAChartXYSeriesSpec spec;
    spec.xData   = mData;
    spec.xColumn = QStringLiteral("a");

    const DA::DAChartMultiSeriesResult res = DA::extractMultiXYSeries(spec);
    QVERIFY(!res.isOk());
    QCOMPARE(res.error.code, DA::DAChartSeriesDataError::NoYSeries);
}

void DAChartSeriesDataTest::testRegistryFamilies()
{
    DA::DAChartAddItemWidgetFactory& f = DA::DAChartAddItemWidgetFactory::instance();
    QVERIFY(f.isRegistered(DA::DAChartTypes::Curve));
    QVERIFY(f.isRegistered(DA::DAChartTypes::StatsHistplot));
    QCOMPARE(f.descriptors(DA::DAChartAddWidgetFamily::Curve2D).size(), 10);
    QCOMPARE(f.descriptors(DA::DAChartAddWidgetFamily::Surface3D).size(), 3);
    QCOMPARE(f.descriptors(DA::DAChartAddWidgetFamily::Stats).size(), 9);
    QVERIFY(DA::DADialogChartGuide::is3DChartType(DA::DAChartTypes::Surface3D));
    QVERIFY(!DA::DADialogChartGuide::is3DChartType(DA::DAChartTypes::Curve));
}

void DAChartSeriesDataTest::testGuideSharedPage()
{
    DA::DADialogChartGuide dlg;
    DA::DAAbstractChartAddItemWidget* curve   = dlg.getChartAddItemWidget(DA::DAChartTypes::Curve);
    DA::DAAbstractChartAddItemWidget* scatter = dlg.getChartAddItemWidget(DA::DAChartTypes::Scatter);
    QVERIFY(curve != nullptr);
    // 散点图与曲线共用同一个页面，切换类型时已选数据不丢
    QCOMPARE(scatter, curve);
    // 统计类型不属于本对话框，不应被创建
    QVERIFY(dlg.getChartAddItemWidget(DA::DAChartTypes::StatsHistplot) == nullptr);
    // 三维页面由注册表的 family 决定，按需创建
    QVERIFY(dlg.getChartAdd3DItemWidget(DA::DAChartTypes::Surface3D) != nullptr);
}

void DAChartSeriesDataTest::testGuideCreatesItems()
{
    DA::DADialogChartGuide dlg;
    dlg.setCurrentChartType(DA::DAChartTypes::Curve);
    auto* curve = qobject_cast< DA::DAChartAddCurveWidget* >(dlg.getChartAddItemWidget(DA::DAChartTypes::Curve));
    QVERIFY(curve != nullptr);
    curve->setX(mData, QStringLiteral("a"));
    curve->setY(mData, QStringLiteral("b"));

    QScopedPointer< QwtPlotItem > item(dlg.createPlotItem());
    QVERIFY2(!item.isNull(), "curve item should be created from selected series");
    QCOMPARE(item->rtti(), static_cast< int >(QwtPlotItem::Rtti_PlotCurve));
    QCOMPARE(static_cast< QwtPlotCurve* >(item.data())->dataSize(), 3);

    // 同一页面切到散点：注册表的 item 后置修饰应把线条改为点状
    dlg.setCurrentChartType(DA::DAChartTypes::Scatter);
    QScopedPointer< QwtPlotItem > dotItem(dlg.createPlotItem());
    QVERIFY2(!dotItem.isNull(), "scatter item should reuse the curve series");
    QCOMPARE(static_cast< QwtPlotCurve* >(dotItem.data())->style(), QwtPlotCurve::Dots);
}

// 与 DATableDataTest 相同：嵌入式 Python 的静态对象在进程 teardown 阶段析构顺序不可控，
// 结果写完后直接终止进程，保证退出码可信
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DAChartSeriesDataTest tc;
    int ret = QTest::qExec(&tc, argc, argv);
    std::fflush(nullptr);
#ifdef Q_OS_WIN
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(ret));
#else
    std::_Exit(ret);
#endif
    return ret;
}
#include "main.moc"
