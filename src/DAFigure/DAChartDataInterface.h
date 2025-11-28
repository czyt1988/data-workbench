#ifndef DACHARTDATAINTERFACE_H
#define DACHARTDATAINTERFACE_H

#include "DAFigureAPI.h"
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QString>

class QwtPlotItem;
class QwtPlotCurve;
class QwtPlotBarChart;
class QwtPlotIntervalCurve;
class QwtPlotMarker;
class QwtPlotSpectrogram;
class QwtGridRasterData;
namespace DA
{

/**
 * @brief 图表数据操作接口 - 负责数据的添加、删除和管理
 */
class DAFIGURE_API DAChartDataInterface
{
public:
    virtual ~DAChartDataInterface() = default;

    // ==================== 曲线操作 ====================
    virtual QwtPlotCurve*
    addCurve(const QVector< double >& xData, const QVector< double >& yData, const QString& title = "") = 0;
    virtual QwtPlotCurve* addCurve(const QVector< QPointF >& points, const QString& title = "")         = 0;
    virtual QList< QwtPlotCurve* > getCurves() const                                                    = 0;
    virtual void removeCurve(QwtPlotCurve* curve)                                                       = 0;

    // ==================== 散点图 ====================
    virtual QwtPlotCurve*
    addScatter(const QVector< double >& xData, const QVector< double >& yData, const QString& title = "") = 0;

    // ==================== 柱状图 ====================
    virtual QwtPlotBarChart* addBarChart(const QVector< double >& values, const QString& title = "")  = 0;
    virtual QwtPlotBarChart* addBarChart(const QVector< QPointF >& points, const QString& title = "") = 0;

    // ==================== 误差图 ====================
    virtual QwtPlotIntervalCurve* addIntervalCurve(const QVector< double >& values,
                                                   const QVector< double >& mins,
                                                   const QVector< double >& maxs,
                                                   const QString& title = "") = 0;

    // ==================== 标记线 ====================
    virtual QwtPlotMarker* addVerticalLine(double x, const QString& title = "")        = 0;
    virtual QwtPlotMarker* addHorizontalLine(double y, const QString& title = "")      = 0;
    virtual QwtPlotMarker* addCrossLine(double x, double y, const QString& title = "") = 0;

    // ==================== 高级图表 ====================
    virtual QwtPlotSpectrogram* addSpectrogram(QwtGridRasterData* gridData, const QString& title = "") = 0;

    // ==================== 通用数据操作 ====================
    virtual QList< QwtPlotItem* > getAllPlotItems() const = 0;
    virtual void removePlotItem(QwtPlotItem* item)        = 0;
    virtual void clearAllData()                           = 0;

    // ==================== 数据工具函数 ====================
    virtual QRectF getDataBounds() const = 0;
    virtual bool hasData() const         = 0;
};

}  // namespace DA

#endif  // DACHARTDATAINTERFACE_H
