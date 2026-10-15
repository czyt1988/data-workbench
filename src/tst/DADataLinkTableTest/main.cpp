// DADataLinkTableTest：DAChartOperateWidget::createFigure 与 DADataLinkTableWidget 联动回归测试
// 背景：onFigureCreated 曾对 lambda 槽使用 Qt::UniqueConnection，Qt Debug 版会在
// QObject::connect 内触发 Q_ASSERT 断言直接中止进程（Debug 版点击“添加绘图”必现崩溃）
#include <QtTest/QtTest>
#include <QApplication>
#include <cstdlib>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "Chart/DAChartOperateWidget.h"
#include "Chart/DADataLinkTableWidget.h"

class DADataLinkTableTest : public QObject
{
    Q_OBJECT
private slots:
    // 创建 figure 时联动表正常接入，不触发 Qt 断言崩溃
    void testCreateFigureWithLinkTable()
    {
        DA::DAChartOperateWidget chartOpt;
        DA::DADataLinkTableWidget linkTable;
        linkTable.setChartOperateWidget(&chartOpt);

        DA::DAFigureWidget* fig = chartOpt.createFigure();
        QVERIFY(nullptr != fig);
        QCOMPARE(chartOpt.getFigureCount(), 1);
        QCOMPARE(chartOpt.getFigureList().size(), 1);

        // 重复接入同一 chartOpt 不应产生重复连接（onFigureCreated 先断开再连接，幂等）
        linkTable.setChartOperateWidget(&chartOpt);
        QCOMPARE(chartOpt.getFigureCount(), 1);
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DADataLinkTableTest tc;
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
