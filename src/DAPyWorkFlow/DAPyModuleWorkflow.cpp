#include "DAPyModuleWorkflow.h"
#include <QDebug>
#include "DAPybind11InQt.h"

namespace DA
{
class DAPyModuleWorkflow::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyModuleWorkflow)
public:
    PrivateData(DAPyModuleWorkflow* p);

    // 释放模块
    void del();

public:
    QString mLastErrorString;
    // 缓存 Python 类引用，避免每次 attr() 查找
    pybind11::object mObjWorkflowClass;
    pybind11::object mObjNodeRegistryClass;
    pybind11::object mObjNodeDefDecorator;
};

//===================================================
// DAPyModuleWorkflowPrivate
//===================================================

DAPyModuleWorkflow::PrivateData::PrivateData(DAPyModuleWorkflow* p) : q_ptr(p)
{
}

void DAPyModuleWorkflow::PrivateData::del()
{
    if (!q_ptr->isImport()) {
        return;
    }
    q_ptr->object()       = pybind11::none();
    mObjWorkflowClass     = pybind11::none();
    mObjNodeRegistryClass = pybind11::none();
    mObjNodeDefDecorator  = pybind11::none();
}

//===================================================
// DAPyModuleWorkflow
//===================================================
DAPyModuleWorkflow::DAPyModuleWorkflow() : DAPyModule(), DA_PIMPL_CONSTRUCT
{
    import();  // 1. 先导入模块
    try {
        // 2. 惰性缓存关键 Python 对象
        d_ptr->mObjWorkflowClass     = attr("DAWorkflow");
        d_ptr->mObjNodeRegistryClass = attr("DANodeRegistry");
        d_ptr->mObjNodeDefDecorator  = attr("NodeDef");
    } catch (const std::exception& e) {
        dealException(e);
    }
}

DAPyModuleWorkflow::~DAPyModuleWorkflow()
{
}

/**
 * @brief 获取实例（单例模式）
 * @return
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
    d_ptr->del();
}

/**
 * @brief 获取最后的错误信息
 * @return
 */
QString DAPyModuleWorkflow::getLastErrorString() const
{
    return d_ptr->mLastErrorString;
}

/**
 * @brief 导入模块
 * @return
 */
bool DAPyModuleWorkflow::import()
{
    return DAPyModule::import("DAWorkbench.DAWorkFlowPy");  // 3. 调用基类 importModule
}

/**
 * @brief 判断是否为 DAWorkflow 实例
 * @param obj
 * @return
 */
bool DAPyModuleWorkflow::isInstanceWorkflow(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjWorkflowClass);
}

/**
 * @brief 判断是否为 DANodeRegistry 实例
 * @param obj
 * @return
 */
bool DAPyModuleWorkflow::isInstanceNodeRegistry(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjNodeRegistryClass);
}

/**
 * @brief 判断是否为 NodeDef 装饰器类
 * @param obj
 * @return
 */
bool DAPyModuleWorkflow::isInstanceNodeDef(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjNodeDefDecorator);
}

/**
 * @brief 获取缓存的 DAWorkflow 类引用
 * @return
 */
pybind11::object DAPyModuleWorkflow::getWorkflowClass() const
{
    return d_ptr->mObjWorkflowClass;
}

/**
 * @brief 获取缓存的 DANodeRegistry 类引用
 * @return
 */
pybind11::object DAPyModuleWorkflow::getNodeRegistryClass() const
{
    return d_ptr->mObjNodeRegistryClass;
}

/**
 * @brief 获取缓存的 NodeDef 装饰器引用
 * @return
 */
pybind11::object DAPyModuleWorkflow::getNodeDefDecorator() const
{
    return d_ptr->mObjNodeDefDecorator;
}

/**
 * @brief 处理异常（参考 DAPyModulePandas 模式）
 * @param e
 */
void DAPyModuleWorkflow::dealException(const std::exception& e)
{
    d_ptr->mLastErrorString = e.what();
    qCritical() << "DAPyModuleWorkflow error:" << d_ptr->mLastErrorString;
}
}  // namespace DA