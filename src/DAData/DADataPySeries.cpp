#include "DADataPySeries.h"
namespace DA
{

DADataPySeries::DADataPySeries(const DAPySeries& d)
{
    _pyObject = d;
    _ser      = d;
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

DAPySeries& DADataPySeries::series()
{
    return _ser;
}

const DAPySeries& DADataPySeries::series() const
{
    return _ser;
}
}
