#ifndef DACHARTELEMENTHITTESTER_H
#define DACHARTELEMENTHITTESTER_H
#include "DAFigureAPI.h"
#include <QPointF>
#include <QRect>
class QwtPlot;
class QwtPlotItem;
class QwtScaleWidget;
class QwtPlotMarker;
class QwtPlotArrowMarker;
class QwtPlotShapeItem;
class QwtPlotLegendItem;
class QwtPlotTextLabel;
namespace DA
{

/**
 * @brief 绘图元素命中测试工具
 *
 * 提供三类静态能力：
 * 1. plot 级命中（标题/footer/坐标轴/canvas 的区域判定）
 * 2. canvas 内 plotitem 命中（按 rtti 分派命中策略，z 序从高到低）
 * 3. item 几何的记录/恢复/平移（用于拖动与 undo）
 */
class DAFIGURE_API DAChartElementHitTester
{
public:
    /**
     * @brief plot 内非 canvas 元素的命中类型
     */
    enum PlotElementType
    {
        HitNothing,  ///< 未命中任何元素
        HitTitle,    ///< 命中标题
        HitFooter,   ///< 命中脚注
        HitAxis,     ///< 命中坐标轴（此时 axisWidget、axisId 有效）
        HitCanvas    ///< 命中画布区域（可进一步做 item 命中测试）
    };

    /**
     * @brief plot 级命中结果
     */
    struct PlotHitResult
    {
        PlotElementType type { HitNothing };   ///< 命中类型
        QwtScaleWidget* axisWidget { nullptr };  ///< HitAxis 时有效
        int axisId { -1 };                     ///< HitAxis 时有效
    };

    /**
     * @brief item 几何快照（数据坐标）
     *
     * marker 类记录锚点(p1)；箭头记录起点(p1)与终点(p2)
     */
    struct ItemGeometry
    {
        QPointF p1;        ///< 锚点/起点
        QPointF p2;        ///< 终点（仅箭头有效）
        bool valid { false };  ///< 几何是否有效
    };

public:
    DAChartElementHitTester() = delete;
    // 对 plot 本地坐标做区域命中测试（标题/footer/轴/canvas）
    static PlotHitResult hitTestPlot(QwtPlot* plot, const QPoint& plotLocalPos);
    // 在 canvas 像素坐标下做 plotitem 命中测试，返回 z 序最高的命中项，未命中返回 nullptr
    static QwtPlotItem* itemAt(QwtPlot* plot, const QPoint& canvasPos, int tolerance = 8);
    // 获取 item 的选中框（canvas 像素坐标），无效返回空矩形
    static QRect itemSelectionRect(QwtPlot* plot, const QwtPlotItem* item, int margin = 4);
    // item 是否支持拖动移动
    static bool isMovableItem(const QwtPlotItem* item);
    // 记录 item 当前几何（数据坐标）
    static ItemGeometry itemGeometry(const QwtPlotItem* item);
    // 恢复 item 几何（数据坐标）
    static void setItemGeometry(QwtPlotItem* item, const ItemGeometry& g);
    // 按数据坐标平移 item（仅对 movable item 有效）
    static void moveItemBy(QwtPlotItem* item, const QPointF& deltaData);

private:
    // 单个 item 的命中判定（按 rtti 分派）
    static bool isItemHit(QwtPlot* plot, const QwtPlotItem* item, const QPoint& canvasPos, int tolerance);
    // 曲线类命中：最近采样点像素距离
    static bool curveHit(const QwtPlotItem* item, const QPoint& canvasPos, int tolerance);
    // marker 类命中：线条距离或 label 文字矩形
    static bool markerHit(QwtPlot* plot, const QwtPlotMarker* marker, const QPoint& canvasPos, int tolerance);
    // 箭头命中：点到线段距离或端点区域
    static bool arrowHit(QwtPlot* plot, const QwtPlotArrowMarker* arrow, const QPoint& canvasPos, int tolerance);
    // 形状类命中：包围盒粗筛 + path 精筛
    static bool shapeHit(QwtPlot* plot, const QwtPlotShapeItem* shape, const QPoint& canvasPos, int tolerance);
    // 数据系列类命中（柱状/直方图/等高线等）：数据包围盒粗筛
    static bool seriesRectHit(QwtPlot* plot, const QwtPlotItem* item, const QPoint& canvasPos);
    // canvas 内图例命中
    static bool legendHit(QwtPlot* plot, const QwtPlotLegendItem* legend, const QPoint& canvasPos, int tolerance);
    // canvas 内文字项命中
    static bool textLabelHit(QwtPlot* plot, const QwtPlotTextLabel* label, const QPoint& canvasPos);
    // marker 的 label 文字矩形（canvas 像素坐标）
    static QRectF markerLabelRect(QwtPlot* plot, const QwtPlotMarker* marker);
    // 数据坐标矩形转 canvas 像素矩形（使用 item 自己的 x/y 轴映射）
    static QRectF transformDataRect(QwtPlot* plot, const QwtPlotItem* item, const QRectF& dataRect);
};
}  // namespace DA
#endif  // DACHARTELEMENTHITTESTER_H
