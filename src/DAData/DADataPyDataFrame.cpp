#include "DADataPyDataFrame.h"
#include <iterator>
// DAMessageHandler
#include "DALogCategory.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataPyDataFrame
//===================================================
DADataPyDataFrame::DADataPyDataFrame(const DAPyDataFrame& d) : DADataPyObject(d)
{
}

DADataPyDataFrame::~DADataPyDataFrame()
{
}

DAAbstractData::DataType DADataPyDataFrame::getDataType() const
{
    return TypePythonDataFrame;
}

QVariant DADataPyDataFrame::toVariant(std::size_t dim1, std::size_t dim2) const
{
    DAPyDataFrame df(mPyObject.object());
    return df.iat(dim1, dim2);
}

bool DADataPyDataFrame::setValue(std::size_t dim1, std::size_t dim2, const QVariant& v)
{
    try {
        DAPyDataFrame df(mPyObject.object());
        df.iat(dim1, dim2, v);
        return true;
    } catch (const std::exception& e) {
        daWarning << QString("DADataPyDataFrame::setValue failed: %1").arg(e.what());
        return false;
    }
}

DAPyDataFrame DADataPyDataFrame::dataframe() const
{
    return DAPyDataFrame(mPyObject.object());
}

QList< QString > DADataPyDataFrame::columns() const
{
    return DAPyDataFrame(mPyObject.object()).columns();
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
        DAPyDataFrame df = dataframe();
        DAPySeries ser   = df[ name ];
        if (ser.isNone()) {
            return res;
        }
        ser.castTo< double >(std::back_insert_iterator< QVector< double > >(res));
    } catch (const std::exception& e) {
        daWarning << QString("getSeriesByVector failed for column '%1': %2").arg(name).arg(e.what());
        return QVector< double >();
    } catch (...) {
        daWarning << QString("getSeriesByVector failed for column '%1': unknown exception").arg(name);
        return QVector< double >();
    }
    return res;
}
