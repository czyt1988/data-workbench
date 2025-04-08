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
		return true;
	} catch (const std::exception& e) {
		dealException(e);
	}
	return false;
}

/**
 * @brief 导入模块
 * @param module_n
 * @return
 */
DAPyModule DAPyModule::importModule(const char* module_n)
{
	// 获取 pandas 模块
	DAPyModule m(pybind11::module_::import("pandas"));
	return m;
}
