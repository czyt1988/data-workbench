#include "DAAbstractChartMarker.h"
namespace DA
{

DAAbstractChartMarker::DAAbstractChartMarker(const QString& title) : QwtPlotMarker(title)
{
}

DAAbstractChartMarker::DAAbstractChartMarker(const QwtText& title) : QwtPlotMarker(title)
{
}

DAAbstractChartMarker::~DAAbstractChartMarker()
{
}

}  // End Of Namespace DA
