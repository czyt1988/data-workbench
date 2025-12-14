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
    m_isValid = true;
}

/**
 * @brief 在脚本端更新后，设置回的新对象
 * @param obj
 */
void DADataUndoCommand::setNewObject(const pybind11::object& obj)
{
    m_data.toPyObject() = obj;
}

void DADataUndoCommand::undo()
{
    // 1. 将当前新对象 pickle 到磁盘（为 redo 准备）
    if (m_newObjectPath.isEmpty()) {
        QString newPath = g_tempDir.path() + "/obj_new_" + QUuid::createUuid().toString(QUuid::Id128) + ".pkl";
        m_newObjectPath = newPath;
        dumpObj(m_data.toPyObject(), newPath);
    }
    // 2. 从旧文件加载对象
    m_data.toPyObject() = loadObj(m_oldObjectPath);
}

void DADataUndoCommand::redo()
{
    if (m_newObjectPath.isEmpty()) {
        // 说明是首次redo（第一次推入回退栈），这时m_newObjectPath为空，直接跳过
        return;
    }
    m_data.toPyObject() = loadObj(m_newObjectPath);
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
    return m_isValid;
}


}  // end namspace DA
