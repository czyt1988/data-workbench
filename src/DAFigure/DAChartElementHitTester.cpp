#include "DAChartElementHitTester.h"
// Qt
#include <QApplication>
#include <QWidget>
#include <QPainterPath>
// qwt
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_arrowmarker.h"
#include "qwt_plot_textlabel.h"
#include "qwt_plot_legenditem.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_map.h"
#include "qwt_symbol.h"
#include "qwt_text.h"
#include "qwt_text_label.h"
// DA
#include "DADataProbeMarker.h"
#include "DAChartTextMarker.h"

namespace DA
{
//===================================================
// 工具函数（匿名命名空间）
//===================================================
namespace
{
/**
 * @brief 计算点到线段的距离平方
 */
double distanceSquareToPointSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF ab   = b - a;
    const QPointF ap   = p - a;
    const double denom = ab.x() * ab.x() + ab.y() * ab.y();
    if (qFuzzyIsNull(denom)) {
        // 线段退化为点
        return ap.x() * ap.x() + ap.y() * ap.y();
    }
    double t = (ap.x() * ab.x() + ap.y() * ab.y()) / denom;
    t        = qBound(0.0, t, 1.0);
    const QPointF closest = a + t * ab;
    const QPointF d       = p - closest;
    return d.x() * d.x() + d.y() * d.y();
}

/**
 * @brief 获取 QwtText 的渲染字体
 *
 * QwtText 未显式设置字体时返回无效字体，用应用字体兜底
 */
QFont textRenderFont(const QwtText& text)
{
    QFont f = text.font();
    if (!f.family().isEmpty() && f.pointSize() > 0) {
        return f;
    }
    return QApplication::font();
}
}  // namespace

//===================================================
// DAChartElementHitTester::PlotHitResult / 区域命中
//===================================================

/**
 * @brief 对 plot 本地坐标做区域命中测试
 *
 * 判定顺序：标题 → footer → 坐标轴 → canvas → 无。
 * 标题/footer 用 label 控件的 geometry 判定；坐标轴用 QwtScaleWidget::isOnScale
 * 判定纯刻度区（排除轴标题/色条等边缘区域），轴命中前按 Y 左、Y 右、X 底、X 顶顺序遍历。
 * @param plot 目标绘图
 * @param plotLocalPos plot 本地坐标（plot->mapFromGlobal 换算所得）
 * @return 命中结果
 */
DAChartElementHitTester::PlotHitResult DAChartElementHitTester::hitTestPlot(QwtPlot* plot, const QPoint& plotLocalPos)
{
    PlotHitResult res;
    if (!plot) {
        return res;
    }
    // 标题/footer 是 plot 的子控件，用控件矩形判定
    if (plot->titleLabel() && plot->titleLabel()->isVisible() && plot->titleLabel()->geometry().contains(plotLocalPos)) {
        res.type = HitTitle;
        return res;
    }
    if (plot->footerLabel() && plot->footerLabel()->isVisible()
        && plot->footerLabel()->geometry().contains(plotLocalPos)) {
        res.type = HitFooter;
        return res;
    }
    // 坐标轴：把 plot 本地坐标换算为 scaleWidget 本地坐标后用 isOnScale 判定
    const QList< int > axes { QwtAxis::YLeft, QwtAxis::YRight, QwtAxis::XBottom, QwtAxis::XTop };
    for (int axisId : axes) {
        QwtScaleWidget* sw = plot->axisWidget(axisId);
        if (!sw || !sw->isVisible()) {
            continue;
        }
        const QPoint scaleLocal = sw->mapFrom(plot, plotLocalPos);
        if (sw->rect().contains(scaleLocal) && sw->isOnScale(scaleLocal)) {
            res.type       = HitAxis;
            res.axisWidget = sw;
            res.axisId     = axisId;
            return res;
        }
    }
    // canvas 区域
    if (plot->canvas() && plot->canvas()->geometry().contains(plotLocalPos)) {
        res.type = HitCanvas;
        return res;
    }
    return res;
}

//===================================================
// item 命中
//===================================================

/**
 * @brief 在 canvas 像素坐标下做 plotitem 命中测试
 *
 * 遍历 plot 的 itemList（z 序从高到低，即列表从尾到头），第一个命中的可见 item 胜出。
 * 注意 QwtPlotItemInfo::isDecoratorItem 会把 marker/shape/textLabel/legend/arrow 全部
 * 归为装饰类，而这些正是指针工具的可选目标，因此这里只排除网格与刻度尺
 * @param plot 目标绘图
 * @param canvasPos canvas 像素坐标
 * @param tolerance 命中容差（像素）
 * @return 命中的 item，未命中返回 nullptr
 */
QwtPlotItem* DAChartElementHitTester::itemAt(QwtPlot* plot, const QPoint& canvasPos, int tolerance)
{
    if (!plot) {
        return nullptr;
    }
    const QwtPlotItemList items = plot->itemList();
    for (int i = items.size() - 1; i >= 0; --i) {
        QwtPlotItem* item = items.at(i);
        if (!item->isVisible()) {
            continue;
        }
        const int rtti = item->rtti();
        if (rtti == QwtPlotItem::Rtti_PlotGrid || rtti == QwtPlotItem::Rtti_PlotScale) {
            continue;
        }
        if (isItemHit(plot, item, canvasPos, tolerance)) {
            return item;
        }
    }
    return nullptr;
}

/**
 * @brief 单个 item 的命中判定，按 rtti 分派到具体策略
 * @param plot 目标绘图
 * @param item 待判定 item
 * @param canvasPos canvas 像素坐标
 * @param tolerance 命中容差（像素）
 * @return 命中返回 true
 */
bool DAChartElementHitTester::isItemHit(QwtPlot* plot, const QwtPlotItem* item, const QPoint& canvasPos, int tolerance)
{
    const int rtti = item->rtti();
    switch (rtti) {
    case QwtPlotItem::Rtti_PlotCurve:
        return curveHit(item, canvasPos, tolerance);
    case QwtPlotItem::Rtti_PlotMarker:
        return markerHit(plot, dynamic_cast< const QwtPlotMarker* >(item), canvasPos, tolerance);
    case DADataProbeMarker::Rtti_DataProbeMarker:
    case DAChartTextMarker::Rtti_TextMarker:
        // DA 特有 marker 继承自 QwtPlotMarker，但 rtti 被覆写，需动态识别
        if (const DADataProbeMarker* probe = dynamic_cast< const DADataProbeMarker* >(item)) {
            return markerHit(plot, probe, canvasPos, tolerance);
        }
        if (const DAChartTextMarker* tm = dynamic_cast< const DAChartTextMarker* >(item)) {
            return markerHit(plot, tm, canvasPos, tolerance);
        }
        return false;
    case QwtPlotItem::Rtti_PlotArrowMarker:
        return arrowHit(plot, dynamic_cast< const QwtPlotArrowMarker* >(item), canvasPos, tolerance);
    case QwtPlotItem::Rtti_PlotShape:
        return shapeHit(plot, dynamic_cast< const QwtPlotShapeItem* >(item), canvasPos, tolerance);
    case QwtPlotItem::Rtti_PlotLegend:
        return legendHit(plot, dynamic_cast< const QwtPlotLegendItem* >(item), canvasPos, tolerance);
    case QwtPlotItem::Rtti_PlotTextLabel:
        return textLabelHit(plot, dynamic_cast< const QwtPlotTextLabel* >(item), canvasPos);
    case QwtPlotItem::Rtti_PlotHistogram:
    case QwtPlotItem::Rtti_PlotBarChart:
    case QwtPlotItem::Rtti_PlotMultiBarChart:
    case QwtPlotItem::Rtti_PlotSpectrogram:
    case QwtPlotItem::Rtti_PlotSpectroCurve:
    case QwtPlotItem::Rtti_PlotIntervalCurve:
    case QwtPlotItem::Rtti_PlotTradingCurve:
    case QwtPlotItem::Rtti_PlotVectorField:
    case QwtPlotItem::Rtti_PlotBoxChart:
        return seriesRectHit(plot, item, canvasPos);
    default:
        // 其他类型（含网格、刻度尺、user item 未知类型）用数据包围盒粗筛
        return seriesRectHit(plot, item, canvasPos);
    }
}

/**
 * @brief 曲线类命中判定
 *
 * QwtPlotCurve 族用 closestPoint 获取最近采样点（传入 canvas 像素坐标），
 * 再计算像素距离；非 QwtPlotCurve 派生的曲线类（SpectroCurve/VectorField 等）
 * 退化为采样点距离扫描
 * @param item 曲线类 item
 * @param canvasPos canvas 像素坐标
 * @param tolerance 命中容差（像素）
 * @return 命中返回 true
 */
bool DAChartElementHitTester::curveHit(const QwtPlotItem* item, const QPoint& canvasPos, int tolerance)
{
    const QwtPlotCurve* curve = dynamic_cast< const QwtPlotCurve* >(item);
    if (!curve || !curve->plot()) {
        return false;
    }
    const int index = curve->closestPoint(canvasPos);
    if (index < 0) {
        return false;
    }
    const QPointF sample     = curve->sample(index);
    const QwtScaleMap& xMap  = curve->plot()->canvasMap(curve->xAxis());
    const QwtScaleMap& yMap  = curve->plot()->canvasMap(curve->yAxis());
    const double dx          = xMap.transform(sample.x()) - canvasPos.x();
    const double dy          = yMap.transform(sample.y()) - canvasPos.y();
    const double dist        = std::sqrt(dx * dx + dy * dy);
    return dist <= tolerance;
}

/**
 * @brief marker 类命中判定
 *
 * 按 lineStyle 分派：HLine/VLine/Cross 用线距判定，NoLine（纯文字标注）
 * 用 label 文字矩形判定，两者任一命中即视为命中
 * @param plot 目标绘图
 * @param marker marker 类 item
 * @param canvasPos canvas 像素坐标
 * @param tolerance 命中容差（像素）
 * @return 命中返回 true
 */
bool DAChartElementHitTester::markerHit(QwtPlot* plot, const QwtPlotMarker* marker, const QPoint& canvasPos, int tolerance)
{
    if (!plot || !marker) {
        return false;
    }
    bool hit = false;
    const QwtScaleMap& xMap = plot->canvasMap(marker->xAxis());
    const QwtScaleMap& yMap = plot->canvasMap(marker->yAxis());
    const QPointF value = marker->value();
    const double px     = xMap.transform(value.x());
    const double py     = yMap.transform(value.y());
    const QwtPlotMarker::LineStyle style = marker->lineStyle();
    if (style == QwtPlotMarker::HLine || style == QwtPlotMarker::Cross) {
        hit = hit || std::abs(py - canvasPos.y()) <= tolerance;
    }
    if (style == QwtPlotMarker::VLine || style == QwtPlotMarker::Cross) {
        hit = hit || std::abs(px - canvasPos.x()) <= tolerance;
    }
    // label 文字矩形命中（文字标注/带标签的探针等）
    if (!hit) {
        const QwtText& label = marker->label();
        if (!label.isEmpty()) {
            hit = markerLabelRect(plot, marker).toRect().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(
                canvasPos);
        }
    }
    // 无线条且无 label 的 marker（如 NoLine 无文字）用锚点小范围兜底
    if (!hit && style == QwtPlotMarker::NoLine) {
        const double dx = px - canvasPos.x();
        const double dy = py - canvasPos.y();
        hit             = std::sqrt(dx * dx + dy * dy) <= tolerance;
    }
    return hit;
}

/**
 * @brief 箭头命中判定
 *
 * 点到起终点线段的像素距离在容差内，或命中起/终点端点图形区域
 * @param plot 目标绘图
 * @param arrow 箭头 item
 * @param canvasPos canvas 像素坐标
 * @param tolerance 命中容差（像素）
 * @return 命中返回 true
 */
bool DAChartElementHitTester::arrowHit(QwtPlot* plot, const QwtPlotArrowMarker* arrow, const QPoint& canvasPos, int tolerance)
{
    if (!plot || !arrow) {
        return false;
    }
    const QwtScaleMap& xMap = plot->canvasMap(arrow->xAxis());
    const QwtScaleMap& yMap = plot->canvasMap(arrow->yAxis());
    const QPointF start = arrow->startPoint();
    const QPointF end   = arrow->endPoint();
    const QPointF ps(xMap.transform(start.x()), yMap.transform(start.y()));
    const QPointF pe(xMap.transform(end.x()), yMap.transform(end.y()));
    const double tol2 = static_cast< double >(tolerance) * tolerance;
    // 线段距离
    if (distanceSquareToPointSegment(QPointF(canvasPos), ps, pe) <= tol2) {
        return true;
    }
    // 端点图形（箭头头部/尾部）区域
    const double headR = std::max(arrow->headSize().width(), arrow->headSize().height()) / 2.0 + tolerance;
    const double tailR = std::max(arrow->tailSize().width(), arrow->tailSize().height()) / 2.0 + tolerance;
    const QPointF pc   = canvasPos;
    if ((pc - pe).manhattanLength() <= headR * headR || (pc - ps).manhattanLength() <= tailR * tailR) {
        return true;
    }
    return false;
}

/**
 * @brief 形状类命中判定（选区/形状图元）
 *
 * 两级筛选：数据包围盒粗筛 → shape path 精筛（Qwt itemeditor 示例范式）
 * @param plot 目标绘图
 * @param shape 形状 item
 * @param canvasPos canvas 像素坐标
 * @param tolerance 命中容差（像素）
 * @return 命中返回 true
 */
bool DAChartElementHitTester::shapeHit(QwtPlot* plot, const QwtPlotShapeItem* shape, const QPoint& canvasPos, int tolerance)
{
    if (!plot || !shape) {
        return false;
    }
    const QPainterPath path = shape->shape();
    if (path.isEmpty()) {
        return false;
    }
    const QwtScaleMap& xMap = plot->canvasMap(shape->xAxis());
    const QwtScaleMap& yMap = plot->canvasMap(shape->yAxis());
    // 粗筛：像素包围盒（含容差）
    const QRectF pixelRect = transformDataRect(plot, shape, path.boundingRect());
    if (!pixelRect.adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(canvasPos)) {
        return false;
    }
    // 精筛：数据坐标 path 包含
    const QPointF dataPos(xMap.invTransform(canvasPos.x()), yMap.invTransform(canvasPos.y()));
    return path.contains(dataPos);
}

/**
 * @brief 数据系列类命中判定（柱状图/直方图/等高线图等）
 *
 * 把点击位置换算为数据坐标后与 item 的数据包围盒做包含判定。
 * 这些图元自身有绘制面积，包围盒判定即可满足选中需求
 * @param plot 目标绘图
 * @param item 系列 item
 * @param canvasPos canvas 像素坐标
 * @return 命中返回 true
 */
bool DAChartElementHitTester::seriesRectHit(QwtPlot* plot, const QwtPlotItem* item, const QPoint& canvasPos)
{
    if (!plot || !item) {
        return false;
    }
    const QRectF br = item->boundingRect();
    if (!br.isValid() || br.isEmpty()) {
        return false;
    }
    const QwtScaleMap& xMap = plot->canvasMap(item->xAxis());
    const QwtScaleMap& yMap = plot->canvasMap(item->yAxis());
    const QPointF dataPos(xMap.invTransform(canvasPos.x()), yMap.invTransform(canvasPos.y()));
    return br.contains(dataPos);
}

/**
 * @brief canvas 内图例命中判定
 *
 * QwtPlotLegendItem::geometry 返回的是 canvas 像素矩形，直接包含判定
 * @param plot 目标绘图
 * @param legend 图例 item
 * @param canvasPos canvas 像素坐标
 * @param tolerance 命中容差（像素）
 * @return 命中返回 true
 */
bool DAChartElementHitTester::legendHit(QwtPlot* plot, const QwtPlotLegendItem* legend, const QPoint& canvasPos, int tolerance)
{
    if (!plot || !legend || !plot->canvas()) {
        return false;
    }
    const QRect geo = legend->geometry(plot->canvas()->contentsRect());
    return geo.adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(canvasPos);
}

/**
 * @brief canvas 内文字项命中判定
 *
 * 复现 QwtPlotTextLabel::draw 的 textRect 计算（canvasRect 按 margin 缩减后按对齐布局）
 * @param plot 目标绘图
 * @param label 文字 item
 * @param canvasPos canvas 像素坐标
 * @return 命中返回 true
 */
bool DAChartElementHitTester::textLabelHit(QwtPlot* plot, const QwtPlotTextLabel* label, const QPoint& canvasPos)
{
    if (!plot || !label || !plot->canvas()) {
        return false;
    }
    const int m = label->margin();
    const QRectF canvasRect = QRectF(plot->canvas()->contentsRect());
    const QSizeF textSize   = label->text().textSize(textRenderFont(label->text()));
    const QRectF textRect   = label->textRect(canvasRect.adjusted(m, m, -m, -m), textSize);
    return textRect.toRect().adjusted(-2, -2, 2, 2).contains(canvasPos);
}

//===================================================
// 选中框
//===================================================

/**
 * @brief 获取 item 的选中框（canvas 像素坐标）
 *
 * 按类型分派：系列类用数据包围盒映射（并夹紧到 canvas 内）；
 * marker 用锚点/label 矩形；箭头用起终点矩形；图例用其 canvas 矩形
 * @param plot 目标绘图
 * @param item 目标 item
 * @param margin 选中框外扩边距（像素）
 * @return canvas 像素坐标矩形，item 无效时返回空矩形
 */
QRect DAChartElementHitTester::itemSelectionRect(QwtPlot* plot, const QwtPlotItem* item, int margin)
{
    if (!plot || !item || !plot->canvas()) {
        return QRect();
    }
    QRectF rect;
    const int rtti = item->rtti();
    switch (rtti) {
    case QwtPlotItem::Rtti_PlotMarker:
    case DADataProbeMarker::Rtti_DataProbeMarker:
    case DAChartTextMarker::Rtti_TextMarker: {
        const QwtPlotMarker* marker = dynamic_cast< const QwtPlotMarker* >(item);
        if (!marker) {
            return QRect();
        }
        const QwtText& label = marker->label();
        if (!label.isEmpty()) {
            rect = markerLabelRect(plot, marker);
        } else {
            // 无label的线条marker按lineStyle画细长框：水平线全宽、垂直线全高、十字全画布
            const QwtScaleMap& xMap = plot->canvasMap(marker->xAxis());
            const QwtScaleMap& yMap = plot->canvasMap(marker->yAxis());
            const qreal px          = xMap.transform(marker->xValue());
            const qreal py          = yMap.transform(marker->yValue());
            const QRectF canvasRect(plot->canvas()->contentsRect());
            switch (marker->lineStyle()) {
            case QwtPlotMarker::HLine:
                rect = QRectF(canvasRect.left(), py - 4, canvasRect.width(), 8);
                break;
            case QwtPlotMarker::VLine:
                rect = QRectF(px - 4, canvasRect.top(), 8, canvasRect.height());
                break;
            case QwtPlotMarker::Cross:
                rect = canvasRect;
                break;
            default:
                // NoLine无label：锚点处一个小方框
                rect = QRectF(px - 12, py - 12, 24, 24);
                break;
            }
        }
    } break;
    case QwtPlotItem::Rtti_PlotArrowMarker: {
        const QwtPlotArrowMarker* arrow = dynamic_cast< const QwtPlotArrowMarker* >(item);
        if (!arrow) {
            return QRect();
        }
        rect = transformDataRect(plot, item, arrow->boundingRect());
    } break;
    case QwtPlotItem::Rtti_PlotShape: {
        const QwtPlotShapeItem* shape = dynamic_cast< const QwtPlotShapeItem* >(item);
        if (!shape) {
            return QRect();
        }
        rect = transformDataRect(plot, item, shape->shape().boundingRect());
    } break;
    case QwtPlotItem::Rtti_PlotLegend: {
        const QwtPlotLegendItem* legend = dynamic_cast< const QwtPlotLegendItem* >(item);
        if (!legend) {
            return QRect();
        }
        rect = QRectF(legend->geometry(plot->canvas()->contentsRect()));
    } break;
    case QwtPlotItem::Rtti_PlotTextLabel: {
        const QwtPlotTextLabel* label = dynamic_cast< const QwtPlotTextLabel* >(item);
        if (!label) {
            return QRect();
        }
        const int m = label->margin();
        const QRectF canvasRect = QRectF(plot->canvas()->contentsRect());
        const QSizeF textSize   = label->text().textSize(textRenderFont(label->text()));
        rect                    = label->textRect(canvasRect.adjusted(m, m, -m, -m), textSize);
    } break;
    default: {
        // 曲线/系列类：数据包围盒映射到像素并夹紧到 canvas
        const QRectF br = item->boundingRect();
        if (br.isValid() && !br.isEmpty()) {
            rect = transformDataRect(plot, item, br);
        } else {
            return QRect();
        }
    } break;
    }
    if (!rect.isValid()) {
        return QRect();
    }
    // 夹紧到 canvas 范围（系列类包围盒常超出可视区）
    const QRectF canvasRect = QRectF(plot->canvas()->contentsRect());
    rect                    = rect.intersected(canvasRect);
    if (rect.isEmpty()) {
        return QRect();
    }
    return rect.adjusted(-margin, -margin, margin, margin).toAlignedRect();
}

//===================================================
// 几何记录/恢复/平移
//===================================================

/**
 * @brief 判断 item 是否支持拖动移动
 *
 * 仅可定位类元素可移动：marker 族（文字标注/水平线/垂直线/十字标记/数据探针）
 * 与箭头。曲线/系列类图元的几何由数据决定，不支持移动
 * @param item 目标 item
 * @return 可移动返回 true
 */
bool DAChartElementHitTester::isMovableItem(const QwtPlotItem* item)
{
    if (!item) {
        return false;
    }
    const int rtti = item->rtti();
    switch (rtti) {
    case QwtPlotItem::Rtti_PlotMarker:
    case DADataProbeMarker::Rtti_DataProbeMarker:
    case DAChartTextMarker::Rtti_TextMarker:
    case QwtPlotItem::Rtti_PlotArrowMarker:
        return true;
    default:
        return false;
    }
}

/**
 * @brief 记录 item 当前几何（数据坐标）
 * @param item 目标 item
 * @return 几何快照，item 不可移动时返回 valid=false
 */
DAChartElementHitTester::ItemGeometry DAChartElementHitTester::itemGeometry(const QwtPlotItem* item)
{
    ItemGeometry g;
    if (!item) {
        return g;
    }
    if (const QwtPlotArrowMarker* arrow = dynamic_cast< const QwtPlotArrowMarker* >(item)) {
        g.p1    = arrow->startPoint();
        g.p2    = arrow->endPoint();
        g.valid = true;
    } else if (const QwtPlotMarker* marker = dynamic_cast< const QwtPlotMarker* >(item)) {
        g.p1    = marker->value();
        g.valid = true;
    }
    return g;
}

/**
 * @brief 恢复 item 几何（数据坐标）
 *
 * 数据探针恢复后重新捕获数据以刷新标签
 * @param item 目标 item
 * @param g 几何快照
 */
void DAChartElementHitTester::setItemGeometry(QwtPlotItem* item, const ItemGeometry& g)
{
    if (!item || !g.valid) {
        return;
    }
    if (QwtPlotArrowMarker* arrow = dynamic_cast< QwtPlotArrowMarker* >(item)) {
        arrow->setPoints(g.p1, g.p2);
    } else if (DADataProbeMarker* probe = dynamic_cast< DADataProbeMarker* >(item)) {
        // setProbeValue 内部会触发标签更新；updateLabel 为私有接口不可外部调用
        if (DADataProbeMarker::VerticalProbe == probe->probeType()) {
            probe->setProbeValue(g.p1.x());
        } else {
            probe->setProbeValue(g.p1.y());
        }
        probe->captureData();
    } else if (QwtPlotMarker* marker = dynamic_cast< QwtPlotMarker* >(item)) {
        marker->setValue(g.p1);
    }
}

/**
 * @brief 按数据坐标平移 item
 *
 * 数据探针按其类型仅移动对应方向的分量并重新捕获数据；
 * 其余 marker 平移锚点，箭头平移起终点
 * @param item 目标 item
 * @param deltaData 数据坐标平移量
 */
void DAChartElementHitTester::moveItemBy(QwtPlotItem* item, const QPointF& deltaData)
{
    if (!item) {
        return;
    }
    if (QwtPlotArrowMarker* arrow = dynamic_cast< QwtPlotArrowMarker* >(item)) {
        arrow->setPoints(arrow->startPoint() + deltaData, arrow->endPoint() + deltaData);
    } else if (DADataProbeMarker* probe = dynamic_cast< DADataProbeMarker* >(item)) {
        if (DADataProbeMarker::VerticalProbe == probe->probeType()) {
            probe->setProbeValue(probe->probeValue() + deltaData.x());
        } else {
            probe->setProbeValue(probe->probeValue() + deltaData.y());
        }
        probe->captureData();
    } else if (QwtPlotMarker* marker = dynamic_cast< QwtPlotMarker* >(item)) {
        marker->setValue(marker->value() + deltaData);
    }
}

//===================================================
// 私有辅助
//===================================================

/**
 * @brief 计算 marker 的 label 文字矩形（canvas 像素坐标）
 *
 * 完整复刻 QwtPlotMarker::drawLabel 的落位算法（qwt_plot_marker.cpp），
 * 保证命中矩形与实际绘制位置一致：
 * - 对齐语义与 Qt 控件相反：AlignLeft 文字在锚点左侧、AlignTop 文字在锚点上方
 * - HLine/VLine 样式时对齐标志相对 canvas 边缘解释，锚点分量被替换
 * - 偏移含 pen 半宽、symbol 尺寸与 spacing
 * @param plot 目标绘图
 * @param marker 目标 marker
 * @return label 文字矩形，无 label 返回空矩形
 */
QRectF DAChartElementHitTester::markerLabelRect(QwtPlot* plot, const QwtPlotMarker* marker)
{
    if (!plot || !marker) {
        return QRectF();
    }
    const QwtText& label = marker->label();
    if (label.isEmpty()) {
        return QRectF();
    }
    const QwtScaleMap& xMap = plot->canvasMap(marker->xAxis());
    const QwtScaleMap& yMap = plot->canvasMap(marker->yAxis());
    const QPointF pos(xMap.transform(marker->xValue()), yMap.transform(marker->yValue()));
    const QSizeF textSize = label.textSize(textRenderFont(label));

    Qt::Alignment align  = marker->labelAlignment();
    const Qt::Orientation orientation = marker->labelOrientation();
    QPointF alignPos     = pos;

    // HLine/VLine 时对齐标志相对 canvas 边缘，锚点分量被 canvas 几何替换（同步 drawLabel）
    QSizeF symbolOff(0, 0);
    switch (marker->lineStyle()) {
    case QwtPlotMarker::VLine: {
        if (align & Qt::AlignTop) {
            alignPos.setY(plot->canvas()->contentsRect().top());
            align &= ~Qt::AlignTop;
            align |= Qt::AlignBottom;
        } else if (align & Qt::AlignBottom) {
            alignPos.setY(plot->canvas()->contentsRect().bottom() - 1);
            align &= ~Qt::AlignBottom;
            align |= Qt::AlignTop;
        } else {
            alignPos.setY(plot->canvas()->contentsRect().center().y());
        }
        break;
    }
    case QwtPlotMarker::HLine: {
        if (align & Qt::AlignLeft) {
            alignPos.setX(plot->canvas()->contentsRect().left());
            align &= ~Qt::AlignLeft;
            align |= Qt::AlignRight;
        } else if (align & Qt::AlignRight) {
            alignPos.setX(plot->canvas()->contentsRect().right() - 1);
            align &= ~Qt::AlignRight;
            align |= Qt::AlignLeft;
        } else {
            alignPos.setX(plot->canvas()->contentsRect().center().x());
        }
        break;
    }
    default: {
        if (marker->symbol() && (marker->symbol()->style() != QwtSymbol::NoSymbol)) {
            symbolOff = marker->symbol()->size() + QSizeF(1, 1);
            symbolOff /= 2;
        }
        break;
    }
    }

    qreal pw2 = marker->linePen().widthF() / 2.0;
    if (pw2 == 0.0) {
        pw2 = 0.5;
    }
    const qreal xOff = qMax< qreal >(pw2, symbolOff.width());
    const qreal yOff = qMax< qreal >(pw2, symbolOff.height());
    const qreal spacing = marker->spacing();

    // 以下偏移逻辑与 QwtPlotMarker::drawLabel 逐分支一致
    if (align & Qt::AlignLeft) {
        alignPos.rx() -= xOff + spacing;
        if (orientation == Qt::Vertical) {
            alignPos.rx() -= textSize.height();
        } else {
            alignPos.rx() -= textSize.width();
        }
    } else if (align & Qt::AlignRight) {
        alignPos.rx() += xOff + spacing;
    } else {
        if (orientation == Qt::Vertical) {
            alignPos.rx() -= textSize.height() / 2;
        } else {
            alignPos.rx() -= textSize.width() / 2;
        }
    }

    if (align & Qt::AlignTop) {
        alignPos.ry() -= yOff + spacing;
        if (orientation != Qt::Vertical) {
            alignPos.ry() -= textSize.height();
        }
    } else if (align & Qt::AlignBottom) {
        alignPos.ry() += yOff + spacing;
        if (orientation == Qt::Vertical) {
            alignPos.ry() += textSize.width();
        }
    } else {
        if (orientation == Qt::Vertical) {
            alignPos.ry() += textSize.width() / 2;
        } else {
            alignPos.ry() -= textSize.height() / 2;
        }
    }

    // drawLabel 在 alignPos 处 translate 后绘制 (0,0,textSize) 文字矩形
    return QRectF(alignPos, textSize);
}

/**
 * @brief 数据坐标矩形转 canvas 像素矩形
 *
 * 使用 item 自己的 xAxis/yAxis 对应的 canvasMap，保证多轴 item 映射正确
 * @param plot 目标绘图
 * @param item 目标 item
 * @param dataRect 数据坐标矩形
 * @return canvas 像素矩形
 */
QRectF DAChartElementHitTester::transformDataRect(QwtPlot* plot, const QwtPlotItem* item, const QRectF& dataRect)
{
    if (!plot || !item || !dataRect.isValid()) {
        return QRectF();
    }
    const QwtScaleMap& xMap = plot->canvasMap(item->xAxis());
    const QwtScaleMap& yMap = plot->canvasMap(item->yAxis());
    return QwtScaleMap::transform(xMap, yMap, dataRect);
}
}  // namespace DA
