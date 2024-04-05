#include "DAPyModule.h"
#include <QDebug>
#include <QObject>
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

DAPyModule::DAPyModule(const char* moduleName)
{
    import(moduleName);
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
        object() = pybind11::module::import(module_n);
        return true;
    } catch (const std::exception& e) {
        dealException(e);
    }
    return false;
}
