﻿#include "DAAppCommand.h"
#include "Commands/DACommandWithTemplateData.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppCommand
//===================================================
DAAppCommand::DAAppCommand(DAAppUIInterface* u) : DACommandInterface(u)
{
    //创建临时数据目录
    DACommandWithTemplateData::ensureTemplateDirExists();
}

DAAppCommand::~DAAppCommand()
{
}

void DAAppCommand::setDataManagerStack(QUndoStack* s)
{
    m_dataManagerStack = s;
    undoGroup().addStack(s);
}

QUndoStack* DAAppCommand::getDataManagerStack() const
{
    return m_dataManagerStack.data();
}
