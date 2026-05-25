#include "DAPyModuleWorkflow.h"
#include <QDebug>
#include "DAPybind11InQt.h"

namespace DA
{

//===================================================
// DAPyModuleWorkflow
//===================================================

DAPyModuleWorkflow::DAPyModuleWorkflow() : DAPyModule()
{
    import();  // 1. 先导入模块
}

DAPyModuleWorkflow::~DAPyModuleWorkflow()
{
}

/**
 * @brief 导入模块
 */
bool DAPyModuleWorkflow::import()
{
    return DAPyModule::import("DAWorkbench.DAWorkFlowPy");
}

/**
 * @brief 判断是否为 DAWorkflow 实例
 */
bool DAPyModuleWorkflow::isInstanceWorkflow(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, getWorkflowObject());
}

/**
 * @brief 判断是否为 DANodeRegistry 实例
 */
bool DAPyModuleWorkflow::isInstanceNodeRegistry(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, getNodeRegistryObject());
}

/**
 * @brief 判断是否为 NodeDef 装饰器类
 */
bool DAPyModuleWorkflow::isInstanceNodeDef(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, getNodeDefDecoratorObject());
}

/**
 * @brief 获取缓存的 DAWorkflow 类引用
 */
pybind11::object DAPyModuleWorkflow::getWorkflowObject() const
{
    return attr("DAWorkflow");
}

/**
 * @brief 获取缓存的 DANodeRegistry 类引用
 */
pybind11::object DAPyModuleWorkflow::getNodeRegistryObject() const
{
    return attr("DANodeRegistry");
}

/**
 * @brief 获取缓存的 NodeDef 装饰器引用
 */
pybind11::object DAPyModuleWorkflow::getNodeDefDecoratorObject() const
{
    return attr("NodeDef");
}

/**
 * @brief 获取缓存的 DANodeFactory 类引用
 */
pybind11::object DAPyModuleWorkflow::getNodeFactoryObject() const
{
    return attr("DANodeFactory");
}

}  // namespace DA
