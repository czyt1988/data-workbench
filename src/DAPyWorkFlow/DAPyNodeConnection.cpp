#include "DAPyNodeConnection.h"
#include "DAPybind11InQt.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPybind11QtCaster.hpp"
namespace DA
{

//===================================================
// DAPyNodeConnection
//===================================================

/**
 * @brief 构造一个none connection
 */
DAPyNodeConnection::DAPyNodeConnection() : DAPyObjectWrapper()
{
}

/**
 * @brief 从 pybind11::object 构造连接代理
 * @param[in] pyConnection Python 连接对象
 */
DAPyNodeConnection::DAPyNodeConnection(const pybind11::object& pyConnection) : DAPyObjectWrapper(pyConnection)
{
}

/**
 * @brief 从 pybind11::object 移动构造连接代理
 * @param[in] pyConnection Python 连接对象（右值引用）
 */
DAPyNodeConnection::DAPyNodeConnection(pybind11::object&& pyConnection) : DAPyObjectWrapper(std::move(pyConnection))
{
}

/**
 * @brief 从 DAPyObjectWrapper 构造连接代理
 * @param[in] pyConnection 已有的 Python 对象包装器
 */
DAPyNodeConnection::DAPyNodeConnection(const DAPyObjectWrapper& pyConnection) : DAPyObjectWrapper(pyConnection)
{
}

/**
 * @brief 拷贝构造连接代理
 * @param[in] other 另一个连接代理
 */
DAPyNodeConnection::DAPyNodeConnection(const DAPyNodeConnection& other) : DAPyObjectWrapper(other)
{
}

/**
 * @brief 从4个字符串构造Python DAConnection实例
 *
 * 通过DAPyModuleWorkflow导入DAWorkbench.DAWorkFlowPy模块，
 * 获取DAConnection类并创建实例。
 */
DAPyNodeConnection::DAPyNodeConnection(const QString& source_node_id,
                                       const QString& source_output_channel,
                                       const QString& target_node_id,
                                       const QString& target_input_channel)
    : DAPyObjectWrapper()
{
    try {
        DAPyModuleWorkflow pyModule = DAPyModuleWorkflow();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyNodeConnection: cannot import DAWorkbench.DAWorkFlowPy";
                return;
            }
        }
        pybind11::object connClass = pyModule.attr("DAConnection");
        object()                   = connClass(pybind11::cast(source_node_id),
                             pybind11::cast(source_output_channel),
                             pybind11::cast(target_node_id),
                             pybind11::cast(target_input_channel));
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 析构函数
 */
DAPyNodeConnection::~DAPyNodeConnection()
{
}

/**
 * @brief 获取源节点ID
 * @return 源节点ID字符串，代理为空时返回空字符串
 */
QString DAPyNodeConnection::getSourceNodeId() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return attr("source_node_id").cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取源输出通道
 * @return 源输出通道字符串，代理为空时返回空字符串
 */
QString DAPyNodeConnection::getSourceOutputChannel() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return attr("source_output_channel").cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取目标节点ID
 * @return 目标节点ID字符串，代理为空时返回空字符串
 */
QString DAPyNodeConnection::getTargetNodeId() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return attr("target_node_id").cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取目标输入通道
 * @return 目标输入通道字符串，代理为空时返回空字符串
 */
QString DAPyNodeConnection::getTargetInputChannel() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return attr("target_input_channel").cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 获取连接ID
 * @return 连接ID字符串，代理为空时返回空字符串
 */
QString DAPyNodeConnection::getConnectionId() const
{
    if (isNone()) {
        return QString();
    }
    try {
        return attr("connection_id").cast< QString >();
    } catch (const std::exception& e) {
        dealException(e);
    }
    return QString();
}

/**
 * @brief 设置源节点ID
 * @param[in] source_node_id 源节点ID
 */
void DAPyNodeConnection::setSourceNodeId(const QString& source_node_id)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::setattr(object(), "source_node_id", pybind11::cast(source_node_id));
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 设置源输出通道
 * @param[in] source_output_channel 源输出通道
 */
void DAPyNodeConnection::setSourceOutputChannel(const QString& source_output_channel)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::setattr(object(), "source_output_channel", pybind11::cast(source_output_channel));
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 设置目标节点ID
 * @param[in] target_node_id 目标节点ID
 */
void DAPyNodeConnection::setTargetNodeId(const QString& target_node_id)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::setattr(object(), "target_node_id", pybind11::cast(target_node_id));
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 设置目标输入通道
 * @param[in] target_input_channel 目标输入通道
 */
void DAPyNodeConnection::setTargetInputChannel(const QString& target_input_channel)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::setattr(object(), "target_input_channel", pybind11::cast(target_input_channel));
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 设置连接ID
 * @param[in] connection_id 连接ID
 */
void DAPyNodeConnection::setConnectionId(const QString& connection_id)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::setattr(object(), "connection_id", pybind11::cast(connection_id));
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 批量设置连接信息
 * @param[in] source_node_id 源节点ID
 * @param[in] source_output_channel 源输出通道
 * @param[in] target_node_id 目标节点ID
 * @param[in] target_input_channel 目标输入通道
 */
void DAPyNodeConnection::setConnection(const QString& source_node_id,
                                       const QString& source_output_channel,
                                       const QString& target_node_id,
                                       const QString& target_input_channel)
{
    if (isNone()) {
        return;
    }
    try {
        pybind11::setattr(object(), "source_node_id", pybind11::cast(source_node_id));
        pybind11::setattr(object(), "source_output_channel", pybind11::cast(source_output_channel));
        pybind11::setattr(object(), "target_node_id", pybind11::cast(target_node_id));
        pybind11::setattr(object(), "target_input_channel", pybind11::cast(target_input_channel));
    } catch (const std::exception& e) {
        dealException(e);
    }
}

/**
 * @brief 相等比较运算符（以 connection_id 为判据）
 * @param[in] other 另一个连接代理
 * @return connection_id 相同返回 true
 */
bool DAPyNodeConnection::operator==(const DAPyNodeConnection& other) const
{
    return getConnectionId() == other.getConnectionId();
}

/**
 * @brief 不等比较运算符
 * @param[in] other 另一个连接代理
 * @return connection_id 不同返回 true
 */
bool DAPyNodeConnection::operator!=(const DAPyNodeConnection& other) const
{
    return !(*this == other);
}

/**
 * @brief 小于运算符（用于排序）
 * @param[in] other 另一个连接代理
 * @return 当前连接 connection_id 小于 other 的返回 true
 */
bool DAPyNodeConnection::operator<(const DAPyNodeConnection& other) const
{
    return getConnectionId() < other.getConnectionId();
}

/**
 * @brief 计算 DAPyNodeConnection 的哈希值
 * @param[in] key 连接代理
 * @param[in] seed 哈希种子
 * @return 哈希值
 */
uint qHash(const DAPyNodeConnection& key, uint seed)
{
    return ::qHash(key.getConnectionId(), seed);
}

/**
 * @brief 输出 DAPyNodeConnection 信息到 QDebug
 * @param[in] dbg QDebug 对象
 * @param[in] conn 连接代理
 * @return QDebug 对象
 */
QDebug operator<<(QDebug dbg, const DAPyNodeConnection& conn)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "DAPyNodeConnection(" << conn.getSourceNodeId() << ":" << conn.getSourceOutputChannel() << " -> "
                  << conn.getTargetNodeId() << ":" << conn.getTargetInputChannel() << ", id=" << conn.getConnectionId()
                  << ")";
    return dbg;
}

}  // namespace DA
