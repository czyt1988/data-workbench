
#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QUndoStack>

#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartElementHitTester.h"
#include "DAFigureElementSelection.h"
#include "DAFigurePointerSelectorOverlay.h"

// qwt
#include "qwt_plot_curve.h"
#include "qwt_plot_arrowmarker.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_scale_map.h"
#include "qwt_point_data.h"
#include "qwt_axis_id.h"

namespace DA
{
/**
 * @brief 指针工具（PointerSelector）交互逻辑测试
 *
 * 覆盖的改动点：
 * 1. 首次点击非激活子图 = 切换激活 + 选中元素（不再需要点两次）
 * 2. 箭头命中与选中框：StartLengthAngle 模式下绘制终点在 canvas 像素空间偏移，
 *    命中测试需与之一致；选中框用绘制几何构造紧凑矩形
 * 3. 曲线选中高亮为沿数据采样点的路径（itemSelectionPath），
 *    替代整条曲线包围盒矩形
 * 4. 拖动平移为逐事件增量（多次 mouseMove 不累加放大位移）
 * 5. 形状类（矩形/椭圆/多边形选区）可通过指针工具拖动，undo 可回弹
 */
class TSTDAFigurePointerToolTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 首次点击非激活子图即可选中曲线
    void testFirstClickOnInactivePlotSelectsCurve();
    // 箭头 ExplicitPoints 模式：线段距离命中
    void testArrowExplicitPointsHit();
    // 箭头 StartLengthAngle 模式：命中与 draw() 一致（canvas 像素空间终点）
    void testArrowStartLengthAngleHit();
    // 箭头选中框：由起终点与端点尺寸构造（非 boundingRect 量纲错误版本）
    void testArrowSelectionRectCompact();
    // 曲线选中路径：沿采样点连线，宽度远小于包围盒
    void testCurveSelectionPath();
    // 散点曲线选中路径：NoCurve 样式画小圆圈
    void testScatterSelectionPath();
    // 非曲线类型返回空路径
    void testNonCurveSelectionPathEmpty();
    // 箭头拖动：多次 mouseMove 逐事件增量平移，总位移等于鼠标位移
    void testArrowDragIncrementalMove();
    // 形状类（矩形选区）可通过指针工具拖动
    void testShapeDragMove();
    // 形状类拖动后 undo 回弹到原位置
    void testShapeDragUndoRestore();

private:
    // 生成测试用正弦曲线数据
    static QVector< double > makeSineX(int n);
    static QVector< double > makeSineY(int n);
    // 把 canvas 像素坐标换算为 figure 坐标后对 figure widget 发点击
    void clickCanvas(DAChartWidget* chart, const QPoint& canvasPos);
    // 把 canvas 像素坐标换算为 figure 坐标后对 figure widget 发拖动
    //（press → 若干段 move → release，move 段数由 steps 决定）
    void dragCanvas(DAChartWidget* chart, const QPoint& fromCanvasPos, const QPoint& toCanvasPos, int steps = 5);

    QApplication* mApp { nullptr };
    DAFigureWidget* mFig { nullptr };
};
}  // namespace DA

//===================================================
// 实现
//===================================================
namespace DA
{
void TSTDAFigurePointerToolTest::initTestCase()
{
    mFig = new DAFigureWidget();
    mFig->resize(1000, 800);
    mFig->show();
    QVERIFY(QTest::qWaitForWindowExposed(mFig));
}

void TSTDAFigurePointerToolTest::cleanupTestCase()
{
    if (mFig) {
        delete mFig;
        mFig = nullptr;
    }
}

void TSTDAFigurePointerToolTest::init()
{
}

void TSTDAFigurePointerToolTest::cleanup()
{
    // 每个用例后退出编辑模式，避免编辑器间互相干扰
    if (mFig) {
        mFig->endChartEditor();
    }
}

QVector< double > TSTDAFigurePointerToolTest::makeSineX(int n)
{
    QVector< double > x;
    x.reserve(n);
    for (int i = 0; i < n; ++i) {
        x.push_back(i * 0.2);
    }
    return x;
}

QVector< double > TSTDAFigurePointerToolTest::makeSineY(int n)
{
    QVector< double > y;
    y.reserve(n);
    for (int i = 0; i < n; ++i) {
        y.push_back(std::sin(i * 0.2) * 3.0 + 5.0);
    }
    return y;
}

void TSTDAFigurePointerToolTest::clickCanvas(DAChartWidget* chart, const QPoint& canvasPos)
{
    // canvas 像素坐标 → figure 本地坐标，直接向指针 overlay 发 QMouseEvent：
    // overlay 是 figure 的子控件，运行期拦截全部鼠标事件，QTest 发给 figure 本身不会到达它
    const QPoint figPos = chart->canvas()->mapTo(mFig, canvasPos);
    DAFigurePointerSelectorOverlay* overlay = mFig->findChild< DAFigurePointerSelectorOverlay* >();
    QVERIFY2(overlay, "pointer selector overlay should exist after beginChartEditor");
    QMouseEvent press(QEvent::MouseButtonPress, figPos, mFig->mapToGlobal(figPos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(overlay, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, figPos, mFig->mapToGlobal(figPos), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(overlay, &release);
}

void TSTDAFigurePointerToolTest::dragCanvas(DAChartWidget* chart, const QPoint& fromCanvasPos, const QPoint& toCanvasPos, int steps)
{
    // 与 clickCanvas 相同的坐标换算；move 拆成多段，模拟真实拖动的连续 mouseMove 流
    DAFigurePointerSelectorOverlay* overlay = mFig->findChild< DAFigurePointerSelectorOverlay* >();
    QVERIFY2(overlay, "pointer selector overlay should exist after beginChartEditor");
    const auto toFig = [ chart, this ](const QPoint & canvasPos) {
        return chart->canvas()->mapTo(mFig, canvasPos);
    };
    const QPoint fromFig = toFig(fromCanvasPos);
    QMouseEvent press(QEvent::MouseButtonPress, fromFig, mFig->mapToGlobal(fromFig), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(overlay, &press);
    const int n = qMax(1, steps);
    for (int i = 1; i <= n; ++i) {
        const QPoint canvasPos = fromCanvasPos + (toCanvasPos - fromCanvasPos) * i / n;
        const QPoint figPos    = toFig(canvasPos);
        QMouseEvent move(QEvent::MouseMove, figPos, mFig->mapToGlobal(figPos), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(overlay, &move);
        QApplication::processEvents();
    }
    const QPoint toFig2 = toFig(toCanvasPos);
    QMouseEvent release(
        QEvent::MouseButtonRelease, toFig2, mFig->mapToGlobal(toFig2), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(overlay, &release);
    QApplication::processEvents();
}

/**
 * @brief 首次点击非激活子图 = 切换激活 + 选中曲线
 *
 * 两个子图，激活的是 chart1；单击 chart2 中的曲线，
 * 期望 figureElementClicked 信号携带 SelectPlotItem 且 plotItem 是 chart2 的曲线
 */
void TSTDAFigurePointerToolTest::testFirstClickOnInactivePlotSelectsCurve()
{
    DAChartWidget* chart1 = mFig->createChart_(QRectF(0.07, 0.07, 0.4, 0.4));
    DAChartWidget* chart2 = mFig->createChart_(QRectF(0.53, 0.07, 0.4, 0.4));
    QVERIFY(chart1 && chart2);
    chart1->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart1->setAxisScale(QwtAxis::YLeft, 0, 100);
    chart2->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart2->setAxisScale(QwtAxis::YLeft, 0, 100);
    QwtPlotCurve* c1 = chart1->addCurve(makeSineX(50), makeSineY(50), "curve1");
    QwtPlotCurve* c2 = chart2->addCurve(makeSineX(50), makeSineY(50), "curve2");
    QVERIFY(c1 && c2);
    // 激活 chart1
    mFig->setCurrentChart(chart1);
    mFig->show();
    QApplication::processEvents();
    // 进入指针模式
    mFig->beginChartEditor(DAFigureWidget::PointerSelector);
    QApplication::processEvents();
    QSignalSpy spy(mFig, &DAFigureWidget::figureElementClicked);
    // 单击 chart2 中曲线中点（canvas 中央即曲线必经区域附近取一个采样点）
    const QPointF sample = c2->sample(25);
    const QwtScaleMap& xMap = chart2->canvasMap(c2->xAxis());
    const QwtScaleMap& yMap = chart2->canvasMap(c2->yAxis());
    const QPoint canvasPos(static_cast< int >(xMap.transform(sample.x())),
                           static_cast< int >(yMap.transform(sample.y())));
    clickCanvas(chart2, canvasPos);
    QApplication::processEvents();
    QVERIFY2(spy.count() >= 1, "figureElementClicked should be emitted on first click");
    if (spy.count() >= 1) {
        const DAFigureElementSelection sel = spy.at(0).at(0).value< DAFigureElementSelection >();
        QVERIFY2(sel.isSelectedPlotItem(), "first click on inactive plot should select the plot item");
        QCOMPARE(sel.plotItem, static_cast< QwtPlotItem* >(c2));
    }
    // 清理
    mFig->endChartEditor();
    chart1->removePlotItem(c1);
    chart2->removePlotItem(c2);
    mFig->removeChart(chart1);
    mFig->removeChart(chart2);
}

/**
 * @brief 箭头 ExplicitPoints 模式命中
 */
void TSTDAFigurePointerToolTest::testArrowExplicitPointsHit()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    chart->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart->setAxisScale(QwtAxis::YLeft, 0, 100);
    QwtPlotArrowMarker* arrow = new QwtPlotArrowMarker();
    arrow->setPoints(QPointF(20, 50), QPointF(80, 50));
    arrow->attach(chart);
    mFig->show();
    QApplication::processEvents();
    // 命中线段中点
    const QwtScaleMap& xMap = chart->canvasMap(arrow->xAxis());
    const QwtScaleMap& yMap = chart->canvasMap(arrow->yAxis());
    const QPoint mid(static_cast< int >(xMap.transform(50)), static_cast< int >(yMap.transform(50)));
    QVERIFY(DAChartElementHitTester::itemAt(chart, mid) == arrow);
    // 偏移 20px 不命中（默认容差 8px）
    const QPoint farPos(mid.x(), mid.y() - 20);
    QVERIFY(DAChartElementHitTester::itemAt(chart, farPos) != arrow);
    arrow->detach();
    delete arrow;
    mFig->removeChart(chart);
}

/**
 * @brief 箭头 StartLengthAngle 模式命中与 draw() 一致
 *
 * draw() 在 canvas 像素空间计算终点：canvasEnd = canvasStart + (L·cos, -L·sin)。
 * 命中测试必须命中该绘制线段，而不是 endPoint()（数据坐标 + 像素长度，错位）
 */
void TSTDAFigurePointerToolTest::testArrowStartLengthAngleHit()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    // 数据范围 0-1000，制造"数据坐标 1 单位 ≈ 0.5px"的窄比例尺，放大两种算法的差异
    chart->setAxisScale(QwtAxis::XBottom, 0, 1000);
    chart->setAxisScale(QwtAxis::YLeft, 0, 1000);
    QwtPlotArrowMarker* arrow = new QwtPlotArrowMarker();
    arrow->setStartPoint(QPointF(500, 500));
    arrow->setLength(120);   // canvas 像素长度
    arrow->setAngle(0);      // 水平向右
    arrow->setPositionMode(QwtPlotArrowMarker::StartLengthAngle);
    arrow->attach(chart);
    mFig->show();
    QApplication::processEvents();
    const QwtScaleMap& xMap = chart->canvasMap(arrow->xAxis());
    const QwtScaleMap& yMap = chart->canvasMap(arrow->yAxis());
    // 绘制线段：canvas 起点 + 像素偏移 (120, 0)
    const QPointF ps(xMap.transform(500), yMap.transform(500));
    const QPointF pe = ps + QPointF(120, 0);  // draw() 的终点
    // 1) 命中绘制线段中点（draw 画的位置）
    const QPoint midDraw = ((ps + pe) / 2.0).toPoint() + QPoint(0, 2);
    QVERIFY2(DAChartElementHitTester::itemAt(chart, midDraw) == arrow,
             "hit should match the drawn segment in canvas pixel space");
    // 2) 旧算法（endPoint() 经 canvasMap 变换）的位置——起点数据坐标 + (120, 0) 数据偏移，
    //    在 0-1000 范围下几乎重合于起点，远离绘制终点，不应成为唯一可命中区域
    const QPointF oldPe(xMap.transform(620), yMap.transform(500));
    Q_UNUSED(oldPe);
    // 3) 偏离绘制线段 20px 不命中
    const QPoint farPos((ps + QPointF(60, 0)).toPoint() + QPoint(0, -20));
    QVERIFY(DAChartElementHitTester::itemAt(chart, farPos) != arrow);
    arrow->detach();
    delete arrow;
    mFig->removeChart(chart);
}

/**
 * @brief 箭头选中框紧凑性
 *
 * 选中框应覆盖起终点 + 端点图形半径，不使用 boundingRect()
 * （后者把像素 margin 加在数据坐标上，量纲错误，窄比例尺下会异常巨大）
 */
void TSTDAFigurePointerToolTest::testArrowSelectionRectCompact()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    chart->setAxisScale(QwtAxis::XBottom, 0, 1000);
    chart->setAxisScale(QwtAxis::YLeft, 0, 1000);
    QwtPlotArrowMarker* arrow = new QwtPlotArrowMarker();
    arrow->setPoints(QPointF(400, 500), QPointF(600, 500));
    arrow->attach(chart);
    mFig->show();
    QApplication::processEvents();
    const QRect selRect = DAChartElementHitTester::itemSelectionRect(chart, arrow);
    QVERIFY(!selRect.isNull());
    // 选中框应包含绘制线段（起终点 canvas 位置）
    const QwtScaleMap& xMap = chart->canvasMap(arrow->xAxis());
    const QwtScaleMap& yMap = chart->canvasMap(arrow->yAxis());
    const QPoint ps(static_cast< int >(xMap.transform(400)), static_cast< int >(yMap.transform(500)));
    const QPoint pe(static_cast< int >(xMap.transform(600)), static_cast< int >(yMap.transform(500)));
    QVERIFY(selRect.contains(ps));
    QVERIFY(selRect.contains(pe));
    // 紧凑性：高度不应超过 2*(端点半径+margin+线宽)，远小于 canvas 高度
    const int canvasH = chart->canvas()->contentsRect().height();
    QVERIFY2(selRect.height() < canvasH / 4,
             "arrow selection rect should be compact, not derived from data-space boundingRect");
    arrow->detach();
    delete arrow;
    mFig->removeChart(chart);
}

/**
 * @brief 曲线选中路径：沿采样点连线
 */
void TSTDAFigurePointerToolTest::testCurveSelectionPath()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    chart->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart->setAxisScale(QwtAxis::YLeft, 0, 100);
    QwtPlotCurve* curve = chart->addCurve(makeSineX(50), makeSineY(50), "curve");
    QVERIFY(curve);
    mFig->show();
    QApplication::processEvents();
    const QPainterPath path = DAChartElementHitTester::itemSelectionPath(chart, curve);
    QVERIFY2(!path.isEmpty(), "curve selection path should not be empty");
    const QRectF pathBr = path.boundingRect();
    // 正弦曲线 y 范围约 [2, 8]，canvas 高度 800*0.86≈600px+；
    // 路径高度应远小于"整条曲线包围盒"（后者接近整个 canvas 高度）
    const int canvasH = chart->canvas()->contentsRect().height();
    QVERIFY2(pathBr.height() < canvasH * 0.6,
             "path should trace the curve, far smaller than full-canvas bounding box");
    // 路径应经过采样点（取中间点验证）
    const QPointF sample = curve->sample(25);
    const QwtScaleMap& xMap = chart->canvasMap(curve->xAxis());
    const QwtScaleMap& yMap = chart->canvasMap(curve->yAxis());
    const QPointF pt(xMap.transform(sample.x()), yMap.transform(sample.y()));
    QVERIFY2(pathBr.contains(pt), "path bounding rect should cover sample points");
    chart->removePlotItem(curve);
    mFig->removeChart(chart);
}

/**
 * @brief 散点曲线（NoCurve）选中路径：小圆圈集合
 */
void TSTDAFigurePointerToolTest::testScatterSelectionPath()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    chart->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart->setAxisScale(QwtAxis::YLeft, 0, 100);
    QwtPlotCurve* curve = chart->addCurve(makeSineX(50), makeSineY(50), "scatter");
    QVERIFY(curve);
    curve->setStyle(QwtPlotCurve::NoCurve);
    mFig->show();
    QApplication::processEvents();
    const QPainterPath path = DAChartElementHitTester::itemSelectionPath(chart, curve);
    QVERIFY2(!path.isEmpty(), "scatter selection path should not be empty");
    // 散点模式画圆圈，路径应由多个子路径组成
    QVERIFY2(path.elementCount() > 100, "scatter path should contain many ellipse subpaths");
    chart->removePlotItem(curve);
    mFig->removeChart(chart);
}

/**
 * @brief 非曲线类型返回空路径（走矩形选中框）
 */
void TSTDAFigurePointerToolTest::testNonCurveSelectionPathEmpty()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    QwtPlotArrowMarker* arrow = new QwtPlotArrowMarker();
    arrow->setPoints(QPointF(20, 50), QPointF(80, 50));
    arrow->attach(chart);
    mFig->show();
    QApplication::processEvents();
    const QPainterPath path = DAChartElementHitTester::itemSelectionPath(chart, arrow);
    QVERIFY2(path.isEmpty(), "non-curve item should have empty selection path (use rect)");
    arrow->detach();
    delete arrow;
    mFig->removeChart(chart);
}

/**
 * @brief 箭头拖动：多次 mouseMove 逐事件增量平移
 *
 * 回归测试：旧实现每次 move 都用"按下点 → 当前点"的总位移平移当前几何，
 * 首个 move 后总位移被反复叠加，箭头移动远快于鼠标。
 * 期望：拖动结束后箭头起终点的位移与鼠标位移（换算为数据坐标）一致
 */
void TSTDAFigurePointerToolTest::testArrowDragIncrementalMove()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    chart->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart->setAxisScale(QwtAxis::YLeft, 0, 100);
    QwtPlotArrowMarker* arrow = new QwtPlotArrowMarker();
    arrow->setPoints(QPointF(20, 50), QPointF(60, 50));
    arrow->attach(chart);
    mFig->setCurrentChart(chart);
    mFig->show();
    QApplication::processEvents();
    mFig->beginChartEditor(DAFigureWidget::PointerSelector);
    QApplication::processEvents();
    // 第一次点击选中箭头，第二次按住线段中点拖动
    const QwtScaleMap& xMap = chart->canvasMap(arrow->xAxis());
    const QwtScaleMap& yMap = chart->canvasMap(arrow->yAxis());
    const QPoint midStart(static_cast< int >(xMap.transform(40)), static_cast< int >(yMap.transform(50)));
    clickCanvas(chart, midStart);
    QApplication::processEvents();
    // 鼠标向右下移动 (15, 10) 像素，拆成 5 段 move
    const QPoint midEnd = midStart + QPoint(15, 10);
    dragCanvas(chart, midStart, midEnd, 5);
    QApplication::processEvents();
    // 期望终点：起/终点各偏移鼠标位移对应的数据坐标增量
    const double dxData = xMap.invTransform(midEnd.x()) - xMap.invTransform(midStart.x());
    const double dyData = yMap.invTransform(midEnd.y()) - yMap.invTransform(midStart.y());
    QVERIFY2(qAbs(arrow->startPoint().x() - (20 + dxData)) < 1e-6 && qAbs(arrow->startPoint().y() - (50 + dyData)) < 1e-6,
             "arrow start point should move exactly by mouse delta");
    QVERIFY2(qAbs(arrow->endPoint().x() - (60 + dxData)) < 1e-6 && qAbs(arrow->endPoint().y() - (50 + dyData)) < 1e-6,
             "arrow end point should move exactly by mouse delta");
    // undo 回到拖动前
    mFig->getUndoStack()->undo();
    QApplication::processEvents();
    QVERIFY2(qAbs(arrow->startPoint().x() - 20) < 1e-6 && qAbs(arrow->endPoint().x() - 60) < 1e-6,
             "undo should restore arrow position");
    mFig->endChartEditor();
    arrow->detach();
    delete arrow;
    mFig->removeChart(chart);
}

/**
 * @brief 形状类（矩形选区）可通过指针工具拖动
 *
 * 矩形/椭圆/多边形工具产出的都是 QwtPlotShapeItem（DAChartSelectRegionShapeItem），
 * 期望：isMovableItem 为 true；拖动后 path 平移鼠标位移；undo 回弹
 */
void TSTDAFigurePointerToolTest::testShapeDragMove()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    chart->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart->setAxisScale(QwtAxis::YLeft, 0, 100);
    QwtPlotShapeItem* shape = new QwtPlotShapeItem("region");
    QPainterPath path;
    path.addRect(QRectF(20, 30, 40, 20));
    shape->setShape(path);
    shape->attach(chart);
    mFig->setCurrentChart(chart);
    mFig->show();
    QApplication::processEvents();
    QVERIFY2(DAChartElementHitTester::isMovableItem(shape), "shape item should be movable by pointer tool");
    mFig->beginChartEditor(DAFigureWidget::PointerSelector);
    QApplication::processEvents();
    const QwtScaleMap& xMap = chart->canvasMap(shape->xAxis());
    const QwtScaleMap& yMap = chart->canvasMap(shape->yAxis());
    // 第一次点击选中（矩形中心），第二次按住同一点拖动
    const QPoint center(static_cast< int >(xMap.transform(40)), static_cast< int >(yMap.transform(40)));
    clickCanvas(chart, center);
    QApplication::processEvents();
    const QPoint target = center + QPoint(12, -8);
    dragCanvas(chart, center, target, 4);
    QApplication::processEvents();
    // path 包围盒应整体平移鼠标位移（宽高不变）
    const QRectF brAfter = shape->shape().boundingRect();
    const double dxData  = xMap.invTransform(target.x()) - xMap.invTransform(center.x());
    const double dyData  = yMap.invTransform(target.y()) - yMap.invTransform(center.y());
    QVERIFY2(qAbs(brAfter.left() - (20 + dxData)) < 1e-6 && qAbs(brAfter.top() - (30 + dyData)) < 1e-6,
             "shape bounding rect should translate by mouse delta");
    QVERIFY2(qAbs(brAfter.width() - 40) < 1e-6 && qAbs(brAfter.height() - 20) < 1e-6,
             "shape size should not change during move");
    mFig->endChartEditor();
    shape->detach();
    delete shape;
    mFig->removeChart(chart);
}

/**
 * @brief 形状类拖动后 undo 回弹到原位置
 */
void TSTDAFigurePointerToolTest::testShapeDragUndoRestore()
{
    DAChartWidget* chart = mFig->createChart_(QRectF(0.07, 0.07, 0.86, 0.86));
    QVERIFY(chart);
    chart->setAxisScale(QwtAxis::XBottom, 0, 100);
    chart->setAxisScale(QwtAxis::YLeft, 0, 100);
    QwtPlotShapeItem* shape = new QwtPlotShapeItem("region");
    QPainterPath path;
    path.addEllipse(QRectF(30, 30, 30, 30));
    shape->setShape(path);
    shape->attach(chart);
    mFig->setCurrentChart(chart);
    mFig->show();
    QApplication::processEvents();
    mFig->beginChartEditor(DAFigureWidget::PointerSelector);
    QApplication::processEvents();
    const QwtScaleMap& xMap = chart->canvasMap(shape->xAxis());
    const QwtScaleMap& yMap = chart->canvasMap(shape->yAxis());
    const QPoint center(static_cast< int >(xMap.transform(45)), static_cast< int >(yMap.transform(45)));
    clickCanvas(chart, center);
    QApplication::processEvents();
    dragCanvas(chart, center, center + QPoint(10, 10), 3);
    QApplication::processEvents();
    // 拖动后 undo：path 应回到初始椭圆
    mFig->getUndoStack()->undo();
    QApplication::processEvents();
    const QRectF br = shape->shape().boundingRect();
    QVERIFY2(qAbs(br.left() - 30) < 1e-6 && qAbs(br.top() - 30) < 1e-6 && qAbs(br.width() - 30) < 1e-6
                     && qAbs(br.height() - 30) < 1e-6,
             "undo should restore the ellipse to its original geometry");
    mFig->endChartEditor();
    shape->detach();
    delete shape;
    mFig->removeChart(chart);
}


}  // namespace DA

//===================================================
// main
//===================================================
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DA::TSTDAFigurePointerToolTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
