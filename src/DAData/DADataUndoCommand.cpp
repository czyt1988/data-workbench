#include "DADataUndoCommand.h"
#include <QTemporaryDir>
#include <QFile>
#include <QUuid>
#include <QDebug>
#include "DADataManager.h"

// 全局临时目录
static QTemporaryDir g_tempDir;

namespace DA
{

DADataAbstractUndoCommand::DADataAbstractUndoCommand(QUndoCommand* par) : QUndoCommand(par), DACallBackInterface()
{
}

DADataAbstractUndoCommand::~DADataAbstractUndoCommand()
{
}

void DADataAbstractUndoCommand::setSkipFirstRedo(bool skip)
{
    m_skipFirstRedo = skip;
}

bool DADataAbstractUndoCommand::isSkipFirstRedo() const
{
    return m_skipFirstRedo;
}

void DADataAbstractUndoCommand::skipFirstRedo()
{
    m_skipFirstRedo = false;
}

//----------------------------------------------------
// DADataUndoCommand
//----------------------------------------------------
DADataObjectSwapUndoCommand::DADataObjectSwapUndoCommand(QUndoCommand* par) : DADataAbstractUndoCommand(par)
{
}

DADataObjectSwapUndoCommand::~DADataObjectSwapUndoCommand()
{
}

void DADataObjectSwapUndoCommand::setOldData(const DAData& data)
{
    m_oldObject = data.toPyObject();
    m_data      = data;
}

void DADataObjectSwapUndoCommand::setNewData(const DAData& data)
{
    m_newObject = data.toPyObject();
    m_data      = data;
}

void DADataObjectSwapUndoCommand::undo()
{
    m_data.setPyObject(m_oldObject);
    DADataManager* mgr = m_data.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(m_data, DADataManager::ChangeValue);
    }
    callback();
}

void DADataObjectSwapUndoCommand::redo()
{
    m_data.setPyObject(m_newObject);
    DADataManager* mgr = m_data.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(m_data, DADataManager::ChangeValue);
    }
    callback();
}
//----------------------------------------------------
// DADataFileCacheUndoCommand
//----------------------------------------------------

DADataObjectPersistUndoCommand::DADataObjectPersistUndoCommand(QUndoCommand* par) : DADataAbstractUndoCommand(par)
{
    if (!g_tempDir.isValid()) {
        qDebug() << "invalid temp dir,can not create temporary dir";
    }
}

DADataObjectPersistUndoCommand::~DADataObjectPersistUndoCommand()
{
    // 清理临时文件
    if (!m_oldObjectPath.isEmpty() && QFile::exists(m_oldObjectPath)) {
        QFile::remove(m_oldObjectPath);
    }
    if (!m_newObjectPath.isEmpty() && QFile::exists(m_newObjectPath)) {
        QFile::remove(m_newObjectPath);
    }
}

void DADataObjectPersistUndoCommand::setOldData(const DAData& data)
{
    if (!data.isDataFrame() && !data.isSeries()) {
        return;
    }
    QString path    = g_tempDir.path() + "/obj_old_" + QUuid::createUuid().toString(QUuid::Id128) + ".pkl";
    m_oldObjectPath = path;
    m_data          = data;
    dumpObj(data.toPyObject(), m_oldObjectPath);
}

void DADataObjectPersistUndoCommand::setNewData(const DAData& data)
{
    if (!data.isDataFrame() && !data.isSeries()) {
        return;
    }
    QString path    = g_tempDir.path() + "/obj_new_" + QUuid::createUuid().toString(QUuid::Id128) + ".pkl";
    m_newObjectPath = path;
    m_data          = data;
    dumpObj(data.toPyObject(), m_newObjectPath);
}

void DADataObjectPersistUndoCommand::undo()
{
    // 2. 从旧文件加载对象
    if (m_oldObjectPath.isEmpty()) {
        qDebug() << "m_oldObjectPath is empty";
        return;
    }
    m_data.setPyObject(loadObj(m_oldObjectPath));
    DADataManager* mgr = m_data.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(m_data, DADataManager::ChangeValue);
    }
    callback();
}

void DADataObjectPersistUndoCommand::redo()
{
    if (isSkipFirstRedo()) {
        skipFirstRedo();
        return;
    }
    if (m_newObjectPath.isEmpty()) {
        qDebug() << "m_newObjectPath is empty";
        return;
    }
    m_data.setPyObject(loadObj(m_newObjectPath));
    DADataManager* mgr = m_data.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(m_data, DADataManager::ChangeValue);
    }
    callback();
}

void DADataObjectPersistUndoCommand::dumpObj(const pybind11::object& obj, const QString& path)
{
    static auto dumps = pybind11::module_::import("pickle").attr("dumps");
    static auto open  = pybind11::module_::import("builtins").attr("open");

    auto bytes = dumps(obj);  // 二进制 bytes
    auto io    = open(path.toStdString(), "wb");
    io.attr("write")(bytes);
    io.attr("close")();
}

pybind11::object DADataObjectPersistUndoCommand::loadObj(const QString& path)
{
    static auto pickle = pybind11::module_::import("pickle");
    static auto open   = pybind11::module_::import("builtins").attr("open");
    auto io            = open(path.toStdString(), "rb");
    auto bytes         = io.attr("read")();
    io.attr("close")();
    return pickle.attr("loads")(bytes);
}

bool DADataObjectPersistUndoCommand::isValid() const
{
    return (!m_oldObjectPath.isEmpty()) && (!m_newObjectPath.isEmpty());
}

}  // end namspace DA
