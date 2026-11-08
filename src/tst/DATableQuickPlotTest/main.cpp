#include <QtTest/QtTest>
#include <QApplication>
#include <QDateTime>
#include <cstdio>
#include <cstdlib>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QDir>
#include <QFileInfo>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QUndoStack>
#include <QMetaType>
#include <QVariant>
#include <QAction>
#include <QComboBox>
#include <QElapsedTimer>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <memory>

#include "DAAbstractData.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "DAChartOperateWidget.h"
#include "Table/DADataOperateOfDataFrameWidget.h"
#include "DAFigureWidget.h"
#include "Table/Metadata/DATableMetadataRegistry.h"
#include "Table/Views/DADataTableView.h"
#include "DAPyInterpreter.h"
#include "DAPyScripts.h"
#include "DAPybind11InQt.h"
#include <pybind11/eval.h>
#include "DAData.h"
#include "DATableDataBlock.h"
#include "DATableDataSource.h"
#include "Table/Chart/DAChartQuickPlot.h"
#include "Table/Chart/DATableQuickPlotWindow.h"
#include "Table/Chart/DATableChartRolePanel.h"
#include "Table/Chart/DAChartSeriesSampler.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "Table/Chart/DATableChartTypes.h"
#include "Table/Views/DATableChildWindow.h"
#include "Table/DADataOperateWidget.h"
#include "Common/DANoDataGuideWidget.h"

namespace
{
/**
 * @brief 内存合成表格源（不依赖 Python）
 *
 * 列类型由 metaType 显式给出，值来自预置数组；无效 QVariant 视为缺失值。
 * 行数固定为最长列的长度，短列按越界视为缺失。
 */
class DAQuickPlotTestSource : public DA::DAAbstractData, public DA::DATableDataSource
{
public:
    struct Column
    {
        QString name;
        int metaType { QMetaType::Double };
        QVector< QVariant > values;
    };

    QVector< Column > columns;
    mutable int mFetchBlockCount = 0;

public:  // DAAbstractData
    DA::DAAbstractData::DataType getDataType() const override
    {
        return DA::DAAbstractData::TypeInnerData;
    }
    QVariant toVariant(std::size_t dim1, std::size_t dim2) const override
    {
        return cellAt(dim1, dim2);
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
    QString typeIdentifier() const override
    {
        return QStringLiteral("DATableQuickPlotTest.InMemoryTable");
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
        std::size_t n = 0;
        for (const Column& c : columns) {
            n = std::max(n, static_cast< std::size_t >(c.values.size()));
        }
        return n;
    }
    std::size_t tableColumnCount() const override
    {
        return static_cast< std::size_t >(columns.size());
    }
    QString tableColumnName(std::size_t column) const override
    {
        return column < static_cast< std::size_t >(columns.size()) ? columns.at(static_cast< int >(column)).name : QString();
    }
    int tableColumnType(std::size_t column) const override
    {
        return column < static_cast< std::size_t >(columns.size()) ? columns.at(static_cast< int >(column)).metaType
                                                                   : QMetaType::UnknownType;
    }
    DA::DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override
    {
        ++mFetchBlockCount;
        const std::size_t total = tableRowCount();
        if (startRow >= total) {
            return DA::DATableDataBlock();
        }
        const std::size_t n = std::min(rowCount, total - startRow);
        DA::DATableDataBlock block(startRow, tableColumnCount());
        for (std::size_t r = 0; r < n; ++r) {
            QVariantList row;
            for (std::size_t c = 0; c < tableColumnCount(); ++c) {
                row.append(cellAt(startRow + r, c));
            }
            block.appendRow(row);
        }
        return block;
    }
    bool isTableEditable() const override
    {
        return false;
    }

public:
    QVariant cellAt(std::size_t row, std::size_t col) const
    {
        if (col >= static_cast< std::size_t >(columns.size())) {
            return QVariant();
        }
        const Column& c = columns.at(static_cast< int >(col));
        return row < static_cast< std::size_t >(c.values.size()) ? c.values.at(static_cast< int >(row)) : QVariant();
    }
};

DA::DAData makeData(DAQuickPlotTestSource* source)
{
    return DA::DAData(DA::DAAbstractData::Pointer(source));
}

/// 数值列：0,1,2,…；第 5 个值为 NaN（无效 QVariant）
QVector< QVariant > makeNumeric(int rows, double offset = 0.0)
{
    QVector< QVariant > values;
    values.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        if (i == 5) {
            values.append(QVariant());  // 缺失值
        } else {
            values.append(offset + i * 1.0);
        }
    }
    return values;
}

/// 分类列：a/b/c 循环
QVector< QVariant > makeCategory(int rows)
{
    const char* names[] = { "a", "b", "c" };
    QVector< QVariant > values;
    values.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        values.append(QString::fromLatin1(names[i % 3]));
    }
    return values;
}

/// 低基数数值列：0/1 循环（可「作为分类」）
QVector< QVariant > makeLowCardinality(int rows)
{
    QVector< QVariant > values;
    values.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        values.append(i % 2);
    }
    return values;
}

QVector< QVariant > makeDateTime(int rows)
{
    const QDateTime base(QDate(2024, 1, 1), QTime(0, 0));
    QVector< QVariant > values;
    values.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        values.append(base.addDays(i));
    }
    return values;
}

DA::DATableChartRequest requestFromColumns(const DA::DAData& data, const QList< int >& dataColumns)
{
    DA::DATableChartRequest request;
    request.data     = data;
    request.rowCount = data.tableSource()->tableRowCount();
    for (int col : dataColumns) {
        request.columns.append(DA::DATableChartPlanner::makeColumn(data, col, 0));
    }
    return request;
}

bool containsType(const QList< DA::DATableChartType >& types, DA::DATableChartType type)
{
    return types.contains(type);
}

// 构造一个 pandas 背书的 DAData（走精确聚合路径）
DA::DAData makePandasData()
{
    pybind11::exec(R"(
import pandas as pd
_da_quick_plot_df = pd.DataFrame({
    'cat': ['a', 'b', 'a', 'c', 'b', 'a'],
    'val': [1.0, 2.0, 3.0, 4.0, 5.0, 6.0],
    'val2': [2.0, 4.0, 6.0, 8.0, 10.0, 12.0],
})
)");
    pybind11::object obj = pybind11::globals()["_da_quick_plot_df"];
    return DA::DAData(obj);
}
}  // namespace

/**
 * @brief 表内快速绘图纯逻辑测试
 *
 * 覆盖：列角色判定、能力矩阵（默认图型/可选图型）、分类聚合默认口径、
 * X 轴候选（含低基数数值「作为分类」）、采样策略（小表全量/大表确定性分块/
 * 缺失值剔除）与规格比较。
 */
class DATableQuickPlotTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testRoleDetection();
    void testPlanSingleNumeric();
    void testPlanSingleCategory();
    void testPlanSingleTimeRejected();
    void testPlanTwoNumeric();
    void testPlanCategoryAndNumeric();
    void testPlanTimeAndNumeric();
    void testPlanMultiNumeric();
    void testPlanThreeNumericNewTypes();
    void testPlanFourNumericVectorField();
    void testPlanCategoryTwoNumericMultiBar();
    void testPlanUnsupportedCombinations();
    void testXAxisCandidates();
    void testApplyXOptionAsCategory();
    void testSamplerSmallTableFull();
    void testSamplerLargeTableDeterministic();
    void testSamplerDropsMissing();
    // 方案 §8.4：取数预算耗尽时停止采样并标记「样本受限」
    void testSamplerBudgetStopsSampling();
    // 默认预算下正常采样不应被标记受限（避免误报）
    void testSamplerDefaultBudgetNotLimited();
    void testSpecEquality();
    void testRenderLineAndHistogram();
    void testRenderBarSampled();
    void testRenderBarExactWithTextCategory();
    void testRenderBoxplotExact();
    void testRenderContourErrorBarVectorField();
    void testRenderMultiBar();
    void testCreate3DLineItem();
    void testTransferNewTypes();
    void testWindowRecommendedSize();
    void testRolePanelManualMode();
    // 页面级端到端：真实选区 → 浮窗出图（Python 聚合）→ 转入绘图区 → 撤销/重做
    void testPageSelectPlotAndTransfer();
    // 方案 §5.3：删除当前列时自动关闭浮窗
    void testPageClosesOnColumnRemoval();
    // 方案 §5.3：切换数据集时自动关闭浮窗
    void testPageClosesOnDataSetChange();
    // 回归：从「popup + aboutToHide→deleteLater」的列头菜单触发快速绘图时，
    // 取数绘制必须延后到下一轮事件循环（不能在调用方的事件处理栈上同步跑）
    void testShowQuickPlotDefersRenderFromMenuAction();
    // 子窗口承载：快速绘图贴入 DATableChildWindow 框架，完整落在主视口内
    //（不压表头与滚动条），关闭重开保持上次几何，页面收缩自动夹回
    void testQuickPlotChildWindowFrame();
    // 操作窗口「无数据引导页」：空内容时显示、新建内容后隐藏、清空后恢复，入口发请求信号
    void testNoDataGuide();

    void initTestCase();
};

void DATableQuickPlotTest::testRoleDetection()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("num"), QMetaType::Double, makeNumeric(10) });
    source->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(10) });
    source->columns.append({ QStringLiteral("ts"), QMetaType::QDateTime, makeDateTime(10) });
    source->columns.append({ QStringLiteral("flag"), QMetaType::Bool, makeCategory(10) });
    const DA::DAData data = makeData(source);

    QCOMPARE(DA::DATableChartPlanner::detectRole(data, 0), DA::DATableColumnRole::Numeric);
    QCOMPARE(DA::DATableChartPlanner::detectRole(data, 1), DA::DATableColumnRole::Categorical);
    QCOMPARE(DA::DATableChartPlanner::detectRole(data, 2), DA::DATableColumnRole::DateTime);
    // 布尔按分类处理（实施方案 §4.1）
    QCOMPARE(DA::DATableChartPlanner::detectRole(data, 3), DA::DATableColumnRole::Categorical);
}

void DATableQuickPlotTest::testPlanSingleNumeric()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("num"), QMetaType::Double, makeNumeric(20) });
    const DA::DAData data    = makeData(source);
    const auto request       = requestFromColumns(data, { 0 });
    const DA::DATableChartPlan plan = DA::DATableChartPlanner::plan(request);

    QVERIFY(plan.isPlottable());
    QCOMPARE(plan.types.size(), 3);
    QCOMPARE(plan.types.first(), DA::DATableChartType::Line);
    QVERIFY(containsType(plan.types, DA::DATableChartType::Histogram));
    QVERIFY(containsType(plan.types, DA::DATableChartType::Boxplot));
    QCOMPARE(plan.defaultSpec.type, DA::DATableChartType::Line);
    QCOMPARE(plan.defaultSpec.xColumn, -1);  // 行号
    QCOMPARE(plan.defaultSpec.valueColumns, QList< int > { 0 });
}

void DATableQuickPlotTest::testPlanSingleCategory()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(30) });
    const DA::DAData data           = makeData(source);
    const DA::DATableChartPlan plan = DA::DATableChartPlanner::plan(requestFromColumns(data, { 0 }));

    QVERIFY(plan.isPlottable());
    QCOMPARE(plan.types, QList< DA::DATableChartType > { DA::DATableChartType::Bar });
    // 纯分类 → countplot（没有数值列）
    QCOMPARE(plan.defaultSpec.categoryColumn, 0);
    QVERIFY(plan.defaultSpec.valueColumns.isEmpty());
}

void DATableQuickPlotTest::testPlanSingleTimeRejected()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("ts"), QMetaType::QDateTime, makeDateTime(20) });
    const DA::DAData data           = makeData(source);
    const auto plan                 = DA::DATableChartPlanner::plan(requestFromColumns(data, { 0 }));
    QVERIFY(!plan.isPlottable());
    QCOMPARE(plan.reject, DA::DATableChartRejectReason::OnlyTimeColumn);
}

void DATableQuickPlotTest::testPlanTwoNumeric()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("x"), QMetaType::Double, makeNumeric(20) });
    source->columns.append({ QStringLiteral("y"), QMetaType::Double, makeNumeric(20, 100.0) });
    const DA::DAData data           = makeData(source);
    const auto request              = requestFromColumns(data, { 0, 1 });
    const DA::DATableChartPlan plan = DA::DATableChartPlanner::plan(request);

    QVERIFY(plan.isPlottable());
    QCOMPARE(plan.types.first(), DA::DATableChartType::Scatter);
    QCOMPARE(plan.defaultSpec.xColumn, 0);
    QCOMPARE(plan.defaultSpec.valueColumns, QList< int > { 1 });
    QVERIFY(containsType(plan.types, DA::DATableChartType::Line));
    QVERIFY(containsType(plan.types, DA::DATableChartType::Boxplot));
}

void DATableQuickPlotTest::testPlanCategoryAndNumeric()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(20) });
    source->columns.append({ QStringLiteral("val"), QMetaType::Double, makeNumeric(20) });
    const DA::DAData data           = makeData(source);
    const auto request              = requestFromColumns(data, { 0, 1 });
    const DA::DATableChartPlan plan = DA::DATableChartPlanner::plan(request);

    QVERIFY(plan.isPlottable());
    QCOMPARE(plan.types.first(), DA::DATableChartType::Bar);
    QVERIFY(containsType(plan.types, DA::DATableChartType::Boxplot));
    QCOMPARE(plan.defaultSpec.type, DA::DATableChartType::Bar);
    QCOMPARE(plan.defaultSpec.categoryColumn, 0);
    QCOMPARE(plan.defaultSpec.valueColumns, QList< int > { 1 });
    // 分类聚合默认口径 mean、按聚合值降序（D1/D6）
    QCOMPARE(plan.defaultSpec.aggregation, DA::DATableChartAggregation::Mean);
    QVERIFY(plan.defaultSpec.sortByValueDesc);
}

void DATableQuickPlotTest::testPlanTimeAndNumeric()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("ts"), QMetaType::QDateTime, makeDateTime(20) });
    source->columns.append({ QStringLiteral("val"), QMetaType::Double, makeNumeric(20) });
    const DA::DAData data           = makeData(source);
    const auto plan                 = DA::DATableChartPlanner::plan(requestFromColumns(data, { 0, 1 }));

    QVERIFY(plan.isPlottable());
    QCOMPARE(plan.types.first(), DA::DATableChartType::Line);
    QVERIFY(containsType(plan.types, DA::DATableChartType::Scatter));
    QVERIFY(!containsType(plan.types, DA::DATableChartType::Boxplot));  // 含时间列不出箱线图
    QCOMPARE(plan.defaultSpec.xColumn, 0);
    QCOMPARE(plan.defaultSpec.valueColumns, QList< int > { 1 });
}

void DATableQuickPlotTest::testPlanMultiNumeric()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("a"), QMetaType::Double, makeNumeric(20) });
    source->columns.append({ QStringLiteral("b"), QMetaType::Double, makeNumeric(20, 10.0) });
    source->columns.append({ QStringLiteral("c"), QMetaType::Double, makeNumeric(20, 20.0) });
    const DA::DAData data           = makeData(source);
    const auto plan                 = DA::DATableChartPlanner::plan(requestFromColumns(data, { 0, 1, 2 }));

    QVERIFY(plan.isPlottable());
    // v3 能力矩阵：3 列数值首项为散点（实施方案 §2.2）
    QCOMPARE(plan.types.first(), DA::DATableChartType::Scatter);
    QCOMPARE(plan.defaultSpec.xColumn, 0);
    QCOMPARE(plan.defaultSpec.valueColumns, QList< int > { 1 });
    QVERIFY(containsType(plan.types, DA::DATableChartType::MultiLine));
    QVERIFY(containsType(plan.types, DA::DATableChartType::Boxplot));
    QCOMPARE(plan.typeSpecs.size(), plan.types.size());
}

void DATableQuickPlotTest::testPlanThreeNumericNewTypes()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("x"), QMetaType::Double, makeNumeric(20) });
    source->columns.append({ QStringLiteral("y"), QMetaType::Double, makeNumeric(20, 10.0) });
    source->columns.append({ QStringLiteral("z"), QMetaType::Double, makeNumeric(20, 20.0) });
    const DA::DAData data           = makeData(source);
    const DA::DATableChartPlan plan = DA::DATableChartPlanner::plan(requestFromColumns(data, { 0, 1, 2 }));

    QVERIFY(plan.isPlottable());
    QVERIFY(containsType(plan.types, DA::DATableChartType::Contour));
    QVERIFY(containsType(plan.types, DA::DATableChartType::ErrorBar));
    QVERIFY(containsType(plan.types, DA::DATableChartType::Surface3D));
    QVERIFY(containsType(plan.types, DA::DATableChartType::Line3D));
    QVERIFY(!containsType(plan.types, DA::DATableChartType::VectorField));  // 四列才出向量场

    const DA::DATableChartSpec contour = DA::DATableChartPlanner::defaultSpec(
        requestFromColumns(data, { 0, 1, 2 }), DA::DATableChartType::Contour);
    QCOMPARE(contour.xColumn, 0);
    QCOMPARE(contour.yColumn, 1);
    QCOMPARE(contour.zColumn, 2);

    const DA::DATableChartSpec errorBar = DA::DATableChartPlanner::defaultSpec(
        requestFromColumns(data, { 0, 1, 2 }), DA::DATableChartType::ErrorBar);
    QCOMPARE(errorBar.errorColumn, 2);
}

void DATableQuickPlotTest::testPlanFourNumericVectorField()
{
    auto* source = new DAQuickPlotTestSource;
    for (int i = 0; i < 4; ++i) {
        source->columns.append({ QStringLiteral("n%1").arg(i), QMetaType::Double, makeNumeric(20, i * 10.0) });
    }
    const DA::DAData data           = makeData(source);
    const auto request              = requestFromColumns(data, { 0, 1, 2, 3 });
    const DA::DATableChartPlan plan = DA::DATableChartPlanner::plan(request);

    QVERIFY(plan.isPlottable());
    QVERIFY(containsType(plan.types, DA::DATableChartType::VectorField));
    const DA::DATableChartSpec spec =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::VectorField);
    QCOMPARE(spec.xColumn, 0);
    QCOMPARE(spec.yColumn, 1);
    QCOMPARE(spec.uColumn, 2);
    QCOMPARE(spec.vColumn, 3);
}

void DATableQuickPlotTest::testPlanCategoryTwoNumericMultiBar()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(20) });
    source->columns.append({ QStringLiteral("v1"), QMetaType::Double, makeNumeric(20) });
    source->columns.append({ QStringLiteral("v2"), QMetaType::Double, makeNumeric(20, 5.0) });
    const DA::DAData data           = makeData(source);
    const auto request              = requestFromColumns(data, { 0, 1, 2 });
    const DA::DATableChartPlan plan = DA::DATableChartPlanner::plan(request);

    QVERIFY(plan.isPlottable());
    QCOMPARE(plan.types.first(), DA::DATableChartType::MultiBar);
    QVERIFY(containsType(plan.types, DA::DATableChartType::Boxplot));
    QCOMPARE(plan.defaultSpec.categoryColumn, 0);
    QCOMPARE(plan.defaultSpec.valueColumns, QList< int >({ 1, 2 }));
}

void DATableQuickPlotTest::testPlanUnsupportedCombinations()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("c1"), QMetaType::QString, makeCategory(20) });
    source->columns.append({ QStringLiteral("c2"), QMetaType::QString, makeCategory(20) });
    const DA::DAData data = makeData(source);

    // 空选区
    DA::DATableChartRequest empty;
    empty.data          = data;
    const auto emptyPlan = DA::DATableChartPlanner::plan(empty);
    QVERIFY(!emptyPlan.isPlottable());
    QCOMPARE(emptyPlan.reject, DA::DATableChartRejectReason::EmptySelection);

    // 2 分类（v1 不做交叉柱状）
    const auto catPlan = DA::DATableChartPlanner::plan(requestFromColumns(data, { 0, 1 }));
    QVERIFY(!catPlan.isPlottable());
    QCOMPARE(catPlan.reject, DA::DATableChartRejectReason::UnsupportedCombination);

    // 分类 + 2 数值（v3 起可画多重柱状，不再是拒绝组合）
    auto* mixed = new DAQuickPlotTestSource;
    mixed->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(20) });
    mixed->columns.append({ QStringLiteral("v1"), QMetaType::Double, makeNumeric(20) });
    mixed->columns.append({ QStringLiteral("v2"), QMetaType::Double, makeNumeric(20, 5.0) });
    const DA::DAData mixedData = makeData(mixed);
    const auto mixedPlan       = DA::DATableChartPlanner::plan(requestFromColumns(mixedData, { 0, 1, 2 }));
    QVERIFY(mixedPlan.isPlottable());
    QVERIFY(containsType(mixedPlan.types, DA::DATableChartType::MultiBar));

    // 2 分类 + 1 数值：仍维持拒绝（统计管线 hue 能力缺交互与基数守卫，留后续）
    auto* twoCats = new DAQuickPlotTestSource;
    twoCats->columns.append({ QStringLiteral("c1"), QMetaType::QString, makeCategory(20) });
    twoCats->columns.append({ QStringLiteral("c2"), QMetaType::QString, makeCategory(20) });
    twoCats->columns.append({ QStringLiteral("v"), QMetaType::Double, makeNumeric(20) });
    const auto twoCatPlan = DA::DATableChartPlanner::plan(
        requestFromColumns(makeData(twoCats), { 0, 1, 2 }));
    QVERIFY(!twoCatPlan.isPlottable());
    QCOMPARE(twoCatPlan.reject, DA::DATableChartRejectReason::UnsupportedCombination);
}

void DATableQuickPlotTest::testXAxisCandidates()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("a"), QMetaType::Double, makeNumeric(20) });
    source->columns.append({ QStringLiteral("b"), QMetaType::Double, makeNumeric(20, 10.0) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1 });

    const auto options = DA::DATableChartPlanner::xAxisCandidates(request, DA::DATableChartType::Scatter);
    // 行号 + 两列数值
    QVERIFY(options.size() >= 3);
    QVERIFY(options.first().isRowIndex());

    // 1 列数值时不给 X 下拉
    const auto single = DA::DATableChartPlanner::xAxisCandidates(requestFromColumns(data, { 0 }),
                                                                 DA::DATableChartType::Line);
    QVERIFY(single.isEmpty());

    // 分类柱状不给 X 下拉（没有低基数数值列）
    auto* catSource = new DAQuickPlotTestSource;
    catSource->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(20) });
    catSource->columns.append({ QStringLiteral("val"), QMetaType::Double, makeNumeric(20) });
    const DA::DAData catData     = makeData(catSource);
    const auto catReq            = requestFromColumns(catData, { 0, 1 });
    const auto catOptions        = DA::DATableChartPlanner::xAxisCandidates(catReq, DA::DATableChartType::Bar);
    QVERIFY(catOptions.isEmpty());
}

void DATableQuickPlotTest::testApplyXOptionAsCategory()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("cat"), QMetaType::Double, makeLowCardinality(20) });
    source->columns.append({ QStringLiteral("val"), QMetaType::Double, makeNumeric(20) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1 });

    // 低基数数值列应出现「作为分类」候选，且规划里追加柱状图选项
    const auto plan = DA::DATableChartPlanner::plan(request);
    QVERIFY(containsType(plan.types, DA::DATableChartType::Bar));

    const auto options = DA::DATableChartPlanner::xAxisCandidates(request, DA::DATableChartType::Scatter);
    int asCategoryIndex = -1;
    for (int i = 0; i < options.size(); ++i) {
        if (options.at(i).asCategory) {
            asCategoryIndex = i;
            break;
        }
    }
    QVERIFY(asCategoryIndex >= 0);

    DA::DATableChartSpec spec = DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Scatter);
    DA::DATableChartPlanner::applyXOption(request, spec, options.at(asCategoryIndex));
    QCOMPARE(spec.type, DA::DATableChartType::Bar);
    QCOMPARE(spec.categoryColumn, options.at(asCategoryIndex).column);
    QVERIFY(!spec.valueColumns.contains(spec.categoryColumn));
}

void DATableQuickPlotTest::testSamplerSmallTableFull()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("x"), QMetaType::Double, makeNumeric(100) });
    source->columns.append({ QStringLiteral("y"), QMetaType::Double, makeNumeric(100, 50.0) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1 });

    const auto sample = DA::DAChartSeriesSampler::sampleXY(request, 0, 1, DA::DAChartSeriesSampler::kMaxLinePoints);
    QVERIFY(!sample.info.sampled);          // 小表全量
    QCOMPARE(sample.info.totalRows, 100);
    QCOMPARE(sample.points.size(), 99);     // 第 6 行 x 缺失被剔除
    QCOMPARE(sample.info.droppedMissing, 1);
    QCOMPARE(sample.points.first().x(), 0.0);
}

void DATableQuickPlotTest::testSamplerLargeTableDeterministic()
{
    constexpr int rows = 50000;
    auto* source       = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("v"), QMetaType::Double, makeNumeric(rows) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0 });

    const auto first  = DA::DAChartSeriesSampler::sampleXY(request, -1, 0, DA::DAChartSeriesSampler::kMaxLinePoints);
    const auto second = DA::DAChartSeriesSampler::sampleXY(request, -1, 0, DA::DAChartSeriesSampler::kMaxLinePoints);

    QVERIFY(first.info.sampled);
    QCOMPARE(first.info.totalRows, rows);
    // 采样点数受目标点数约束（确定性分块，不引入随机数）
    QVERIFY(first.points.size() <= DA::DAChartSeriesSampler::kMaxLinePoints);
    QVERIFY(first.points.size() > 0);
    // 两次采样完全一致 + 覆盖整个行区间
    QCOMPARE(first.points.size(), second.points.size());
    QCOMPARE(first.points.first().x(), second.points.first().x());
    QCOMPARE(first.points.last().x(), second.points.last().x());
    QVERIFY(first.points.first().x() < 100.0);
    QVERIFY(first.points.last().x() > rows - 2000.0);
}

void DATableQuickPlotTest::testSamplerDropsMissing()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("v"), QMetaType::Double, makeNumeric(50) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0 });

    DA::DAChartSeriesSampler::SampleInfo info;
    const QVector< double > values =
        DA::DAChartSeriesSampler::sampleValues(request, 0, DA::DAChartSeriesSampler::kMaxHistogramValues, &info);
    QCOMPARE(values.size(), 49);
    QCOMPARE(info.droppedMissing, 1);
    QVERIFY(!info.sampled);
}

void DATableQuickPlotTest::testSamplerBudgetStopsSampling()
{
    constexpr int rows = 50000;
    auto* source       = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("v"), QMetaType::Double, makeNumeric(rows) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0 });

    // 只给 2 次取数机会：第 3 个窗口取不了 → 停止采样并标记受限
    DA::DAChartSeriesSampler::SampleBudget budget;
    budget.maxFetchCalls = 2;
    const auto sample = DA::DAChartSeriesSampler::sampleXY(
        request, -1, 0, DA::DAChartSeriesSampler::kMaxLinePoints, &budget);

    QVERIFY(sample.info.limited);
    QVERIFY(sample.info.sampled);
    QCOMPARE(budget.fetchCalls, 2);
    QVERIFY(!sample.points.isEmpty());  // 已经取到的窗口仍然画出
    // 受限采样点数必然少于正常采样
    const auto full = DA::DAChartSeriesSampler::sampleXY(
        request, -1, 0, DA::DAChartSeriesSampler::kMaxLinePoints);
    QVERIFY(sample.points.size() < full.points.size());

    // 行数预算同样生效：一次窗口都不给
    DA::DAChartSeriesSampler::SampleBudget rowBudget;
    rowBudget.maxFetchRows = 10;
    const auto blocked     = DA::DAChartSeriesSampler::sampleXY(
        request, -1, 0, DA::DAChartSeriesSampler::kMaxLinePoints, &rowBudget);
    QVERIFY(blocked.info.limited);
    QVERIFY(blocked.points.isEmpty());

    // 直方图路径共用同一预算语义
    DA::DAChartSeriesSampler::SampleBudget valueBudget;
    valueBudget.maxFetchCalls = 1;
    DA::DAChartSeriesSampler::SampleInfo valueInfo;
    const QVector< double > values = DA::DAChartSeriesSampler::sampleValues(
        request, 0, DA::DAChartSeriesSampler::kMaxHistogramValues, &valueInfo, &valueBudget);
    QVERIFY(valueInfo.limited);
    QVERIFY(!values.isEmpty());
}

void DATableQuickPlotTest::testSamplerDefaultBudgetNotLimited()
{
    constexpr int rows = 50000;
    auto* source       = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("v"), QMetaType::Double, makeNumeric(rows) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0 });

    const auto sample = DA::DAChartSeriesSampler::sampleXY(
        request, -1, 0, DA::DAChartSeriesSampler::kMaxLinePoints);
    QVERIFY(!sample.info.limited);  // 正常采样不应被标成受限
    QCOMPARE(DA::DAChartSeriesSampler::SampleBudget().maxFetchCalls, 128);
}

void DATableQuickPlotTest::testSpecEquality()
{
    DA::DATableChartSpec a;
    a.type            = DA::DATableChartType::Bar;
    a.categoryColumn  = 0;
    a.valueColumns    = { 1 };
    a.aggregation     = DA::DATableChartAggregation::Mean;
    a.categoryTopN    = 30;
    DA::DATableChartSpec b = a;
    QVERIFY(a.equals(b));
    b.aggregation = DA::DATableChartAggregation::Max;
    QVERIFY(!a.equals(b));
}



namespace
{
// 选中 [firstCol, lastCol] 的整列范围（模拟用户选列）
bool selectColumnRange(DA::DADataOperateOfDataFrameWidget& page, int firstCol, int lastCol)
{
    DA::DADataTableView* view = page.getDataTableView();
    if (!view || !view->model()) {
        return false;
    }
    QAbstractItemModel* model = view->model();
    if (model->rowCount() <= 0 || lastCol >= model->columnCount()) {
        return false;
    }
    const QItemSelection sel(model->index(0, firstCol), model->index(model->rowCount() - 1, lastCol));
    page.tableSelectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
    return page.isDataframeTableHaveSelection();
}

// 浮窗改为「先出界面、下一轮事件循环再取数绘制」后，出图需要跑一下事件循环
bool waitForQuickPlotChart(DA::DATableQuickPlotWindow* window, int timeoutMs = 5000)
{
    if (!window) {
        return false;
    }
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (DA::DAChartWidget* chart = window->findChild< DA::DAChartWidget* >()) {
            if (!chart->itemList().isEmpty()) {
                return true;
            }
        }
        QTest::qWait(20);
    }
    return false;
}
}  // namespace

void DATableQuickPlotTest::testPageSelectPlotAndTransfer()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    DA::DATableMetadataRegistry registry(nullptr);
    const DA::DAData data = makePandasData();
    DA::DADataOperateOfDataFrameWidget page(data, &registry);
    page.resize(900, 520);
    page.show();
    QTest::qWait(50);

    // cat + val 两列（分类 + 数值）→ 分类柱状图，走 Python 精确聚合
    QVERIFY(selectColumnRange(page, 0, 1));
    const DA::DATableChartRequest request = page.buildQuickPlotRequest();
    QCOMPARE(request.columns.size(), 2);
    QCOMPARE(request.columns.at(0).role, DA::DATableColumnRole::Categorical);
    QCOMPARE(request.columns.at(1).role, DA::DATableColumnRole::Numeric);

    QVERIFY2(page.showQuickPlot(), qPrintable(page.isQuickPlotVisible() ? QString() : QStringLiteral("window not shown")));
    QVERIFY(page.isQuickPlotVisible());
    auto* window = page.findChild< DA::DATableQuickPlotWindow* >();
    QVERIFY(window);
    QCOMPARE(window->spec().type, DA::DATableChartType::Bar);
    QVERIFY2(waitForQuickPlotChart(window), "quick plot chart was not rendered");
    auto* chart = window->findChild< DA::DAChartWidget* >();
    QVERIFY(chart);
    QVERIFY(!chart->itemList().isEmpty());
    // 取数/构建结束后忙碌状态必须解除（浮窗恢复可交互）
    QVERIFY(window->isEnabled());

    // 转入绘图区：走页面注入的处理器（与 APP 注入的是同一条编排路径）
    DA::DAChartOperateWidget chartOperate;
    QString transferError;
    page.setQuickPlotTransferHandler([ &chartOperate, &transferError ](const DA::DATableChartRequest& req,
                                                                      const DA::DATableChartSpec& spec,
                                                                      bool append,
                                                                      QString* error) -> bool {
        const bool ok = DA::DAChartQuickPlot::transferToFigure(req, spec, &chartOperate, append, nullptr, error) != nullptr;
        if (!ok && error) {
            transferError = *error;
        }
        return ok;
    });
    window->transferRequested(window->request(), window->spec(), false);
    QVERIFY2(transferError.isEmpty(), qPrintable(transferError));
    QCOMPARE(chartOperate.getFigureCount(), 1);
    DA::DAFigureWidget* figure = chartOperate.getCurrentFigure();
    QVERIFY(figure);
    DA::DAChartWidget* formalChart = chartOperate.getCurrentChart();
    QVERIFY(formalChart);
    QVERIFY(!formalChart->itemList().isEmpty());
    // 转入成功后浮窗关闭
    QVERIFY(!window->isVisible());

    // 一次撤销 = 撤销整个转入（figure 摘 dock 但保活）
    QUndoStack* stack = figure->getUndoStack();
    QVERIFY(stack);
    stack->undo();
    QCOMPARE(chartOperate.getFigureList().size(), 0);
    stack->redo();
    QCOMPARE(chartOperate.getFigureCount(), 1);
    QVERIFY(chartOperate.getCurrentChart());
    QVERIFY(!chartOperate.getCurrentChart()->itemList().isEmpty());
}

void DATableQuickPlotTest::testPageClosesOnColumnRemoval()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    DA::DATableMetadataRegistry registry(nullptr);
    const DA::DAData data = makePandasData();
    DA::DADataOperateOfDataFrameWidget page(data, &registry);
    page.resize(900, 520);
    page.show();
    QTest::qWait(50);

    QVERIFY(selectColumnRange(page, 0, 1));
    QVERIFY(page.showQuickPlot());
    QVERIFY(page.isQuickPlotVisible());

    QVERIFY(page.removeColumnAt(0) > 0);
    QVERIFY(!page.isQuickPlotVisible());  // 删除当前列 → 浮窗自动关闭
}

void DATableQuickPlotTest::testPageClosesOnDataSetChange()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    DA::DATableMetadataRegistry registry(nullptr);
    const DA::DAData data = makePandasData();
    DA::DADataOperateOfDataFrameWidget page(data, &registry);
    page.resize(900, 520);
    page.show();
    QTest::qWait(50);

    QVERIFY(selectColumnRange(page, 0, 1));
    QVERIFY(page.showQuickPlot());
    QVERIFY(page.isQuickPlotVisible());

    // 切换到另一个数据集 → 浮窗自动关闭
    page.setDAData(makePandasData());
    QVERIFY(!page.isQuickPlotVisible());
}

void DATableQuickPlotTest::testShowQuickPlotDefersRenderFromMenuAction()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    DA::DATableMetadataRegistry registry(nullptr);
    const DA::DAData data = makePandasData();
    DA::DADataOperateOfDataFrameWidget page(data, &registry);
    page.resize(900, 520);
    page.show();
    QTest::qWait(50);
    QVERIFY(selectColumnRange(page, 0, 1));

    // 复刻 DADataHeaderView 的列头右键菜单：popup + aboutToHide→deleteLater，
    // 动作槽里调用页面的 showQuickPlot()。修复前 showQuickPlot 会在取数前执行
    // 「同步取数 + QCoreApplication::processEvents()」，既在调用方（QMenu）自己的
    // 事件处理栈上跑了重活，也会把已经排队删除的菜单当场销毁（真实工程里返回
    // QMenu::mouseReleaseEvent 时即 use-after-free 崩溃）。
    QMenu* menu = new QMenu(&page);
    QAction* actQuickPlot = menu->addAction(QStringLiteral("Quick Plot"));
    bool triggered        = false;
    connect(actQuickPlot, &QAction::triggered, &page, [ &page, &triggered ]() {
        triggered = true;
        page.showQuickPlot();
    });
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    QPointer< QMenu > menuGuard(menu);
    menu->popup(page.mapToGlobal(QPoint(40, 40)));
    QTest::qWait(50);
    QVERIFY(!menu->actions().isEmpty());

    // 用 sendEvent 派发鼠标事件，与事件循环派发走同一条 notifyInternal2 路径
    const QPoint clickPos = menu->actionGeometry(actQuickPlot).center();
    QMouseEvent press(QEvent::MouseButtonPress,
                      clickPos,
                      menu->mapToGlobal(clickPos),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(menu, &press);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        clickPos,
                        menu->mapToGlobal(clickPos),
                        Qt::LeftButton,
                        Qt::LeftButton,
                        Qt::NoModifier);
    QApplication::sendEvent(menu, &release);

    // 动作处理期间：浮窗已出来（带忙碌提示），但取数绘制还没开始——重活不能跑在
    // 调用方的事件处理栈上；同时调用方的菜单必须仍然存活
    QVERIFY(triggered);
    QVERIFY2(!menuGuard.isNull(), "caller menu was destroyed inside the action handler");
    QVERIFY(page.isQuickPlotVisible());
    auto* window = page.findChild< DA::DATableQuickPlotWindow* >();
    QVERIFY(window);
    auto* chart = window->findChild< DA::DAChartWidget* >();
    QVERIFY(chart);
    QVERIFY2(chart->itemList().isEmpty(),
             "quick plot must not fetch/render synchronously inside the caller's action handler");

    // 出图照常完成（重活已挪到下一轮事件循环）
    QVERIFY2(waitForQuickPlotChart(window), "quick plot chart was not rendered");
    QVERIFY(!chart->itemList().isEmpty());

    // 菜单最终仍应被 deleteLater 正常回收（只是不能在事件处理中途删）
    QTest::qWait(50);
    QVERIFY(menuGuard.isNull());
}

void DATableQuickPlotTest::testQuickPlotChildWindowFrame()
{
    if (!DA::DAPyScripts::isInitScripts()) {
        QSKIP("DAPyScripts not initialized");
    }
    DA::DATableMetadataRegistry registry(nullptr);
    const DA::DAData data = makePandasData();
    DA::DADataOperateOfDataFrameWidget page(data, &registry);
    page.resize(900, 520);
    page.show();
    QTest::qWait(50);

    QVERIFY(selectColumnRange(page, 0, 1));
    QVERIFY(page.showQuickPlot());
    QVERIFY(page.isQuickPlotVisible());

    // 内容部件被贴入 DATableChildWindow 框架（表格子窗口容器承载）
    auto* window = page.findChild< DA::DATableQuickPlotWindow* >();
    QVERIFY(window);
    auto* frame = page.findChild< DA::DATableChildWindow* >();
    QVERIFY(frame);
    QVERIFY(frame->isVisible());
    QCOMPARE(window->parentWidget(), frame);

    // 允许区域 = 主视口（页面坐标）：子窗口完整落在其中，永不压表头与滚动条
    QWidget* vp = page.getDataTableView()->viewport();
    QVERIFY(vp);
    const auto viewportRect = [ &page, vp ]() {
        return QRect(vp->mapTo(&page, QPoint(0, 0)), vp->size());
    };
    QVERIFY(viewportRect().contains(frame->geometry()));

    // 关闭 → 重开：保持用户上次几何（贴入后位置不再重置）
    const QRect geometryBefore = frame->geometry();
    page.closeQuickPlot();
    QVERIFY(!page.isQuickPlotVisible());
    QVERIFY(selectColumnRange(page, 1, 1));
    QVERIFY(page.showQuickPlot());
    QVERIFY(page.isQuickPlotVisible());
    QCOMPARE(frame->geometry(), geometryBefore);

    // 页面收缩：容器把子窗口夹回新允许区域（仍完整在主视口内）
    page.resize(520, 460);
    QTest::qWait(30);
    QVERIFY(viewportRect().contains(frame->geometry()));
}

void DATableQuickPlotTest::initTestCase()
{
    // 与 APP main.cpp / DATableDataTest 一致的 Python 初始化：
    // 解析解释器路径设置 Python Home，再把 sys.path 指向仓库内的 PyScripts，
    // 并顶掉测试环境缺失的 da_app/da_interface/da_data 绑定模块
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

void DATableQuickPlotTest::testRenderLineAndHistogram()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("num"), QMetaType::Double, makeNumeric(200) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0 });

    DA::DAChartWidget chart;
    DA::DATableChartBuildInfo info;
    QString error;
    DA::DATableChartSpec lineSpec = DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Line);
    QVERIFY2(DA::DAChartQuickPlot::render(request, lineSpec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());

    DA::DATableChartSpec histSpec = DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Histogram);
    QVERIFY2(DA::DAChartQuickPlot::render(request, histSpec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
}

void DATableQuickPlotTest::testRenderBarSampled()
{
    // 非 pandas 源 → 采样 + 列表聚合路径
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(300) });
    source->columns.append({ QStringLiteral("val"), QMetaType::Double, makeNumeric(300) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1 });

    DA::DAChartWidget chart;
    DA::DATableChartBuildInfo info;
    QString error;
    const DA::DATableChartSpec spec = DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Bar);
    QVERIFY2(DA::DAChartQuickPlot::render(request, spec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
    QCOMPARE(info.totalCategories, 3);
    QVERIFY(info.approximate);
}

void DATableQuickPlotTest::testRenderBarExactWithTextCategory()
{
    // pandas 源 + 文本分类列：精确聚合路径（不新造临时 DataFrame）
    const DA::DAData data = makePandasData();
    const auto request    = requestFromColumns(data, { 0, 1 });

    DA::DAChartWidget chart;
    DA::DATableChartBuildInfo info;
    QString error;
    const DA::DATableChartSpec spec = DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Bar);
    QVERIFY2(DA::DAChartQuickPlot::render(request, spec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
    QCOMPARE(info.totalCategories, 3);   // a/b/c
    QVERIFY(!info.approximate);          // 精确聚合
    // 按聚合值降序：a(mean=10/3) < b(3.5) < c(4.0) → c 最高
    QCOMPARE(info.displayedCategories, 3);
}

void DATableQuickPlotTest::testRenderBoxplotExact()
{
    const DA::DAData data = makePandasData();
    const auto request    = requestFromColumns(data, { 1, 2 });

    DA::DAChartWidget chart;
    DA::DATableChartBuildInfo info;
    QString error;
    const DA::DATableChartSpec spec = DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Boxplot);
    QVERIFY2(DA::DAChartQuickPlot::render(request, spec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
    QVERIFY(!info.approximate);

    // 分类 + 数值的按类别箱线图（hue）同样走精确路径
    const auto hueRequest = requestFromColumns(data, { 0, 1 });
    const DA::DATableChartSpec hueSpec =
        DA::DATableChartPlanner::defaultSpec(hueRequest, DA::DATableChartType::Boxplot);
    QVERIFY2(DA::DAChartQuickPlot::render(hueRequest, hueSpec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
}

void DATableQuickPlotTest::testRenderContourErrorBarVectorField()
{
    auto* source = new DAQuickPlotTestSource;
    for (int i = 0; i < 4; ++i) {
        source->columns.append({ QStringLiteral("n%1").arg(i), QMetaType::Double, makeNumeric(80, i * 3.0) });
    }
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1, 2, 3 });

    DA::DAChartWidget chart;
    DA::DATableChartBuildInfo info;
    QString error;

    const DA::DATableChartSpec contour =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Contour);
    QVERIFY2(DA::DAChartQuickPlot::render(request, contour, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());

    const DA::DATableChartSpec errorBar =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::ErrorBar);
    QVERIFY2(DA::DAChartQuickPlot::render(request, errorBar, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());

    const DA::DATableChartSpec vectorField =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::VectorField);
    QVERIFY2(DA::DAChartQuickPlot::render(request, vectorField, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
}

void DATableQuickPlotTest::testRenderMultiBar()
{
    const DA::DAData data = makePandasData();
    const auto request    = requestFromColumns(data, { 0, 1, 2 });

    DA::DAChartWidget chart;
    DA::DATableChartBuildInfo info;
    QString error;
    const DA::DATableChartSpec spec =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::MultiBar);
    QVERIFY2(DA::DAChartQuickPlot::render(request, spec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
    QCOMPARE(info.totalCategories, 3);  // a/b/c
    QVERIFY(!info.approximate);         // pandas 源走精确聚合

    // 分类 + 2 数值的箱线图：默认按类别分组（精确与采样两条路径）
    const DA::DATableChartSpec boxSpec =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Boxplot);
    QCOMPARE(boxSpec.categoryColumn, 0);
    QCOMPARE(boxSpec.valueColumns, QList< int >({ 1, 2 }));
    QVERIFY2(DA::DAChartQuickPlot::render(request, boxSpec, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());

    auto* sampledSource = new DAQuickPlotTestSource;
    sampledSource->columns.append({ QStringLiteral("cat"), QMetaType::QString, makeCategory(300) });
    sampledSource->columns.append({ QStringLiteral("v1"), QMetaType::Double, makeNumeric(300) });
    sampledSource->columns.append({ QStringLiteral("v2"), QMetaType::Double, makeNumeric(300, 5.0) });
    const DA::DAData sampledData = makeData(sampledSource);
    const auto sampledRequest    = requestFromColumns(sampledData, { 0, 1, 2 });
    const DA::DATableChartSpec sampledBox =
        DA::DATableChartPlanner::defaultSpec(sampledRequest, DA::DATableChartType::Boxplot);
    QVERIFY2(DA::DAChartQuickPlot::render(sampledRequest, sampledBox, &chart, &info, &error), qPrintable(error));
    QVERIFY(!chart.itemList().isEmpty());
    QVERIFY(info.approximate);  // 非 pandas 源走采样
}

void DATableQuickPlotTest::testCreate3DLineItem()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("x"), QMetaType::Double, makeNumeric(40) });
    source->columns.append({ QStringLiteral("y"), QMetaType::Double, makeNumeric(40, 5.0) });
    source->columns.append({ QStringLiteral("z"), QMetaType::Double, makeNumeric(40, 9.0) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1, 2 });

    DA::DATableChartBuildInfo info;
    QString error;
    const DA::DATableChartSpec spec =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Line3D);
    QVERIFY(DA::DAChartQuickPlot::is3DType(spec.type));
    Qwt3DPlotItem* item = DA::DAChartQuickPlot::create3DItem(request, spec, &info, &error);
    QVERIFY2(item != nullptr, qPrintable(error));
    delete item;
}

void DATableQuickPlotTest::testTransferNewTypes()
{
    auto* source = new DAQuickPlotTestSource;
    for (int i = 0; i < 3; ++i) {
        source->columns.append({ QStringLiteral("n%1").arg(i), QMetaType::Double, makeNumeric(40, i * 2.0) });
    }
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1, 2 });

    // 2D 新图型（多重柱状需要分类列，这里用等高线验证新渲染分支的转入）
    DA::DAChartOperateWidget chartOperate2D;
    QString error;
    const DA::DATableChartSpec contour =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Contour);
    DA::DAChartWidget* chart = DA::DAChartQuickPlot::transferToFigure(
        request, contour, &chartOperate2D, false, nullptr, &error);
    QVERIFY2(chart != nullptr, qPrintable(error));
    QVERIFY(!chart->itemList().isEmpty());

    // 3D 线图转入：新建 Figure + 3D 图表 + 图元
    // 3D GL 图表在测试进程内析构容易卡住（无事件循环的 GL 上下文销毁），
    // 测试进程最后走 TerminateProcess，这里故意不回收该容器
    auto* chartOperate3D = new DA::DAChartOperateWidget;
    const DA::DATableChartSpec line3D =
        DA::DATableChartPlanner::defaultSpec(request, DA::DATableChartType::Line3D);
    QVERIFY2(DA::DAChartQuickPlot::transferToFigure3D(
                 request, line3D, chartOperate3D, false, nullptr, &error),
             qPrintable(error));
    QCOMPARE(chartOperate3D->getFigureCount(), 1);
    DA::DAFigureWidget* figure = chartOperate3D->getCurrentFigure();
    QVERIFY(figure != nullptr);
    DA::DAChart3DWidget* chart3d = figure->getCurrent3DChart();
    QVERIFY(chart3d != nullptr);
    QCOMPARE(chart3d->itemList().size(), 1);
}

void DATableQuickPlotTest::testWindowRecommendedSize()
{
    QCOMPARE(DA::DATableQuickPlotWindow::recommendedSize(), QSize(640, 560));
}

void DATableQuickPlotTest::testRolePanelManualMode()
{
    auto* source = new DAQuickPlotTestSource;
    source->columns.append({ QStringLiteral("a"), QMetaType::Double, makeNumeric(30) });
    source->columns.append({ QStringLiteral("b"), QMetaType::Double, makeNumeric(30, 7.0) });
    const DA::DAData data = makeData(source);
    const auto request    = requestFromColumns(data, { 0, 1 });

    DA::DATableQuickPlotWindow window(nullptr);
    window.showForRequest(request);
    QTest::qWait(50);
    QVERIFY(window.isVisible());
    QVERIFY(!window.isManualMode());

    auto* panel = window.findChild< DA::DATableChartRolePanel* >();
    QVERIFY(panel != nullptr);
    const QList< QComboBox* > combos = panel->findChildren< QComboBox* >();
    QVERIFY(combos.size() >= 2);  // 散点：X / Y
    QComboBox* xCombo = combos.first();
    const int target  = xCombo->findData(1);
    QVERIFY(target >= 0);
    xCombo->setCurrentIndex(target);
    QTest::qWait(50);

    QVERIFY(window.isManualMode());
    QCOMPARE(window.spec().xColumn, 1);

    // 「更新为当前选区」回归选区跟随，撤销手动模式
    window.updateRequest(request);
    QTest::qWait(30);
    QVERIFY(!window.isManualMode());
    window.close();
}

/**
 * @brief 操作窗口「无数据引导页」回归
 *
 * 数据/绘图操作窗口在没有任何内容时显示引导页（尚未载入数据 + 打开已有工程/导入数据），
 * 新建内容后隐藏、清空后恢复；引导页两个入口分别发出请求信号，由 APP 层接到主程序动作。
 */
void DATableQuickPlotTest::testNoDataGuide()
{
    // 数据操作窗口：没有任何数据页时显示引导页
    DA::DADataOperateWidget dataOperate;
    QVERIFY(dataOperate.isNoDataGuideVisible());

    // 绘图操作窗口：空时显示；新建绘图后隐藏；关掉最后一个绘图后恢复
    DA::DAChartOperateWidget chartOperate;
    QVERIFY(chartOperate.isNoDataGuideVisible());
    DA::DAFigureWidget* fig = chartOperate.createFigure();
    QVERIFY(nullptr != fig);
    QVERIFY(!chartOperate.isNoDataGuideVisible());
    chartOperate.removeFigure(fig);
    QVERIFY(chartOperate.isNoDataGuideVisible());

    // 引导页入口信号：两个按钮分别发出「打开已有工程」「导入数据」请求
    DA::DANoDataGuideWidget guide(DA::DANoDataGuideWidget::ChartOperateRole);
    const QList< QPushButton* > buttons = guide.findChildren< QPushButton* >();
    QCOMPARE(buttons.size(), 2);
    QSignalSpy openSpy(&guide, &DA::DANoDataGuideWidget::openProjectRequested);
    QSignalSpy importSpy(&guide, &DA::DANoDataGuideWidget::importDataRequested);
    for (QPushButton* button : buttons) {
        if (button->text() == QStringLiteral("Open Existing Project")) {
            button->click();
        } else if (button->text() == QStringLiteral("Import Data")) {
            button->click();
        }
    }
    QCOMPARE(openSpy.count(), 1);
    QCOMPARE(importSpy.count(), 1);
}

/**
 * @brief 测试入口
 *
 * 本测试会初始化内嵌 Python 并创建 pandas DataFrame 走精确聚合路径。进程退出时
 * 解释器析构在测试环境下会挂住（与 DADataFrameTest 的退出阶段问题同源），因此
 * 在 QtTest 报告输出完成后直接 _Exit，跳过静态析构，保证 ctest 正常结束。
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DATableQuickPlotTest tc;
    const int rc = QTest::qExec(&tc, argc, argv);
    std::fflush(stdout);
    std::fflush(stderr);
#ifdef Q_OS_WIN
    // ExitProcess 会依次执行各 DLL 的 DllMain(DLL_PROCESS_DETACH)，Python 的清理在
    // 测试环境会死锁；TerminateProcess 跳过 detach，直接结束进程
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(rc));
#else
    std::_Exit(rc);
#endif
    return rc;  // 不可达
}
#include "main.moc"
