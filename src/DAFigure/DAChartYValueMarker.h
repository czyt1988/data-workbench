#ifndef DACHARTYVALUEMARKER_H
#define DACHARTYVALUEMARKER_H
#include "DAFigureAPI.h"
#include "DAAbstractChartMarker.h"
namespace DA
{
/**
 * @brief Y值标记
 */
class DAFIGURE_API DAChartYValueMarker : public DAAbstractChartMarker
{
public:
    DAChartYValueMarker(const QString& title = QString());
    DAChartYValueMarker(const QwtText& title);
    DAChartYValueMarker(const double& value);
    virtual ~DAChartYValueMarker();
    virtual int markerType() const
    {
        return DAAbstractChartMarker::YValueMarker;
    }
};
}  // End Of Namespace DA
#endif  // DACHARTYVALUEMARKER_H
