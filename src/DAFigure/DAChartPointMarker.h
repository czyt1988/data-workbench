#ifndef DACHARTPOINTMARKER_H
#define DACHARTPOINTMARKER_H
#include "DAFigureAPI.h"
#include "DAAbstractChartMarker.h"
#include <QPointF>
namespace DA
{
/**
 * @brief 点标注marker
 */
class DAFIGURE_API DAChartPointMarker : public DAAbstractChartMarker
{
public:
    DAChartPointMarker(const QString& title = QString());
    DAChartPointMarker(const QwtText& title);
    DAChartPointMarker(const QPointF& point);
    virtual ~DAChartPointMarker();
    virtual int markerType() const
    {
        return DAAbstractChartMarker::PointMarker;
    }
};
}  // End Of Namespace DA
#endif  // DACHARTPOINTMARKER_H
