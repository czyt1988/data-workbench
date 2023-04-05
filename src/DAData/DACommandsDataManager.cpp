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
    : QUndoCommand(par), m_data(d), _dmgr(mgr)
{
    setText(QObject::tr("add data"));
}

void DACommandDataManagerAdd::redo()
{
    _dmgr->addData(m_data);
}

void DACommandDataManagerAdd::undo()
{
    _dmgr->removeData(m_data);
}

//==============================================================
// DACommandDataManagerRemove
//==============================================================
DACommandDataManagerRemove::DACommandDataManagerRemove(const DAData& d, DADataManager* mgr, QUndoCommand* par)
    : QUndoCommand(par), m_data(d), _dmgr(mgr)
{
    setText(QObject::tr("remove data"));
}

void DACommandDataManagerRemove::redo()
{
    _dmgr->removeData(m_data);
}

void DACommandDataManagerRemove::undo()
{
    _dmgr->addData(m_data);
}
