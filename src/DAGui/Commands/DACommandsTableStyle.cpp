#include "DACommandsTableStyle.h"
#include "DATableStyleManager.h"
#include <QObject>

namespace DA
{

/**
 * @brief 构造函数
 * @param mgr 样式管理器（非拥有）
 * @param parent 父命令
 */
DACommandTableStyle::DACommandTableStyle(DATableStyleManager* mgr, QUndoCommand* parent)
    : QUndoCommand(parent), mMgr(mgr)
{
    setText(QObject::tr("change table style"));  // cn:改变表格样式
}

/**
 * @brief 追加一条变更记录
 * @param target 目标层级（Cell/Row/Column）
 * @param key1 主键（Cell:row, Row:row, Column:col）
 * @param key2 次键（Cell:col, Row/Column:未用）
 * @param oldStyle 旧样式（undo 时恢复）
 * @param newStyle 新样式（redo 时应用）
 * @param merge 是否合并（此参数记录用，实际应用时 redo/undo 用 replace 模式）
 */
void DACommandTableStyle::addChange(StyleTarget target, int key1, int key2,
                                    const DATableCellStyle& oldStyle,
                                    const DATableCellStyle& newStyle,
                                    bool merge)
{
    ChangeRecord r;
    r.target  = target;
    r.key1    = key1;
    r.key2    = key2;
    r.oldStyle = oldStyle;
    r.newStyle = newStyle;
    r.merge   = merge;
    mRecords.append(r);
}

/**
 * @brief redo：应用所有变更记录的新样式
 */
void DACommandTableStyle::redo()
{
    applyRecords(true);
}

/**
 * @brief 是否没有变更记录
 * @return 无变更记录返回 true（用于避免推送空命令到 undo 栈）
 */
bool DACommandTableStyle::isEmpty() const
{
    return mRecords.isEmpty();
}

/**
 * @brief undo：恢复所有变更记录的旧样式
 */
void DACommandTableStyle::undo()
{
    applyRecords(false);
}

/**
 * @brief 应用变更记录
 *
 * redo 用 newStyle，undo 用 oldStyle。若目标样式为空（isNull）则清除，
 * 否则用 replace 模式（merge=false）设置。
 * @param isRedo true 为 redo，false 为 undo
 */
void DACommandTableStyle::applyRecords(bool isRedo)
{
    if (!mMgr) {
        return;
    }
    for (const ChangeRecord& r : mRecords) {
        // redo 用 newStyle，undo 用 oldStyle；若目标样式为空则清除
        const DATableCellStyle& style = isRedo ? r.newStyle : r.oldStyle;
        switch (r.target) {
        case Cell:
            if (style.isNull()) {
                mMgr->clearCell(r.key1, r.key2);
            } else {
                mMgr->setCellStyle(r.key1, r.key2, style, false);
            }
            break;
        case Row:
            if (style.isNull()) {
                mMgr->clearRow(r.key1);
            } else {
                mMgr->setRowStyle(r.key1, style, false);
            }
            break;
        case Column:
            if (style.isNull()) {
                mMgr->clearColumn(r.key1);
            } else {
                mMgr->setColumnStyle(r.key1, style, false);
            }
            break;
        }
    }
}

}  // end of namespace DA
