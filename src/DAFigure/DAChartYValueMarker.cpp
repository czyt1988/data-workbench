#include "DAChartYValueMarker.h"
#include <QPen>
namespace DA
{
DAChartYValueMarker::DAChartYValueMarker(const QString& title) : DAAbstractChartMarker(title)
{
}

DAChartYValueMarker::DAChartYValueMarker(const QwtText& title) : DAAbstractChartMarker(title)
{
}

DAChartYValueMarker::DAChartYValueMarker(const double& value)
    : DAAbstractChartMarker(QString("YValueMarker:%1").arg(value))
{
    QPen pen(QColor(Qt::black), 1);
    setLineStyle(QwtPlotMarker::HLine);
    setYValue(value);
    setLinePen(pen);
}

DAChartYValueMarker::~DAChartYValueMarker()
{
}
}  // End Of Namespace DA
