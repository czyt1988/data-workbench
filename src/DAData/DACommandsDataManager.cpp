#include "DACommandsDataManager.h"
#include "DADataManager.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DACommandDataManagerAdd
//===================================================

/**
 * @brief 构造函数
 * @param d 数据
 * @param mgr 数据管理器
 * @param par 父命令
 */
DACommandDataManagerAdd::DACommandDataManagerAdd(const DAData& d, DADataManager* mgr, QUndoCommand* par)
    : QUndoCommand(par), mData(d), mDataMgr(mgr)
{
    setText(QObject::tr("add data"));  // cn:添加数据
}

/**
 * @brief 重做，添加数据
 */
void DACommandDataManagerAdd::redo()
{
    mDataMgr->addData(mData);
}

/**
 * @brief 撤销，移除数据
 */
void DACommandDataManagerAdd::undo()
{
    mDataMgr->removeData(mData);
}

//==============================================================
// DACommandDataManagerRemove
//==============================================================

/**
 * @brief 构造函数
 * @param d 数据
 * @param mgr 数据管理器
 * @param par 父命令
 */
DACommandDataManagerRemove::DACommandDataManagerRemove(const DAData& d, DADataManager* mgr, QUndoCommand* par)
    : QUndoCommand(par), mData(d), mDataMgr(mgr)
{
    setText(QObject::tr("remove data"));  // cn:移除数据
}

/**
 * @brief 重做，移除数据
 */
void DACommandDataManagerRemove::redo()
{
    mDataMgr->removeData(mData);
}

/**
 * @brief 撤销，添加数据
 */
void DACommandDataManagerRemove::undo()
{
    mDataMgr->addData(mData);
}

//==============================================================
// DACommandDataManagerRenameData
//==============================================================

/**
 * @brief 构造函数
 * @param d 数据
 * @param newName 新名称
 * @param par 父命令
 */
DACommandDataManagerRenameData::DACommandDataManagerRenameData(const DAData& d, const QString& newName, QUndoCommand* par)
    : QUndoCommand(par), mData(d)
{
    setText(QObject::tr("rename data"));  // cn:重命名数据
    mOldName = d.getName();
    mNewName = newName;
}

/**
 * @brief 重做，设置新名称
 */
void DACommandDataManagerRenameData::redo()
{
    mData.setName(mNewName);
}

/**
 * @brief 撤销，恢复旧名称
 */
void DACommandDataManagerRenameData::undo()
{
    mData.setName(mOldName);
}
