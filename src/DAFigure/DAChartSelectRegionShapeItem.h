#ifndef DACHARTSELECTREGIONSHAPEITEM_H
#define DACHARTSELECTREGIONSHAPEITEM_H
#include "qwt_plot_shapeitem.h"
#include "DAFigureAPI.h"
namespace DA
{
/**
 * @brief 选区基类
 */
class DAFIGURE_API DAChartSelectRegionShapeItem : public QwtPlotShapeItem
{
public:
    DAChartSelectRegionShapeItem(const QString& title = QString());
    virtual void draw(QPainter* p, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& rect) const;
    void setEllipse(const QRectF& rect);
};
}  // End Of Namespace DA
#endif  // DACHARTSELECTREGIONSHAPEITEM_H
