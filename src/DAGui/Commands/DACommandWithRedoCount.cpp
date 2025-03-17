#include "DACommandWithRedoCount.h"
namespace DA
{

//===================================================
// DACommandWithRedoCount
//===================================================
DACommandWithRedoCount::DACommandWithRedoCount(QUndoCommand* par) : QUndoCommand(par)
{
}

DACommandWithRedoCount::~DACommandWithRedoCount()
{
}

void DACommandWithRedoCount::redo()
{
	if (mIsFirstRedo) {
		// 第一次执行redo，将跳过，这里exec函数认为已经执行
		mIsFirstRedo = false;
		return;
	}
	exec();
}

bool DACommandWithRedoCount::exec()
{
	return false;
}

}
