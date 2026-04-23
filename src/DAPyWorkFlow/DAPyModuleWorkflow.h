#ifndef DAPYMODULEWORKFLOW_H
#define DAPYMODULEWORKFLOW_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyModule.h"
namespace DA
{
/**
 * @brief Python工作流引擎模块导入包装
 *
 * 遵循 Scenario D 模式（Python 模块导入），继承 DAPyModule（不是 QObject，不是自定义桥接类）。
 * 负责导入 DAWorkFlowPy Python 包并缓存关键类引用，供 C++ 侧调用。
 */
class DAPYWORKFLOW_API DAPyModuleWorkflow : public DAPyModule
{
    DA_DECLARE_PRIVATE(DAPyModuleWorkflow)
    DAPyModuleWorkflow();

public:
    ~DAPyModuleWorkflow();
    // 获取实例（单例模式）
    static DAPyModuleWorkflow& getInstance();
    // 析构模块
    void finalize();
    // 获取最后的错误信息
    QString getLastErrorString() const;
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
    pybind11::object getWorkflowClass() const;
    // 获取缓存的 DANodeRegistry 类引用
    pybind11::object getNodeRegistryClass() const;
    // 获取缓存的 NodeDef 装饰器引用
    pybind11::object getNodeDefDecorator() const;

private:
    // 处理异常（参考 DAPyModulePandas 模式）
    void dealException(const std::exception& e);
};
}  // namespace DA
#endif  // DAPYMODULEWORKFLOW_H