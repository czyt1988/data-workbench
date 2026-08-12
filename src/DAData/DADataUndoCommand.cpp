#include "DADataUndoCommand.h"
#include <QTemporaryDir>
#include <QFile>
#include <QUuid>
#include <QDebug>
#include "DADataManager.h"
#include "DAPybind11QtCaster.hpp"

// 全局临时目录
static QTemporaryDir g_tempDir;

namespace DA
{

/**
 * @brief 构造函数
 * @param par 父命令
 */
DADataAbstractUndoCommand::DADataAbstractUndoCommand(QUndoCommand* par) : QUndoCommand(par), DACallBackInterface()
{
}

/**
 * @brief 析构函数
 */
DADataAbstractUndoCommand::~DADataAbstractUndoCommand()
{
}

/**
 * @brief 设置是否跳过第一次redo
 * @param skip true表示跳过
 */
void DADataAbstractUndoCommand::setSkipFirstRedo(bool skip)
{
    mSkipFirstRedo = skip;
}

/**
 * @brief 判断是否跳过第一次redo
 * @return true表示跳过
 */
bool DADataAbstractUndoCommand::isSkipFirstRedo() const
{
    return mSkipFirstRedo;
}

/**
 * @brief 消费跳过标记，使后续 redo 正常执行
 */
void DADataAbstractUndoCommand::consumeSkipFirstRedo()
{
    mSkipFirstRedo = false;
}

//----------------------------------------------------
// DADataUndoCommand
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param par 父命令
 */
DADataObjectSwapUndoCommand::DADataObjectSwapUndoCommand(QUndoCommand* par) : DADataAbstractUndoCommand(par)
{
}

/**
 * @brief 析构函数
 */
DADataObjectSwapUndoCommand::~DADataObjectSwapUndoCommand()
{
}

/**
 * @brief 设置旧对象（立即 pickle 到临时文件）
 * @param data 数据
 */
void DADataObjectSwapUndoCommand::setOldData(const DAData& data)
{
    mOldObject = data.toPyObject();
    mData      = data;
}

/**
 * @brief 设置新对象（把新对象也pickle到临时文件）
 * @param data 数据
 */
void DADataObjectSwapUndoCommand::setNewData(const DAData& data)
{
    mNewObject = data.toPyObject();
    mData      = data;
}

/**
 * @brief 撤销操作，把旧对象设置回去
 */
void DADataObjectSwapUndoCommand::undo()
{
    mData.setPyObject(mOldObject);
    DADataManager* mgr = mData.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeValue);
    }
    callback();
}

/**
 * @brief 重做操作，把新对象设置进去
 */
void DADataObjectSwapUndoCommand::redo()
{
    mData.setPyObject(mNewObject);
    DADataManager* mgr = mData.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeValue);
    }
    callback();
}
//----------------------------------------------------
// DADataFileCacheUndoCommand
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param par 父命令
 */
DADataObjectPersistUndoCommand::DADataObjectPersistUndoCommand(QUndoCommand* par) : DADataAbstractUndoCommand(par)
{
    if (!g_tempDir.isValid()) {
        qDebug() << "invalid temp dir,can not create temporary dir";
    }
}

/**
 * @brief 析构函数，清理临时文件
 */
DADataObjectPersistUndoCommand::~DADataObjectPersistUndoCommand()
{
    // 清理临时文件
    if (!mOldObjectPath.isEmpty() && QFile::exists(mOldObjectPath)) {
        QFile::remove(mOldObjectPath);
    }
    if (!mNewObjectPath.isEmpty() && QFile::exists(mNewObjectPath)) {
        QFile::remove(mNewObjectPath);
    }
}

/**
 * @brief 设置旧对象（立即 pickle 到临时文件）
 * @param data 数据
 */
void DADataObjectPersistUndoCommand::setOldData(const DAData& data)
{
    if (!g_tempDir.isValid()) {
        qWarning() << "Temporary directory is invalid, cannot cache data";
        return;
    }
    if (!data.isDataFrame() && !data.isSeries()) {
        return;
    }
    QString path    = g_tempDir.path() + "/obj_old_" + QUuid::createUuid().toString(QUuid::Id128) + ".pkl";
    mOldObjectPath = path;
    mData          = data;
    dumpObj(data.toPyObject(), mOldObjectPath);
}

/**
 * @brief 设置新对象（把新对象也pickle到临时文件）
 * @param data 数据
 */
void DADataObjectPersistUndoCommand::setNewData(const DAData& data)
{
    if (!g_tempDir.isValid()) {
        qWarning() << "Temporary directory is invalid, cannot cache data";
        return;
    }
    if (!data.isDataFrame() && !data.isSeries()) {
        return;
    }
    QString path    = g_tempDir.path() + "/obj_new_" + QUuid::createUuid().toString(QUuid::Id128) + ".pkl";
    mNewObjectPath = path;
    mData          = data;
    dumpObj(data.toPyObject(), mNewObjectPath);
}

/**
 * @brief 撤销操作，从旧文件加载对象
 */
void DADataObjectPersistUndoCommand::undo()
{
    // 2. 从旧文件加载对象
    if (mOldObjectPath.isEmpty()) {
        qDebug() << "mOldObjectPath is empty";
        return;
    }
    mData.setPyObject(loadObj(mOldObjectPath));
    DADataManager* mgr = mData.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeValue);
    }
    callback();
}

/**
 * @brief 重做操作，从新文件加载对象
 */
void DADataObjectPersistUndoCommand::redo()
{
    if (isSkipFirstRedo()) {
        consumeSkipFirstRedo();
        return;
    }
    if (mNewObjectPath.isEmpty()) {
        qDebug() << "mNewObjectPath is empty";
        return;
    }
    mData.setPyObject(loadObj(mNewObjectPath));
    DADataManager* mgr = mData.getDataManager();
    if (mgr) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeValue);
    }
    callback();
}

/**
 * @brief 把pybind11::object通过pickle写入文件
 * @param obj python对象
 * @param path 文件路径
 */
void DADataObjectPersistUndoCommand::dumpObj(const pybind11::object& obj, const QString& path)
{
    static auto dumps = pybind11::module_::import("pickle").attr("dumps");
    static auto open  = pybind11::module_::import("builtins").attr("open");

    auto bytes = dumps(obj);
    auto io    = open(DA::PY::toPyObject(path), "wb");
    try {
        io.attr("write")(bytes);
    } catch (...) {
        try { io.attr("close")(); } catch (...) {}
        throw;
    }
    io.attr("close")();
}

/**
 * @brief 从文件通过pickle加载pybind11::object
 * @param path 文件路径
 * @return 加载的python对象
 */
pybind11::object DADataObjectPersistUndoCommand::loadObj(const QString& path)
{
    static auto pickle = pybind11::module_::import("pickle");
    static auto open   = pybind11::module_::import("builtins").attr("open");
    auto io            = open(DA::PY::toPyObject(path), "rb");
    pybind11::object bytes;
    try {
        bytes = io.attr("read")();
    } catch (...) {
        try { io.attr("close")(); } catch (...) {}
        throw;
    }
    io.attr("close")();
    return pickle.attr("loads")(bytes);
}

/**
 * @brief 判断命令是否有效
 * @return 新旧对象路径都不为空时返回true
 */
bool DADataObjectPersistUndoCommand::isValid() const
{
    return (!mOldObjectPath.isEmpty()) && (!mNewObjectPath.isEmpty());
}

}  // end namspace DA
