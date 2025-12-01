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
    : _object(pybind11::none()), _errcallback([](const char* e) { qCritical() << e; })
{
}

DAPyObjectWrapper::DAPyObjectWrapper(const DAPyObjectWrapper& obj)
    : _object(obj._object), _errcallback(obj._errcallback)
{
}

DAPyObjectWrapper::DAPyObjectWrapper(DAPyObjectWrapper&& obj)
    : _object(std::move(obj._object)), _errcallback(std::move(obj._errcallback))
{
}

DAPyObjectWrapper::DAPyObjectWrapper(const pybind11::object& obj)
    : _object(obj), _errcallback([](const char* e) { qCritical() << e; })
{
}

DAPyObjectWrapper::DAPyObjectWrapper(pybind11::object&& obj)
    : _object(std::move(obj)), _errcallback([](const char* e) { qCritical() << e; })
{
}

DAPyObjectWrapper::~DAPyObjectWrapper()
{
}

bool DAPyObjectWrapper::isNone() const
{
    return _object.is_none();
}

DAPyObjectWrapper& DAPyObjectWrapper::operator=(const DAPyObjectWrapper& obj)
{
    if (this != &obj) {
        _object      = obj._object;
        _errcallback = obj._errcallback;
    }
    return *this;
}

DAPyObjectWrapper& DAPyObjectWrapper::operator=(DAPyObjectWrapper&& obj)
{
    if (this != &obj) {
        _object      = std::move(obj._object);
        _errcallback = std::move(obj._errcallback);
    }
    return *this;
}

DAPyObjectWrapper& DAPyObjectWrapper::operator=(const pybind11::object& obj)
{
    _object = obj;
    return *this;
}

DAPyObjectWrapper& DAPyObjectWrapper::operator=(pybind11::object&& obj)
{
    _object = std::move(obj);
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

DAPyObjectWrapper::operator bool() const
{
    return !isNone();
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

/**
 * @brief 深拷贝
 * @return
 */
DAPyObjectWrapper DAPyObjectWrapper::deepCopy() const
{
    if (isNone()) {
        return DAPyObjectWrapper();
    }

    try {
        static pybind11::module s_copy_mod = pybind11::module::import("copy");
        pybind11::object copied            = s_copy_mod.attr("deepcopy")(_object);
        return DAPyObjectWrapper(copied);
    } catch (const std::exception& e) {
        dealException(e);
        return DAPyObjectWrapper();
    }
}

QVariant DAPyObjectWrapper::toVariant() const
{
    return DA::PY::toVariant(object());
}

bool DAPyObjectWrapper::isinstance(const pybind11::handle& type) const
{
    return pybind11::isinstance(_object, type);
}

/**
 * @brief 是否为int
 * @return
 */
bool DAPyObjectWrapper::isInt() const
{
    return pybind11::isinstance< pybind11::int_ >(_object);
}

/**
 * @brief 是否为Module
 * @return
 */
bool DAPyObjectWrapper::isModule() const
{
    return pybind11::isinstance< pybind11::module_ >(_object);
}
/**
 * @brief 是否为float
 * @return
 */
bool DAPyObjectWrapper::isFloat() const
{
    return pybind11::isinstance< pybind11::float_ >(_object);
}
/**
 * @brief 是否为str
 * @return
 */
bool DAPyObjectWrapper::isStr() const
{
    return pybind11::isinstance< pybind11::str >(_object);
}

bool DAPyObjectWrapper::isBool() const
{
    return pybind11::isinstance< pybind11::bool_ >(_object);
}

bool DAPyObjectWrapper::isList() const
{
    return pybind11::isinstance< pybind11::list >(_object);
}

bool DAPyObjectWrapper::isDict() const
{
    return pybind11::isinstance< pybind11::dict >(_object);
}

bool DAPyObjectWrapper::isTuple() const
{
    return pybind11::isinstance< pybind11::tuple >(_object);
}

bool DAPyObjectWrapper::isCallable() const
{
    if (isNone()) {
        return false;
    }
    return pybind11::isinstance< pybind11::function >(_object) || PyCallable_Check(_object.ptr());
}

bool DAPyObjectWrapper::isSequence() const
{
    if (isNone()) {
        return false;
    }
    return PySequence_Check(_object.ptr());
}

bool DAPyObjectWrapper::isNumeric() const
{
    // 使用Python C API来检查是否是数字类型
    if (isNone()) {
        return false;
    }

    // 检查是否是整数、浮点数、复数等数字类型
    return PyNumber_Check(_object.ptr()) != 0;
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
        return PY::toString(n);
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyObjectWrapper::__str__() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return PY::toString(pybind11::str(_object));
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyObjectWrapper::__repr__() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return PY::toString(pybind11::repr(_object));
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

QString DAPyObjectWrapper::typeName() const
{
    if (isNone()) {
        return QString("none");
    }
    try {
        return PY::toString(pybind11::str(_object.get_type().attr("__name__")));
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

size_t DAPyObjectWrapper::refCount() const
{
    if (isNone()) {
        return 0;
    }
    return _object.ref_count();
}
