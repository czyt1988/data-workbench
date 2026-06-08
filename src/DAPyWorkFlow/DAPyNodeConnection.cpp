#include "DAPyNodeConnection.h"
#include <QDebug>
#include "DAPybind11InQt.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyBindQt/DAPyGILGuard.h"
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

DAPyNodeConnection::DAPyNodeConnection(const pybind11::object& pyConnection) : DAPyObjectWrapper(pyConnection)
{
}

DAPyNodeConnection::DAPyNodeConnection(pybind11::object&& pyConnection) : DAPyObjectWrapper(std::move(pyConnection))
{
}

DAPyNodeConnection::DAPyNodeConnection(const DAPyObjectWrapper& pyConnection) : DAPyObjectWrapper(pyConnection)
{
}

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
        pybind11::object connClass  = pyModule.attr("DAConnection");
        object()                    = connClass(pybind11::cast(source_node_id),
                             pybind11::cast(source_output_channel),
                             pybind11::cast(target_node_id),
                             pybind11::cast(target_input_channel));
    } catch (const pybind11::error_already_set& e) {
        dealException(e);
    } catch (const std::exception& e) {
        dealException(e);
    }
}

DAPyNodeConnection::~DAPyNodeConnection()
{
}

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

bool DAPyNodeConnection::operator==(const DAPyNodeConnection& other) const
{
    return getConnectionId() == other.getConnectionId();
}

bool DAPyNodeConnection::operator!=(const DAPyNodeConnection& other) const
{
    return !(*this == other);
}

bool DAPyNodeConnection::operator<(const DAPyNodeConnection& other) const
{
    return getConnectionId() < other.getConnectionId();
}

uint qHash(const DAPyNodeConnection& key, uint seed)
{
    return ::qHash(key.getConnectionId(), seed);
}

QDebug operator<<(QDebug dbg, const DAPyNodeConnection& conn)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "DAPyNodeConnection(" << conn.getSourceNodeId() << ":" << conn.getSourceOutputChannel() << " -> "
                  << conn.getTargetNodeId() << ":" << conn.getTargetInputChannel() << ", id=" << conn.getConnectionId()
                  << ")";
    return dbg;
}

}  // namespace DA
