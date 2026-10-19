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
    for (const ChangeRecord& r : std::as_const(mRecords)) {
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


//===================================================
// 单元格批注命令（005 T1.26）
//===================================================

/**
 * @brief 构造
 * @param store 批注子存储（非拥有，QPointer 防悬挂）
 * @param row 数据源绝对行
 * @param col 列位置
 * @param oldNote 旧条目（text 空 = 原先无批注）
 * @param newNote 新条目（text 空 = 删除批注）
 * @param parent 父命令
 */
DACommandTableNote_set::DACommandTableNote_set(DATableNoteStore* store,
                                               int row,
                                               int col,
                                               const DATableNoteStore::Note& oldNote,
                                               const DATableNoteStore::Note& newNote,
                                               QUndoCommand* parent)
    : QUndoCommand(parent), mStore(store), mRow(row), mCol(col), mOldNote(oldNote), mNewNote(newNote)
{
    // 文案按情形区分（撤销面板可读，红线5）
    if (mOldNote.text.isEmpty() && !mNewNote.text.isEmpty()) {
        setText(QObject::tr("add note"));  // cn:添加批注
    } else if (!mOldNote.text.isEmpty() && mNewNote.text.isEmpty()) {
        setText(QObject::tr("delete note"));  // cn:删除批注
    } else {
        setText(QObject::tr("edit note"));  // cn:编辑批注
    }
}

/**
 * @brief 写入一侧条目（text 空 = 移除该格批注）
 */
void DACommandTableNote_set::applyEntry(const DATableNoteStore::Note& note)
{
    if (!mStore) {
        return;
    }
    if (note.text.isEmpty()) {
        mStore->removeNote(mRow, mCol);
    } else {
        mStore->setNoteEntry(mRow, mCol, note);  // 保留原时间戳（undo/redo 精确还原）
    }
}

void DACommandTableNote_set::redo()
{
    applyEntry(mNewNote);
}

void DACommandTableNote_set::undo()
{
    applyEntry(mOldNote);
}

/**
 * @brief 构造
 * @param store 批注子存储（非拥有，QPointer 防悬挂）
 * @param parent 父命令
 */
DACommandTableNote_clearAll::DACommandTableNote_clearAll(DATableNoteStore* store, QUndoCommand* parent)
    : QUndoCommand(parent), mStore(store)
{
    setText(QObject::tr("clear all notes"));  // cn:清除本表批注
}

/**
 * @brief redo：首次捕获整店快照后清空
 */
void DACommandTableNote_clearAll::redo()
{
    if (!mStore) {
        return;
    }
    if (!mCaptured) {
        mSnapshot = mStore->snapshot();
        mCaptured = true;
    }
    mStore->clearAll();
}

/**
 * @brief undo：整店还原（含时间戳）
 */
void DACommandTableNote_clearAll::undo()
{
    if (mStore) {
        mStore->restore(mSnapshot);
    }
}

}  // end of namespace DA
