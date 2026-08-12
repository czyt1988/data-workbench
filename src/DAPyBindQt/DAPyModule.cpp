#include "DAPyModule.h"
#include <QDebug>
#include <QObject>
#include "DAPybind11QtCaster.hpp"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyModule
//===================================================

/**
 * @brief 构造一个空的DAPyModule对象
 */
DAPyModule::DAPyModule() : DAPyObjectWrapper()
{
}

/**
 * @brief 构造并通过模块名导入模块
 * @param moduleName 模块名
 */
DAPyModule::DAPyModule(const char* moduleName) : DAPyObjectWrapper()
{
    import(moduleName);
}

/**
 * @brief 构造，从pybind11::object构造
 * @param obj pybind11对象
 */
DAPyModule::DAPyModule(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
}

/**
 * @brief 构造，从pybind11::object右值构造
 * @param obj pybind11对象右值
 */
DAPyModule::DAPyModule(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
}

/**
 * @brief 析构
 */
DAPyModule::~DAPyModule()
{
}

/**
 * @brief 赋值操作符
 * @param obj DAPyObjectWrapper对象
 * @return 返回自身的引用
 */
DAPyModule& DAPyModule::operator=(const DAPyObjectWrapper& obj)
{
    object() = obj.object();
    return *this;
}

/**
 * @brief 赋值操作符
 * @param obj pybind11对象
 * @return 返回自身的引用
 */
DAPyModule& DAPyModule::operator=(const pybind11::object& obj)
{
    object() = obj;
    return *this;
}

/**
 * @brief 判断是否导入
 * @return
 */
bool DAPyModule::isImport() const
{
    if (isNone()) {
        return false;
    }
    return isModule();
}

/**
 * @brief 获取模块名
 * @return
 */
QString DAPyModule::moduleName() const
{
    if (!isImport()) {
        return QString();
    }
    return __name__();
}

/**
 * @brief 重新加载模块
 */
void DAPyModule::reload()
{
    if (object().is_none()) {
        return;
    }
    try {
        pybind11::module m = object();
        if (!m.is_none()) {
            m.reload();
        }
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 导入模块
 * @param module_n 模块名
 * @return 成功导入返回true
 */
bool DAPyModule::import(const char* module_n) noexcept
{
    try {
        pybind11::module m = pybind11::module::import(module_n);
        object()           = m;
    } catch (const std::exception& e) {
        dealException(e);
        return false;
    }
    return true;
}

/**
 * @brief 导入模块
 * @param module_n
 * @return
 */
DAPyModule DAPyModule::importModule(const char* module_n)
{
    DAPyModule m(pybind11::module_::import(module_n));
    return m;
}

/**
 * @brief 判断对象是否为datetime.time实例
 * @param obj 要判断的对象
 * @return 如果是datetime.time实例返回true
 */
bool DAPyModule::isInstanceTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        static DAPyObjectWrapper time_type = DA::PY::importPyType("datetime", "time");
        if (time_type) {
            return pybind11::isinstance(obj, time_type.object());
        }
    } catch (const std::exception&) {
    }
    return false;
}

/**
 * @brief 判断对象是否为datetime.date实例
 * @param obj 要判断的对象
 * @return 如果是datetime.date实例返回true
 */
bool DAPyModule::isInstanceDate(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        static DAPyObjectWrapper date_type = DA::PY::importPyType("datetime", "date");
        if (date_type) {
            return pybind11::isinstance(obj, date_type.object());
        }
    } catch (const std::exception&) {
    }
    return false;
}

/**
 * @brief 判断对象是否为datetime.datetime实例
 * @param obj 要判断的对象
 * @return 如果是datetime.datetime实例返回true
 */
bool DAPyModule::isInstanceDateTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        static DAPyObjectWrapper datetime_type = DA::PY::importPyType("datetime", "datetime");
        if (datetime_type) {
            return pybind11::isinstance(obj, datetime_type.object());
        }
    } catch (const std::exception&) {
    }
    return false;
}

/**
 * @brief 判断对象是否为pandas.Timestamp实例
 * @param obj 要判断的对象
 * @return 如果是pandas.Timestamp实例返回true
 */
bool DAPyModule::isInstancePandasDateTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        static DAPyObjectWrapper timestamp_type = DA::PY::importPyType("pandas", "Timestamp");
        if (timestamp_type) {
            return pybind11::isinstance(obj, timestamp_type.object());
        }
    } catch (const std::exception&) {
    }
    return false;
}

/**
 * @brief 判断对象是否为numpy.datetime64实例
 * @param obj 要判断的对象
 * @return 如果是numpy.datetime64实例返回true
 */
bool DAPyModule::isInstanceNumpyDateTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        static DAPyObjectWrapper datetime64_type = DA::PY::importPyType("numpy", "datetime64");
        if (datetime64_type) {
            return pybind11::isinstance(obj, datetime64_type.object());
        }
    } catch (const std::exception&) {
    }
    return false;
}

/**
 * @brief 判断对象是否为datetime.timedelta实例
 * @param obj 要判断的对象
 * @return 如果是datetime.timedelta实例返回true
 */
bool DAPyModule::isInstanceTimedelta(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        static DAPyObjectWrapper timedelta_type = DA::PY::importPyType("datetime", "timedelta");
        if (timedelta_type) {
            return pybind11::isinstance(obj, timedelta_type.object());
        }
    } catch (const std::exception&) {
    }
    return false;
}
