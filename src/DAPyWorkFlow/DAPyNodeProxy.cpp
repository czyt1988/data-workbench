#include "DAPyNodeProxy.h"
#include "DAPybind11InQt.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyGILGuard.h"
#include "DAPyJsonCast.h"
#include "DAPybind11QtCaster.hpp"
#include "DAPyNodeStyle.h"
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
    void dealException(const std::exception& e) const;

    // 清理Python节点引用
    void clearPyNodeRef();

    // 从Python节点同步元信息到本地缓存
    void syncMetaFromPyNode(const pybind11::object& pyNode);

public:
    QString mQualifiedName;                            ///< Python节点的限定名（如"package.module.ClassName"）
    QString mNodeName;                                 ///< 节点名称（本地缓存，也从Python node_name读取）
    QList< QString > mInputKeys;                       ///< 输入key列表（本地缓存）
    QList< QString > mOutputKeys;                      ///< 输出key列表（本地缓存）
    QString mNodePrototype;                            ///< 节点原型（本地缓存）
    QString mNodeGroup;                                ///< 节点分组（本地缓存）
    unsigned int mId { 0 };                            ///< 节点ID（独立管理）
    DAPySafePyObjectHolder mPyNodeRef;                 ///< Python节点实例的安全持有者
    DAPyNodeState mNodeState { DAPyNodeState::Idle };  ///< 节点执行状态
    mutable QString mLastErrorString;                  ///< 最后一次错误信息（mutable允许const方法修改）
    DANodeStyle mNodeStyle;                            ///< 节点样式配置（从Python描述符同步）
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
void DAPyNodeProxy::PrivateData::dealException(const std::exception& e) const
{
    mLastErrorString = e.what();
    qCritical() << "DAPyNodeProxy error:" << mLastErrorString;
}

/**
 * @brief 清理Python节点引用
 *
 * 安全释放Python节点对象引用。
 * DAPySafePyObjectHolder析构时检查Py_IsInitialized()确保安全释放。
 */
void DAPyNodeProxy::PrivateData::clearPyNodeRef()
{
    mPyNodeRef = DAPySafePyObjectHolder();
}

/**
 * @brief 从Python节点同步元信息到本地缓存
 *
 * 在setPyNodeRef时调用，从Python节点提取元信息缓存到C++本地变量，
 * 避免每次查询都需要获取GIL访问Python属性。
 *
 * 提取内容：qualified_name、node_name、input_keys、output_keys、
 * node_prototype、group、tooltip
 *
 * @param[in] pyNode Python节点实例对象
 * @note 需在GIL保护下调用此函数
 */
void DAPyNodeProxy::PrivateData::syncMetaFromPyNode(const pybind11::object& pyNode)
{
    try {
        // 提取限定名
        if (pybind11::hasattr(pyNode, "qualified_name")) {
            pybind11::object qname = pyNode.attr("qualified_name");
            mQualifiedName         = pybind11::cast< QString >(qname);
        }

        // 提取节点名称
        if (pybind11::hasattr(pyNode, "node_name")) {
            pybind11::object nodeNameObj = pyNode.attr("node_name");
            mNodeName                    = pybind11::cast< QString >(nodeNameObj);
        }

        // 同步输入key
        mInputKeys.clear();
        if (pybind11::hasattr(pyNode, "input_keys")) {
            pybind11::object inputKeysObj = pyNode.attr("input_keys");
            if (pybind11::isinstance< pybind11::list >(inputKeysObj)
                || pybind11::isinstance< pybind11::tuple >(inputKeysObj)) {
                pybind11::list inputKeysList = pybind11::cast< pybind11::list >(inputKeysObj);
                for (auto item : inputKeysList) {
                    mInputKeys.append(pybind11::cast< QString >(item));
                }
            }
        }

        // 同步输出key
        mOutputKeys.clear();
        if (pybind11::hasattr(pyNode, "output_keys")) {
            pybind11::object outputKeysObj = pyNode.attr("output_keys");
            if (pybind11::isinstance< pybind11::list >(outputKeysObj)
                || pybind11::isinstance< pybind11::tuple >(outputKeysObj)) {
                pybind11::list outputKeysList = pybind11::cast< pybind11::list >(outputKeysObj);
                for (auto item : outputKeysList) {
                    mOutputKeys.append(pybind11::cast< QString >(item));
                }
            }
        }

        // 提取节点原型
        if (pybind11::hasattr(pyNode, "node_prototype")) {
            pybind11::object protoObj = pyNode.attr("node_prototype");
            mNodePrototype            = pybind11::cast< QString >(protoObj);
        }

        // 提取分组信息
        if (pybind11::hasattr(pyNode, "group")) {
            pybind11::object groupObj = pyNode.attr("group");
            mNodeGroup                = pybind11::cast< QString >(groupObj);
        }

        // 同步样式信息（从 _node_descriptor 提取）
        if (pybind11::hasattr(pyNode, "_node_descriptor")) {
            pybind11::object descObj = pyNode.attr("_node_descriptor");
            if (pybind11::isinstance< pybind11::dict >(descObj)) {
                pybind11::dict descDict = pybind11::cast< pybind11::dict >(descObj);
                if (descDict.contains("style")) {
                    pybind11::object styleObj = descDict["style"];
                    if (!styleObj.is_none()) {
                        // 将 pybind11::dict 转换为 QJsonObject
                        pybind11::dict styleDict = pybind11::cast< pybind11::dict >(styleObj);
                        QJsonObject styleJson = DA::PY::pyDictToQJsonObject(styleDict);
                        if (!styleJson.isEmpty()) {
                            mNodeStyle = DANodeStyleFromJson(styleJson);
                        }
                    }
                }
            }
        }

    } catch (const pybind11::error_already_set& e) {
        mLastErrorString = e.what();
        dealException(e);
    } catch (const std::exception& e) {
        mLastErrorString = e.what();
        dealException(e);
    }
}

//===================================================
// DAPyNodeProxy
//===================================================

/**
 * @brief 构造Python节点代理
 *
 * 构造独立代理节点，初始状态为DAPyNodeState::Idle，未关联任何Python节点实例。
 * 不继承任何QObject或DAAbstractNode基类，使用PIMPL模式管理私有数据。
 *
 * @note 需后续调用setPyNodeRef()关联Python节点实例才能执行
 */
DAPyNodeProxy::DAPyNodeProxy() : DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构Python节点代理
 *
 * 析构时安全释放Python节点引用。
 * DAPySafePyObjectHolder会检查Py_IsInitialized()确保安全释放。
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
 * 6. 释放GIL
 *
 * error_already_set异常必须在gil_scoped_acquire作用域内消费，
 * 否则异常析构时尝试获取GIL会导致死锁。
 *
 * @return true表示执行成功，false表示执行失败或节点无效
 * @see DAPyGILGuard DAPyNodeState
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
        d->mLastErrorString = e.what();
        d->mNodeState       = DAPyNodeState::Error;
        d->dealException(e);
        return false;

    } catch (const std::exception& e) {
        d->mLastErrorString = e.what();
        d->mNodeState       = DAPyNodeState::Error;
        d->dealException(e);
        return false;
    }

    // GIL在gilGuard析构时自动释放
}

/**
 * @brief 设置Python节点实例引用
 *
 * 关联一个Python节点对象到此C++代理节点。
 * 设置后会从Python节点提取元信息（名称、输入/输出key、原型、分组等），
 * 缓存到C++本地变量，避免后续查询频繁获取GIL。
 *
 * @param[in] pyNode Python节点实例对象（必须具有execute()方法）
 * @note 需在GIL保护下调用此函数
 */
void DAPyNodeProxy::setPyNodeRef(const pybind11::object& pyNode)
{
    DA_D(d);
    if (!Py_IsInitialized()) {
        qWarning() << "DAPyNodeProxy::setPyNodeRef: Python interpreter is not initialized";
        return;
    }

    d->mPyNodeRef = DAPySafePyObjectHolder(pyNode);

    // 获取GIL并同步元信息
    DAPyGILGuard gilGuard;
    if (!gilGuard.isAcquired()) {
        qWarning() << "DAPyNodeProxy::setPyNodeRef: Failed to acquire GIL";
        return;
    }

    d->syncMetaFromPyNode(pyNode);
}

/**
 * @brief 获取Python节点实例引用
 *
 * 返回关联的Python节点对象。
 * 调用者需确保在GIL保护下使用返回的对象。
 *
 * @return Python节点实例的pybind11::object引用
 * @note 调用者需确持有GIL后再使用返回值进行Python操作
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
 * @brief 获取节点名称
 *
 * 返回本地缓存的节点名称。
 * 节点名称在setPyNodeRef时从Python节点的node_name属性同步，
 * 也可通过setNodeName()手动设置。
 *
 * @return 节点名称字符串
 */
QString DAPyNodeProxy::getNodeName() const
{
    DA_DC(d);
    return d->mNodeName;
}

/**
 * @brief 设置节点名称
 *
 * 设置本地缓存的节点名称。
 * 此方法仅修改C++侧的缓存，不回写到Python节点。
 *
 * @param[in] name 要设置的节点名称
 */
void DAPyNodeProxy::setNodeName(const QString& name)
{
    DA_D(d);
    d->mNodeName = name;
}

/**
 * @brief 获取输入key列表
 *
 * 返回本地缓存的输入key列表。
 * 输入key在setPyNodeRef时从Python节点的input_keys属性同步。
 *
 * @return 输入key的QList<QString>列表
 */
QList< QString > DAPyNodeProxy::getInputKeys() const
{
    DA_DC(d);
    return d->mInputKeys;
}

/**
 * @brief 获取输出key列表
 *
 * 返回本地缓存的输出key列表。
 * 输出key在setPyNodeRef时从Python节点的output_keys属性同步。
 *
 * @return 输出key的QList<QString>列表
 */
QList< QString > DAPyNodeProxy::getOutputKeys() const
{
    DA_DC(d);
    return d->mOutputKeys;
}

/**
 * @brief 获取节点原型
 *
 * 返回本地缓存的节点原型字符串。
 * 节点原型在setPyNodeRef时从Python节点的node_prototype属性同步。
 *
 * @return 节点原型字符串，如"package.module.ClassName"
 */
QString DAPyNodeProxy::getNodePrototype() const
{
    DA_DC(d);
    return d->mNodePrototype;
}

/**
 * @brief 获取节点分组
 *
 * 返回本地缓存的节点分组字符串。
 * 分组信息在setPyNodeRef时从Python节点的group属性同步。
 *
 * @return 节点分组字符串
 */
QString DAPyNodeProxy::getNodeGroup() const
{
    DA_DC(d);
    return d->mNodeGroup;
}

/**
 * @brief 获取Python节点描述符
 *
 * 从Python节点获取元信息描述符，包含节点的输入/输出定义、
 * 参数说明等，转换为QJsonObject供C++侧使用。
 * 此方法每次调用都会获取GIL访问Python节点，开销较高，
 * 建议仅在需要完整描述信息时调用。
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
 * @brief 获取节点样式配置
 *
 * 返回从 Python 侧 _node_descriptor 中同步的 DANodeStyle。
 * 若 Python 侧未提供样式信息，返回默认样式。
 *
 * @return 节点样式配置
 */
DANodeStyle DAPyNodeProxy::getNodeStyle() const
{
    DA_DC(d);
    return d->mNodeStyle;
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

/**
 * @brief 获取节点ID
 *
 * 返回独立管理的节点ID，不继承DAAbstractNode的ID系统。
 *
 * @return 节点ID（unsigned int）
 */
unsigned int DAPyNodeProxy::getID() const
{
    DA_DC(d);
    return d->mId;
}

/**
 * @brief 设置节点ID
 *
 * 设置独立管理的节点ID。
 *
 * @param[in] id 要设置的节点ID
 */
void DAPyNodeProxy::setID(unsigned int id)
{
    DA_D(d);
    d->mId = id;
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

}  // namespace DA
