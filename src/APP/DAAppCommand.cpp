#include "DAAppCommand.h"
#include "DADataUndoCommand.h"
#include "DAUIInterface.h"
#include "DAAppDockingArea.h"
#include "DADataOperateWidget.h"
#include "Commands/DACommandWithTemporaryData.h"
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

DADataAbstractUndoCommand* DAAppCommand::beginDataOperateCommand(
    const DAData& data, const QString& text, bool isObjectPersist, bool isSkipFirstRedo
)
{
    if (isObjectPersist) {
        mDataOperateCommand = std::make_unique< DADataObjectPersistUndoCommand >();
    } else {
        mDataOperateCommand = std::make_unique< DADataObjectSwapUndoCommand >();
    }
    mDataOperateCommand->setSkipFirstRedo(isSkipFirstRedo);
    mDataOperateCommand->setText(text);
    mDataOperateCommand->setOldData(data);
    return mDataOperateCommand.get();
}

bool DAAppCommand::endDataOperateCommand(const DAData& data)
{
    // 先获取当前的命令栈
    DADockingAreaInterface* dock = ui()->getDockingArea();
    if (!dock) {
        return false;
    }
    auto dataOptWidget = dock->getDataOperateWidget();
    if (!dataOptWidget) {
        return false;
    }
    QUndoStack* undoStack = dataOptWidget->getUndoStack();
    if (!undoStack) {
        return false;
    }
    mDataOperateCommand->setNewData(data);
    undoStack->push(mDataOperateCommand.release());
    // 激活
    if (undoGroup().activeStack() != undoStack) {
        undoGroup().setActiveStack(undoStack);
    }
    return true;
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
