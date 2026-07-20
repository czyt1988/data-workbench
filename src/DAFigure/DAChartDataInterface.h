#ifndef DACHARTDATAINTERFACE_H
#define DACHARTDATAINTERFACE_H

#include "DAFigureAPI.h"
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>

#include "qwt_samples.h"   // for QwtBoxSample, QwtIntervalSample
#include "qwt_point_3d.h"  // for QwtPoint3D

class QwtPlotItem;
class QwtPlotCurve;
class QwtPlotBarChart;
class QwtPlotIntervalCurve;
class QwtPlotMarker;
class QwtPlotSpectrogram;
class QwtGridRasterData;
class QwtPlotBoxChart;
class QwtPlotHistogram;
class QwtPlotMultiBarChart;
class QwtPlotSpectroCurve;
class QwtPlotShapeItem;
class QPainterPath;
namespace DA
{

/**
 * @brief 图表数据操作接口 - 负责数据的添加、删除和管理
 */
class DAFIGURE_API DAChartDataInterface
{
public:
    virtual ~DAChartDataInterface() = default;

    // ==================== 通用操作 ====================
    // 获取所有数据相关的rtti
    virtual QList< int > dataRttis() const = 0;

    // ==================== 曲线操作 ====================
    virtual QwtPlotCurve* addCurve(const QVector< double >& xData,
                                   const QVector< double >& yData,
                                   const QString& title = QString())                                   = 0;
    virtual QwtPlotCurve* addCurve(const QVector< QPointF >& points, const QString& title = QString()) = 0;
    virtual QList< QwtPlotCurve* > getCurves() const                                                   = 0;
    virtual void removeCurve(QwtPlotCurve* curve)                                                      = 0;

    // ==================== 散点图 ====================
    virtual QwtPlotCurve* addScatter(const QVector< QPointF >& points, const QString& title = QString()) = 0;

    // ==================== 柱状图 ====================
    virtual QwtPlotBarChart* addBarChart(const QVector< double >& values, const QString& title = QString())  = 0;
    virtual QwtPlotBarChart* addBarChart(const QVector< QPointF >& points, const QString& title = QString()) = 0;

    // ==================== 误差图 ====================
    virtual QwtPlotIntervalCurve* addIntervalCurve(const QVector< double >& values,
                                                   const QVector< double >& mins,
                                                   const QVector< double >& maxs,
                                                   const QString& title = QString()) = 0;

    // ==================== 标记线 ====================
    virtual QwtPlotMarker* addVerticalLine(double x, const QString& title = QString())        = 0;
    virtual QwtPlotMarker* addHorizontalLine(double y, const QString& title = QString())      = 0;
    virtual QwtPlotMarker* addCrossLine(double x, double y, const QString& title = QString()) = 0;

    // ==================== 高级图表 ====================
    virtual QwtPlotSpectrogram* addSpectrogram(QwtGridRasterData* gridData, const QString& title = QString()) = 0;

    // ==================== 箱线图 ====================
    virtual QwtPlotBoxChart* addBoxChart(const QVector< QwtBoxSample >& samples,
                                         const QString& title = QString()) = 0;

    // ==================== 直方图 ====================
    virtual QwtPlotHistogram* addHistogram(const QVector< QwtIntervalSample >& samples,
                                            const QString& title = QString()) = 0;

    // ==================== 分组柱状图 ====================
    virtual QwtPlotMultiBarChart* addMultiBarChart(const QVector< double >& positions,
                                                    const QVector< QVector< double > >& values,
                                                    const QStringList& titles = QStringList()) = 0;

    // ==================== 等高线 ====================
    virtual QwtPlotSpectroCurve* addContour(const QVector< QwtPoint3D >& points,
                                            const QVector< double >& levels,
                                            const QString& title = QString()) = 0;

    // ==================== 形状项（填充区域） ====================
    virtual QwtPlotShapeItem* addShapeItem(const QPainterPath& path,
                                            const QString& title = QString()) = 0;

    // ==================== 通用数据操作 ====================
    virtual void removePlotItem(QwtPlotItem* item) = 0;
    virtual void clearAllData()                    = 0;

    // ==================== 数据工具函数 ====================
    virtual QRectF getDataBounds() const = 0;
    virtual bool hasData() const         = 0;
};

}  // namespace DA

#endif  // DACHARTDATAINTERFACE_H
