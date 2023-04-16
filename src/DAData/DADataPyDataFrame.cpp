#include "DADataPyDataFrame.h"
#include <iterator>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataPyDataFrame
//===================================================
DADataPyDataFrame::DADataPyDataFrame(const DAPyDataFrame& d)
{
    mPyObject  = d;
    mDataframe = d;
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
    return mDataframe;
}

const DAPyDataFrame& DADataPyDataFrame::dataframe() const
{
    return mDataframe;
}

QList< QString > DADataPyDataFrame::columns() const
{
    return mDataframe.columns();
}

/**
 * @brief 尝试把df[name]转换为vector<double>
 * @param name
 * @return 如果获取失败，返回一个空的vector
 */
QVector< double > DADataPyDataFrame::getSeriesByVector(const QString& name) const
{
    QVector< double > res;
    try {
        DAPySeries ser = mDataframe[ name ];
        if (ser.isNone()) {
            return res;
        }
        ser.castTo< double >(std::back_insert_iterator< QVector< double > >(res));
    } catch (...) {
        return QVector< double >();
    }
    return res;
}
