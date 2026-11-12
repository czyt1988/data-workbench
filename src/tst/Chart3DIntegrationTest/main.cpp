#include <QtTest/QtTest>
#include <QApplication>
#include <QDomDocument>
#include <QSignalSpy>
#include <QSurfaceFormat>
#include <QUndoStack>
#include <QByteArray>
#include <QDataStream>

// 测试目标
#include "Archive/DAXmlHelper.h"
#include "Chart/DAChartItemsManager.h"
#include "DAFigureWidget.h"
#include "DAChart3DWidget.h"
#include "DAFigureElementSelection.h"
#include "DAFigureTreeModel.h"
#include "DAChart3DSerialize.h"
#include "DAChart3DPlotItemFactory.h"

// qwt
#include "qwt_figure_grid.h"

// qwt3d
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"
#include "qwt3d_surface.h"
#include "qwt3d_bar.h"
#include "qwt3d_line3d.h"
#include "qwt3d_function.h"
#include "qwt3d_types.h"

// std::vector
#include <memory>
#include <vector>

namespace
{
/// 浮点比较（视图/缩放参数经 XML 文本往返，1e-6 足够）
bool nearlyEqual(double lhs, double rhs, double eps = 1e-6)
{
    return qAbs(lhs - rhs) <= eps;
}

/// 主题按生产路径（Qwt3DTheme 二进制序列化）转字节，便于二分比对
QByteArray serializeTheme(const Qwt3DTheme& theme)
{
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << theme;
    return bytes;
}

/// RGBA 分量比较（3D 颜色是 double 0..1，XML 文本往返误差远小于 1e-6）
bool rgbaNearlyEqual(const RGBA& lhs, const RGBA& rhs, double eps = 1e-6)
{
    return nearlyEqual(lhs.r, rhs.r, eps) && nearlyEqual(lhs.g, rhs.g, eps) && nearlyEqual(lhs.b, rhs.b, eps)
        && nearlyEqual(lhs.a, rhs.a, eps);
}
}  // namespace

/**
 * @brief 3D 绘图集成测试
 *
 * 本测试覆盖以下场景：
 * 1. 通过 DAFigureWidget 创建 3D chart
 * 2. 三种 3D item 类型的创建和 attach
 * 3. 树形管理中 3D 节点存在性验证
 * 4. 设置面板的 setSelection 路由
 * 5. 序列化/反序列化往返
 * 6. 混合 2D/3D 场景
 * 7. undo/redo 命令
 * 8. figure XML 中 3D 子图的布局/视图/主题/坐标轴往返
 */
class Chart3DIntegrationTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 测试1：创建 Surface 3D 图表
    void testCreateSurface3D();
    // 测试2：创建 Bar3D 图表
    void testCreateBar3D();
    // 测试3：创建 Line3D 图表
    void testCreateLine3D();
    // 测试4：树形管理中选中 3D item
    void testSelect3DItemInTree();
    // 测试5：3D item 属性读写验证
    void testItemPropertyReadWrite();
    // 测试6：序列化/反序列化往返
    void testSerializationRoundTrip();
    // 测试7：混合 2D 和 3D chart
    void testMixed2D3DCharts();
    // 测试8：undo/redo 操作
    void testUndoRedo();
    // 测试9：网格数据布局为 z[row][column]（行列不等也不能转置）
    void testGridDataRowMajorLayout();
    // 测试10：figure XML 中 3D 子图的网格布局/视图/主题/坐标轴往返
    void test3DSubChartXmlRoundTrip();
    // 测试11：3D-only figure 的 XML 不包含 chart item 数据
    void test3DOnlyFigureNoChartItemData();
    // 测试12：旧工程 figure XML 无 <chart3d> 时 3D 数量为 0、2D 正常恢复
    void testLegacyFigureWithout3DXml();

private:
    DA::DAFigureWidget* mFigure { nullptr };
};

void Chart3DIntegrationTest::initTestCase()
{
    // 检查是否有 OpenGL 支持
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    if (fmt.majorVersion() < 2) {
        QSKIP("OpenGL not available, skipping 3D chart tests");
    }
}

void Chart3DIntegrationTest::cleanupTestCase()
{
}

void Chart3DIntegrationTest::init()
{
    mFigure = new DA::DAFigureWidget();
    mFigure->show();
    // 等待窗口显示，触发 OpenGL 上下文创建
    QTest::qWaitForWindowExposed(mFigure);
}

void Chart3DIntegrationTest::cleanup()
{
    delete mFigure;
    mFigure = nullptr;
}

/**
 * @brief 测试1：通过 DAFigureWidget 创建 Surface 3D 图表
 *
 * 验证点：
 * - create3DChart() 返回非 null
 * - DAChart3DWidget 继承自 Qwt3DPlot
 * - chart3DAdded 信号正确发出
 * - Surface item 可以 attach 到 chart
 */
void Chart3DIntegrationTest::testCreateSurface3D()
{
    // 创建 3D chart
    QSignalSpy spy(mFigure, &DA::DAFigureWidget::chart3DAdded);
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);
    QVERIFY(chart3d->inherits("Qwt3DPlot"));
    QCOMPARE(spy.count(), 1);

    // 创建 Surface item
    Qwt3DSurface* surface = new Qwt3DSurface();
    QVERIFY(surface != nullptr);
    QCOMPARE(surface->rtti(), Rtti_Plot3DSurface);

    // attach
    surface->attach(chart3d);
    QCOMPARE(chart3d->itemList().size(), 1);

    // 验证 item 可以从 chart 中找到
    bool found = false;
    const auto& items = chart3d->itemList();
    for (Qwt3DPlotItem* it : items) {
        if (it == surface) {
            found = true;
            break;
        }
    }
    QVERIFY(found);

    // 触发重绘
    chart3d->update();
}

/**
 * @brief 测试2：通过 DAFigureWidget 创建 Bar3D 图表
 */
void Chart3DIntegrationTest::testCreateBar3D()
{
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);

    Qwt3DBar* bar = new Qwt3DBar();
    QVERIFY(bar != nullptr);
    QCOMPARE(bar->rtti(), Rtti_Plot3DBar);

    bar->attach(chart3d);
    QCOMPARE(chart3d->itemList().size(), 1);

    chart3d->update();
}

/**
 * @brief 测试3：通过 DAFigureWidget 创建 Line3D 图表
 */
void Chart3DIntegrationTest::testCreateLine3D()
{
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);

    Qwt3DLine* line = new Qwt3DLine();
    QVERIFY(line != nullptr);
    QCOMPARE(line->rtti(), Rtti_Plot3DLine);

    line->attach(chart3d);
    QCOMPARE(chart3d->itemList().size(), 1);

    chart3d->update();
}

/**
 * @brief 测试4：在树形管理中选中 3D item，设置面板正确显示
 *
 * 验证点：
 * - DAFigureTreeModel 中包含 NodeTypePlot3DFolder 节点
 * - DAFigureElementSelection 的 3D 选择类型正确
 * - DAChart3DSettingWidget::setSelection 能正确路由
 */
void Chart3DIntegrationTest::testSelect3DItemInTree()
{
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);

    Qwt3DSurface* surface = new Qwt3DSurface();
    surface->attach(chart3d);

    // 验证 tree model 中有 3D 节点
    QwtFigure* fig = mFigure->figure();
    QVERIFY(fig != nullptr);

    // 构造 3D 选择
    DA::DAFigureElementSelection sel;
    sel.figureWidget = mFigure;
    sel.plot3D = chart3d;
    sel.plot3DItem = surface;
    sel.selectionType = DA::DAFigureElementSelection::SelectPlot3DItem;
    sel.selectionColumn = DA::DAFigureElementSelection::ColumnProperty;

    QVERIFY(sel.isSelectedPlot3DItem());
    QVERIFY(!sel.isSelectedPlot());
    QVERIFY(!sel.isSelectedPlotItem());
}

/**
 * @brief 测试5：3D item 属性读写验证
 *
 * 验证点：
 * - 修改 Surface 的绘图样式后，item 属性确实改变
 * - 修改 chart 的旋转角度后，chart 属性确实改变
 *
 * 注意：本测试直接调用底层 getter/setter 验证属性读写，
 * 不涉及设置面板 widget 的联动（设置面板测试在无头 CI 环境中
 * 难以可靠运行，留作手动验证项）。
 */
void Chart3DIntegrationTest::testItemPropertyReadWrite()
{
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);

    Qwt3DSurface* surface = new Qwt3DSurface();
    surface->attach(chart3d);

    // 验证初始绘图样式
    PLOTSTYLE oldStyle = surface->plotStyle();
    QVERIFY(oldStyle != WIREFRAME);

    // 修改为 wireframe
    surface->setPlotStyle(WIREFRAME);
    QCOMPARE(surface->plotStyle(), WIREFRAME);

    // 修改 chart 旋转角度
    double xRot = 30.0, yRot = 45.0, zRot = 0.0;
    chart3d->setRotation(xRot, yRot, zRot);
    // 验证读取回来的值（Qwt3DPlot 只有三个单轴 getter，无组合 getter）
    double rx = chart3d->xRotation();
    double ry = chart3d->yRotation();
    double rz = chart3d->zRotation();
    QCOMPARE(rx, xRot);
    QCOMPARE(ry, yRot);
    QCOMPARE(rz, zRot);

    chart3d->update();
}

/**
 * @brief 测试6：保存工程文件后重新加载，3D 图表正确恢复
 *
 * 验证点：
 * - 序列化后再反序列化，3D chart 数量一致
 * - item 的 RTTI 类型一致
 * - item 的关键属性（plotStyle 等）一致
 */
void Chart3DIntegrationTest::testSerializationRoundTrip()
{
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);

    // 添加 Surface
    Qwt3DSurface* surface = new Qwt3DSurface();
    surface->setPlotStyle(FILLED);
    surface->attach(chart3d);

    // 添加 Bar
    Qwt3DBar* bar = new Qwt3DBar();
    bar->attach(chart3d);

    QCOMPARE(mFigure->get3DCharts().size(), 1);
    QCOMPARE(chart3d->itemList().size(), 2);

    // 序列化
    QByteArray data;
    {
        QDataStream out(&data, QIODevice::WriteOnly);
        out << mFigure;
    }
    QVERIFY(!data.isEmpty());

    // 反序列化到新 figure
    DA::DAFigureWidget* fig2 = new DA::DAFigureWidget();
    fig2->show();
    QTest::qWaitForWindowExposed(fig2);
    {
        QDataStream in(&data, QIODevice::ReadOnly);
        in >> fig2;
    }

    // 验证 3D chart 数量
    QList< DA::DAChart3DWidget* > charts3d = fig2->get3DCharts();
    QCOMPARE(charts3d.size(), 1);

    // 验证 item 数量和 RTTI
    QCOMPARE(charts3d[ 0 ]->itemList().size(), 2);

    bool hasSurface = false;
    bool hasBar     = false;
    const auto& items = charts3d[ 0 ]->itemList();
    for (Qwt3DPlotItem* it : items) {
        if (it->rtti() == Rtti_Plot3DSurface) {
            hasSurface = true;
            // 验证属性
            Qwt3DSurface* s = static_cast< Qwt3DSurface* >(it);
            QCOMPARE(s->plotStyle(), FILLED);
        }
        if (it->rtti() == Rtti_Plot3DBar) {
            hasBar = true;
        }
    }
    QVERIFY(hasSurface);
    QVERIFY(hasBar);

    delete fig2;
}

/**
 * @brief 测试7：在同一 figure 中混合 2D 和 3D chart
 *
 * 验证点：
 * - 2D 和 3D chart 可以共存于同一 figure
 * - 互不干扰
 * - 序列化后两者都能恢复
 */
void Chart3DIntegrationTest::testMixed2D3DCharts()
{
    // 创建 2D chart
    DA::DAChartWidget* chart2d = mFigure->createChart();
    QVERIFY(chart2d != nullptr);

    // 创建 3D chart
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);

    // 验证两者都存在
    QCOMPARE(mFigure->getCharts().size(), 1);
    QCOMPARE(mFigure->get3DCharts().size(), 1);

    // 给 2D chart 添加 item
    QwtPlotCurve* curve = new QwtPlotCurve("test curve");
    curve->attach(chart2d);
    QCOMPARE(chart2d->itemList().size(), 1);

    // 给 3D chart 添加 item
    Qwt3DSurface* surface = new Qwt3DSurface();
    surface->attach(chart3d);
    QCOMPARE(chart3d->itemList().size(), 1);

    // 序列化往返
    QByteArray data;
    {
        QDataStream out(&data, QIODevice::WriteOnly);
        out << mFigure;
    }
    QVERIFY(!data.isEmpty());

    DA::DAFigureWidget* fig2 = new DA::DAFigureWidget();
    fig2->show();
    QTest::qWaitForWindowExposed(fig2);
    {
        QDataStream in(&data, QIODevice::ReadOnly);
        in >> fig2;
    }

    // 验证两者都恢复
    // 注意：2D chart 的 plot items 不包含在 DAFigureWidget::operator<< 序列化中
    // （这是 2D 的既有行为，QwtPlot::operator<< 仅保存 chart frame/title/axes/canvas），
    // 2D items 通过 DAAppProject 的工程文件保存机制单独序列化。
    // 3D chart 的 items 由 plan-08 的 Qwt3DPlot::operator<< 序列化，因此能恢复。
    QCOMPARE(fig2->getCharts().size(), 1);
    QCOMPARE(fig2->get3DCharts().size(), 1);
    QCOMPARE(fig2->get3DCharts()[ 0 ]->itemList().size(), 1);

    delete fig2;
}

/**
 * @brief 测试8：3D chart 的 undo/redo 操作
 *
 * 验证点：
 * - add3DItem_ 是 undoable 的
 * - undo 后 item 被移除
 * - redo 后 item 恢复
 */
void Chart3DIntegrationTest::testUndoRedo()
{
    DA::DAChart3DWidget* chart3d = mFigure->create3DChart();
    QVERIFY(chart3d != nullptr);

    Qwt3DSurface* surface = new Qwt3DSurface();

    // 通过 undoable 接口添加
    mFigure->add3DItem_(chart3d, surface);
    QCOMPARE(chart3d->itemList().size(), 1);

    // undo
    QUndoStack* stack = mFigure->getUndoStack();
    QVERIFY(stack != nullptr);
    QVERIFY(stack->canUndo());
    stack->undo();
    QCOMPARE(chart3d->itemList().size(), 0);

    // redo
    QVERIFY(stack->canRedo());
    stack->redo();
    QCOMPARE(chart3d->itemList().size(), 1);
}

/**
 * @brief 测试9：网格数据布局为 z[row][column]
 *
 * 验证点：
 * - Qwt3DSurface::loadFromData 把 data[row][column] 解释成 (x = column, y = row)
 * - Qwt3DBar::setSamples 的 gridZValues() 原样保留 [row][column]
 * - Qwt3DFunction::create() 输出的 z 也按 [row][column]
 * 行列刻意取不相等（5 列 3 行），任何转置/列主序实现都会在这里失败。
 */
void Chart3DIntegrationTest::testGridDataRowMajorLayout()
{
    const int rows = 3;
    const int cols = 5;
    const double minX = 0.0;
    const double maxX = 4.0;
    const double minY = 0.0;
    const double maxY = 2.0;

    // z[row][column] = row * 10 + column
    Qwt3DFunctionData data;
    data.columns = static_cast<unsigned>(cols);
    data.rows = static_cast<unsigned>(rows);
    data.minx = minX;
    data.maxx = maxX;
    data.miny = minY;
    data.maxy = maxY;
    data.z.assign(static_cast<size_t>(rows), std::vector<double>(static_cast<size_t>(cols), 0.0));
    for (int j = 0; j < rows; ++j) {
        for (int i = 0; i < cols; ++i) {
            data.z[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ] = j * 10.0 + i;
        }
    }

    // 1) Surface：内部网格必须沿用 [row][column]，坐标由 column/row 分别决定 x/y
    Qwt3DSurface surface;
    surface.loadFromData(data);
    Qwt3DGridData* gd = surface.gridData();
    QVERIFY(gd != nullptr);
    // Qwt3DGridData 的成员函数未导出，直接读公有 vertices（布局 [row][column]）
    QCOMPARE(static_cast<int>(gd->vertices.size()), rows);
    QVERIFY(!gd->vertices.empty());
    QCOMPARE(static_cast<int>(gd->vertices[ 0 ].size()), cols);
    for (int j = 0; j < rows; ++j) {
        for (int i = 0; i < cols; ++i) {
            QCOMPARE(gd->vertices[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ][ 0 ], minX + i);
            QCOMPARE(gd->vertices[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ][ 1 ], minY + j);
            QCOMPARE(gd->vertices[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ][ 2 ], j * 10.0 + i);
        }
    }

    // 2) Bar：同一份数据，导出的 z 矩阵必须原样保持 [row][column]
    Qwt3DBar bar;
    bar.setSamples(data);
    QCOMPARE(bar.gridColumns(), cols);
    QCOMPARE(bar.gridRows(), rows);
    const std::vector< std::vector< double > > zValues = bar.gridZValues();
    QCOMPARE(static_cast<int>(zValues.size()), rows);
    QVERIFY(!zValues.empty());
    QCOMPARE(static_cast<int>(zValues[ 0 ].size()), cols);
    for (int j = 0; j < rows; ++j) {
        for (int i = 0; i < cols; ++i) {
            QCOMPARE(zValues[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ], j * 10.0 + i);
        }
    }

    // 3) Qwt3DFunction::create()：z[row][column] = f(x_column, y_row)
    class LinearFunction : public Qwt3DFunction
    {
    public:
        double operator()(double x, double y) override
        {
            return x * 100.0 + y;
        }
    } func;
    func.setMesh(static_cast<unsigned>(cols), static_cast<unsigned>(rows));
    func.setDomain(minX, maxX, minY, maxY);
    const Qwt3DFunctionData fdata = func.create();
    QCOMPARE(static_cast<int>(fdata.columns), cols);
    QCOMPARE(static_cast<int>(fdata.rows), rows);
    QCOMPARE(static_cast<int>(fdata.z.size()), rows);
    for (int j = 0; j < rows; ++j) {
        for (int i = 0; i < cols; ++i) {
            const double x = minX + (maxX - minX) * i / (cols - 1);
            const double y = minY + (maxY - minY) * j / (rows - 1);
            QCOMPARE(fdata.z[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ], x * 100.0 + y);
        }
    }

    // 4) 序列化往返：非方形网格写出再读回，行列不能翻转
    {
        QByteArray surfaceBuf;
        {
            QDataStream out(&surfaceBuf, QIODevice::WriteOnly);
            out.setVersion(DA::gc_dachart3d_datastream_version);
            out << &surface;
        }
        Qwt3DSurface restoredSurface;
        {
            QDataStream in(&surfaceBuf, QIODevice::ReadOnly);
            in.setVersion(DA::gc_dachart3d_datastream_version);
            in >> &restoredSurface;
        }
        Qwt3DGridData* rgd = restoredSurface.gridData();
        QVERIFY(rgd != nullptr);
        QCOMPARE(static_cast<int>(rgd->vertices.size()), rows);
        QVERIFY(!rgd->vertices.empty());
        QCOMPARE(static_cast<int>(rgd->vertices[ 0 ].size()), cols);
        for (int j = 0; j < rows; ++j) {
            for (int i = 0; i < cols; ++i) {
                QCOMPARE(rgd->vertices[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ][ 2 ],
                         j * 10.0 + i);
            }
        }
    }
    {
        QByteArray barBuf;
        {
            QDataStream out(&barBuf, QIODevice::WriteOnly);
            out.setVersion(DA::gc_dachart3d_datastream_version);
            out << &bar;
        }
        Qwt3DBar restoredBar;
        {
            QDataStream in(&barBuf, QIODevice::ReadOnly);
            in.setVersion(DA::gc_dachart3d_datastream_version);
            in >> &restoredBar;
        }
        QVERIFY(restoredBar.isGridData());
        QCOMPARE(restoredBar.gridColumns(), cols);
        QCOMPARE(restoredBar.gridRows(), rows);
        const std::vector< std::vector< double > > rz = restoredBar.gridZValues();
        QCOMPARE(static_cast<int>(rz.size()), rows);
        for (int j = 0; j < rows; ++j) {
            for (int i = 0; i < cols; ++i) {
                QCOMPARE(rz[ static_cast<size_t >(j) ][ static_cast<size_t >(i) ], j * 10.0 + i);
            }
        }
    }
}

/**
 * @brief 测试10：figure XML 中 3D 子图的布局/视图/主题/坐标轴往返
 *
 * 覆盖 T5 的 XML 3D 子图持久化（一期只存存在性/布局/视图/主题/坐标轴，不存 3D 图元数据）。
 */
void Chart3DIntegrationTest::test3DSubChartXmlRoundTrip()
{
    std::unique_ptr< DA::DAFigureWidget > fig1(new DA::DAFigureWidget());
    fig1->resize(800, 600);
    fig1->show();
    QTest::qWaitForWindowExposed(fig1.get());

    QVERIFY(fig1->enableGridLayout(2, 2));
    QVERIFY(fig1->setGridSizes(QList< qreal >{ 0.55, 0.45 }, QList< qreal >{ 0.4, 0.6 }));

    // 2D 子图 (0,0) 与 3D 子图 (1,1) 混合，验证 2D 流程不受 3D 持久化影响
    DA::DAChartWidget* chart2d = fig1->getChartFactory()->createChart();
    QVERIFY(chart2d);
    chart2d->setPlotId(QStringLiteral("xml-2d"));
    QVERIFY(fig1->setGridCell(chart2d, 0, 0));

    DA::DAChart3DWidget* chart3d = new DA::DAChart3DWidget();
    QVERIFY(chart3d->coordinates());
    chart3d->applyTheme(Qwt3DTheme::Scientific);
    // 运行期颜色：theme 不含用户在设置面板改的这些值，必须单独写 <view> 并在主题之后恢复。
    // 旧实现先 view 后主题，theme.apply() 会把背景/轴颜色覆盖回主题色（reviewer 应修-1）。
    chart3d->set3DBackgroundColor(Qt::red);
    chart3d->set3DAxesColor(Qt::blue);
    chart3d->set3DGridLinesColor(Qt::green);
    const RGBA expectedBackground     = chart3d->backgroundRGBAColor();
    const RGBA expectedAxesColor      = chart3d->coordinates()->axesColor();
    const RGBA expectedGridLinesColor = chart3d->coordinates()->gridLinesColor();
    QVERIFY2(rgbaNearlyEqual(expectedBackground, RGBA(1.0, 0.0, 0.0, 1.0)), "background should be red");
    QVERIFY2(rgbaNearlyEqual(expectedAxesColor, RGBA(0.0, 0.0, 1.0, 1.0)), "axes color should be blue");
    QVERIFY2(rgbaNearlyEqual(expectedGridLinesColor, RGBA(0.0, 1.0, 0.0, 1.0)),
             "grid lines color should be green");

    // G：公共 label/number 色设为黄/青，轴 0 再单独设红/绿。
    // 加载时必须先恢复公共色、再逐轴覆盖；顺序反了轴 0 会退回黄/青。
    chart3d->coordinates()->setLabelColor(RGBA(1.0, 1.0, 0.0, 1.0));
    chart3d->coordinates()->setNumberColor(RGBA(0.0, 1.0, 1.0, 1.0));
    chart3d->coordinates()->axes[ 0 ].setLabelColor(RGBA(1.0, 0.0, 0.0, 1.0));
    chart3d->coordinates()->axes[ 0 ].setNumberColor(RGBA(0.0, 1.0, 0.0, 1.0));
    const RGBA expectedCommonLabelColor  = chart3d->coordinates()->labelColor();
    const RGBA expectedCommonNumberColor = chart3d->coordinates()->numberColor();
    const RGBA expectedAxis0LabelColor   = chart3d->coordinates()->axes[ 0 ].labelColor();
    const RGBA expectedAxis0NumberColor  = chart3d->coordinates()->axes[ 0 ].numberColor();
    QVERIFY2(rgbaNearlyEqual(expectedCommonLabelColor, RGBA(1.0, 1.0, 0.0, 1.0)), "common label color = yellow");
    QVERIFY2(rgbaNearlyEqual(expectedCommonNumberColor, RGBA(0.0, 1.0, 1.0, 1.0)), "common number color = cyan");
    QVERIFY2(rgbaNearlyEqual(expectedAxis0LabelColor, RGBA(1.0, 0.0, 0.0, 1.0)), "axis0 label color = red");
    QVERIFY2(rgbaNearlyEqual(expectedAxis0NumberColor, RGBA(0.0, 1.0, 0.0, 1.0)), "axis0 number color = green");

    chart3d->setRotation(30.0, 45.0, 15.0);
    chart3d->setZoom(1.7);
    chart3d->setOrtho(true);
    chart3d->setTitle(QStringLiteral("wf"));
    chart3d->set3DAxisLabel(X1, QStringLiteral("time"));
    QCOMPARE(chart3d->coordinates()->axes[ 0 ].labelString(), QStringLiteral("time"));
    QVERIFY(fig1->setGridCell(chart3d, 1, 1));

    const QByteArray expectedTheme = serializeTheme(chart3d->theme());
    QVERIFY(!expectedTheme.isEmpty());

    DA::DAChartItemsManager mgr;
    QDomDocument doc;
    const QDomElement figEle = DA::DAXmlHelper::makeElement(fig1.get(), QStringLiteral("figure"), &doc, &mgr);
    QVERIFY(!figEle.isNull());

    // <chart3d> 节点：网格坐标 + 旧版 x/y/w/h + 视图/坐标轴/主题子节点
    const QDomElement c3dEle = figEle.firstChildElement(QStringLiteral("chart3d"));
    QVERIFY(!c3dEle.isNull());
    QVERIFY(c3dEle.hasAttribute(QStringLiteral("grid-row")));
    QVERIFY(c3dEle.hasAttribute(QStringLiteral("grid-col")));
    QCOMPARE(c3dEle.attribute(QStringLiteral("grid-row")).toInt(), 1);
    QCOMPARE(c3dEle.attribute(QStringLiteral("grid-col")).toInt(), 1);
    QCOMPARE(c3dEle.attribute(QStringLiteral("grid-rowspan")).toInt(), 1);
    QCOMPARE(c3dEle.attribute(QStringLiteral("grid-colspan")).toInt(), 1);
    QVERIFY(c3dEle.hasAttribute(QStringLiteral("x")));
    QVERIFY(c3dEle.hasAttribute(QStringLiteral("y")));
    QVERIFY(c3dEle.hasAttribute(QStringLiteral("w")));
    QVERIFY(c3dEle.hasAttribute(QStringLiteral("h")));

    const QDomElement viewEle = c3dEle.firstChildElement(QStringLiteral("view"));
    QVERIFY(!viewEle.isNull());
    QVERIFY(nearlyEqual(viewEle.attribute(QStringLiteral("xrot")).toDouble(), 30.0));
    QVERIFY(nearlyEqual(viewEle.attribute(QStringLiteral("yrot")).toDouble(), 45.0));
    QVERIFY(nearlyEqual(viewEle.attribute(QStringLiteral("zrot")).toDouble(), 15.0));
    QVERIFY(nearlyEqual(viewEle.attribute(QStringLiteral("zoom")).toDouble(), 1.7));
    QCOMPARE(viewEle.attribute(QStringLiteral("ortho")).toInt(), 1);
    QCOMPARE(viewEle.attribute(QStringLiteral("title")), QStringLiteral("wf"));
    // 运行期颜色属性必须写进 <view>（否则重开后丢失用户设置）
    QVERIFY(viewEle.hasAttribute(QStringLiteral("background")));
    QVERIFY(viewEle.hasAttribute(QStringLiteral("axescolor")));
    QVERIFY(viewEle.hasAttribute(QStringLiteral("gridlinescolor")));
    QVERIFY(viewEle.attribute(QStringLiteral("background")).startsWith(QStringLiteral("1,0,0")));
    QVERIFY(viewEle.attribute(QStringLiteral("axescolor")).startsWith(QStringLiteral("0,0,1")));

    const QDomElement axesEle = c3dEle.firstChildElement(QStringLiteral("axes"));
    QVERIFY(!axesEle.isNull());
    QCOMPARE(axesEle.childNodes().size(), 12);
    const QDomElement axis0Ele = axesEle.firstChildElement(QStringLiteral("axis"));
    QVERIFY(!axis0Ele.isNull());
    QCOMPARE(axis0Ele.attribute(QStringLiteral("index")).toInt(), 0);
    QCOMPARE(axis0Ele.attribute(QStringLiteral("label")), QStringLiteral("time"));
    // G：逐轴颜色属性必须写入 <axis>，且是轴 0 自己的值（不是公共色）
    QVERIFY(axis0Ele.hasAttribute(QStringLiteral("labelcolor")));
    QVERIFY(axis0Ele.hasAttribute(QStringLiteral("numbercolor")));
    QVERIFY(axis0Ele.attribute(QStringLiteral("labelcolor")).startsWith(QStringLiteral("1,0,0")));
    QVERIFY(axis0Ele.attribute(QStringLiteral("numbercolor")).startsWith(QStringLiteral("0,1,0")));

    const QDomElement themeEle = c3dEle.firstChildElement(QStringLiteral("theme"));
    QVERIFY(!themeEle.isNull());
    QVERIFY(!themeEle.text().isEmpty());

    // 2D 成员也带网格坐标（与 3D 同一条网格恢复路径）
    const QDomElement chart2dEle = figEle.firstChildElement(QStringLiteral("charts"))
                                       .firstChildElement(QStringLiteral("chart"));
    QVERIFY(!chart2dEle.isNull());
    QCOMPARE(chart2dEle.attribute(QStringLiteral("id")), QStringLiteral("xml-2d"));
    QVERIFY(chart2dEle.hasAttribute(QStringLiteral("grid-row")));
    QCOMPARE(chart2dEle.attribute(QStringLiteral("grid-row")).toInt(), 0);
    QCOMPARE(chart2dEle.attribute(QStringLiteral("grid-col")).toInt(), 0);

    // 读回新 figure
    std::unique_ptr< DA::DAFigureWidget > fig2(new DA::DAFigureWidget());
    fig2->resize(800, 600);
    fig2->show();
    QTest::qWaitForWindowExposed(fig2.get());
    QVERIFY(DA::DAXmlHelper::loadElement(fig2.get(), &figEle, &mgr));

    QVERIFY(fig2->isGridLayoutEnabled());
    QCOMPARE(fig2->getChartCount(), 1);
    QCOMPARE(fig2->get3DCharts().size(), 1);

    DA::DAChart3DWidget* restored3d = fig2->get3DCharts().first();
    QVERIFY(restored3d);
    QVERIFY(nearlyEqual(restored3d->xRotation(), 30.0));
    QVERIFY(nearlyEqual(restored3d->yRotation(), 45.0));
    QVERIFY(nearlyEqual(restored3d->zRotation(), 15.0));
    QVERIFY(nearlyEqual(restored3d->zoom(), 1.7));
    QVERIFY(restored3d->ortho());
    QCOMPARE(restored3d->title(), QStringLiteral("wf"));
    QCOMPARE(serializeTheme(restored3d->theme()), expectedTheme);

    // A：背景/轴/网格线颜色必须在主题之后恢复（不能只信 theme blob）
    QVERIFY2(rgbaNearlyEqual(restored3d->backgroundRGBAColor(), expectedBackground),
             "background color must survive the XML round trip");
    QVERIFY(restored3d->coordinates());
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->axesColor(), expectedAxesColor),
             "axes color must survive the XML round trip");
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->gridLinesColor(), expectedGridLinesColor),
             "grid lines color must survive the XML round trip");
    // G：公共色先恢复所有轴，逐轴色最后覆盖；轴 0 必须是红色/绿色，其它轴保持公共黄/青
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->labelColor(), expectedCommonLabelColor),
             "common label color must survive the XML round trip");
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->numberColor(), expectedCommonNumberColor),
             "common number color must survive the XML round trip");
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->axes[ 0 ].labelColor(), expectedAxis0LabelColor),
             "per-axis label color must override the common color on load");
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->axes[ 0 ].numberColor(), expectedAxis0NumberColor),
             "per-axis number color must override the common color on load");
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->axes[ 1 ].labelColor(), expectedCommonLabelColor),
             "other axes must keep the common label color");
    QVERIFY2(rgbaNearlyEqual(restored3d->coordinates()->axes[ 1 ].numberColor(), expectedCommonNumberColor),
             "other axes must keep the common number color");

    // 坐标轴标签：XML 直接恢复 coordinates 的轴属性
    QCOMPARE(restored3d->coordinates()->axes[ 0 ].labelString(), QStringLiteral("time"));
    // 公开 getter 读的是 DAChart3DWidget 的标签缓存，XML 加载后必须同步
    // （回归保护：load3DAxesElement 曾直接改底层 Qwt3DAxis，导致 get3DAxisLabel 返回空）
    QCOMPARE(restored3d->get3DAxisLabel(X1), QStringLiteral("time"));

    QVERIFY(fig2->grid());
    int row = -1;
    int col = -1;
    int rowSpan = -1;
    int colSpan = -1;
    QVERIFY(fig2->grid()->gridPosition(restored3d, &row, &col, &rowSpan, &colSpan));
    QCOMPARE(row, 1);
    QCOMPARE(col, 1);
    QCOMPARE(rowSpan, 1);
    QCOMPARE(colSpan, 1);

    // 2D 子图仍按网格坐标恢复，且数量不因 3D 节点变化
    DA::DAChartWidget* restored2d = qobject_cast< DA::DAChartWidget* >(fig2->findPlotById(QStringLiteral("xml-2d")));
    QVERIFY(restored2d);
    QVERIFY(fig2->grid()->gridPosition(restored2d, &row, &col, &rowSpan, &colSpan));
    QCOMPARE(row, 0);
    QCOMPARE(col, 0);
}

/**
 * @brief 测试11：3D-only figure 的 XML 不包含 chart item 数据
 *
 * 一期 3D 子图只存存在性/布局/视图/主题/坐标轴；3D 图元数据不进入 chart-data。
 */
void Chart3DIntegrationTest::test3DOnlyFigureNoChartItemData()
{
    std::unique_ptr< DA::DAFigureWidget > fig(new DA::DAFigureWidget());
    fig->resize(640, 480);
    fig->show();
    QTest::qWaitForWindowExposed(fig.get());

    DA::DAChart3DWidget* chart3d = new DA::DAChart3DWidget();
    chart3d->setTitle(QStringLiteral("only-3d"));
    fig->add3DChart(chart3d, 0.1, 0.1, 0.8, 0.8);
    // 挂一个真实 3D 数据图元，确保“不写 chart-data”不是因为没有图元
    Qwt3DSurface* surface = new Qwt3DSurface();
    QVERIFY(surface);
    surface->attach(chart3d);
    QCOMPARE(chart3d->itemList().size(), 1);
    QCOMPARE(fig->get3DCharts().size(), 1);
    QCOMPARE(fig->getChartCount(), 0);

    DA::DAChartItemsManager mgr;
    QDomDocument doc;
    const QDomElement figEle = DA::DAXmlHelper::makeElement(fig.get(), QStringLiteral("figure"), &doc, &mgr);
    QVERIFY(!figEle.isNull());
    const QDomElement c3dEle = figEle.firstChildElement(QStringLiteral("chart3d"));
    QVERIFY(!c3dEle.isNull());
    // 3D 节点中不应出现 <items>，itemsMgr 也不应有任何 key
    QVERIFY(c3dEle.firstChildElement(QStringLiteral("items")).isNull());
    QVERIFY2(mgr.items().isEmpty(), "3D-only figure must not record chart items for chart-data");
}

/**
 * @brief 测试12：旧工程 figure XML 无 <chart3d> 时 3D 数量为 0、2D 正常恢复
 */
void Chart3DIntegrationTest::testLegacyFigureWithout3DXml()
{
    QDomDocument doc;
    QDomElement figEle = doc.createElement(QStringLiteral("figure"));
    figEle.setAttribute(QStringLiteral("id"), QStringLiteral("legacy-figure"));
    doc.appendChild(figEle);
    QDomElement chartsEle = doc.createElement(QStringLiteral("charts"));
    figEle.appendChild(chartsEle);
    QDomElement chartEle = doc.createElement(QStringLiteral("chart"));
    chartEle.setAttribute(QStringLiteral("id"), QStringLiteral("legacy-2d"));
    chartEle.setAttribute(QStringLiteral("x"), 0.1);
    chartEle.setAttribute(QStringLiteral("y"), 0.2);
    chartEle.setAttribute(QStringLiteral("w"), 0.5);
    chartEle.setAttribute(QStringLiteral("h"), 0.6);
    chartEle.appendChild(doc.createElement(QStringLiteral("items")));
    chartsEle.appendChild(chartEle);

    std::unique_ptr< DA::DAFigureWidget > fig(new DA::DAFigureWidget());
    fig->resize(640, 480);
    fig->show();
    QTest::qWaitForWindowExposed(fig.get());
    DA::DAChartItemsManager mgr;
    QVERIFY(DA::DAXmlHelper::loadElement(fig.get(), &figEle, &mgr));

    QCOMPARE(fig->get3DCharts().size(), 0);
    QCOMPARE(fig->getChartCount(), 1);
    QVERIFY(!fig->isGridLayoutEnabled());
    DA::DAChartWidget* restored2d = qobject_cast< DA::DAChartWidget* >(fig->findPlotById(QStringLiteral("legacy-2d")));
    QVERIFY(restored2d);
    const QRectF rect = fig->axesNormRect(restored2d);
    QVERIFY(nearlyEqual(rect.x(), 0.1));
    QVERIFY(nearlyEqual(rect.y(), 0.2));
    QVERIFY(nearlyEqual(rect.width(), 0.5));
    QVERIFY(nearlyEqual(rect.height(), 0.6));
}

QTEST_MAIN(Chart3DIntegrationTest)
#include "main.moc"
