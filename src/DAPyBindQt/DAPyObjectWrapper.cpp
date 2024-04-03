#include "DAPyObjectWrapper.h"
#include "DAPybind11QtTypeCast.h"
#include <QDebug>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyObjectWrapper
//===================================================
DAPyObjectWrapper::DAPyObjectWrapper()
{
    _object      = pybind11::none();
    _errcallback = [](const char* e) { qCritical() << e; };
}

DAPyObjectWrapper::DAPyObjectWrapper(const DAPyObjectWrapper& obj)
{
    _object      = obj._object;
    _errcallback = obj._errcallback;
}

DAPyObjectWrapper::DAPyObjectWrapper(DAPyObjectWrapper&& obj)
{
    _object      = std::move(obj._object);
    _errcallback = std::move(obj._errcallback);
}

DAPyObjectWrapper::DAPyObjectWrapper(const pybind11::object& obj)
{
    _object      = obj;
    _errcallback = [](const char* e) { qCritical() << e; };
}

DAPyObjectWrapper::DAPyObjectWrapper(pybind11::object&& obj)
{
    _object      = std::move(obj);
    _errcallback = [](const char* e) { qCritical() << e; };
}

DAPyObjectWrapper::~DAPyObjectWrapper()
{
    //析构的时候打印一下引用情况，看看是否有内存泄漏
    //    if (!isNone()) {
    //        qDebug() << "python object ptr (" << object().ptr() << " ) ref count=" << object().ref_count() << ",will be "
    //                 << object().ref_count() - 1;
    //    }
}

bool DAPyObjectWrapper::isNone() const
{
    return _object.is_none();
}

DAPyObjectWrapper& DAPyObjectWrapper::operator=(const DAPyObjectWrapper& obj)
{
    _object      = obj._object;
    _errcallback = obj._errcallback;
    return *this;
}

DAPyObjectWrapper& DAPyObjectWrapper::operator=(const pybind11::object& obj)
{
    _object = obj;
    return *this;
}

bool DAPyObjectWrapper::operator==(void* ptr) const
{
    return (_object.ptr() == ptr);
}

bool DAPyObjectWrapper::operator==(const pybind11::object& obj) const
{
    return _object.is(obj);
}

bool DAPyObjectWrapper::operator==(const DAPyObjectWrapper& obj) const
{
    return _object.is(obj._object);
}

/**
 * @brief 统一的异常处理函数
 *
 * 此函数会调用赋予的回调
 * @param e
 */
void DAPyObjectWrapper::dealException(const std::exception& e) const
{
    if (_errcallback) {
        _errcallback(e.what());
    }
}

QVariant DAPyObjectWrapper::toVariant() const
{
    return DA::PY::toVariant(object());
}

/**
 * @brief 是否为int
 * @return
 */
bool DAPyObjectWrapper::isInt() const
{
    return pybind11::isinstance< pybind11::int_ >(object());
}

/**
 * @brief 是否为Module
 * @return
 */
bool DAPyObjectWrapper::isModule() const
{
    return pybind11::isinstance< pybind11::module_ >(object());
}
/**
 * @brief 是否为float
 * @return
 */
bool DAPyObjectWrapper::isFloat() const
{
    return pybind11::isinstance< pybind11::float_ >(object());
}
/**
 * @brief 是否为str
 * @return
 */
bool DAPyObjectWrapper::isStr() const
{
    return pybind11::isinstance< pybind11::str >(object());
}

/**
 * @brief 设置错误处理回调
 * @param fun
 */
void DAPyObjectWrapper::setErrCallback(const DAPyObjectWrapper::ErrCallback& fun)
{
    _errcallback = fun;
}

DAPyObjectWrapper::ErrCallback DAPyObjectWrapper::getErrCallback() const
{
    return _errcallback;
}

/**
 * @brief 获取属性
 * @param c_att
 * @return
 */
pybind11::object DAPyObjectWrapper::attr(const char* c_att)
{
    return _object.attr(c_att);
}

pybind11::object DAPyObjectWrapper::attr(const char* c_att) const
{
    return _object.attr(c_att);
}

/**
 * @brief 对应__name__
 * @return
 */
QString DAPyObjectWrapper::__name__() const
{
    if (isNone()) {
        return QString();
    }
    try {
        pybind11::str n = _object.attr("__name__");
        return DA::PY::toString(n);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

DAPyObjectWrapper::operator bool() const
{
    return !isNone();
}
