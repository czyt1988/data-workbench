#include "DADataPySeries.h"
namespace DA
{

DADataPySeries::DADataPySeries(const DAPySeries& d) : DADataPyObject(d)
{
}

DAAbstractData::DataType DADataPySeries::getDataType() const
{
    return TypePythonSeries;
}

QVariant DADataPySeries::toVariant() const
{
    return QVariant();
}

bool DADataPySeries::setValue(const QVariant& v)
{
    return false;
}

DAPySeries DADataPySeries::series() const
{
    return DAPySeries(mPyObject.object());
}
}
