#ifndef DAPYMODULEWORKFLOW_H
#define DAPYMODULEWORKFLOW_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyModule.h"
namespace DA
{
/**
 * @brief Python工作流引擎模块导入包装
 *
 * 继承 DAPyModule（→ DAPyObjectWrapper），负责导入 DAWorkbench.DAWorkFlowPy Python 包。
 * 关键类引用（DAWorkflow、DANodeRegistry、NodeDef、DANodeFactory）作为直接成员变量缓存，
 * 供 C++ 侧调用。错误处理使用基类 DAPyObjectWrapper 的 dealException() 机制。
 */
class DAPYWORKFLOW_API DAPyModuleWorkflow : public DAPyModule
{
public:
    DAPyModuleWorkflow();
    ~DAPyModuleWorkflow();
    // 导入模块
    bool import();

public:
    // 判断是否为 DAWorkflow 实例
    bool isInstanceWorkflow(const pybind11::object& obj) const;
    // 判断是否为 DANodeRegistry 实例
    bool isInstanceNodeRegistry(const pybind11::object& obj) const;
    // 判断是否为 NodeDef 装饰器类
    bool isInstanceNodeDef(const pybind11::object& obj) const;
    // 获取缓存的 DAWorkflow 类引用
    pybind11::object getWorkflowObject() const;
    // 获取缓存的 DANodeRegistry 类引用
    pybind11::object getNodeRegistryObject() const;
    // 获取缓存的 NodeDef 装饰器引用
    pybind11::object getNodeDefDecoratorObject() const;
    // 获取缓存的 DANodeFactory 类引用
    pybind11::object getNodeFactoryObject() const;
    // 获取缓存的 DAWorkflowExecutor 类引用
    pybind11::object getWorkflowExecutorObject() const;
    // 获取缓存的 DASignalManager 类引用
    pybind11::object getSignalManagerObject() const;
    // 获取缓存的 DAWorkflowSerializer 类引用
    pybind11::object getWorkflowSerializerObject() const;

private:
    // 缓存 Python 类引用，避免每次 attr() 查找
    pybind11::object mObjWorkflowClass;
    pybind11::object mObjNodeRegistryClass;
    pybind11::object mObjNodeDefDecorator;
    pybind11::object mObjNodeFactoryClass;
    pybind11::object mObjWorkflowExecutorClass;
    pybind11::object mObjSignalManagerClass;
    pybind11::object mObjWorkflowSerializerClass;
};
}  // namespace DA
#endif  // DAPYMODULEWORKFLOW_H
