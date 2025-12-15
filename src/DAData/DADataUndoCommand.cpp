#include "DADataUndoCommand.h"
#include <QTemporaryDir>
#include <QFile>
#include <QUuid>
#include <QDebug>

// 全局临时目录
static QTemporaryDir g_tempDir;

namespace DA
{
DADataUndoCommand::DADataUndoCommand(QUndoCommand* par) : QUndoCommand(par)
{
    if (!g_tempDir.isValid()) {
        qDebug() << "invalid temp dir,can not create temporary dir";
    }
}

DADataUndoCommand::~DADataUndoCommand()
{
    // 清理临时文件
    if (!m_oldObjectPath.isEmpty() && QFile::exists(m_oldObjectPath)) {
        QFile::remove(m_oldObjectPath);
    }
    if (!m_newObjectPath.isEmpty() && QFile::exists(m_newObjectPath)) {
        QFile::remove(m_newObjectPath);
    }
}

void DADataUndoCommand::setOldData(const DAData& data)
{
    if (!data.isDataFrame() && !data.isSeries()) {
        return;
    }
    QString path    = g_tempDir.path() + "/obj_old_" + QUuid::createUuid().toString(QUuid::Id128) + ".pkl";
    m_oldObjectPath = path;
    m_data          = data;
    dumpObj(data.toPyObject(), m_oldObjectPath);
}

void DADataUndoCommand::setNewData(const DAData& data)
{
    if (!data.isDataFrame() && !data.isSeries()) {
        return;
    }
    QString path    = g_tempDir.path() + "/obj_new_" + QUuid::createUuid().toString(QUuid::Id128) + ".pkl";
    m_newObjectPath = path;
    m_data          = data;
    dumpObj(data.toPyObject(), m_newObjectPath);
}

void DADataUndoCommand::undo()
{
    // 2. 从旧文件加载对象
    if (m_oldObjectPath.isEmpty()) {
        qDebug() << "m_oldObjectPath is empty";
        return;
    }
    qDebug() << "DADataUndoCommand,undo,load m_oldObjectPath:" << m_oldObjectPath;
    m_data.setPyObject(loadObj(m_oldObjectPath));
}

void DADataUndoCommand::redo()
{
    if (m_skipFirstRedo) {
        m_skipFirstRedo = false;
        return;
    }
    if (m_newObjectPath.isEmpty()) {
        qDebug() << "m_newObjectPath is empty";
        return;
    }
    qDebug() << "DADataUndoCommand,redo,load m_newObjectPath:" << m_newObjectPath;
    m_data.setPyObject(loadObj(m_newObjectPath));
}

void DADataUndoCommand::dumpObj(const pybind11::object& obj, const QString& path)
{
    static auto dumps = pybind11::module_::import("pickle").attr("dumps");
    static auto open  = pybind11::module_::import("builtins").attr("open");

    auto bytes = dumps(obj);  // 二进制 bytes
    auto io    = open(path.toStdString(), "wb");
    io.attr("write")(bytes);
    io.attr("close")();
}

pybind11::object DADataUndoCommand::loadObj(const QString& path)
{
    static auto pickle = pybind11::module_::import("pickle");
    static auto open   = pybind11::module_::import("builtins").attr("open");
    auto io            = open(path.toStdString(), "rb");
    auto bytes         = io.attr("read")();
    io.attr("close")();
    return pickle.attr("loads")(bytes);
}

bool DADataUndoCommand::isValid() const
{
    return (!m_oldObjectPath.isEmpty()) && (!m_newObjectPath.isEmpty());
}

}  // end namspace DA
