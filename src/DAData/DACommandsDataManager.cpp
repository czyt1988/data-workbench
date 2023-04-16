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
    setText(QObject::tr("add data"));
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
    setText(QObject::tr("remove data"));
}

void DACommandDataManagerRemove::redo()
{
    mDataMgr->removeData(mData);
}

void DACommandDataManagerRemove::undo()
{
    mDataMgr->addData(mData);
}
