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

QVariant DADataPySeries::toVariant(std::size_t dim1, std::size_t dim2) const
{
    Q_UNUSED(dim2);
    DAPySeries ser(mPyObject.object());
    return ser.value(dim1);
}

bool DADataPySeries::setValue(std::size_t dim1, std::size_t dim2, const QVariant& v)
{
    Q_UNUSED(dim2);
    DAPySeries ser(mPyObject.object());
    return ser.setValue(dim1, v);
}

DAPySeries DADataPySeries::series() const
{
    return DAPySeries(mPyObject.object());
}
}
