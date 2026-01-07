#include "DADataPyObject.h"
#include "DAPybind11QtTypeCast.h"
#include "DAPybind11QtCaster.hpp"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataPyObject
//===================================================
DADataPyObject::DADataPyObject() : DAAbstractData()
{
}

DADataPyObject::DADataPyObject(const DAPyObjectWrapper& d) : DAAbstractData(), mPyObject(d)
{
}

DADataPyObject::~DADataPyObject()
{
}

DAAbstractData::DataType DADataPyObject::getDataType() const
{
    return TypePythonObject;
}

QVariant DADataPyObject::toVariant(size_t dim1, size_t dim2) const
{
    Q_UNUSED(dim1);
    Q_UNUSED(dim2);
    return DA::PY::toVariant(mPyObject.object());
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

const DAPyObjectWrapper& DADataPyObject::object() const
{
    return mPyObject;
}
