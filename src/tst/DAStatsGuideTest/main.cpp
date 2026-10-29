#include <QtTest/QtTest>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QJsonObject>
#include <QVariantMap>
#include <cstdio>
#include <cstdlib>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "DAChartAddStatsBarplotWidget.h"
#include "DAChartWidget.h"
#include "DAStatsPlotCoordinator.h"
#include "DAData.h"
#include "DAPyInterpreter.h"
#include "DAPyScripts.h"
#include "DAPyScriptsStatistics.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtCaster.hpp"
#include <pybind11/eval.h>
#include "qwt_plot_item.h"

namespace
{
// 估计器下拉的规范 key（顺序与 .ui 一致，默认是第一个 mean）
const QStringList kEstimatorKeys = { QStringLiteral("mean"),
                                     QStringLiteral("median"),
                                     QStringLiteral("sum"),
                                     QStringLiteral("count"),
                                     QStringLiteral("std"),
                                     QStringLiteral("var"),
                                     QStringLiteral("min"),
                                     QStringLiteral("max") };

// 从 Python 返回字典里取字符串列表
QStringList dictStringList(const pybind11::dict& d, const char* key)
{
    QStringList res;
    pybind11::str k(key);
    if (!d.contains(k) || d[k].is_none()) {
        return res;
    }
    for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(d[k])) {
        res.append(pybind11::cast< QString >(item));
    }
    return res;
}

// 从 Python 返回字典里取浮点列表
QVector< double > dictDoubleList(const pybind11::dict& d, const char* key)
{
    QVector< double > res;
    pybind11::str k(key);
    if (!d.contains(k) || d[k].is_none()) {
        return res;
    }
    for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(d[k])) {
        res.append(pybind11::cast< double >(item));
    }
    return res;
}

}  // namespace

/**
 * @brief 统计绘图向导参数面板回归
 *
 * 重点：分组柱状图（barplot）的估计器下拉必须把「规范估计器 key」通过 itemData 传给
 * Python，而不是界面显示文本——显示文本会被翻译成中文，用 currentText 会让 median/sum/
 * std/var/min/max 全部落到 ESTIMATOR_MAP 的默认值 mean。
 */
class DAStatsGuideTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    // 下拉项与 itemData：8 个估计器、默认 mean、逐项切换得到对应 key
    void testEstimatorItemDataAndDefault();
    // 中文界面下（显示文本已翻译）仍必须送出规范 key
    void testEstimatorKeySurvivesTranslation();
    // min/max 口径真的生效：同一份数据 mean/min/max 给出不同结果
    void testMinMaxAggregation();
    // 统计向导 barplot 编排链路：每个估计器（默认/mean/min/max）都能渲染出柱状图
    void testCoordinatorBarplotRendersEachEstimator();

    void initPython();

private:
    bool mTranslationLoaded { false };
};

void DAStatsGuideTest::initTestCase()
{
    initPython();
    // 安装中文翻译，模拟真实中文界面（.ui 里的估计器文本会被翻译）
    const QString qmPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../translations/da_zh_CN.qm");
    auto* translator     = new QTranslator(qApp);
    if (QFileInfo::exists(qmPath) && translator->load(qmPath) && qApp->installTranslator(translator)) {
        mTranslationLoaded = true;
    } else {
        delete translator;
    }
}

void DAStatsGuideTest::initPython()
{
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
        const QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
        if (!pypath.isEmpty()) {
            pythonHomePath = QFileInfo(pypath).absolutePath();
        }
    }
    DA::DAPyInterpreter::initializePythonInterpreter(pythonHomePath);
    try {
        pybind11::module::import("pandas");
    } catch (const std::exception& e) {
        qWarning() << "pandas not available:" << e.what();
        return;
    }
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
    if (!DA::DAPyScripts::initScripts()) {
        qWarning() << "DAPyScripts::initScripts failed";
    }
}

void DAStatsGuideTest::testEstimatorItemDataAndDefault()
{
    DA::DAChartAddStatsBarplotWidget widget;
    auto* combo = widget.findChild< QComboBox* >(QStringLiteral("comboBoxEstimator"));
    QVERIFY2(combo, "comboBoxEstimator not found");

    QCOMPARE(combo->count(), kEstimatorKeys.size());
    for (int i = 0; i < kEstimatorKeys.size(); ++i) {
        QCOMPARE(combo->itemData(i).toString(), kEstimatorKeys.at(i));
    }
    // 默认仍是 mean（列表第一项）
    QCOMPARE(combo->currentIndex(), 0);
    QCOMPARE(widget.buildPlotParams().value(QStringLiteral("estimator")).toString(), QStringLiteral("mean"));

    // 逐项切换：buildPlotParams 必须给出对应 key
    for (int i = 0; i < kEstimatorKeys.size(); ++i) {
        combo->setCurrentIndex(i);
        QCOMPARE(widget.buildPlotParams().value(QStringLiteral("estimator")).toString(), kEstimatorKeys.at(i));
    }
}

void DAStatsGuideTest::testEstimatorKeySurvivesTranslation()
{
    DA::DAChartAddStatsBarplotWidget widget;
    auto* combo = widget.findChild< QComboBox* >(QStringLiteral("comboBoxEstimator"));
    QVERIFY(combo);

    if (!mTranslationLoaded) {
        QSKIP("da_zh_CN.qm not available, translation-specific assertion skipped");
    }
    // 翻译生效：显示文本不再是英文 key（这正是不能用 currentText 的原因）
    QVERIFY(combo->itemText(0) != QStringLiteral("mean"));

    combo->setCurrentIndex(combo->findData(QStringLiteral("min")));
    QCOMPARE(widget.buildPlotParams().value(QStringLiteral("estimator")).toString(), QStringLiteral("min"));
    combo->setCurrentIndex(combo->findData(QStringLiteral("max")));
    QCOMPARE(widget.buildPlotParams().value(QStringLiteral("estimator")).toString(), QStringLiteral("max"));
    combo->setCurrentIndex(combo->findData(QStringLiteral("median")));
    QCOMPARE(widget.buildPlotParams().value(QStringLiteral("estimator")).toString(), QStringLiteral("median"));
}

void DAStatsGuideTest::testMinMaxAggregation()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    pybind11::exec(R"(
import pandas as pd
_da_stats_guide_df = pd.DataFrame({
    'cat': ['a', 'b', 'a', 'b'],
    'val': [1.0, 5.0, 3.0, 2.0],
})
)");
    DA::DAData data(pybind11::globals()["_da_stats_guide_df"]);
    DA::DAPyScriptsStatistics& stats = DA::DAPyScripts::getStatistics();

    const auto agg = [ & ](const QString& estimator) {
        QVariantMap args;
        if (!estimator.isEmpty()) {
            args[QStringLiteral("estimator")] = estimator;
        }
        return stats.aggregateByCategory(data.toDataFrame(),
                                         QStringLiteral("cat"),
                                         QStringLiteral("val"),
                                         args);
    };

    const QStringList categories = dictStringList(agg(QString()), "categories");
    QCOMPARE(categories, QStringList({ QStringLiteral("a"), QStringLiteral("b") }));

    // 默认（不传 estimator）= mean：a=(1+3)/2=2, b=(5+2)/2=3.5
    QCOMPARE(dictDoubleList(agg(QString()), "values"), QVector< double >({ 2.0, 3.5 }));
    // min：a=1, b=2
    QCOMPARE(dictDoubleList(agg(QStringLiteral("min")), "values"), QVector< double >({ 1.0, 2.0 }));
    // max：a=3, b=5
    QCOMPARE(dictDoubleList(agg(QStringLiteral("max")), "values"), QVector< double >({ 3.0, 5.0 }));
}

void DAStatsGuideTest::testCoordinatorBarplotRendersEachEstimator()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    pybind11::exec(R"(
import pandas as pd
_da_stats_guide_df2 = pd.DataFrame({
    'cat': ['a', 'b', 'a', 'b'],
    'val': [1.0, 5.0, 3.0, 2.0],
})
)");
    DA::DAData data(pybind11::globals()["_da_stats_guide_df2"]);

    const QStringList estimators = { QString(), QStringLiteral("mean"), QStringLiteral("min"), QStringLiteral("max") };
    for (const QString& estimator : estimators) {
        DA::DAChartWidget chart;
        QJsonObject params;
        params[QStringLiteral("column")]   = QStringLiteral("cat");
        params[QStringLiteral("y_column")] = QStringLiteral("val");
        if (!estimator.isEmpty()) {
            params[QStringLiteral("estimator")] = estimator;
        }
        DAStatsPlotCoordinator coordinator;
        QVERIFY2(coordinator.execute(QStringLiteral("barplot"), params, nullptr, &chart, data),
                 qPrintable(QStringLiteral("estimator=%1").arg(estimator)));

        int barCount = 0;
        for (QwtPlotItem* item : chart.itemList()) {
            if (item && item->rtti() == QwtPlotItem::Rtti_PlotBarChart) {
                ++barCount;
            }
        }
        QCOMPARE(barCount, 1);  // 每个估计器都渲染出一根柱状图元
    }
}

/**
 * @brief 测试入口
 *
 * 同 DATableQuickPlotTest：初始化内嵌 Python 后，进程退出阶段的 DLL detach 会死锁，
 * 因此在 QtTest 报告输出完成后用 TerminateProcess 直接结束进程。
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DAStatsGuideTest tc;
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
