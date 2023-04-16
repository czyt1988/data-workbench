#include "DADataPySeries.h"
namespace DA
{

DADataPySeries::DADataPySeries(const DAPySeries& d)
{
    mPyObject = d;
    mSeries      = d;
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
    return mSeries;
}

const DAPySeries& DADataPySeries::series() const
{
    return mSeries;
}
}
