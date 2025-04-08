#include "DAAppCommand.h"
#if DA_ENABLE_PYTHON
#include "Commands/DACommandWithTemporaryData.h"
#endif
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppCommand
//===================================================
DAAppCommand::DAAppCommand(DAUIInterface* u) : DACommandInterface(u)
{
}

DAAppCommand::~DAAppCommand()
{
}

void DAAppCommand::setDataManagerStack(QUndoStack* s)
{
	mDataManagerStack = s;
	addStack(s);
}

QUndoStack* DAAppCommand::getDataManagerStack() const
{
	return mDataManagerStack.data();
}
