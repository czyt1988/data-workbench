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

/**
 * @brief 构造函数
 * @param d dataframe
 */
DADataPyDataFrame::DADataPyDataFrame(const DAPyDataFrame& d) : DADataPyObject(d)
{
}

/**
 * @brief 析构函数
 */
DADataPyDataFrame::~DADataPyDataFrame()
{
}

/**
 * @brief 获取数据类型
 * @return 返回TypePythonDataFrame
 */
DAAbstractData::DataType DADataPyDataFrame::getDataType() const
{
    return TypePythonDataFrame;
}

/**
 * @brief 获取变量值
 * @param dim1 第一维索引（行）
 * @param dim2 第二维索引（列）
 * @return 对应位置的QVariant
 */
QVariant DADataPyDataFrame::toVariant(std::size_t dim1, std::size_t dim2) const
{
    DAPyDataFrame df(mPyObject.object());
    return df.iat(dim1, dim2);
}

/**
 * @brief 设置值
 * @param dim1 第一维索引（行）
 * @param dim2 第二维索引（列）
 * @param v 要设置的值
 * @return 设置成功返回true
 */
bool DADataPyDataFrame::setValue(std::size_t dim1, std::size_t dim2, const QVariant& v)
{
    try {
        DAPyDataFrame df(mPyObject.object());
        df.iat(dim1, dim2, v);
        return true;
    } catch (const std::exception& e) {
        qWarning() << QString("DADataPyDataFrame::setValue failed: %1").arg(e.what());
        return false;
    }
}

/**
 * @brief 获取dataframe
 * @return DAPyDataFrame
 */
DAPyDataFrame DADataPyDataFrame::dataframe() const
{
    return DAPyDataFrame(mPyObject.object());
}

/**
 * @brief 获取列名列表
 * @return 列名列表
 */
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
        qWarning() << QString("getSeriesByVector failed for column '%1': %2").arg(name).arg(e.what());
        return QVector< double >();
    } catch (...) {
        qWarning() << QString("getSeriesByVector failed for column '%1': unknown exception").arg(name);
        return QVector< double >();
    }
    return res;
}
