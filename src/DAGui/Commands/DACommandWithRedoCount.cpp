﻿#include "DACommandWithRedoCount.h"
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

bool DACommandWithRedoCount::isEqualTwo() const
{
    return m_redocnt == 2;
}

size_t DACommandWithRedoCount::getRedoCnt() const
{
    return m_redocnt;
}

bool DACommandWithRedoCount::isSetSuccess() const
{
    return m_isSuccess;
}

void DACommandWithRedoCount::setSuccess(bool on)
{
    m_isSuccess = on;
}

}
