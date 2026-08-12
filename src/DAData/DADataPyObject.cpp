#include "DADataPyObject.h"
#include "DAPybind11QtCaster.hpp"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataPyObject
//===================================================

/**
 * @brief 默认构造函数
 */
DADataPyObject::DADataPyObject() : DAAbstractData()
{
}

/**
 * @brief 通过DAPyObjectWrapper构造
 * @param d python对象包装器
 */
DADataPyObject::DADataPyObject(const DAPyObjectWrapper& d) : DAAbstractData(), mPyObject(d)
{
}

/**
 * @brief 析构函数
 */
DADataPyObject::~DADataPyObject()
{
}

/**
 * @brief 获取数据类型
 * @return 返回TypePythonObject
 */
DAAbstractData::DataType DADataPyObject::getDataType() const
{
    return TypePythonObject;
}

/**
 * @brief 获取变量值
 * @param dim1 第一维索引
 * @param dim2 第二维索引
 * @return python对象转换为的QVariant
 */
QVariant DADataPyObject::toVariant(size_t dim1, size_t dim2) const
{
    Q_UNUSED(dim1);
    Q_UNUSED(dim2);
    return DA::PY::fromPyVariant(mPyObject.object());
}

/**
 * @brief 设置值
 * @param v
 * @return 永远返回true
 */
bool DADataPyObject::setValue(std::size_t dim1, std::size_t dim2, const QVariant& v)
{
    Q_UNUSED(dim1);
    Q_UNUSED(dim2);
    pybind11::object obj = pybind11::cast(v);
    mPyObject.object()   = obj;
    return true;
}

/**
 * @brief 判断是否为null
 * @return
 */
bool DADataPyObject::isNull() const
{
    return mPyObject.isNone();
}

/**
 * @brief 获取python object
 * @return
 */
DAPyObjectWrapper& DADataPyObject::object()
{
    return mPyObject;
}

/**
 * @brief 获取python object（const版本）
 * @return const引用
 */
const DAPyObjectWrapper& DADataPyObject::object() const
{
    return mPyObject;
}
