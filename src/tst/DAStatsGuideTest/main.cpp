#include <QtTest/QtTest>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QVariantMap>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "Chart/AddItem/DAChartAddStatsBarplotWidget.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "Chart/DAStatsPlotCoordinator.h"
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
#include "DAData.h"
#include "DAPyInterpreter.h"
#include "DAPyScripts.h"
#include "DAPyScriptsStatistics.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtCaster.hpp"
#include "Chart/DADataStatsWidget.h"
#include <QAction>
#include <QMenu>
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
    // toPythonParams 键集白名单：每类型一个用例，锁死 C++↔Python 契约
    void testToPythonParamsKeyWhitelist();
    // isValid()/errorString()：空列、空 data、空 figure 各一例（错误消息非空）
    void testIsValidErrorString();
    // heatmap 死链修复回归：indexColumn/columnsColumn/valueColumn → 渲染出谱图图元
    void testHeatmapRendersSpectrogram();
    // compute_describe：多列描述统计 + 箱线图五数概括（统计窗口的数据源）
    void testComputeDescribe();
    // compute_pareto：分箱降序 + 分布率 + 累计分布（统计窗口帕累托图的数据源）
    void testComputePareto();
    // 统计窗口命令动作：动作齐全、默认勾选、分位数下拉、绘图类型互斥与帕累托选项可用性
    void testStatsWidgetCommands();

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
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(widget.createPlotRequest());
        QCOMPARE(static_cast< DA::DAStatsBarplotRequest* >(req.get())->estimator, QStringLiteral("mean"));
    }

    // 逐项切换：createPlotRequest 必须给出对应 key
    for (int i = 0; i < kEstimatorKeys.size(); ++i) {
        combo->setCurrentIndex(i);
        std::unique_ptr< DA::DAStatsPlotRequest > req(widget.createPlotRequest());
        QCOMPARE(static_cast< DA::DAStatsBarplotRequest* >(req.get())->estimator, kEstimatorKeys.at(i));
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

    const auto estimatorOf = [ &widget ]() -> QString {
        std::unique_ptr< DA::DAStatsPlotRequest > req(widget.createPlotRequest());
        return static_cast< DA::DAStatsBarplotRequest* >(req.get())->estimator;
    };
    combo->setCurrentIndex(combo->findData(QStringLiteral("min")));
    QCOMPARE(estimatorOf(), QStringLiteral("min"));
    combo->setCurrentIndex(combo->findData(QStringLiteral("max")));
    QCOMPARE(estimatorOf(), QStringLiteral("max"));
    combo->setCurrentIndex(combo->findData(QStringLiteral("median")));
    QCOMPARE(estimatorOf(), QStringLiteral("median"));
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
        DA::DAStatsBarplotRequest req;
        req.data    = data;
        req.chart   = &chart;
        req.xColumn = QStringLiteral("cat");
        req.yColumn = QStringLiteral("val");
        if (!estimator.isEmpty()) {
            req.estimator = estimator;
        }
        DAStatsPlotCoordinator coordinator;
        QVERIFY2(coordinator.execute(req),
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
 * @brief toPythonParams 键集白名单：每类型一个用例，锁死 C++↔Python 契约
 *
 * 键集与 src/PyScripts/DAWorkbench/DAStatistics 各函数实际读取的 params 键一一对应
 * （见重构方案 §3.4 审计表）；QVariantMap::keys() 按字母序返回，断言即排序后精确相等。
 * 可选键（binrange/bw_value/hue/vmin 等）只在有值时出现，此处同时断言两种形态。
 */
void DAStatsGuideTest::testToPythonParamsKeyWhitelist()
{
    using DA::DAStatsPlotRequest;
    // histplot：bins/binwidth/stat/cumulative；binrange 有值才出现；kde 为 C++ 键不进 params
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsHistplotRequest);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("bins"), QStringLiteral("binwidth"), QStringLiteral("cumulative"), QStringLiteral("stat") }));
        static_cast< DA::DAStatsHistplotRequest* >(req.get())->binrange = qMakePair(0.0, 4.0);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("binrange"), QStringLiteral("bins"), QStringLiteral("binwidth"),
                               QStringLiteral("cumulative"), QStringLiteral("stat") }));
    }
    // kdeplot_1d：bw_method/grid_size/cumulative；bw_value 仅 custom 时出现
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsKdeplot1dRequest);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("bw_method"), QStringLiteral("cumulative"), QStringLiteral("grid_size") }));
        static_cast< DA::DAStatsKdeplot1dRequest* >(req.get())->bwValue = 0.5;
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("bw_method"), QStringLiteral("bw_value"), QStringLiteral("cumulative"),
                               QStringLiteral("grid_size") }));
    }
    // kdeplot_2d：bw_method/grid_size/levels/thresh；fillContours 为 C++ 键不进 params
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsKdeplot2dRequest);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("bw_method"), QStringLiteral("grid_size"), QStringLiteral("levels"), QStringLiteral("thresh") }));
    }
    // boxplot：columns/whis；hue 非空才出现；width/showfliers/showmeans 为死键不进 params
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsBoxplotRequest);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("columns"), QStringLiteral("whis") }));
        static_cast< DA::DAStatsBoxplotRequest* >(req.get())->hue = QStringLiteral("g");
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("columns"), QStringLiteral("hue"), QStringLiteral("whis") }));
    }
    // heatmap：aggfunc；standard_scale/vmin/vmax/center 有值才出现；annot/fmt 为死键不进 params
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsHeatmapRequest);
        QCOMPARE(req->toPythonParams().keys(), QStringList({ QStringLiteral("aggfunc") }));
        auto* hm = static_cast< DA::DAStatsHeatmapRequest* >(req.get());
        hm->standardScale = QStringLiteral("row");
        hm->vmin = 0.0;
        hm->vmax = 1.0;
        hm->center = 0.5;
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("aggfunc"), QStringLiteral("center"), QStringLiteral("standard_scale"),
                               QStringLiteral("vmax"), QStringLiteral("vmin") }));
    }
    // scatterplot：纯 C++ 提取绘制，不调 Python → 空 map
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsScatterplotRequest);
        QVERIFY(req->toPythonParams().isEmpty());
    }
    // barplot：estimator/n_boot；hue 非空才出现；ci 为 C++ 键（computeCI 时补 ciArgs）；orient/width/legend 为死键
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsBarplotRequest);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("estimator"), QStringLiteral("n_boot") }));
        static_cast< DA::DAStatsBarplotRequest* >(req.get())->hue = QStringLiteral("g");
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("estimator"), QStringLiteral("hue"), QStringLiteral("n_boot") }));
    }
    // regplot：order/n_boot；ci>0 才出现；scatter/fitReg/scatter_kws/reg_kws 为死键
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsRegplotRequest);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("n_boot"), QStringLiteral("order") }));
        static_cast< DA::DAStatsRegplotRequest* >(req.get())->ci = 95;
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("ci"), QStringLiteral("n_boot"), QStringLiteral("order") }));
    }
    // ecdfplot：stat/complementary；weights 非空才出现；line_width/legend/hue 为死键
    {
        std::unique_ptr< DA::DAStatsPlotRequest > req(new DA::DAStatsEcdfplotRequest);
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("complementary"), QStringLiteral("stat") }));
        static_cast< DA::DAStatsEcdfplotRequest* >(req.get())->weights = QStringLiteral("w");
        QCOMPARE(req->toPythonParams().keys(),
                 QStringList({ QStringLiteral("complementary"), QStringLiteral("stat"), QStringLiteral("weights") }));
    }
}

/**
 * @brief isValid()/errorString() 回归：空 figure、空 data、空列逐一拦截且消息非空
 */
void DAStatsGuideTest::testIsValidErrorString()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    pybind11::exec(R"(
import pandas as pd
_da_stats_valid_df = pd.DataFrame({'a': [1.0, 2.0, 3.0]})
)");
    DA::DAData data(pybind11::globals()["_da_stats_valid_df"]);

    DA::DAChartWidget chart;
    DA::DAFigureWidget figure;
    DA::DAStatsHistplotRequest req;
    req.chart = &chart;

    // 无 figure
    QVERIFY(!req.isValid());
    QVERIFY(!req.errorString().isEmpty());

    req.figure = &figure;
    // 无 data
    QVERIFY(!req.isValid());
    QVERIFY(!req.errorString().isEmpty());

    req.data = data;
    // 无 column
    QVERIFY(!req.isValid());
    QVERIFY(!req.errorString().isEmpty());

    req.column = QStringLiteral("a");
    QVERIFY(req.isValid());
    QVERIFY(req.errorString().isEmpty());
}

/**
 * @brief heatmap 死链修复回归：indexColumn/columnsColumn/valueColumn → 渲染出谱图图元
 *
 * 修复前 widget 写 column/y_column/value_column、coordinator 读 index_col/columns_col/values_col，
 * 键名三元组断裂导致热力图点确定后静默无图。
 */
void DAStatsGuideTest::testHeatmapRendersSpectrogram()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    pybind11::exec(R"(
import pandas as pd
_da_stats_heatmap_df = pd.DataFrame({
    'cat': ['a', 'b', 'a', 'b'],
    's': ['x', 'x', 'y', 'y'],
    'val': [1.0, 2.0, 3.0, 4.0],
})
)");
    DA::DAData data(pybind11::globals()["_da_stats_heatmap_df"]);

    DA::DAChartWidget chart;
    DA::DAStatsHeatmapRequest req;
    req.data = data;
    req.chart = &chart;
    req.indexColumn = QStringLiteral("s");       // 行方向（Y 轴）
    req.columnsColumn = QStringLiteral("cat");  // 列方向（X 轴）
    req.valueColumn = QStringLiteral("val");
    req.aggfunc = QStringLiteral("mean");

    DAStatsPlotCoordinator coordinator;
    QVERIFY2(coordinator.execute(req), "heatmap execute failed");

    int spectrogramCount = 0;
    for (QwtPlotItem* item : chart.itemList()) {
        if (item && item->rtti() == QwtPlotItem::Rtti_PlotSpectrogram) {
            ++spectrogramCount;
        }
    }
    QCOMPARE(spectrogramCount, 1);  // 修复前：plotHeatmap 开头即 return，渲染不出任何图元
}

/**
 * @brief compute_describe 回归：默认集 + 扩展统计 + 文本统计 + 箱线图五数概括
 *
 * 数据刻意设计成可手算验证：
 * - a = 1..8 + 一个 NaN：count=8 / missing=1，mean=4.5，std=√6，Q1=2.75，中位=4.5，Q3=6.25
 * - b = [10, 20] + 7 个 NaN：count=2 / missing=7 的边界样本——skew/kurt/正态性均为 None
 * - s = 字符串列：非数值列被跳过（labels 只剩 a/b）
 */
void DAStatsGuideTest::testComputeDescribe()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    pybind11::exec(R"(
import numpy as np
import pandas as pd
_da_describe_df = pd.DataFrame({
    'a': [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, np.nan],
    'b': [10.0, 20.0] + [np.nan] * 7,
    's': ['x', 'y', 'x', 'y', 'x', 'y', 'x', 'y', 'x'],
})
)");
    DA::DAData data(pybind11::globals()["_da_describe_df"]);
    DA::DAPyScriptsStatistics& stats = DA::DAPyScripts::getStatistics();

    QVariantMap args;
    QVariantList columnsArg;
    columnsArg << 0 << 1 << 2;  // 按位置传列（a, b, s）
    args[ QStringLiteral("columns") ] = columnsArg;
    QVariantList labelsArg;
    labelsArg << QStringLiteral("a") << QStringLiteral("b") << QStringLiteral("s");
    args[ QStringLiteral("labels") ] = labelsArg;
    QVariantList statsArg;
    statsArg << QStringLiteral("count") << QStringLiteral("missing") << QStringLiteral("unique") << QStringLiteral("sum")
             << QStringLiteral("mean") << QStringLiteral("std") << QStringLiteral("var") << QStringLiteral("sem")
             << QStringLiteral("min") << QStringLiteral("max") << QStringLiteral("range") << QStringLiteral("q05")
             << QStringLiteral("q1") << QStringLiteral("median") << QStringLiteral("q3") << QStringLiteral("q95")
             << QStringLiteral("iqr") << QStringLiteral("mode") << QStringLiteral("cv") << QStringLiteral("mad")
             << QStringLiteral("skew") << QStringLiteral("kurt")
             << QStringLiteral("norm_stat") << QStringLiteral("norm_p") << QStringLiteral("norm_verdict")
             << QStringLiteral("norm_test")
             << QStringLiteral("ci_low") << QStringLiteral("ci_high")
             << QStringLiteral("outliers") << QStringLiteral("outlier_low") << QStringLiteral("outlier_high");
    args[ QStringLiteral("stats") ] = statsArg;

    pybind11::dict result = stats.computeDescribe(data.toDataFrame(), args);
    QVERIFY(!result.is_none() && pybind11::len(result) > 0);

    // 非数值列 s 被跳过：labels 只剩 a/b
    QCOMPARE(dictStringList(result, "labels"), QStringList({ QStringLiteral("a"), QStringLiteral("b") }));

    // 某个统计键的数值行（None → NaN）
    const auto statRowNumbers = [](const pybind11::dict& d, const char* key) {
        QVector< double > res;
        pybind11::str statsKey("stats");
        for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(d[ statsKey ])) {
            auto sd = pybind11::cast< pybind11::dict >(item);
            if (pybind11::cast< QString >(sd[ "key" ]) == QLatin1String(key)) {
                for (pybind11::handle v : pybind11::reinterpret_borrow< pybind11::iterable >(sd[ "values" ])) {
                    res.append(v.is_none() ? qQNaN() : pybind11::cast< double >(v));
                }
                break;
            }
        }
        return res;
    };
    // 某个统计键的文本行（None → 空串）
    const auto statRowTexts = [](const pybind11::dict& d, const char* key) {
        QStringList res;
        pybind11::str statsKey("stats");
        for (pybind11::handle item : pybind11::reinterpret_borrow< pybind11::iterable >(d[ statsKey ])) {
            auto sd = pybind11::cast< pybind11::dict >(item);
            if (pybind11::cast< QString >(sd[ "key" ]) == QLatin1String(key)) {
                for (pybind11::handle v : pybind11::reinterpret_borrow< pybind11::iterable >(sd[ "values" ])) {
                    res.append(v.is_none() ? QString() : pybind11::cast< QString >(v));
                }
                break;
            }
        }
        return res;
    };

    // ---- 默认集：a=1..8（缺 1），b=[10,20] ----
    QCOMPARE(statRowNumbers(result, "count"), QVector< double >({ 8, 2 }));
    QCOMPARE(statRowNumbers(result, "missing"), QVector< double >({ 1, 7 }));
    QCOMPARE(statRowNumbers(result, "unique"), QVector< double >({ 8, 2 }));
    QCOMPARE(statRowNumbers(result, "sum"), QVector< double >({ 36, 30 }));
    QCOMPARE(statRowNumbers(result, "mean"), QVector< double >({ 4.5, 15.0 }));
    QVERIFY(qFuzzyCompare(statRowNumbers(result, "std").at(0), std::sqrt(6.0)));   // a: √6
    QVERIFY(qFuzzyCompare(statRowNumbers(result, "std").at(1), std::sqrt(50.0)));  // b: √50
    QCOMPARE(statRowNumbers(result, "var"), QVector< double >({ 6.0, 50.0 }));
    QCOMPARE(statRowNumbers(result, "min"), QVector< double >({ 1.0, 10.0 }));
    QCOMPARE(statRowNumbers(result, "q1"), QVector< double >({ 2.75, 12.5 }));
    QCOMPARE(statRowNumbers(result, "median"), QVector< double >({ 4.5, 15.0 }));
    QCOMPARE(statRowNumbers(result, "q3"), QVector< double >({ 6.25, 17.5 }));
    QCOMPARE(statRowNumbers(result, "max"), QVector< double >({ 8.0, 20.0 }));

    // ---- 扩展统计 ----
    QCOMPARE(statRowNumbers(result, "range"), QVector< double >({ 7.0, 10.0 }));
    QCOMPARE(statRowNumbers(result, "iqr"), QVector< double >({ 3.5, 5.0 }));
    QCOMPARE(statRowNumbers(result, "mode"), QVector< double >({ 1.0, 10.0 }));
    QCOMPARE(statRowNumbers(result, "mad"), QVector< double >({ 2.0, 5.0 }));
    QCOMPARE(statRowNumbers(result, "q05").at(0), 1.35);
    QCOMPARE(statRowNumbers(result, "q05").at(1), 10.5);
    QVERIFY(qFuzzyCompare(statRowNumbers(result, "q95").at(0), 7.65));  // 浮点插值，模糊比较
    QCOMPARE(statRowNumbers(result, "q95").at(1), 19.5);
    QVERIFY(qFuzzyCompare(statRowNumbers(result, "sem").at(0), std::sqrt(6.0) / std::sqrt(8.0)));
    QVERIFY(qFuzzyCompare(statRowNumbers(result, "cv").at(0), std::sqrt(6.0) / 4.5));

    // ---- 分析组：小样本边界 + 正态性 ----
    QVERIFY(!std::isnan(statRowNumbers(result, "skew").at(0)));   // a (n=8)：偏度有值
    QVERIFY(std::isnan(statRowNumbers(result, "skew").at(1)));    // b (n=2)：偏度未定义
    QVERIFY(!std::isnan(statRowNumbers(result, "kurt").at(0)));   // a (n=8)：峰度有值
    QVERIFY(std::isnan(statRowNumbers(result, "kurt").at(1)));    // b (n=4 以下)：峰度未定义
    // a (n=8)：3<=n<=5000 → Shapiro-Wilk；b (n=2)：无可用检验（机器键 "none"，界面翻译为 —）
    QCOMPARE(statRowTexts(result, "norm_test"), QStringList({ QStringLiteral("shapiro"), QStringLiteral("none") }));
    QVERIFY(!std::isnan(statRowNumbers(result, "norm_stat").at(0)));
    QVERIFY(std::isnan(statRowNumbers(result, "norm_stat").at(1)));
    QVERIFY(!std::isnan(statRowNumbers(result, "norm_p").at(0)));
    QVERIFY(statRowTexts(result, "norm_verdict").at(0) == QStringLiteral("yes")
            || statRowTexts(result, "norm_verdict").at(0) == QStringLiteral("no"));
    // 均值 95% CI：对称包住均值（b 的 t 分布 df=1 区间极宽，只验证对称与包含关系）
    const double meanB = 15.0;
    const double ciLowB = statRowNumbers(result, "ci_low").at(1);
    const double ciHighB = statRowNumbers(result, "ci_high").at(1);
    QVERIFY(ciLowB < meanB && meanB < ciHighB);
    QVERIFY(qFuzzyCompare(meanB - ciLowB, ciHighB - meanB));

    // ---- 离群点分析（1.5×IQR）----
    QCOMPARE(statRowNumbers(result, "outliers"), QVector< double >({ 0, 0 }));
    QCOMPARE(statRowNumbers(result, "outlier_low"), QVector< double >({ 2.75 - 5.25, 12.5 - 7.5 }));
    QCOMPARE(statRowNumbers(result, "outlier_high"), QVector< double >({ 6.25 + 5.25, 17.5 + 7.5 }));

    // ---- 箱线图五数概括（与 compute_boxplot_stats 同构，供共用渲染管线）----
    pybind11::str boxKey("boxplot");
    QVERIFY(result.contains(boxKey));
    auto box = pybind11::cast< pybind11::dict >(result[ boxKey ]);
    pybind11::str samplesKey("samples");
    QVERIFY(box.contains(samplesKey));
    auto samples = pybind11::cast< pybind11::list >(box[ samplesKey ]);
    QCOMPARE((int)pybind11::len(samples), 2);  // 每列一个箱体
    {
        auto s0 = pybind11::cast< pybind11::dict >(samples[ 0 ]);
        QCOMPARE(pybind11::cast< QString >(s0[ "column" ]).toStdString(), std::string("a"));
        QCOMPARE(pybind11::cast< double >(s0[ "position" ]), 0.0);
        QCOMPARE(pybind11::cast< double >(s0[ "q1" ]), 2.75);
        QCOMPARE(pybind11::cast< double >(s0[ "median" ]), 4.5);
        QCOMPARE(pybind11::cast< double >(s0[ "q3" ]), 6.25);
        QCOMPARE(pybind11::cast< double >(s0[ "whisker_lower" ]), 1.0);
        QCOMPARE(pybind11::cast< double >(s0[ "whisker_upper" ]), 8.0);
        QVERIFY(pybind11::cast< pybind11::list >(s0[ "outliers" ]).empty());
        auto s1 = pybind11::cast< pybind11::dict >(samples[ 1 ]);
        QCOMPARE(pybind11::cast< QString >(s1[ "column" ]).toStdString(), std::string("b"));
        QCOMPARE(pybind11::cast< double >(s1[ "whisker_lower" ]), 10.0);
        QCOMPARE(pybind11::cast< double >(s1[ "whisker_upper" ]), 20.0);
    }

    // ---- 采样口径：compute_describe_from_lists（统计窗口的惰性/超大表路径）----
    QVariantMap columns;
    QVariantList v0;
    v0 << 1.0 << 2.0 << 3.0 << 4.0;
    QVariantList v1;
    v1 << 5.0 << 6.0;
    columns[ QStringLiteral("__v0") ] = v0;
    columns[ QStringLiteral("__v1") ] = v1;
    // 传给 Python 的列表一律用 QVariantList（QStringList 值的 QVariant 会退化为空串）
    QVariantMap args2;
    QVariantList columnsArg2;
    columnsArg2 << QStringLiteral("__v0") << QStringLiteral("__v1");
    args2[ QStringLiteral("columns") ] = columnsArg2;
    QVariantList labelsArg2;
    labelsArg2 << QStringLiteral("x") << QStringLiteral("y");
    args2[ QStringLiteral("labels") ] = labelsArg2;
    QVariantList statsArg2;
    statsArg2 << QStringLiteral("count") << QStringLiteral("mean") << QStringLiteral("std");
    args2[ QStringLiteral("stats") ] = statsArg2;
    pybind11::dict result2 = stats.computeDescribeFromLists(columns, args2);
    QVERIFY(!result2.is_none() && pybind11::len(result2) > 0);
    QCOMPARE(dictStringList(result2, "labels"), QStringList({ QStringLiteral("x"), QStringLiteral("y") }));
    QCOMPARE(statRowNumbers(result2, "count"), QVector< double >({ 4, 2 }));
    QCOMPARE(statRowNumbers(result2, "mean"), QVector< double >({ 2.5, 5.5 }));
    QVERIFY(qFuzzyCompare(statRowNumbers(result2, "std").at(0), std::sqrt(5.0 / 3.0)));
}

/**
 * @brief compute_pareto 回归：分箱按频数降序 + 分布率 + 累计分布
 *
 * 数据 [1,1,1,2,2,3] 分 3 箱（range 1~3）：箱 0 = 3 个、箱 1 = 2 个、箱 2 = 1 个，
 * 天然就是降序；rates = 3/6、2/6、1/6，cumulative 依次 0.5、0.8333、1.0。
 */
void DAStatsGuideTest::testComputePareto()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    DA::DAPyScriptsStatistics& stats = DA::DAPyScripts::getStatistics();

    QVariantList values;
    values << 1.0 << 1.0 << 1.0 << 2.0 << 2.0 << 3.0;
    QVariantMap args;
    args[ QStringLiteral("bins") ] = 3;
    pybind11::dict result          = stats.computePareto(values, args);
    QVERIFY(!result.is_none() && pybind11::len(result) > 0);

    const QVector< double > counts     = dictDoubleList(result, "counts");
    const QVector< double > rates      = dictDoubleList(result, "rates");
    const QVector< double > cumulative = dictDoubleList(result, "cumulative");
    const QVector< double > lower      = dictDoubleList(result, "bin_lower");
    const QVector< double > upper      = dictDoubleList(result, "bin_upper");
    QCOMPARE(counts.size(), 3);
    QCOMPARE(rates.size(), 3);
    QCOMPARE(cumulative.size(), 3);
    QCOMPARE(lower.size(), 3);
    QCOMPARE(upper.size(), 3);

    // 频数降序
    QCOMPARE(counts, QVector< double >({ 3.0, 2.0, 1.0 }));
    QVERIFY(qFuzzyCompare(rates.at(0), 0.5));
    QVERIFY(qFuzzyCompare(rates.at(1), 2.0 / 6.0));
    QVERIFY(qFuzzyCompare(rates.at(2), 1.0 / 6.0));
    QVERIFY(qFuzzyCompare(cumulative.at(0), 0.5));
    QVERIFY(qFuzzyCompare(cumulative.at(1), 5.0 / 6.0));
    QVERIFY(qFuzzyCompare(cumulative.at(2), 1.0));
    // 分箱边界单调且首尾覆盖数据范围
    QVERIFY(lower.at(0) < upper.at(0));
    QVERIFY(qFuzzyCompare(cumulative.last(), 1.0));

    // 空数据：Python 抛错后包装层返回空字典（界面按"无法绘图"处理）
    QVariantList emptyValues;
    pybind11::dict emptyResult = stats.computePareto(emptyValues, args);
    QVERIFY(emptyResult.is_none() || pybind11::len(emptyResult) == 0);
}

/**
 * @brief 统计窗口的 Ribbon 命令动作回归
 *
 * 窗口自身不再有工具栏，全部命令都以 QAction 暴露给 Ribbon：这里核对动作齐全、
 * 默认勾选集（8 个默认统计量 + 25/50/75 分位数）、绘图类型互斥、帕累托选项只在
 * 帕累托图下可用，以及动作图标能正常从 qrc 载入。
 */
void DAStatsGuideTest::testStatsWidgetCommands()
{
    using Command   = DA::DADataStatsWidget::Command;
    using PlotType  = DA::DADataStatsWidget::PlotType;
    DA::DADataStatsWidget widget;

    const QList< Command > allCommands = {
        Command::Count,      Command::Missing,   Command::Unique,          Command::Sum,
        Command::Mean,       Command::Std,       Command::Var,             Command::Sem,
        Command::Min,        Command::Max,       Command::Range,           Command::Iqr,
        Command::Mad,        Command::Mode,      Command::Cv,              Command::Skew,
        Command::Kurt,       Command::Percentile,
        Command::TestNormality, Command::TestConfidence, Command::TestOutliers,
        Command::Refresh,    Command::PlotBoxplot, Command::PlotPareto,
        Command::ParetoRate, Command::ParetoCumulative, Command::TransferToFigure
    };
    for (Command c : std::as_const(allCommands)) {
        QAction* act = widget.command(c);
        QVERIFY2(act != nullptr, qPrintable(QStringLiteral("missing command action: %1").arg(static_cast< int >(c))));
        QVERIFY2(!act->text().isEmpty(), qPrintable(QStringLiteral("empty text: %1").arg(static_cast< int >(c))));
        QVERIFY2(!act->icon().isNull(), qPrintable(QStringLiteral("missing icon: %1").arg(static_cast< int >(c))));
    }

    // 默认勾选：count/mean/std/min/max + 分位数 25/50/75（分位数在菜单里）
    QVERIFY(widget.command(Command::Count)->isChecked());
    QVERIFY(widget.command(Command::Mean)->isChecked());
    QVERIFY(widget.command(Command::Std)->isChecked());
    QVERIFY(widget.command(Command::Min)->isChecked());
    QVERIFY(widget.command(Command::Max)->isChecked());
    QVERIFY(!widget.command(Command::Missing)->isChecked());
    QVERIFY(!widget.command(Command::Var)->isChecked());
    QVERIFY(!widget.command(Command::Skew)->isChecked());
    QVERIFY(!widget.command(Command::TestNormality)->isChecked());

    // 分位数下拉：5/25/50/75/95 五项，默认勾选 25/50/75
    QMenu* percentileMenu = widget.percentileMenu();
    QVERIFY(percentileMenu);
    const QList< QAction* > percentileActions = percentileMenu->actions();
    QCOMPARE(percentileActions.size(), 5);
    QStringList percentileKeys;
    for (const QAction* act : percentileActions) {
        percentileKeys << act->data().toString();
    }
    QCOMPARE(percentileKeys,
             QStringList({ QStringLiteral("q05"), QStringLiteral("q1"), QStringLiteral("median"),
                           QStringLiteral("q3"), QStringLiteral("q95") }));
    QVERIFY(!percentileActions.at(0)->isChecked());  // 5%
    QVERIFY(percentileActions.at(1)->isChecked());   // 25%
    QVERIFY(percentileActions.at(2)->isChecked());   // 50%
    QVERIFY(percentileActions.at(3)->isChecked());   // 75%
    QVERIFY(!percentileActions.at(4)->isChecked());  // 95%

    // 绘图类型：默认箱线图，互斥切换后帕累托选项才可用
    QCOMPARE(widget.plotType(), PlotType::Boxplot);
    QVERIFY(widget.command(Command::PlotBoxplot)->isChecked());
    QVERIFY(!widget.command(Command::PlotPareto)->isChecked());
    QVERIFY(!widget.command(Command::ParetoRate)->isEnabled());
    QVERIFY(!widget.command(Command::ParetoCumulative)->isEnabled());

    widget.setPlotType(PlotType::Pareto);
    QCOMPARE(widget.plotType(), PlotType::Pareto);
    QVERIFY(!widget.command(Command::PlotBoxplot)->isChecked());
    QVERIFY(widget.command(Command::PlotPareto)->isChecked());
    QVERIFY(widget.command(Command::ParetoRate)->isEnabled());
    QVERIFY(widget.command(Command::ParetoCumulative)->isEnabled());
    QVERIFY(widget.isParetoRateVisible());
    QVERIFY(widget.isParetoCumulativeVisible());

    // 切回箱线图：帕累托选项重新置灰
    widget.setPlotType(PlotType::Boxplot);
    QCOMPARE(widget.plotType(), PlotType::Boxplot);
    QVERIFY(!widget.command(Command::ParetoRate)->isEnabled());
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
