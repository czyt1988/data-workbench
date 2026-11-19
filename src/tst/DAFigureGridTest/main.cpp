// DAFigureGridTest 的测试实现
//
// T4：DAFigureWidget 保留式网格布局（QwtFigureGrid）API 单元测试。
// 覆盖点：
// 1. enableGridLayout(rows, cols) 的返回值、isGridLayoutEnabled()、grid() 非空、行列数与等分轨道；
//    只在 figure 尺寸约束确实塌陷时复位 min/max（有效约束保留），并发出 gridLayoutEnabledChanged。
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
#include <QMouseEvent>
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
#include "DAFigureWidgetCommands.h"
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
    // 启用时保留用户显式设置的有效尺寸约束（非塌陷值不得被覆盖）
    void testEnableGridLayoutKeepsExplicitSizeLimits();
    // 启用时把塌陷的尺寸约束（max<=0）复位为默认，并通知设置面板刷新
    void testEnableGridLayoutResetsCollapsedSizeLimits();
    // gridLayoutEnabledChanged 只在状态确实变化时发射
    void testGridLayoutEnabledSignal();
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
    // 退出网格：成员保持当前几何、状态与登记清理、子图编辑器恢复可用
    void testDisableGridLayoutKeepsGeometry();
    // 退出网格不入撤销栈；重复退出无副作用；可再次启用
    void testDisableGridLayoutNoUndoAndIdempotent();
    // 非绘图控件通过 setGridCell 登记进网格
    void testNonPlotWidgetGridCell();
    // 未加入 figure 的 QwtPlot 通过 setGridCell 直接按网格坐标加入
    void testSetGridCellFreshPlot();
    // 非法单元格：返回 false 且保持原有网格登记与几何不变
    void testSetGridCellInvalidKeepsRegistration();
    // 3D 子图登记进网格（OpenGL 不可用时跳过）
    void testChart3DGridCell();
    // 3D 与 2D 相邻时，分隔条由 2D 一侧负责命中并可拖动（OpenGL 不可用时跳过）
    void testChart3DAdjacent2DDragDivider();
    // 创建子图的撤销命令在网格 figure 上按单元格恢复（不出现自由成员叠压）
    void testCreateChartCommandKeepsGridCell();
    // 移除子图的撤销命令 undo 后回到原单元格
    void testRemoveChartCommandKeepsGridCell();
    // 调整子图尺寸的撤销命令在网格成员上按单元格恢复
    void testResizeWidgetCommandKeepsGridCell();

private:
    // 当前环境能否安全创建 OpenGL 子图（离屏平台、无 GL 2.x 格式或不能建上下文时为 false）
    static bool canUseOpenGL();
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

bool TSTDAFigureGridTest::canUseOpenGL()
{
    // 离屏平台不创建 OpenGL 控件
    if (QGuiApplication::platformName().contains(QStringLiteral("offscreen"), Qt::CaseInsensitive)) {
        return false;
    }
    if (QSurfaceFormat::defaultFormat().majorVersion() < 2) {
        return false;
    }
    QOpenGLContext glContext;
    return glContext.create();
}

QRect TSTDAFigureGridTest::expectedNormGeometry(DAFigureWidget* fig, const QRectF& normRect)
{
    QwtFigure* qfig = fig ? fig->figure() : nullptr;
    if (!qfig) {
        return QRect();
    }
    // 与 QwtFigureLayout::calcActualRect 相同的换算：两条边分别按累计归一化位置取整，
    // 像素宽高由两条边求差（相邻单元边缘落在同一像素），最后裁剪到 figure 矩形内
    const QRect parent        = qfig->rect();
    const int availableWidth  = parent.width();
    const int availableHeight = parent.height();
    const int left   = qRound(normRect.left() * availableWidth);
    const int top    = qRound(normRect.top() * availableHeight);
    const int right  = qRound((normRect.left() + normRect.width()) * availableWidth);
    const int bottom = qRound((normRect.top() + normRect.height()) * availableHeight);
    const int x      = qMax(0, left);
    const int y      = qMax(0, top);
    return QRect(x, y, qMax(0, qMin(right, availableWidth) - x), qMax(0, qMin(bottom, availableHeight) - y));
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

    // 幂等：行列数不变时保留当前轨道比例（不会重建为等分）
    QVERIFY(fig->setGridSizes(QList< qreal > { 0.2, 0.8 }, QList< qreal > { 0.1, 0.2, 0.7 }));
    QVERIFY2(fig->enableGridLayout(2, 3), "enabling the same dimensions again should be idempotent");
    const QList< qreal > rowsIdem = fig->gridRowSizes();
    const QList< qreal > colsIdem = fig->gridColumnSizes();
    QVERIFY2(fuzzyEqual(rowsIdem.at(0), 0.2) && fuzzyEqual(rowsIdem.at(1), 0.8),
             qPrintable(QStringLiteral("idempotent enableGridLayout must keep row ratios, got %1").arg(sizesText(rowsIdem))));
    QVERIFY2(fuzzyEqual(colsIdem.at(0), 0.1) && fuzzyEqual(colsIdem.at(1), 0.2) && fuzzyEqual(colsIdem.at(2), 0.7),
             qPrintable(QStringLiteral("idempotent enableGridLayout must keep column ratios, got %1")
                            .arg(sizesText(colsIdem))));

    // 已启用时传入非法参数：返回 false，且不破坏已启用状态与轨道
    QVERIFY(!fig->enableGridLayout(0, 3));
    QVERIFY(fig->isGridLayoutEnabled());
    QCOMPARE(fig->grid()->rowCount(), 2);
    QCOMPARE(fig->grid()->columnCount(), 3);
}

/**
 * @brief 启用网格时保留用户显式设置的有效尺寸约束
 *
 * DAFigureWidgetSettingPanel 允许用户给 qwt figure 设置 min/max 尺寸。旧实现无条件把它们复位，
 * 用户的显式设置被静默丢弃；现在只在尺寸确实塌陷时才复位。
 */
void TSTDAFigureGridTest::testEnableGridLayoutKeepsExplicitSizeLimits()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QwtFigure* qfig = fig->figure();
    QVERIFY(qfig);

    // 用户显式设置的有效约束（min<=max 且 max>0）
    qfig->setMinimumSize(600, 400);
    qfig->setMaximumSize(900, 700);
    settle(fig.data());
    QCOMPARE(qfig->minimumSize(), QSize(600, 400));
    QCOMPARE(qfig->maximumSize(), QSize(900, 700));

    QSignalSpy sizeSpy(fig.data(), &DAFigureWidget::figureSizeConstraintsChanged);
    QVERIFY(sizeSpy.isValid());
    QVERIFY(fig->enableGridLayout(2, 2));
    QCOMPARE(qfig->minimumSize(), QSize(600, 400));
    QCOMPARE(qfig->maximumSize(), QSize(900, 700));
    QVERIFY2(sizeSpy.isEmpty(), "no size-constraint change means the panel must not be asked to refresh");
}

/**
 * @brief 启用网格时复位塌陷的尺寸约束并通知设置面板
 *
 * 塌陷值来自设置面板（最大宽度被设为 0 时子图会被压成 0 宽），必须复位为默认；
 * 复位后发射 figureSizeConstraintsChanged，设置面板据此重新读取实际值。
 */
void TSTDAFigureGridTest::testEnableGridLayoutResetsCollapsedSizeLimits()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QwtFigure* qfig = fig->figure();
    QVERIFY(qfig);

    // 人为写入塌陷值（模拟设置面板遗留的最大宽度 0）
    qfig->setMaximumWidth(0);
    qfig->setMinimumHeight(300);
    qfig->setMaximumHeight(0);
    settle(fig.data());
    QCOMPARE(qfig->maximumWidth(), 0);
    QCOMPARE(qfig->maximumHeight(), 0);

    QSignalSpy sizeSpy(fig.data(), &DAFigureWidget::figureSizeConstraintsChanged);
    QVERIFY(sizeSpy.isValid());
    QVERIFY(fig->enableGridLayout(2, 2));
    QCOMPARE(qfig->minimumSize(), QSize(0, 0));
    QCOMPARE(qfig->maximumSize(), QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    QCOMPARE(sizeSpy.count(), 1);

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
 * @brief gridLayoutEnabledChanged 只在状态确实变化时发射
 *
 * 供 Ribbon 等上层跟随网格状态刷新：幂等启用、非法参数、仅改变行列数都不应重复发射。
 */
void TSTDAFigureGridTest::testGridLayoutEnabledSignal()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QSignalSpy spy(fig.data(), &DAFigureWidget::gridLayoutEnabledChanged);
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0);

    // 首次启用：状态 false→true，发一次 true
    QVERIFY(fig->enableGridLayout(2, 2));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), true);

    // 幂等启用：状态未变，不发射
    QVERIFY(fig->enableGridLayout(2, 2));
    QCOMPARE(spy.count(), 0);

    // 非法参数：状态未变，不发射
    QVERIFY(!fig->enableGridLayout(0, 2));
    QCOMPARE(spy.count(), 0);
    QVERIFY(fig->isGridLayoutEnabled());

    // 改变行列数：仍是启用状态，不重复发射 true
    QVERIFY(fig->enableGridLayout(3, 3));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(fig->grid()->rowCount(), 3);
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
 * @brief 退出网格：成员保持当前几何、网格登记清理、子图编辑器恢复可用
 *
 * 退出网格用成员「当前实际矩形」转成自由矩形，所以退出瞬间子图既不能跳位也不能缩水；
 * 比例为非等分时同样成立（不能退回等分单元格）。
 */
void TSTDAFigureGridTest::testDisableGridLayoutKeepsGeometry()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));

    DAChartWidget* top         = fig->createChart();
    DAChartWidget* bottomLeft  = fig->createChart();
    DAChartWidget* bottomRight = fig->createChart();
    QVERIFY(top && bottomLeft && bottomRight);
    QVERIFY(fig->setGridCell(top, 0, 0, 1, 2));
    QVERIFY(fig->setGridCell(bottomLeft, 1, 0));
    QVERIFY(fig->setGridCell(bottomRight, 1, 1));
    // 非等分比例：退出后必须保持这套实际几何
    QVERIFY(fig->setGridSizes(QList< qreal > { 0.4, 0.6 }, QList< qreal > { 0.3, 0.7 }));
    settle(fig.data());

    const QRect topGeo         = top->geometry();
    const QRect bottomLeftGeo  = bottomLeft->geometry();
    const QRect bottomRightGeo = bottomRight->geometry();
    QVERIFY2(!topGeo.isEmpty() && !bottomLeftGeo.isEmpty() && !bottomRightGeo.isEmpty(),
             "grid members should have a geometry before leaving the grid");

    QSignalSpy gridSpy(fig.data(), &DAFigureWidget::gridLayoutEnabledChanged);
    QVERIFY(gridSpy.isValid());
    QVERIFY(fig->disableGridLayout());
    QCOMPARE(gridSpy.count(), 1);
    QCOMPARE(gridSpy.takeFirst().at(0).toBool(), false);
    QVERIFY2(!fig->isGridLayoutEnabled(), "isGridLayoutEnabled() must report false after leaving the grid");
    QVERIFY2(fig->grid() == nullptr || !fig->grid()->isEnabled(), "the qwt grid model must be disabled");
    // 比例与成员登记都已丢弃
    QVERIFY(fig->gridRowSizes().isEmpty());
    QVERIFY(fig->gridColumnSizes().isEmpty());
    QVERIFY2(fig->grid() == nullptr || fig->grid()->items().isEmpty(), "grid registrations must be cleared");

    settle(fig.data());
    QVERIFY2(fuzzyRect(top->geometry(), topGeo),
             qPrintable(QStringLiteral("spanning chart moved: actual=%1 before=%2")
                            .arg(rectText(top->geometry()), rectText(topGeo))));
    QVERIFY2(fuzzyRect(bottomLeft->geometry(), bottomLeftGeo),
             qPrintable(QStringLiteral("bottom left chart moved: actual=%1 before=%2")
                            .arg(rectText(bottomLeft->geometry()), rectText(bottomLeftGeo))));
    QVERIFY2(fuzzyRect(bottomRight->geometry(), bottomRightGeo),
             qPrintable(QStringLiteral("bottom right chart moved: actual=%1 before=%2")
                            .arg(rectText(bottomRight->geometry()), rectText(bottomRightGeo))));

    // 网格已退出：登记/比例接口返回 false 且不崩
    QVERIFY2(!fig->setGridCell(top, 0, 0), "setGridCell must fail once the grid is gone");
    QVERIFY(!fig->setGridSizes(QList< qreal > { 0.5, 0.5 }, QList< qreal > { 0.5, 0.5 }));

    // 子图编辑器（仅自由布局可用）恢复
    QVERIFY(!fig->isChartEditorActive());
    fig->beginChartEditor(DAFigureWidget::SubChartEditor);
    QApplication::processEvents();
    QVERIFY2(fig->isChartEditorActive(), "the sub chart editor should work again after leaving the grid");
    fig->endChartEditor();
    QApplication::processEvents();
    QVERIFY(!fig->isChartEditorActive());
}

/**
 * @brief 退出网格不入撤销栈；重复退出无副作用；再次启用从等分轨道开始
 */
void TSTDAFigureGridTest::testDisableGridLayoutNoUndoAndIdempotent()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    // 未启用网格：返回 false，不改变任何状态
    QVERIFY2(!fig->disableGridLayout(), "disabling without a grid should report false");
    QVERIFY(!fig->isGridLayoutEnabled());

    QVERIFY(fig->enableGridLayout(1, 2));
    DAChartWidget* left  = fig->createChart();
    DAChartWidget* right = fig->createChart();
    QVERIFY(left && right);
    QVERIFY(fig->setGridCell(left, 0, 0));
    QVERIFY(fig->setGridCell(right, 0, 1));
    settle(fig.data());

    QUndoStack* stack = fig->getUndoStack();
    QVERIFY(stack);
    const int undoCountBefore = stack->count();

    QSignalSpy gridSpy(fig.data(), &DAFigureWidget::gridLayoutEnabledChanged);
    QVERIFY(gridSpy.isValid());
    QVERIFY(fig->disableGridLayout());
    QCOMPARE(gridSpy.count(), 1);
    QCOMPARE(gridSpy.takeFirst().at(0).toBool(), false);
    QVERIFY2(stack->count() == undoCountBefore, "leaving the grid must not push an undo command");

    // 重复退出：false，且不再发信号、不改撤销栈
    QVERIFY(!fig->disableGridLayout());
    QCOMPARE(gridSpy.count(), 0);
    QCOMPARE(stack->count(), undoCountBefore);

    // 再次启用：等分轨道、无成员登记；原成员仍是自由成员
    QVERIFY(fig->enableGridLayout(2, 2));
    QVERIFY(fig->isGridLayoutEnabled());
    QCOMPARE(fig->grid()->rowCount(), 2);
    QCOMPARE(fig->grid()->columnCount(), 2);
    QVERIFY(fig->grid()->items().isEmpty());
    QCOMPARE(fig->getCharts().size(), 2);
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
 * @brief 尚未加入 figure 的 QwtPlot 通过 setGridCell 直接按网格坐标加入
 *
 * 覆盖 setGridCell 的「新 QwtPlot」分支（与 createChart() 后再登记的在 figure 分支不同）。
 */
void TSTDAFigureGridTest::testSetGridCellFreshPlot()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));

    QSignalSpy addedSpy(fig.data(), &DAFigureWidget::chartAdded);
    QVERIFY(addedSpy.isValid());
    // 新建但尚未加入 figure 的 chart
    DAChartWidget* chart = new DAChartWidget();
    QVERIFY2(chart->parentWidget() != fig->figure(), "a fresh chart should not be part of the figure yet");
    QVERIFY2(fig->setGridCell(chart, 0, 1), "setGridCell should add a chart that is not yet in the figure");
    QCOMPARE(chart->parentWidget(), fig->figure());
    QVERIFY(fig->hasPlot(chart, false));
    QCOMPARE(addedSpy.count(), 1);
    int row = -1, col = -1;
    QVERIFY(fig->grid()->gridPosition(chart, &row, &col, nullptr, nullptr));
    QCOMPARE(row, 0);
    QCOMPARE(col, 1);
    settle(fig.data());
    const QRect expected = expectedCellGeometry(fig.data(), 0, 1);
    QVERIFY2(fuzzyRect(chart->geometry(), expected),
             qPrintable(QStringLiteral("fresh chart actual=%1 expected=%2")
                            .arg(rectText(chart->geometry()), rectText(expected))));

    // 再移动到其它单元格：已在 figure 中，走网格重登记分支
    QVERIFY(fig->setGridCell(chart, 1, 0));
    settle(fig.data());
    const QRect moved = expectedCellGeometry(fig.data(), 1, 0);
    QVERIFY2(fuzzyRect(chart->geometry(), moved),
             qPrintable(QStringLiteral("moved fresh chart actual=%1 expected=%2")
                            .arg(rectText(chart->geometry()), rectText(moved))));
}

/**
 * @brief 非法单元格：返回 false，且保持原有网格登记与几何不变
 *
 * 回归保护：setGridCell 必须先校验目标单元格再注销旧登记，否则一次越界调用就会
 * 把子图踢出网格并让它按旧自由布局矩形叠压其它单元格。
 */
void TSTDAFigureGridTest::testSetGridCellInvalidKeepsRegistration()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    QWidget* plain = new QWidget();
    QVERIFY(fig->setGridCell(plain, 0, 0));
    settle(fig.data());
    const QRect before = plain->geometry();
    int row = -1, col = -1;
    QVERIFY(fig->grid()->gridPosition(plain, &row, &col, nullptr, nullptr));
    QCOMPARE(row, 0);
    QCOMPARE(col, 0);

    // 越界行列 / 非法跨度：全部返回 false
    QVERIFY(!fig->setGridCell(plain, 5, 5));
    QVERIFY(!fig->setGridCell(plain, 1, 1, 2, 1));  // 1 + 2 行 > 2
    QVERIFY(!fig->setGridCell(plain, 0, 0, 1, 3));  // 0 + 3 列 > 2
    QVERIFY(!fig->setGridCell(plain, -1, 0));
    QVERIFY(!fig->setGridCell(plain, 0, 0, 0, 1));
    // 原有登记与几何保持不变
    QVERIFY(fig->grid()->gridPosition(plain, &row, &col, nullptr, nullptr));
    QCOMPARE(row, 0);
    QCOMPARE(col, 0);
    QCOMPARE(plain->geometry(), before);
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
    if (!canUseOpenGL()) {
        QSKIP("OpenGL is not available: skip the 3D grid test");
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

/**
 * @brief 3D 与 2D 相邻时，分隔条由 2D 一侧负责命中并可真实拖动
 *
 * 分隔条从不拦截 3D（QOpenGLWidget）成员的事件：边界处的鼠标事件落在相邻的 2D 子图上
 * 时照常命中。用例把 3D 放左上格、2D 放右上格，从 2D 子图向 3D 一侧拖动列分隔条，
 * 断言两条轨道比例变化、3D 与 2D 子图几何同步跟随（3D 侧不需要也不应收到该事件）。
 */
void TSTDAFigureGridTest::testChart3DAdjacent2DDragDivider()
{
    if (!canUseOpenGL()) {
        QSKIP("OpenGL is not available: skip the 3D grid test");
    }

    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));

    // 左上：3D 子图；右上：2D 子图——两个单元格之间就是列分隔条
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
    QVERIFY2(fig->setGridCell(chart3d, 0, 0), "the 3D chart should be registrable in the grid");

    DAChartWidget* chart2d = fig->createChart();
    QVERIFY(chart2d);
    QVERIFY(fig->setGridCell(chart2d, 0, 1));
    settle(fig.data(), 60);

    QwtFigure* qfig = fig->figure();
    QVERIFY(qfig);
    const int figureWidth = qfig->width();
    QVERIFY(figureWidth > 200);
    QCOMPARE(fig->gridColumnSizes().size(), 2);
    QVERIFY(fuzzyEqual(fig->gridColumnSizes().at(0), 0.5));

    const QRect cell3dBefore = chart3d->geometry();
    const QRect cell2dBefore = chart2d->geometry();

    // 列分隔条在 0.5 处（等分轨道）：在 2D 一侧的命中带内按下（+2px，落在 2D 单元格里）
    const int dividerX = qRound(0.5 * figureWidth);
    const int pressY   = qfig->height() / 4;
    const QPoint pressFigurePos(dividerX + 2, pressY);
    QVERIFY2(chart2d->rect().contains(chart2d->mapFrom(qfig, pressFigurePos)),
             "the press point must be inside the 2D cell");

    const auto sendTo2D = [&](QEvent::Type type, const QPoint& figurePos, Qt::MouseButton button, Qt::MouseButtons buttons) {
        const QPoint localPos = chart2d->mapFrom(qfig, figurePos);
        QMouseEvent event(type,
                          QPointF(localPos),
                          QPointF(chart2d->mapToGlobal(localPos)),
                          button,
                          buttons,
                          Qt::NoModifier);
        QApplication::sendEvent(chart2d, &event);
        return event.isAccepted();
    };

    QVERIFY2(sendTo2D(QEvent::MouseButtonPress, pressFigurePos, Qt::LeftButton, Qt::LeftButton),
             "the divider written on the 2D side must consume the press");

    // 向 3D 一侧拖动：两列比例随之变化，且 3D / 2D 子图几何同步跟随
    const int dragDelta = 30;
    const QPoint dragFigurePos(pressFigurePos.x() - dragDelta, pressY);
    QVERIFY(sendTo2D(QEvent::MouseMove, dragFigurePos, Qt::NoButton, Qt::LeftButton));

    const qreal expectedFirst = 0.5 - static_cast< qreal >(dragDelta) / figureWidth;
    QVERIFY2(fuzzyEqual(fig->gridColumnSizes().at(0), expectedFirst),
             qPrintable(QStringLiteral("first column should be %1, got %2")
                            .arg(expectedFirst)
                            .arg(fig->gridColumnSizes().at(0))));
    QVERIFY(fuzzyEqual(fig->gridColumnSizes().at(1), 1.0 - expectedFirst));

    const QRect expected3d = expectedCellGeometry(fig.data(), 0, 0);
    const QRect expected2d = expectedCellGeometry(fig.data(), 0, 1);
    QVERIFY2(fuzzyRect(chart3d->geometry(), expected3d),
             qPrintable(QStringLiteral("3D chart actual=%1 expected=%2")
                            .arg(rectText(chart3d->geometry()), rectText(expected3d))));
    QVERIFY2(fuzzyRect(chart2d->geometry(), expected2d),
             qPrintable(QStringLiteral("2D chart actual=%1 expected=%2")
                            .arg(rectText(chart2d->geometry()), rectText(expected2d))));
    // 3D 单元格确实变窄了（跟随拖动，而不是留在原位）
    QVERIFY2(chart3d->geometry().width() < cell3dBefore.width(),
             qPrintable(QStringLiteral("3D chart width %1 should shrink from %2")
                            .arg(chart3d->geometry().width())
                            .arg(cell3dBefore.width())));
    QVERIFY(cell2dBefore.width() != chart2d->geometry().width());

    QVERIFY(sendTo2D(QEvent::MouseButtonRelease, dragFigurePos, Qt::LeftButton, Qt::NoButton));
}

/**
 * @brief 创建子图的撤销命令在网格 figure 上按单元格恢复
 *
 * 网格 figure 上新建的子图必须先登记到单元格，否则会作为自由成员按传入矩形压在网格成员之上；
 * undo→redo 必须回到同一单元格，且一次操作只压入一条命令。
 */
void TSTDAFigureGridTest::testCreateChartCommandKeepsGridCell()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    settle(fig.data());

    QUndoStack* stack = fig->getUndoStack();
    QVERIFY(stack);
    QCOMPARE(stack->count(), 0);

    // 传入一个与网格单元格不对齐的自由矩形：修复前它会成为自由成员叠压在其他成员之上
    DAChartWidget* chart = fig->createChart_(QRectF(0.55, 0.55, 0.4, 0.4));
    QVERIFY(chart);
    QCOMPARE(stack->count(), 1);
    settle(fig.data());

    int row = -1;
    int col = -1;
    int rowSpan = 1;
    int colSpan = 1;
    QVERIFY2(fig->grid()->gridPosition(chart, &row, &col, &rowSpan, &colSpan),
             "a chart created on a grid figure must be registered as a grid member");
    QCOMPARE(row, 0);
    QCOMPARE(col, 0);
    QVERIFY2(fuzzyRect(chart->geometry(), expectedCellGeometry(fig.data(), row, col, rowSpan, colSpan)),
             qPrintable(QStringLiteral("chart geometry %1 should match cell (0,0) geometry %2")
                            .arg(rectText(chart->geometry()),
                                 rectText(expectedCellGeometry(fig.data(), row, col, rowSpan, colSpan)))));

    // undo：子图从 figure 移除
    stack->undo();
    settle(fig.data());
    QCOMPARE(fig->getChartCount(), 0);

    // redo：回到同一单元格，仍不是自由成员
    stack->redo();
    settle(fig.data());
    QCOMPARE(fig->getChartCount(), 1);
    int row2 = -1;
    int col2 = -1;
    QVERIFY2(fig->grid()->gridPosition(chart, &row2, &col2, nullptr, nullptr),
             "redo must restore the chart as a grid member");
    QCOMPARE(row2, 0);
    QCOMPARE(col2, 0);
    QVERIFY2(fuzzyRect(chart->geometry(), expectedCellGeometry(fig.data(), 0, 0)),
             qPrintable(QStringLiteral("after redo the chart geometry %1 should still match cell (0,0) %2")
                            .arg(rectText(chart->geometry()), rectText(expectedCellGeometry(fig.data(), 0, 0)))));
    QCOMPARE(stack->count(), 1);
}

/**
 * @brief 移除子图的撤销命令 undo 后回到原单元格
 */
void TSTDAFigureGridTest::testRemoveChartCommandKeepsGridCell()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    settle(fig.data());

    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    QVERIFY(fig->setGridCell(chart, 1, 1));
    settle(fig.data());

    QUndoStack* stack = fig->getUndoStack();
    QVERIFY(stack);
    const int baseCount = stack->count();
    fig->push(new DAFigureWidgetCommandRemoveChart(fig.data(), chart));
    QCOMPARE(stack->count(), baseCount + 1);
    QCOMPARE(fig->getChartCount(), 0);

    // undo：按原单元格恢复（不是自由布局的归一化矩形）
    stack->undo();
    settle(fig.data());
    QCOMPARE(fig->getChartCount(), 1);
    int row = -1;
    int col = -1;
    QVERIFY2(fig->grid()->gridPosition(chart, &row, &col, nullptr, nullptr),
             "undoing a removal must restore the chart as a grid member");
    QCOMPARE(row, 1);
    QCOMPARE(col, 1);
    QVERIFY2(fuzzyRect(chart->geometry(), expectedCellGeometry(fig.data(), 1, 1)),
             qPrintable(QStringLiteral("restored chart geometry %1 should match cell (1,1) %2")
                            .arg(rectText(chart->geometry()), rectText(expectedCellGeometry(fig.data(), 1, 1)))));

    // redo 再次移除、再 undo 仍回同一单元格
    stack->redo();
    QCOMPARE(fig->getChartCount(), 0);
    stack->undo();
    settle(fig.data());
    QCOMPARE(fig->getChartCount(), 1);
    row = -1;
    col = -1;
    QVERIFY(fig->grid()->gridPosition(chart, &row, &col, nullptr, nullptr));
    QCOMPARE(row, 1);
    QCOMPARE(col, 1);
}

/**
 * @brief 调整子图尺寸的撤销命令在网格成员上按单元格恢复
 *
 * 网格成员的几何由轨道比例决定，resize 命令不能再写自由归一化矩形；
 * redo 应把成员移动到新矩形对应的单元格，undo 回到原单元格。
 */
void TSTDAFigureGridTest::testResizeWidgetCommandKeepsGridCell()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    QVERIFY(fig->enableGridLayout(2, 2));
    settle(fig.data());

    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    QVERIFY(fig->setGridCell(chart, 0, 0));
    settle(fig.data());

    const QRectF oldRect = fig->axesNormRect(chart);
    const QRectF newRect(0.55, 0.55, 0.4, 0.4);  // 对应单元格 (1,1)
    QUndoStack* stack = fig->getUndoStack();
    QVERIFY(stack);
    const int baseCount = stack->count();
    fig->push(new DAFigureWidgetCommandResizeWidget(fig.data(), chart, oldRect, newRect));
    QCOMPARE(stack->count(), baseCount + 1);
    settle(fig.data());

    int row = -1;
    int col = -1;
    QVERIFY2(fig->grid()->gridPosition(chart, &row, &col, nullptr, nullptr),
             "resizing a grid member must keep it a grid member");
    QCOMPARE(row, 1);
    QCOMPARE(col, 1);

    // undo：回到原单元格 (0,0)
    stack->undo();
    settle(fig.data());
    row = -1;
    col = -1;
    QVERIFY(fig->grid()->gridPosition(chart, &row, &col, nullptr, nullptr));
    QCOMPARE(row, 0);
    QCOMPARE(col, 0);
    QVERIFY2(fuzzyRect(chart->geometry(), expectedCellGeometry(fig.data(), 0, 0)),
             qPrintable(QStringLiteral("after undo the chart geometry %1 should match cell (0,0) %2")
                            .arg(rectText(chart->geometry()), rectText(expectedCellGeometry(fig.data(), 0, 0)))));
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
