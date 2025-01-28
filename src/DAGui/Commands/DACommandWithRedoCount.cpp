#include "DACommandWithRedoCount.h"
namespace DA
{

//===================================================
// DACommandWithRedoCount
//===================================================
DACommandWithRedoCount::DACommandWithRedoCount(QUndoCommand* par) : QUndoCommand(par), m_redocnt(0), m_isSuccess(false)
{
}

DACommandWithRedoCount::~DACommandWithRedoCount()
{
}

void DACommandWithRedoCount::addRedoCnt()
{
	++m_redocnt;
}

void DACommandWithRedoCount::subRedoCnt()
{
	--m_redocnt;
}

size_t DACommandWithRedoCount::getRedoCnt() const
{
	return m_redocnt;
}

bool DACommandWithRedoCount::isSuccess() const
{
	return m_isSuccess;
}

void DACommandWithRedoCount::setSuccess(bool on)
{
	m_isSuccess = on;
}

bool DACommandWithRedoCount::isFirstRunRedo()
{
	return m_redocnt == 1;
}

}
