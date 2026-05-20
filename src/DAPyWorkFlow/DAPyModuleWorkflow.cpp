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
    try {
        // 2. 缓存关键 Python 对象
        mObjWorkflowClass     = attr("DAWorkflow");
        mObjNodeRegistryClass = attr("DANodeRegistry");
        mObjNodeDefDecorator  = attr("NodeDef");
        mObjNodeFactoryClass = attr("DANodeFactory");
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
}

DAPyModuleWorkflow::~DAPyModuleWorkflow()
{
}

/**
 * @brief 获取实例（单例模式）
 */
DAPyModuleWorkflow& DAPyModuleWorkflow::getInstance()
{
    static DAPyModuleWorkflow s_instance;
    return s_instance;
}

/**
 * @brief 析构模块
 */
void DAPyModuleWorkflow::finalize()
{
    if (!isImport()) {
        return;
    }
    object()                = pybind11::none();
    mObjWorkflowClass       = pybind11::none();
    mObjNodeRegistryClass   = pybind11::none();
    mObjNodeDefDecorator    = pybind11::none();
    mObjNodeFactoryClass   = pybind11::none();
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
    return pybind11::isinstance(obj, mObjWorkflowClass);
}

/**
 * @brief 判断是否为 DANodeRegistry 实例
 */
bool DAPyModuleWorkflow::isInstanceNodeRegistry(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, mObjNodeRegistryClass);
}

/**
 * @brief 判断是否为 NodeDef 装饰器类
 */
bool DAPyModuleWorkflow::isInstanceNodeDef(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, mObjNodeDefDecorator);
}

/**
 * @brief 获取缓存的 DAWorkflow 类引用
 */
pybind11::object DAPyModuleWorkflow::getWorkflowClass() const
{
    return mObjWorkflowClass;
}

/**
 * @brief 获取缓存的 DANodeRegistry 类引用
 */
pybind11::object DAPyModuleWorkflow::getNodeRegistryClass() const
{
    return mObjNodeRegistryClass;
}

/**
 * @brief 获取缓存的 NodeDef 装饰器引用
 */
pybind11::object DAPyModuleWorkflow::getNodeDefDecorator() const
{
    return mObjNodeDefDecorator;
}

/**
 * @brief 获取缓存的 DANodeFactory 类引用
 */
pybind11::object DAPyModuleWorkflow::getNodeFactoryClass() const
{
    return mObjNodeFactoryClass;
}

}  // namespace DA