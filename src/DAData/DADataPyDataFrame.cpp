#include "DADataPyDataFrame.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataPyDataFrame
//===================================================
DADataPyDataFrame::DADataPyDataFrame(const DAPyDataFrame& d)
{
    _pyObject = d;
    _df       = d;
}

DADataPyDataFrame::~DADataPyDataFrame()
{
}

DAAbstractData::DataType DADataPyDataFrame::getDataType() const
{
    return TypePythonDataFrame;
}

QVariant DADataPyDataFrame::toVariant() const
{
    return QVariant();
}

bool DADataPyDataFrame::setValue(const QVariant& v)
{
    return false;
}

DAPyDataFrame& DADataPyDataFrame::dataframe()
{
    return _df;
}

const DAPyDataFrame& DADataPyDataFrame::dataframe() const
{
    return _df;
}
