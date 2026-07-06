#include "DACommandsDataManager.h"
#include "DADataManager.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DACommandDataManagerAdd
//===================================================
DACommandDataManagerAdd::DACommandDataManagerAdd(const DAData& d, DADataManager* mgr, QUndoCommand* par)
    : QUndoCommand(par), mData(d), mDataMgr(mgr)
{
    setText(QObject::tr("add data"));  // cn:添加数据
}

void DACommandDataManagerAdd::redo()
{
    mDataMgr->addData(mData);
}

void DACommandDataManagerAdd::undo()
{
    mDataMgr->removeData(mData);
}

//==============================================================
// DACommandDataManagerRemove
//==============================================================
DACommandDataManagerRemove::DACommandDataManagerRemove(const DAData& d, DADataManager* mgr, QUndoCommand* par)
    : QUndoCommand(par), mData(d), mDataMgr(mgr)
{
    setText(QObject::tr("remove data"));  // cn:移除数据
}

void DACommandDataManagerRemove::redo()
{
    mDataMgr->removeData(mData);
}

void DACommandDataManagerRemove::undo()
{
    mDataMgr->addData(mData);
}

//==============================================================
// DACommandDataManagerRenameData
//==============================================================
DACommandDataManagerRenameData::DACommandDataManagerRenameData(const DAData& d, const QString& newName, QUndoCommand* par)
    : QUndoCommand(par), mData(d)
{
    setText(QObject::tr("rename data"));  // cn:重命名数据
    m_oldNmae = d.getName();
    m_newName = newName;
}

void DACommandDataManagerRenameData::redo()
{
    mData.setName(m_newName);
}

void DACommandDataManagerRenameData::undo()
{
    mData.setName(m_oldNmae);
}
