#include "DACommandsTableColumnFormat.h"
#include "DATableStyleManager.h"
#include <QObject>

namespace DA
{

/**
 * @brief 构造函数
 * @param mgr 样式管理器（非拥有）
 * @param parent 父命令
 */
DACommandTableColumnFormat::DACommandTableColumnFormat(DATableStyleManager* mgr, QUndoCommand* parent)
    : QUndoCommand(parent), mMgr(mgr)
{
    setText(QObject::tr("change table display format"));  // cn:改变表格显示格式
}

/**
 * @brief 追加一条变更记录
 * @param col 列号
 * @param oldFmt 旧格式（undo 时恢复）
 * @param newFmt 新格式（redo 时应用）
 */
void DACommandTableColumnFormat::addChange(int col,
                                           const DATableDisplayFormat& oldFmt,
                                           const DATableDisplayFormat& newFmt)
{
    ChangeRecord r;
    r.col    = col;
    r.oldFmt = oldFmt;
    r.newFmt = newFmt;
    mRecords.append(r);
}

bool DACommandTableColumnFormat::isEmpty() const
{
    return mRecords.isEmpty();
}

void DACommandTableColumnFormat::redo()
{
    applyRecords(true);
}

void DACommandTableColumnFormat::undo()
{
    applyRecords(false);
}

/**
 * @brief 应用变更记录
 *
 * redo 用 newFmt，undo 用 oldFmt。格式 valid 则 setColumnFormat，invalid 则 clearColumnFormat。
 * 批量还原时用 replace 语义（setColumnFormat 内部为整体替换）。
 * @param isRedo true 为 redo，false 为 undo
 */
void DACommandTableColumnFormat::applyRecords(bool isRedo)
{
    if (!mMgr) {
        return;
    }
    for (const ChangeRecord& r : std::as_const(mRecords)) {
        const DATableDisplayFormat& fmt = isRedo ? r.newFmt : r.oldFmt;
        if (fmt.isValid()) {
            mMgr->setColumnFormat(r.col, fmt);
        } else {
            mMgr->clearColumnFormat(r.col);
        }
    }
}

}  // end of namespace DA
