#ifndef DACHARTYDATAPICKER_H
#define DACHARTYDATAPICKER_H
#include "DAFigureAPI.h"
#include <QwtPlotPicker>
#include <QwtText>
#include <QLineF>
class QwtPlotItem;
class QwtPlotCurve;
class QwtPlotBarChart;
namespace DA
{
/**
 * @brief Y值拾取器
 */
class DAFIGURE_API DAChartYDataPicker : public QwtPlotPicker
{
    DA_DECLARE_PRIVATE(DAChartYDataPicker)
public:
    DAChartYDataPicker(QWidget*);
    ~DAChartYDataPicker();

protected:
    virtual QwtText trackerTextF(const QPointF&) const override;
    virtual QRect trackerRect(const QFont&) const override;
    //位置信息对应的关键点
    virtual QPointF keyPoint(QwtPlotItem* item, const QPointF& pos) const;
    //内部实现使用了dynamic_cast
    virtual void drawRubberBand(QPainter* painter) const override;
    virtual void move(const QPoint& pos) override;

private:
    QPointF getKeyPoint(QwtPlotCurve* c, const QPointF& pos) const;
    QPointF getKeyPoint(QwtPlotBarChart* c, const QPointF& pos) const;
    //获取x位置的曲线线段
    QLineF getCurveLine(QwtPlotCurve* curve, double x) const;
    //获取x位置的bar值
    double getBarValue(QwtPlotBarChart* bar, double x) const;
};
}  // End Of Namespace DA
#endif  // SAYDATATRACKER_H
