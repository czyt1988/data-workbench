// DAFigureGridTest 的测试实现
//
// T4：DAFigureWidget 保留式网格布局（QwtFigureGrid）API 单元测试。
// 覆盖点：
// 1. enableGridLayout(rows, cols) 的返回值、isGridLayoutEnabled()、grid() 非空、行列数与等分轨道；
//    启用时把 qwt figure 的人为 min/max 尺寸复位为默认值。
// 2. createChart() + setGridCell(chart, row, col, rowSpan, colSpan) 的单元格几何
//    （与 grid()->normRectFor() 经 figure 尺寸换算一致，含跨列）；窗口缩放后比例保持；
//    qwt 原生自由布局子图与网格成员混用互不干扰。
// 3. 撤销：模拟一次拖动会话（连续多次 setColumnSizes()/setRowSizes() + 一次 finishChange()），
//    撤销栈只新增 1 条命令；undo 恢复旧比例、redo 恢复新比例；setGridSizes() 直接调用不新增命令。
// 4. 互斥：网格模式下 beginChartEditor(SubChartEditor) 无效（不产生 overlay 子控件）；
//    非网格 figure 上同样的调用仍生效（回归用例）。
// 5. 非绘图 QWidget 通过 setGridCell() 走 addGridItem 分支登记进网格，几何正确。
// 6. 3D 子图：仅在能安全创建 DAChart3DWidget（OpenGL 可用）时测试，否则 QSKIP。

#include <QtTest/QtTest>
#include <QApplication>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStringList>
#include <QSurfaceFormat>
#include <QUndoStack>

// std
#include <utility>

// DAFigure
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "DAFigureWidgetOverlay.h"
// qwt
#include "qwt_figure.h"
#include "qwt_figure_grid.h"

namespace DA
{
/**
 * @brief DAFigureWidget 保留式网格布局单元测试
 */
class TSTDAFigureGridTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    // 启用语义：返回值/行列数/等分轨道/非法行列
    void testEnableGridLayout();
    // 启用时把 figure 的 min/max 尺寸复位为默认（规避设置面板写入的塌陷值）
    void testEnableGridLayoutResetsFigureSizeLimits();
    // 单元格几何（跨列/跨行）及窗口 resize 后的几何与比例
    void testGridCellGeometry();
    // setGridSizes 后窗口缩放，轨道比例保持
    void testGridRatiosSurviveResize();
    // 自由布局子图与网格成员混用
    void testFreeAndGridMembersMixed();
    // 一次拖动会话只压入一条撤销命令；undo/redo 恢复比例
    void testGridDragPushesSingleUndoCommand();
    // setGridSizes 直接调用不压入撤销栈
    void testSetGridSizesDoesNotPushUndo();
    // 网格模式下子图编辑器被禁用
    void testSubChartEditorBlockedInGridMode();
    // 非网格 figure 上子图编辑器仍生效（回归）
    void testSubChartEditorWorksWithoutGrid();
    // 非绘图控件通过 setGridCell 登记进网格
    void testNonPlotWidgetGridCell();
    // 3D 子图登记进网格（OpenGL 不可用时跳过）
    void testChart3DGridCell();

private:
    // 创建并显示指定尺寸的 figure，等待窗口暴露与布局完成
    static bool initFigure(DAFigureWidget* fig, int width = 1000, int height = 800);
    // 处理事件并短暂等待，让 QScrollArea 的尺寸同步与 qwt layout 重排完成
    static void settle(DAFigureWidget* fig, int ms = 30);
    // 由归一化矩形与 figure 当前尺寸换算成像素几何（与 QwtFigureLayout::calcActualRect 同算法）
    static QRect expectedNormGeometry(DAFigureWidget* fig, const QRectF& normRect);
    // 由网格单元格位置换算期望几何
    static QRect expectedCellGeometry(DAFigureWidget* fig, int row, int col, int rowSpan = 1, int colSpan = 1);
    // 矩形比较（允许像素误差）
    static bool fuzzyRect(const QRect& actual, const QRect& expected, int tolerance = 2);
    // 比较两个 qreal 是否近似相等
    static bool fuzzyEqual(qreal a, qreal b);
    // 失败信息里打印矩形
    static QString rectText(const QRect& r);
    // 失败信息里打印比例列表
    static QString sizesText(const QList< qreal >& sizes);
};

//===================================================
// 辅助函数
//===================================================

bool TSTDAFigureGridTest::initFigure(DAFigureWidget* fig, int width, int height)
{
    if (!fig) {
        return false;
    }
    fig->resize(width, height);
    fig->show();
    if (!QTest::qWaitForWindowExposed(fig)) {
        return false;
    }
    settle(fig);
    return true;
}

void TSTDAFigureGridTest::settle(DAFigureWidget* fig, int ms)
{
    Q_UNUSED(fig);
    // qWait 内部会处理事件；再前后各 processEvents 一次，确保布局请求被消化
    QApplication::processEvents();
    QTest::qWait(ms);
    QApplication::processEvents();
}

QRect TSTDAFigureGridTest::expectedNormGeometry(DAFigureWidget* fig, const QRectF& normRect)
{
    QwtFigure* qfig = fig ? fig->figure() : nullptr;
    if (!qfig) {
        return QRect();
    }
    // 与 QwtFigureLayout::calcActualRect 相同的换算：按 figure 当前像素矩形取整并裁剪
    const QRect parent          = qfig->rect();
    const qreal availableWidth  = parent.width();
    const qreal availableHeight = parent.height();
    const qreal actualLeft      = qRound(normRect.left() * availableWidth);
    const qreal actualTop       = qRound(normRect.top() * availableHeight);
    const qreal actualWidth     = qRound(normRect.width() * availableWidth);
    const qreal actualHeight    = qRound(normRect.height() * availableHeight);
    return QRect(qMax(0.0, actualLeft),
                 qMax(0.0, actualTop),
                 qMin(actualWidth, availableWidth - actualLeft),
                 qMin(actualHeight, availableHeight - actualTop));
}

QRect TSTDAFigureGridTest::expectedCellGeometry(DAFigureWidget* fig, int row, int col, int rowSpan, int colSpan)
{
    if (!fig || !fig->grid()) {
        return QRect();
    }
    return expectedNormGeometry(fig, fig->grid()->normRectFor(row, col, rowSpan, colSpan));
}

bool TSTDAFigureGridTest::fuzzyRect(const QRect& actual, const QRect& expected, int tolerance)
{
    return qAbs(actual.x() - expected.x()) <= tolerance && qAbs(actual.y() - expected.y()) <= tolerance
           && qAbs(actual.width() - expected.width()) <= tolerance
           && qAbs(actual.height() - expected.height()) <= tolerance;
}

bool TSTDAFigureGridTest::fuzzyEqual(qreal a, qreal b)
{
    return qAbs(a - b) <= 1e-9;
}

QString TSTDAFigureGridTest::rectText(const QRect& r)
{
    return QStringLiteral("(%1,%2 %3x%4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
}

QString TSTDAFigureGridTest::sizesText(const QList< qreal >& sizes)
{
    QStringList parts;
    parts.reserve(sizes.size());
    for (qreal v : sizes) {
        parts.append(QString::number(v, 'g', 10));
    }
    return QStringLiteral("[%1]").arg(parts.join(QStringLiteral(", ")));
}

//===================================================
// 用例实现
//===================================================

/**
 * @brief 启用语义：返回值、grid()、行列数、等分轨道、非法行列
 */
void TSTDAFigureGridTest::testEnableGridLayout()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->figure());

    // 启用前：无网格模型
    QVERIFY2(fig->grid() == nullptr, "grid() should be null before enableGridLayout()");
    QVERIFY(!fig->isGridLayoutEnabled());

    // 非法行列：返回 false，且不留下启用状态
    QVERIFY(!fig->enableGridLayout(0, 2));
    QVERIFY(!fig->enableGridLayout(2, -1));
    QVERIFY(!fig->isGridLayoutEnabled());
    QVERIFY(fig->grid() == nullptr);

    // 正常启用 2x2
    QVERIFY2(fig->enableGridLayout(2, 2), "enableGridLayout(2, 2) should succeed");
    QVERIFY(fig->isGridLayoutEnabled());
    QwtFigureGrid* grid = fig->grid();
    QVERIFY2(grid != nullptr, "grid() should not be null after enableGridLayout()");
    QVERIFY(grid->isEnabled());
    QCOMPARE(grid->rowCount(), 2);
    QCOMPARE(grid->columnCount(), 2);
    // DAFigureWidget 与底层 qwt figure 的启用状态一致
    QVERIFY(fig->figure()->isGridEnabled());

    // 初始轨道等分：2x2 各为 0.5
    const QList< qreal > rows = fig->gridRowSizes();
    const QList< qreal > cols = fig->gridColumnSizes();
    QCOMPARE(rows.size(), 2);
    QCOMPARE(cols.size(), 2);
    for (qreal v : rows) {
        QVERIFY2(fuzzyEqual(v, 0.5), qPrintable(QStringLiteral("row sizes should be equal, got %1").arg(sizesText(rows))));
    }
    for (qreal v : cols) {
        QVERIFY2(fuzzyEqual(v, 0.5),
                 qPrintable(QStringLiteral("column sizes should be equal, got %1").arg(sizesText(cols))));
    }

    // 重新启用为 2x3：轨道数随之变化并重新等分
    QVERIFY(fig->enableGridLayout(2, 3));
    QCOMPARE(fig->grid()->rowCount(), 2);
    QCOMPARE(fig->grid()->columnCount(), 3);
    const QList< qreal > rows23 = fig->gridRowSizes();
    const QList< qreal > cols23 = fig->gridColumnSizes();
    QCOMPARE(rows23.size(), 2);
    QCOMPARE(cols23.size(), 3);
    for (qreal v : rows23) {
        QVERIFY(fuzzyEqual(v, 0.5));
    }
    for (qreal v : cols23) {
        QVERIFY2(fuzzyEqual(v, 1.0 / 3.0),
                 qPrintable(QStringLiteral("column sizes should be 1/3, got %1").arg(sizesText(cols23))));
    }
}

/**
 * @brief 启用网格时把 qwt figure 的 min/max 尺寸复位为默认
 *
 * DAFigureWidgetSettingPanel 会把 qwtfig 的 min/max 尺寸写入配置，遗留的塌陷值会让
 * 网格子图无法铺满；enableGridLayout() 必须把它复位为默认（minimum 0、maximum QWIDGETSIZE_MAX）。
 */
void TSTDAFigureGridTest::testEnableGridLayoutResetsFigureSizeLimits()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QwtFigure* qfig = fig->figure();
    QVERIFY(qfig);

    // 人为写入异常值（模拟设置面板遗留的塌陷尺寸）
    qfig->setMinimumSize(600, 400);
    qfig->setMaximumSize(600, 400);
    settle(fig.data());
    QCOMPARE(qfig->minimumSize(), QSize(600, 400));
    QCOMPARE(qfig->maximumSize(), QSize(600, 400));

    // 启用网格后复位为默认值
    QVERIFY(fig->enableGridLayout(2, 2));
    QCOMPARE(qfig->minimumSize(), QSize(0, 0));
    QCOMPARE(qfig->maximumSize(), QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

    // 复位后 figure 能重新铺满滚动区视口（允许滚动区边框带来的像素误差）
    settle(fig.data());
    QVERIFY2(qAbs(qfig->width() - fig->viewport()->width()) <= 2,
             qPrintable(QStringLiteral("figure width %1 should fill viewport width %2")
                            .arg(qfig->width())
                            .arg(fig->viewport()->width())));
    QVERIFY2(qAbs(qfig->height() - fig->viewport()->height()) <= 2,
             qPrintable(QStringLiteral("figure height %1 should fill viewport height %2")
                            .arg(qfig->height())
                            .arg(fig->viewport()->height())));
}

/**
 * @brief setGridCell 的单元格几何：跨列/跨行与 resize 后的几何、比例
 */
void TSTDAFigureGridTest::testGridCellGeometry()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));

    // 上排跨两列的子图
    DAChartWidget* top = fig->createChart();
    QVERIFY(top);
    QVERIFY2(fig->setGridCell(top, 0, 0, 1, 2), "setGridCell(top, 0, 0, 1, 2) should succeed");
    // 下排两个单列子图
    DAChartWidget* bottomLeft = fig->createChart();
    QVERIFY(bottomLeft);
    QVERIFY(fig->setGridCell(bottomLeft, 1, 0));
    DAChartWidget* bottomRight = fig->createChart();
    QVERIFY(bottomRight);
    QVERIFY(fig->setGridCell(bottomRight, 1, 1));
    settle(fig.data());

    // 三者都已成为 figure 的网格成员
    QCOMPARE(fig->getCharts().size(), 3);
    QCOMPARE(fig->grid()->items().size(), 3);
    const QList< DAChartWidget* > charts = { top, bottomLeft, bottomRight };
    for (DAChartWidget* chart : charts) {
        QVERIFY2(chart->parentWidget() == fig->figure(), "grid member should be parented to the qwt figure");
    }

    // 几何与 grid()->normRectFor() 经 figure 尺寸换算一致（允许 ±2px）
    const QRect expectedTop  = expectedCellGeometry(fig.data(), 0, 0, 1, 2);
    const QRect expectedBL   = expectedCellGeometry(fig.data(), 1, 0);
    const QRect expectedBR   = expectedCellGeometry(fig.data(), 1, 1);
    QVERIFY2(fuzzyRect(top->geometry(), expectedTop),
             qPrintable(QStringLiteral("top cell actual=%1 expected=%2")
                            .arg(rectText(top->geometry()), rectText(expectedTop))));
    QVERIFY2(fuzzyRect(bottomLeft->geometry(), expectedBL),
             qPrintable(QStringLiteral("bottom-left actual=%1 expected=%2")
                            .arg(rectText(bottomLeft->geometry()), rectText(expectedBL))));
    QVERIFY2(fuzzyRect(bottomRight->geometry(), expectedBR),
             qPrintable(QStringLiteral("bottom-right actual=%1 expected=%2")
                            .arg(rectText(bottomRight->geometry()), rectText(expectedBR))));

    // 期望的比例关系：上排整宽半高，下排各半宽半高
    const QRect figureRect = fig->figure()->rect();
    QVERIFY(qAbs(top->geometry().width() - figureRect.width()) <= 2);
    QVERIFY(qAbs(top->geometry().height() - figureRect.height() / 2) <= 2);
    QVERIFY(qAbs(bottomLeft->geometry().width() - figureRect.width() / 2) <= 2);
    QVERIFY(qAbs(bottomLeft->geometry().left()) <= 2);
    QVERIFY(qAbs(bottomRight->geometry().left() - figureRect.width() / 2) <= 2);

    // 窗口 resize 后：几何按新尺寸重算，比例保持
    fig->resize(800, 600);
    settle(fig.data());
    const QRect resizedFigureRect = fig->figure()->rect();
    const QRect resizedTop        = expectedCellGeometry(fig.data(), 0, 0, 1, 2);
    const QRect resizedBR         = expectedCellGeometry(fig.data(), 1, 1);
    QVERIFY2(fuzzyRect(top->geometry(), resizedTop),
             qPrintable(QStringLiteral("after resize top actual=%1 expected=%2")
                            .arg(rectText(top->geometry()), rectText(resizedTop))));
    QVERIFY2(fuzzyRect(bottomRight->geometry(), resizedBR),
             qPrintable(QStringLiteral("after resize bottom-right actual=%1 expected=%2")
                            .arg(rectText(bottomRight->geometry()), rectText(resizedBR))));
    QVERIFY(qAbs(top->geometry().width() - resizedFigureRect.width()) <= 2);
    QVERIFY(qAbs(top->geometry().height() - resizedFigureRect.height() / 2) <= 2);
}

/**
 * @brief setGridSizes 设置的轨道比例在窗口缩放后保持
 */
void TSTDAFigureGridTest::testGridRatiosSurviveResize()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));

    // 程序化设置非等分轨道（XML 恢复路径）
    QVERIFY(fig->setGridSizes(QList< qreal > { 0.3, 0.7 }, QList< qreal > { 0.25, 0.75 }));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    QVERIFY(fig->setGridCell(chart, 1, 1));
    settle(fig.data());

    // 校验右下单元格：左/上偏移 0.25/0.3，宽/高占比 0.75/0.7
    const auto checkRatio = [ &fig, chart ]() -> bool {
        const QRect figureRect = fig->figure()->rect();
        const QRect geo        = chart->geometry();
        if (!fuzzyRect(geo, expectedCellGeometry(fig.data(), 1, 1))) {
            return false;
        }
        if (figureRect.width() <= 0 || figureRect.height() <= 0) {
            return false;
        }
        const qreal wRatio = static_cast< qreal >(geo.width()) / figureRect.width();
        const qreal hRatio = static_cast< qreal >(geo.height()) / figureRect.height();
        const qreal xRatio = static_cast< qreal >(geo.x()) / figureRect.width();
        const qreal yRatio = static_cast< qreal >(geo.y()) / figureRect.height();
        return qAbs(wRatio - 0.75) < 0.01 && qAbs(hRatio - 0.7) < 0.01 && qAbs(xRatio - 0.25) < 0.01
               && qAbs(yRatio - 0.3) < 0.01;
    };
    QVERIFY2(checkRatio(), "cell ratios should match the configured tracks");
    const QList< qreal > rowsBefore = fig->gridRowSizes();
    const QList< qreal > colsBefore = fig->gridColumnSizes();

    // 窗口缩放后比例不变
    fig->resize(700, 500);
    settle(fig.data());
    QVERIFY2(checkRatio(), "cell ratios should survive a figure resize");
    const QList< qreal > rowsAfter = fig->gridRowSizes();
    const QList< qreal > colsAfter = fig->gridColumnSizes();
    QCOMPARE(rowsAfter.size(), rowsBefore.size());
    QCOMPARE(colsAfter.size(), colsBefore.size());
    for (int i = 0; i < rowsAfter.size(); ++i) {
        QVERIFY2(fuzzyEqual(rowsAfter.at(i), rowsBefore.at(i)),
                 qPrintable(QStringLiteral("row ratio changed after resize: %1 -> %2")
                                .arg(sizesText(rowsBefore), sizesText(rowsAfter))));
    }
    for (int i = 0; i < colsAfter.size(); ++i) {
        QVERIFY2(fuzzyEqual(colsAfter.at(i), colsBefore.at(i)),
                 qPrintable(QStringLiteral("column ratio changed after resize: %1 -> %2")
                                .arg(sizesText(colsBefore), sizesText(colsAfter))));
    }
}

/**
 * @brief 自由布局子图（qwt 原生归一化矩形）与网格成员混用
 */
void TSTDAFigureGridTest::testFreeAndGridMembersMixed()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));

    // 网格成员
    DAChartWidget* gridChart = fig->createChart();
    QVERIFY(gridChart);
    QVERIFY(fig->setGridCell(gridChart, 0, 0));
    // 自由布局成员：走 qwt 原生归一化矩形（不登记网格）
    const QRectF freeNorm(0.55, 0.6, 0.3, 0.3);
    DAChartWidget* freeChart = fig->createChart(freeNorm);
    QVERIFY(freeChart);
    settle(fig.data());

    // 自由成员仍按原归一化矩形摆放，且没有登记进网格
    const QRect expectedFree = expectedNormGeometry(fig.data(), freeNorm);
    QVERIFY2(fuzzyRect(freeChart->geometry(), expectedFree),
             qPrintable(QStringLiteral("free chart actual=%1 expected=%2")
                            .arg(rectText(freeChart->geometry()), rectText(expectedFree))));
    int row = 0, col = 0, rowSpan = 0, colSpan = 0;
    QVERIFY2(!fig->grid()->gridPosition(freeChart, &row, &col, &rowSpan, &colSpan),
             "a free-layout chart must not be registered in the grid");

    // 网格成员按单元格摆放
    const QRect expectedGrid = expectedCellGeometry(fig.data(), 0, 0);
    QVERIFY2(fuzzyRect(gridChart->geometry(), expectedGrid),
             qPrintable(QStringLiteral("grid chart actual=%1 expected=%2")
                            .arg(rectText(gridChart->geometry()), rectText(expectedGrid))));

    // 改网格比例：网格成员跟随新轨道，自由成员几何不变
    const QRect freeBefore = freeChart->geometry();
    const QRect gridBefore = gridChart->geometry();
    QVERIFY(fig->setGridSizes(QList< qreal > { 0.5, 0.5 }, QList< qreal > { 0.6, 0.4 }));
    settle(fig.data());
    const QRect expectedGridAfter = expectedCellGeometry(fig.data(), 0, 0);
    QVERIFY2(fuzzyRect(gridChart->geometry(), expectedGridAfter),
             qPrintable(QStringLiteral("grid chart after reflow actual=%1 expected=%2")
                            .arg(rectText(gridChart->geometry()), rectText(expectedGridAfter))));
    QVERIFY2(gridChart->geometry().width() > gridBefore.width(),
             "grid member should follow the widened column track");
    QCOMPARE(freeChart->geometry(), freeBefore);
}

/**
 * @brief 一次拖动会话只压入一条撤销命令；undo/redo 恢复比例
 *
 * 语义：若干次 grid()->setColumnSizes()/setRowSizes()（每次发 sizesChanged）+ 一次
 * grid()->finishChange()（发 sizesChangeFinished）视为一次拖动，undo 栈只 +1。
 */
void TSTDAFigureGridTest::testGridDragPushesSingleUndoCommand()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    settle(fig.data());

    QwtFigureGrid* grid = fig->grid();
    QVERIFY(grid);
    QUndoStack* stack = fig->getUndoStack();
    QVERIFY(stack);
    const int baseCount = stack->count();

    QSignalSpy changedSpy(grid, &QwtFigureGrid::sizesChanged);
    QSignalSpy finishedSpy(grid, &QwtFigureGrid::sizesChangeFinished);
    QVERIFY(changedSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    // 模拟一次拖动：连续多次修改比例
    grid->setColumnSizes(QVector< qreal > { 0.55, 0.45 });
    grid->setColumnSizes(QVector< qreal > { 0.6, 0.4 });
    grid->setRowSizes(QVector< qreal > { 0.4, 0.6 });
    grid->setRowSizes(QVector< qreal > { 0.25, 0.75 });
    QVERIFY2(changedSpy.count() >= 4, "each track change should emit sizesChanged()");
    // 会话进行中不入撤销栈
    QCOMPARE(stack->count(), baseCount);

    // 松手：一次 finishChange 只压入一条命令
    grid->finishChange();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(stack->count(), baseCount + 1);

    // 没有实际变化的再次 finishChange 不新增命令
    grid->finishChange();
    QCOMPARE(stack->count(), baseCount + 1);

    // 比例校验辅助：行/列各两个值
    const auto verifySizes = [](const QList< qreal >& actual, qreal a, qreal b) -> bool {
        return actual.size() == 2 && qAbs(actual.at(0) - a) <= 1e-9 && qAbs(actual.at(1) - b) <= 1e-9;
    };

    // undo：恢复拖动前的等分比例
    stack->undo();
    QVERIFY2(verifySizes(fig->gridRowSizes(), 0.5, 0.5),
             qPrintable(QStringLiteral("undo should restore equal rows, got %1").arg(sizesText(fig->gridRowSizes()))));
    QVERIFY2(verifySizes(fig->gridColumnSizes(), 0.5, 0.5),
             qPrintable(QStringLiteral("undo should restore equal columns, got %1")
                            .arg(sizesText(fig->gridColumnSizes()))));

    // redo：恢复拖动后的比例
    stack->redo();
    QVERIFY2(verifySizes(fig->gridRowSizes(), 0.25, 0.75),
             qPrintable(QStringLiteral("redo should restore dragged rows, got %1").arg(sizesText(fig->gridRowSizes()))));
    QVERIFY2(verifySizes(fig->gridColumnSizes(), 0.6, 0.4),
             qPrintable(QStringLiteral("redo should restore dragged columns, got %1")
                            .arg(sizesText(fig->gridColumnSizes()))));
}

/**
 * @brief setGridSizes 直接调用不压入撤销栈（供 XML 恢复），且同步撤销基线
 */
void TSTDAFigureGridTest::testSetGridSizesDoesNotPushUndo()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    settle(fig.data());

    QUndoStack* stack = fig->getUndoStack();
    QVERIFY(stack);
    QCOMPARE(stack->count(), 0);

    // 程序化设置：不新增命令
    QVERIFY(fig->setGridSizes(QList< qreal > { 0.3, 0.7 }, QList< qreal > { 0.25, 0.75 }));
    QCOMPARE(stack->count(), 0);
    const auto verifyCurrent = [ &fig ](qreal r0, qreal r1, qreal c0, qreal c1) -> bool {
        const QList< qreal > rows = fig->gridRowSizes();
        const QList< qreal > cols = fig->gridColumnSizes();
        return rows.size() == 2 && cols.size() == 2 && qAbs(rows.at(0) - r0) <= 1e-9
               && qAbs(rows.at(1) - r1) <= 1e-9 && qAbs(cols.at(0) - c0) <= 1e-9 && qAbs(cols.at(1) - c1) <= 1e-9;
    };
    QVERIFY2(verifyCurrent(0.3, 0.7, 0.25, 0.75), "setGridSizes should apply the requested ratios");

    // 非法入参：个数不符/非有限正数 → false，比例不变且不压栈
    QVERIFY(!fig->setGridSizes(QList< qreal > { 0.3, 0.7, 0.0 }, QList< qreal > { 0.25, 0.75 }));
    QVERIFY(!fig->setGridSizes(QList< qreal > { 0.3, -0.7 }, QList< qreal > { 0.25, 0.75 }));
    QVERIFY(!fig->setGridSizes(QList< qreal > { 0.3, 0.7 }, QList< qreal > { 0.25, 0.75, 0.0 }));
    QCOMPARE(stack->count(), 0);
    QVERIFY2(verifyCurrent(0.3, 0.7, 0.25, 0.75), "rejected setGridSizes calls must keep the previous ratios");

    // 程序化设置后已同步撤销基线：随后一次交互拖动的 undo 应回到 setGridSizes 的值而非初始等分
    QwtFigureGrid* grid = fig->grid();
    QVERIFY(grid);
    grid->setColumnSizes(QVector< qreal > { 0.5, 0.5 });
    grid->finishChange();
    QCOMPARE(stack->count(), 1);
    stack->undo();
    QVERIFY2(verifyCurrent(0.3, 0.7, 0.25, 0.75),
             qPrintable(QStringLiteral("undo after a programmatic set should restore the programmatic ratios, got "
                                       "rows=%1 cols=%2")
                            .arg(sizesText(fig->gridRowSizes()), sizesText(fig->gridColumnSizes()))));
}

/**
 * @brief 网格模式下子图编辑器（自由缩放）被禁用
 */
void TSTDAFigureGridTest::testSubChartEditorBlockedInGridMode()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    QVERIFY(fig->setGridCell(chart, 0, 0));
    settle(fig.data());

    QVERIFY2(!fig->isChartEditorActive(), "no chart editor should be active before beginChartEditor()");
    fig->beginChartEditor(DAFigureWidget::SubChartEditor);
    QApplication::processEvents();
    QVERIFY2(!fig->isChartEditorActive(), "the sub chart editor must be disabled in grid mode");
    QVERIFY2(fig->figure()->findChild< DAFigureWidgetOverlay* >() == nullptr,
             "no sub chart overlay should be created in grid mode");

    // 结束调用应安全无副作用
    fig->endChartEditor();
    QApplication::processEvents();
    QVERIFY(!fig->isChartEditorActive());
}

/**
 * @brief 非网格 figure 上子图编辑器仍生效（回归用例）
 */
void TSTDAFigureGridTest::testSubChartEditorWorksWithoutGrid()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(!fig->isGridLayoutEnabled());
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    settle(fig.data());

    QVERIFY(!fig->isChartEditorActive());
    fig->beginChartEditor(DAFigureWidget::SubChartEditor);
    QApplication::processEvents();
    QVERIFY2(fig->isChartEditorActive(), "the sub chart editor should still work on a free-layout figure");
    QVERIFY2(fig->figure()->findChild< DAFigureWidgetOverlay* >() != nullptr,
             "the sub chart overlay should be created on a free-layout figure");

    // 回归用例结束：必须退出编辑器
    fig->endChartEditor();
    QApplication::processEvents();
    QVERIFY(!fig->isChartEditorActive());
}

/**
 * @brief 非绘图控件通过 setGridCell 登记进网格（走 addGridItem 分支）
 */
void TSTDAFigureGridTest::testNonPlotWidgetGridCell()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    settle(fig.data());

    // 无父对象的普通 QWidget：走 qwt figure 的 addGridItem 分支
    QWidget* plain = new QWidget();
    plain->setObjectName(QStringLiteral("gridPlainWidget"));
    QVERIFY2(fig->setGridCell(plain, 1, 1), "setGridCell should register a plain widget");
    QCOMPARE(plain->parentWidget(), fig->figure());
    int row = -1, col = -1, rowSpan = 0, colSpan = 0;
    QVERIFY(fig->grid()->gridPosition(plain, &row, &col, &rowSpan, &colSpan));
    QCOMPARE(row, 1);
    QCOMPARE(col, 1);
    QCOMPARE(rowSpan, 1);
    QCOMPARE(colSpan, 1);
    QVERIFY(fig->grid()->items().contains(plain));
    settle(fig.data());
    const QRect expectedSingle = expectedCellGeometry(fig.data(), 1, 1);
    QVERIFY2(fuzzyRect(plain->geometry(), expectedSingle),
             qPrintable(QStringLiteral("plain widget actual=%1 expected=%2")
                            .arg(rectText(plain->geometry()), rectText(expectedSingle))));

    // 跨两列登记
    QWidget* spanWidget = new QWidget();
    QVERIFY(fig->setGridCell(spanWidget, 0, 0, 1, 2));
    settle(fig.data());
    const QRect expectedSpan = expectedCellGeometry(fig.data(), 0, 0, 1, 2);
    QVERIFY2(fuzzyRect(spanWidget->geometry(), expectedSpan),
             qPrintable(QStringLiteral("span widget actual=%1 expected=%2")
                            .arg(rectText(spanWidget->geometry()), rectText(expectedSpan))));

    // 移动单元格：重新登记后几何跟随新单元格
    QVERIFY(fig->setGridCell(plain, 0, 0));
    settle(fig.data());
    QVERIFY(fig->grid()->gridPosition(plain, &row, &col, nullptr, nullptr));
    QCOMPARE(row, 0);
    QCOMPARE(col, 0);
    const QRect expectedMoved = expectedCellGeometry(fig.data(), 0, 0);
    QVERIFY2(fuzzyRect(plain->geometry(), expectedMoved),
             qPrintable(QStringLiteral("moved widget actual=%1 expected=%2")
                            .arg(rectText(plain->geometry()), rectText(expectedMoved))));
}

/**
 * @brief 3D 子图登记进网格
 *
 * 不直接引用 DAChart3DWidget 类型（测试只链接 qwt::plot，不链接 qwt::plot3d），
 * 通过 Qwt3DPlot 类名从 figure 的直接子控件里找到它。
 * 无 OpenGL 上下文/离屏平台时跳过，不引入不稳定用例。
 */
void TSTDAFigureGridTest::testChart3DGridCell()
{
    // 离屏平台不创建 OpenGL 控件
    if (QGuiApplication::platformName().contains(QStringLiteral("offscreen"), Qt::CaseInsensitive)) {
        QSKIP("offscreen platform: skip the 3D grid test");
    }
    const QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    if (format.majorVersion() < 2) {
        QSKIP("OpenGL is not available: skip the 3D grid test");
    }
    QOpenGLContext glContext;
    if (!glContext.create()) {
        QSKIP("cannot create an OpenGL context: skip the 3D grid test");
    }

    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));

    // 创建 3D 子图（返回值类型不在测试的链接范围内，这里只用它的 QWidget 身份）
    (void)fig->create3DChart();
    settle(fig.data(), 60);

    QWidget* chart3d = nullptr;
    const QList< QWidget* > children = fig->figure()->findChildren< QWidget* >(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* w : std::as_const(children)) {
        if (w->inherits("Qwt3DPlot")) {
            chart3d = w;
            break;
        }
    }
    if (!chart3d) {
        QSKIP("DAChart3DWidget is not available in this environment: skip the 3D grid test");
    }

    QVERIFY2(fig->setGridCell(chart3d, 1, 0, 1, 2), "the 3D chart should be registrable in the grid");
    settle(fig.data(), 60);
    const QRect expected = expectedCellGeometry(fig.data(), 1, 0, 1, 2);
    QVERIFY2(fuzzyRect(chart3d->geometry(), expected),
             qPrintable(QStringLiteral("3D chart actual=%1 expected=%2")
                            .arg(rectText(chart3d->geometry()), rectText(expected))));
}

}  // namespace DA

//===================================================
// main
//===================================================
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DA::TSTDAFigureGridTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
