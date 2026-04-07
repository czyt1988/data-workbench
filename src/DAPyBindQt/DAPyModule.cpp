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
DAPyModule::DAPyModule() : DAPyObjectWrapper()
{
}

DAPyModule::DAPyModule(const char* moduleName) : DAPyObjectWrapper()
{
    import(moduleName);
}

DAPyModule::DAPyModule(const pybind11::object& obj) : DAPyObjectWrapper(obj)
{
}

DAPyModule::DAPyModule(pybind11::object&& obj) : DAPyObjectWrapper(std::move(obj))
{
}

DAPyModule::~DAPyModule()
{
}

DAPyModule& DAPyModule::operator=(const DAPyObjectWrapper& obj)
{
    object() = obj.object();
    return *this;
}

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
    // 获取模块
    DAPyModule m(pybind11::module_::import(module_n));
    return m;
}

static pybind11::handle s_time_type;
static pybind11::handle s_date_type;
static pybind11::handle s_datetime_type;
static pybind11::handle s_pandas_timestamp_type;
static pybind11::handle s_numpy_datetime64_type;
static pybind11::handle s_timedelta_type;

void DAPyModule::cleanupStaticCache()
{
    if (s_time_type) {
        Py_DECREF(s_time_type.ptr());
        s_time_type = pybind11::handle();
    }
    if (s_date_type) {
        Py_DECREF(s_date_type.ptr());
        s_date_type = pybind11::handle();
    }
    if (s_datetime_type) {
        Py_DECREF(s_datetime_type.ptr());
        s_datetime_type = pybind11::handle();
    }
    if (s_pandas_timestamp_type) {
        Py_DECREF(s_pandas_timestamp_type.ptr());
        s_pandas_timestamp_type = pybind11::handle();
    }
    if (s_numpy_datetime64_type) {
        Py_DECREF(s_numpy_datetime64_type.ptr());
        s_numpy_datetime64_type = pybind11::handle();
    }
    if (s_timedelta_type) {
        Py_DECREF(s_timedelta_type.ptr());
        s_timedelta_type = pybind11::handle();
    }
}

bool DAPyModule::isInstanceTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        if (!s_time_type) {
            pybind11::object time_obj = pybind11::module::import("datetime").attr("time");
            s_time_type               = time_obj.release();
            Py_INCREF(s_time_type.ptr());
        }
        return pybind11::isinstance(obj, s_time_type);
    } catch (const std::exception& e) {
        return false;
    }
    return false;
}

bool DAPyModule::isInstanceDate(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        if (!s_date_type) {
            pybind11::object date_obj = pybind11::module::import("datetime").attr("date");
            s_date_type               = date_obj.release();
            Py_INCREF(s_date_type.ptr());
        }
        return pybind11::isinstance(obj, s_date_type);
    } catch (const std::exception& e) {
        return false;
    }
    return false;
}

bool DAPyModule::isInstanceDateTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        if (!s_datetime_type) {
            pybind11::object datetime_obj = pybind11::module::import("datetime").attr("datetime");
            s_datetime_type               = datetime_obj.release();
            Py_INCREF(s_datetime_type.ptr());
        }
        return pybind11::isinstance(obj, s_datetime_type);
    } catch (const std::exception& e) {
        return false;
    }
    return false;
}

bool DAPyModule::isInstancePandasDateTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        if (!s_pandas_timestamp_type) {
            pybind11::object ts_obj = pybind11::module::import("pandas").attr("Timestamp");
            s_pandas_timestamp_type = ts_obj.release();
            Py_INCREF(s_pandas_timestamp_type.ptr());
        }
        return pybind11::isinstance(obj, s_pandas_timestamp_type);
    } catch (const std::exception& e) {
        return false;
    }
    return false;
}

bool DAPyModule::isInstanceNumpyDateTime(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        if (!s_numpy_datetime64_type) {
            pybind11::object dt64_obj = pybind11::module::import("numpy").attr("datetime64");
            s_numpy_datetime64_type   = dt64_obj.release();
            Py_INCREF(s_numpy_datetime64_type.ptr());
        }
        return pybind11::isinstance(obj, s_numpy_datetime64_type);
    } catch (const std::exception& e) {
        return false;
    }
    return false;
}

bool DAPyModule::isInstanceTimedelta(const pybind11::handle& obj)
{
    if (!Py_IsInitialized()) {
        return false;
    }
    try {
        if (!s_timedelta_type) {
            pybind11::object td_obj = pybind11::module::import("datetime").attr("timedelta");
            s_timedelta_type        = td_obj.release();
            Py_INCREF(s_timedelta_type.ptr());
        }
        return pybind11::isinstance(obj, s_timedelta_type);
    } catch (const std::exception& e) {
        return false;
    }
    return false;
}
