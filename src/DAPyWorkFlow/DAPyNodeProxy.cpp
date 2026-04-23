#include "DAPyNodeProxy.h"
#include "DAPybind11InQt.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyGILGuard.h"
#include "DAPyJsonCast.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

namespace DA
{
class DAPyNodeProxy::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNodeProxy)
public:
    PrivateData(DAPyNodeProxy* p);

    // 统一异常处理（参考DAPyModulePandas模式）
    void dealException(const std::exception& e);

    // 清理Python节点引用
    void clearPyNodeRef();

public:
    QString mQualifiedName;                 ///< Python节点的限定名（如"package.module.ClassName"）
    DAPySafePyObjectHolder mPyNodeRef;      ///< Python节点实例的安全持有者
    DAPyNodeState mNodeState { DAPyNodeState::Idle };  ///< 节点执行状态
    QString mLastErrorString;               ///< 最后一次错误信息
};

//===================================================
// DAPyNodeProxy::PrivateData
//===================================================

DAPyNodeProxy::PrivateData::PrivateData(DAPyNodeProxy* p) : q_ptr(p)
{
}

/**
 * @brief 统一异常处理
 *
 * 参考DAPyModulePandas的dealException模式，将异常信息存储到mLastErrorString中。
 * 对于pybind11::error_already_set异常，在GIL作用域内消费，
 * 避免异常析构时尝试获取GIL导致死锁。
 *
 * @param[in] e 捕获的异常对象
 */
void DAPyNodeProxy::PrivateData::dealException(const std::exception& e)
{
    mLastErrorString = e.what();
    qCritical() << "DAPyNodeProxy error:" << mLastErrorString;
}

/**
 * @brief 清理Python节点引用
 *
 * 安全释放Python节点对象引用。
 * 析构时检查Py_IsInitialized()，如果Python已终止则使用release()避免崩溃。
 */
void DAPyNodeProxy::PrivateData::clearPyNodeRef()
{
    mPyNodeRef = DAPySafePyObjectHolder();
}

//===================================================
// DAPyNodeProxy
//===================================================

/**
 * @brief 构造Python节点代理
 *
 * 构造时初始化DAAbstractNode基类和DAPyNodeProxy的私有数据。
 * 代理节点初始状态为DAPyNodeState::Idle，未关联任何Python节点实例。
 *
 * @note 需后续调用setPyNodeRef()关联Python节点实例才能执行
 */
DAPyNodeProxy::DAPyNodeProxy() : DAAbstractNode(), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构Python节点代理
 *
 * 析构时安全释放Python节点引用。
 * DAPySafePyObjectHolder会检查Py_IsInitialized()确保安全释放。
 *
 * @note DAAbstractNode基类的析构会自动detachAll()解除所有连接
 */
DAPyNodeProxy::~DAPyNodeProxy()
{
    d_ptr->clearPyNodeRef();
}

/**
 * @brief 执行节点
 *
 * 核心执行流程：
 * 1. 检查Python节点引用是否有效
 * 2. 设置节点状态为Running
 * 3. 获取Python GIL（DAPyGILGuard RAII守卫）
 * 4. 调用Python节点的execute()方法
 * 5. 在GIL作用域内捕获error_already_set异常 → 提取错误信息 → 设置节点状态为Error
 * 6. 释放GIL后通过DAPythonSignalHandler发射完成信号
 *
 * error_already_set异常必须在gil_scoped_acquire作用域内消费，
 * 否则异常析构时尝试获取GIL会导致死锁。
 *
 * @return true表示执行成功，false表示执行失败或节点无效
 * @see DAPyGILGuard DAPyNodeState DAPyModuleWorkflow
 */
bool DAPyNodeProxy::exec()
{
    DA_D(d);
    // 检查Python节点引用是否有效
    if (!d->mPyNodeRef) {
        d->mLastErrorString = QString("Python node reference is not set, cannot execute");
        d->mNodeState       = DAPyNodeState::Error;
        qCritical() << d->mLastErrorString;
        return false;
    }

    // 设置状态为Running
    d->mNodeState = DAPyNodeState::Running;

    // 获取GIL — 所有Python交互必须在GIL保护下进行
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        d->mLastErrorString = QString("Failed to acquire GIL for node execution");
        d->mNodeState       = DAPyNodeState::Error;
        qCritical() << d->mLastErrorString;
        return false;
    }

    try {
        // 获取Python节点实例
        pybind11::object pyNode = d->mPyNodeRef.object();

        // 调用Python节点的execute()方法
        pybind11::object result = pyNode.attr("execute")();

        // 检查返回值是否为bool类型
        if (pybind11::isinstance< pybind11::bool_ >(result)) {
            bool success = result.cast< bool >();
            if (success) {
                d->mNodeState = DAPyNodeState::Success;
            } else {
                d->mNodeState       = DAPyNodeState::Error;
                d->mLastErrorString = QString("Python node execute() returned False");
            }
            return success;
        }

        // 返回值非bool类型，视为成功
        d->mNodeState = DAPyNodeState::Success;
        return true;

    } catch (const pybind11::error_already_set& e) {
        // error_already_set必须在GIL作用域内消费
        // 提取Python异常信息并存储到mLastErrorString
        d->mLastErrorString = e.what();
        d->mNodeState       = DAPyNodeState::Error;
        d->dealException(e);
        return false;

    } catch (const std::exception& e) {
        // 其他C++标准异常
        d->mLastErrorString = e.what();
        d->mNodeState       = DAPyNodeState::Error;
        d->dealException(e);
        return false;
    }

    // GIL在gilGuard析构时自动释放
}

/**
 * @brief 创建节点对应的图形项
 *
 * 返回DAPyNodeGraphicsItem实例用于在前端显示此代理节点。
 * 当前返回nullptr，待DAPyNodeGraphicsItem实现后替换。
 *
 * @return 节点对应的DAAbstractNodeGraphicsItem指针，当前为nullptr
 * @note 此函数由DAWorkFlow在创建节点图形时调用
 */
DAAbstractNodeGraphicsItem* DAPyNodeProxy::createGraphicsItem()
{
    // TODO: 待DAPyNodeGraphicsItem实现后，创建并返回DAPyNodeGraphicsItem实例
    return nullptr;
}

/**
 * @brief 设置Python节点实例引用
 *
 * 关联一个Python节点对象到此C++代理节点。
 * 设置后会从Python节点提取元信息（输入/输出key等）。
 *
 * @param[in] pyNode Python节点实例对象（必须具有execute()方法）
 * @note 需在GIL保护下调用此函数
 * @note 如果Python节点具有input_keys/output_keys属性，将自动同步到DAAbstractNode
 */
void DAPyNodeProxy::setPyNodeRef(const pybind11::object& pyNode)
{
    DA_D(d);
    if (!Py_IsInitialized()) {
        qWarning() << "DAPyNodeProxy::setPyNodeRef: Python interpreter is not initialized";
        return;
    }

    d->mPyNodeRef = DAPySafePyObjectHolder(pyNode);

    // 尝试从Python节点提取元信息
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qWarning() << "DAPyNodeProxy::setPyNodeRef: Failed to acquire GIL";
        return;
    }

    try {
        // 提取限定名
        if (pybind11::hasattr(pyNode, "qualified_name")) {
            pybind11::object qname = pyNode.attr("qualified_name");
            d->mQualifiedName      = pybind11::cast< QString >(qname);
        }

        // 同步输入key
        if (pybind11::hasattr(pyNode, "input_keys")) {
            pybind11::object inputKeysObj = pyNode.attr("input_keys");
            if (pybind11::isinstance< pybind11::list >(inputKeysObj)
                || pybind11::isinstance< pybind11::tuple >(inputKeysObj)) {
                pybind11::list inputKeysList = pybind11::cast< pybind11::list >(inputKeysObj);
                for (auto item : inputKeysList) {
                    QString keyStr = pybind11::cast< QString >(item);
                    addInputKey(keyStr);
                }
            }
        }

        // 同步输出key
        if (pybind11::hasattr(pyNode, "output_keys")) {
            pybind11::object outputKeysObj = pyNode.attr("output_keys");
            if (pybind11::isinstance< pybind11::list >(outputKeysObj)
                || pybind11::isinstance< pybind11::tuple >(outputKeysObj)) {
                pybind11::list outputKeysList = pybind11::cast< pybind11::list >(outputKeysObj);
                for (auto item : outputKeysList) {
                    QString keyStr = pybind11::cast< QString >(item);
                    addOutputKey(keyStr);
                }
            }
        }

        // 提取节点名称
        if (pybind11::hasattr(pyNode, "node_name")) {
            pybind11::object nodeNameObj = pyNode.attr("node_name");
            QString nodeName             = pybind11::cast< QString >(nodeNameObj);
            setNodeName(nodeName);
        }

        // 提取节点原型
        if (pybind11::hasattr(pyNode, "node_prototype")) {
            pybind11::object protoObj = pyNode.attr("node_prototype");
            QString proto             = pybind11::cast< QString >(protoObj);
            metaData().setNodePrototype(proto);
        }

        // 提取分组信息
        if (pybind11::hasattr(pyNode, "group")) {
            pybind11::object groupObj = pyNode.attr("group");
            QString group             = pybind11::cast< QString >(groupObj);
            metaData().setGroup(group);
        }

        // 提取tooltip
        if (pybind11::hasattr(pyNode, "tooltip")) {
            pybind11::object tooltipObj = pyNode.attr("tooltip");
            QString tooltip             = pybind11::cast< QString >(tooltipObj);
            setNodeTooltip(tooltip);
        }

    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    } catch (const std::exception& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    }
}

/**
 * @brief 获取Python节点实例引用
 *
 * 返回关联的Python节点对象。
 * 调用者需确保在GIL保护下使用返回的对象。
 *
 * @return Python节点实例的pybind11::object引用
 * @note 调用者需确保持有GIL后再使用返回值进行Python操作
 */
pybind11::object DAPyNodeProxy::getPyNodeRef() const
{
    DA_DC(d);
    return d->mPyNodeRef.object();
}

/**
 * @brief 判断是否关联了有效的Python节点实例
 *
 * @return true表示已关联有效的Python节点，false表示未关联或引用为None
 */
bool DAPyNodeProxy::hasPyNodeRef() const
{
    DA_DC(d);
    return static_cast< bool >(d->mPyNodeRef);
}

/**
 * @brief 设置Python节点的限定名
 *
 * @param[in] name Python节点的限定名（如"package.module.ClassName"）
 */
void DAPyNodeProxy::setQualifiedName(const QString& name)
{
    DA_D(d);
    d->mQualifiedName = name;
}

/**
 * @brief 获取Python节点的限定名
 *
 * @return Python节点的限定名
 */
QString DAPyNodeProxy::getQualifiedName() const
{
    DA_DC(d);
    return d->mQualifiedName;
}

/**
 * @brief 获取节点执行状态
 *
 * @return 当前节点的DAPyNodeState状态
 * @see DAPyNodeState
 */
DAPyNodeState DAPyNodeProxy::getNodeState() const
{
    DA_DC(d);
    return d->mNodeState;
}

/**
 * @brief 设置节点执行状态
 *
 * @param[in] state 要设置的状态
 * @see DAPyNodeState
 */
void DAPyNodeProxy::setNodeState(DAPyNodeState state)
{
    DA_D(d);
    d->mNodeState = state;
}

/**
 * @brief 获取最后一次错误信息
 *
 * 在exec()执行失败或Python交互出错后，错误信息存储在mLastErrorString中。
 *
 * @return 最后一次错误的描述字符串，若无错误返回空字符串
 */
QString DAPyNodeProxy::getLastErrorString() const
{
    DA_DC(d);
    return d->mLastErrorString;
}

/**
 * @brief 通过pybind11::object设置Python原生输入数据
 *
 * 直接将Python对象作为输入数据传递给Python节点，
 * 避免QVariant中间转换的性能损耗。
 *
 * @param[in] key 输入参数的key名称
 * @param[in] data Python数据对象
 * @note 需在GIL保护下调用此函数
 */
void DAPyNodeProxy::setPyInputData(const QString& key, const pybind11::object& data)
{
    DA_D(d);
    if (!d->mPyNodeRef) {
        qWarning() << "DAPyNodeProxy::setPyInputData: No Python node reference";
        return;
    }

    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qWarning() << "DAPyNodeProxy::setPyInputData: Failed to acquire GIL";
        return;
    }

    try {
        pybind11::object pyNode = d->mPyNodeRef.object();
        if (pybind11::hasattr(pyNode, "set_input_data")) {
            pybind11::str pyKey = pybind11::cast(key);
            pyNode.attr("set_input_data")(pyKey, data);
        }
    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    } catch (const std::exception& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    }
}

/**
 * @brief 通过pybind11::object获取Python原生输出数据
 *
 * 直接从Python节点获取输出数据对象，
 * 避免QVariant中间转换的性能损耗。
 *
 * @param[in] key 输出参数的key名称
 * @return Python输出数据对象，若获取失败返回pybind11::none()
 * @note 需在GIL保护下调用此函数
 */
pybind11::object DAPyNodeProxy::getPyOutputData(const QString& key) const
{
    DA_DC(d);
    if (!d->mPyNodeRef) {
        return pybind11::none();
    }

    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        return pybind11::none();
    }

    try {
        pybind11::object pyNode = d->mPyNodeRef.object();
        if (pybind11::hasattr(pyNode, "get_output_data")) {
            pybind11::str pyKey = pybind11::cast(key);
            return pyNode.attr("get_output_data")(pyKey);
        }
    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    } catch (const std::exception& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    }

    return pybind11::none();
}

/**
 * @brief 获取Python节点描述符
 *
 * 从Python节点获取元信息描述符，包含节点的输入/输出定义、
 * 参数说明等，转换为QJsonObject供C++侧使用。
 *
 * @return QJsonObject格式的节点描述符，若获取失败返回空QJsonObject
 */
QJsonObject DAPyNodeProxy::getDescriptor() const
{
    DA_DC(d);
    if (!d->mPyNodeRef) {
        return QJsonObject();
    }

    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        return QJsonObject();
    }

    try {
        pybind11::object pyNode = d->mPyNodeRef.object();
        if (pybind11::hasattr(pyNode, "get_descriptor")) {
            pybind11::object descObj = pyNode.attr("get_descriptor")();
            if (pybind11::isinstance< pybind11::dict >(descObj)) {
                pybind11::dict descDict = pybind11::cast< pybind11::dict >(descObj);
                return DA::PY::pyDictToQJsonObject(descDict);
            }
        }
    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    } catch (const std::exception& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    }

    return QJsonObject();
}

/**
 * @brief 检查代理节点是否有效
 *
 * 有效条件：Python解释器已初始化且已关联有效的Python节点实例。
 *
 * @return true表示代理节点有效可执行，false表示无效
 */
bool DAPyNodeProxy::isValid() const
{
    DA_DC(d);
    if (!Py_IsInitialized()) {
        return false;
    }
    return static_cast< bool >(d->mPyNodeRef);
}

/**
 * @brief 设置Python节点配置参数
 *
 * 通过QJsonObject设置Python节点的配置参数，
 * 内部转换为pybind11::dict传递给Python节点。
 *
 * @param[in] config QJsonObject格式的配置参数
 * @return true表示设置成功，false表示设置失败
 */
bool DAPyNodeProxy::setConfig(const QJsonObject& config)
{
    DA_D(d);
    if (!d->mPyNodeRef) {
        qWarning() << "DAPyNodeProxy::setConfig: No Python node reference";
        return false;
    }

    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        return false;
    }

    try {
        pybind11::object pyNode = d->mPyNodeRef.object();
        if (pybind11::hasattr(pyNode, "set_config")) {
            pybind11::dict pyConfig = DA::PY::qjsonObjectToPyDict(config);
            pyNode.attr("set_config")(pyConfig);
            return true;
        }
    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    } catch (const std::exception& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    }

    return false;
}

/**
 * @brief 获取Python节点配置参数
 *
 * 从Python节点获取配置参数，
 * 内部从pybind11::dict转换为QJsonObject供C++侧使用。
 *
 * @return QJsonObject格式的配置参数，若获取失败返回空QJsonObject
 */
QJsonObject DAPyNodeProxy::getConfig() const
{
    DA_DC(d);
    if (!d->mPyNodeRef) {
        return QJsonObject();
    }

    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        return QJsonObject();
    }

    try {
        pybind11::object pyNode = d->mPyNodeRef.object();
        if (pybind11::hasattr(pyNode, "get_config")) {
            pybind11::object configObj = pyNode.attr("get_config")();
            if (pybind11::isinstance< pybind11::dict >(configObj)) {
                pybind11::dict configDict = pybind11::cast< pybind11::dict >(configObj);
                return DA::PY::pyDictToQJsonObject(configDict);
            }
        }
    } catch (const pybind11::error_already_set& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    } catch (const std::exception& e) {
        d->mLastErrorString = e.what();
        d->dealException(e);
    }

    return QJsonObject();
}

}  // namespace DA