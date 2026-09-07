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
    if (!data.supportsUndoSnapshot()) {
        // 引用式/惰性数据（如数据库惰性表）无法整表物化快照（pickle到临时文件），
        // 不创建undo命令，返回nullptr；调用方（含python绑定侧）需判空
        qInfo() << "DAAppCommand::beginDataOperateCommand: data" << data.getName()
                << "does not support undo snapshot, command creation skipped";
        mDataOperateCommand.reset();
        return nullptr;
    }
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
    if (!mDataOperateCommand) {
        // beginDataOperateCommand未创建命令（如不支持快照的数据），静默跳过
        return false;
    }
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
