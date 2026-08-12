#include "DADataPySeries.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param d series
 */
DADataPySeries::DADataPySeries(const DAPySeries& d) : DADataPyObject(d)
{
}

/**
 * @brief 获取数据类型
 * @return 返回TypePythonSeries
 */
DAAbstractData::DataType DADataPySeries::getDataType() const
{
    return TypePythonSeries;
}

/**
 * @brief 获取变量值
 * @param dim1 第一维索引
 * @param dim2 第二维索引（未使用）
 * @return 对应位置的QVariant
 */
QVariant DADataPySeries::toVariant(std::size_t dim1, std::size_t dim2) const
{
    Q_UNUSED(dim2);
    DAPySeries ser(mPyObject.object());
    return ser.value(dim1);
}

/**
 * @brief 设置值
 * @param dim1 第一维索引
 * @param dim2 第二维索引（未使用）
 * @param v 要设置的值
 * @return 设置成功返回true
 */
bool DADataPySeries::setValue(std::size_t dim1, std::size_t dim2, const QVariant& v)
{
    Q_UNUSED(dim2);
    DAPySeries ser(mPyObject.object());
    return ser.setValue(dim1, v);
}

/**
 * @brief 获取series
 * @return DAPySeries
 */
DAPySeries DADataPySeries::series() const
{
    return DAPySeries(mPyObject.object());
}
}
