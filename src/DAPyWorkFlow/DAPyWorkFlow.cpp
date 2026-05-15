#include "DAPyWorkFlow.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyGILGuard.h"
#include "DAPybind11InQt.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyNodeProxy.h"
#include <QDebug>

namespace DA
{

//===================================================
// DAPyWorkFlow::PrivateData
//===================================================
class DAPyWorkFlow::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlow)
public:
    PrivateData(DAPyWorkFlow* p);
    // 统一异常处理
    void dealException(const std::exception& e) const;

public:
    DA::PY::safe_pyobject mPyWorkflowObj;  ///< Python DAWorkflow 实例的安全持有者
    mutable QString mLastErrorString;       ///< 最后一次错误信息（mutable允许const方法修改）
};

//===================================================
// DAPyWorkFlow::PrivateData 实现
//===================================================

DAPyWorkFlow::PrivateData::PrivateData(DAPyWorkFlow* p) : q_ptr(p)
{
}

/**
 * @brief 统一异常处理
 *
 * 参考DAPyNodeProxy的dealException模式，将异常信息存储到mLastErrorString中。
 * 对于pybind11::error_already_set异常，在GIL作用域内消费，
 * 避免异常析构时尝试获取GIL导致死锁。
 *
 * @param[in] e 捕获的异常对象
 */
void DAPyWorkFlow::PrivateData::dealException(const std::exception& e) const
{
    mLastErrorString = e.what();
    qCritical() << "DAPyWorkFlow error:" << mLastErrorString;
}

//===================================================
// DAPyWorkFlow
//===================================================

DAPyWorkFlow::DAPyWorkFlow() : DA_PIMPL_CONSTRUCT
{
}

DAPyWorkFlow::~DAPyWorkFlow()
{
}

/**
 * @brief 初始化 Python DAWorkflow 实例
 *
 * 导入 DAWorkbench.DAWorkFlowPy 模块，获取 DAWorkflow 类引用，
 * 创建 Python DAWorkflow 实例并存储到 safe_pyobject。
 * 如果已初始化则跳过重复调用。
 *
 * @note 需在 Python 解释器初始化后调用此方法
 */
void DAPyWorkFlow::initPyWorkflow()
{
    DA_D(d);
    if (!d->mPyWorkflowObj.is_none()) {
        return;
    }
    DAPyGILGuard gil;
    try {
        DAPyModuleWorkflow& pyModule = DAPyModuleWorkflow::getInstance();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlow::initPyWorkflow: cannot import DAWorkbench.DAWorkFlowPy";
                return;
            }
        }
        pybind11::object workflowClass = pyModule.getWorkflowClass();
        if (workflowClass.is_none()) {
            qWarning() << "DAPyWorkFlow::initPyWorkflow: DAWorkflow class is not available";
            return;
        }
        pybind11::object workflowInstance = workflowClass();
        d->mPyWorkflowObj               = DA::PY::safe_pyobject(std::move(workflowInstance));
        qDebug() << "DAPyWorkFlow::initPyWorkflow: Python DAWorkflow instance created";
    } catch (const pybind11::error_already_set& e) {
        qWarning() << "DAPyWorkFlow::initPyWorkflow: Python error:" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "DAPyWorkFlow::initPyWorkflow: error:" << e.what();
    }
}

/**
 * @brief 检查 Python DAWorkflow 实例是否有效
 *
 * @return true 如果 Python 实例已创建且不为 None
 */
bool DAPyWorkFlow::isValid() const
{
    DA_DC(d);
    return !d->mPyWorkflowObj.is_none();
}

//===================================================
// Wave 2 方法 stub — 后续 Wave 2 实现
//===================================================

QString DAPyWorkFlow::addNode(DAPyNodeProxy* proxy)
{
    // TODO: Wave 2 Task 8
    return QString();
}

bool DAPyWorkFlow::removeNode(const QString& nodeId)
{
    // TODO: Wave 2 Task 8
    return false;
}

DAPyWorkFlowConnection DAPyWorkFlow::connectNode(const QString& srcNodeId,
                                                  const QString& srcChannel,
                                                  const QString& dstNodeId,
                                                  const QString& dstChannel)
{
    // TODO: Wave 2 Task 9
    return DAPyWorkFlowConnection();
}

bool DAPyWorkFlow::disconnectNode(const QString& connectionId)
{
    // TODO: Wave 2 Task 9
    return false;
}

bool DAPyWorkFlow::removeConnection(const QString& connectionId)
{
    // TODO: Wave 2 Task 9
    return false;
}

void DAPyWorkFlow::clear()
{
    // TODO: Wave 2 Task 10
}

int DAPyWorkFlow::nodeCount()
{
    // TODO: Wave 2 Task 10
    return 0;
}

bool DAPyWorkFlow::hasNode(const QString& nodeId)
{
    // TODO: Wave 2 Task 10
    return false;
}

pybind11::object DAPyWorkFlow::getNodeById(const QString& nodeId)
{
    // TODO: Wave 2 Task 11
    return pybind11::none();
}

pybind11::list DAPyWorkFlow::getNodes()
{
    // TODO: Wave 2 Task 11
    return pybind11::list();
}

pybind11::list DAPyWorkFlow::getConnections()
{
    // TODO: Wave 2 Task 11
    return pybind11::list();
}

bool DAPyWorkFlow::isValidDag()
{
    // TODO: Wave 2 Task 12
    return false;
}

QStringList DAPyWorkFlow::topologicalSort()
{
    // TODO: Wave 2 Task 12
    return QStringList();
}

bool DAPyWorkFlow::executeAsync()
{
    // TODO: Wave 2 Task 13
    return false;
}

void DAPyWorkFlow::terminate()
{
    // TODO: Wave 2 Task 13
}

bool DAPyWorkFlow::pause()
{
    // TODO: Wave 2 Task 13
    return false;
}

bool DAPyWorkFlow::resume()
{
    // TODO: Wave 2 Task 13
    return false;
}

ExecState DAPyWorkFlow::getExecutorState()
{
    // TODO: Wave 2 Task 14
    return StateIdle;
}

bool DAPyWorkFlow::getResult()
{
    // TODO: Wave 2 Task 14
    return false;
}

bool DAPyWorkFlow::isRunning()
{
    // TODO: Wave 2 Task 14
    return false;
}

QString DAPyWorkFlow::getLastError() const
{
    // TODO: Wave 2 Task 15
    return QString();
}

}  // namespace DA