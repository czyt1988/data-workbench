#include "DACommandsConditionalFormat.h"
#include "DATableConditionalFormatStore.h"
#include <QObject>

namespace DA
{

//===================================================
// DACommandConditionalFormat_addRule
//===================================================
/**
 * @brief 构造新增规则命令
 * @param store 条件格式子存储（非拥有）
 * @param column 列索引（位置键）
 * @param rule 规则
 * @param index 插入位置（-1 = 追加末尾）
 * @param parent 父命令（多列批量提交时聚合）
 */
DACommandConditionalFormat_addRule::DACommandConditionalFormat_addRule(
    DATableConditionalFormatStore* store, int column, const DATableConditionalFormatRule& rule, int index, QUndoCommand* parent
)
    : QUndoCommand(parent), mStore(store), mColumn(column), mIndex(index), mRule(rule)
{
    setText(QObject::tr("Add Conditional Format Rule"));  // cn:添加条件格式规则
}

/**
 * @brief redo：插入规则（首次由 QUndoStack::push 触发）
 */
void DACommandConditionalFormat_addRule::redo()
{
    if (!mStore) {
        return;
    }
    const QVector< DATableConditionalFormatRule >* rules = mStore->rulesForColumn(mColumn);
    if (mIndex < 0 || (rules && mIndex > rules->size())) {
        mIndex = rules ? rules->size() : 0;  // 归一化实际插入位置（undo 精确移除）
    }
    mStore->addRule(mColumn, mRule, mIndex);
}

/**
 * @brief undo：移除插入的规则
 */
void DACommandConditionalFormat_addRule::undo()
{
    if (mStore) {
        mStore->removeRule(mColumn, mIndex);
    }
}

//===================================================
// DACommandConditionalFormat_removeRule
//===================================================
/**
 * @brief 构造删除规则命令
 * @param store 条件格式子存储（非拥有）
 * @param column 列索引
 * @param index 被删规则位置
 * @param parent 父命令
 */
DACommandConditionalFormat_removeRule::DACommandConditionalFormat_removeRule(
    DATableConditionalFormatStore* store, int column, int index, QUndoCommand* parent
)
    : QUndoCommand(parent), mStore(store), mColumn(column), mIndex(index)
{
    setText(QObject::tr("Remove Conditional Format Rule"));  // cn:删除条件格式规则
}

/**
 * @brief redo：捕获并移除规则（首次捕获被删对象，重做复用）
 */
void DACommandConditionalFormat_removeRule::redo()
{
    if (!mStore) {
        return;
    }
    if (!mCaptured) {
        const QVector< DATableConditionalFormatRule >* rules = mStore->rulesForColumn(mColumn);
        if (!rules || mIndex < 0 || mIndex >= rules->size()) {
            return;  // 位置失效（不应发生：命令由持有 store 的页面构造）
        }
        mRule     = rules->at(mIndex);
        mCaptured = true;
    }
    mStore->removeRule(mColumn, mIndex);
}

/**
 * @brief undo：原位插回被删规则
 */
void DACommandConditionalFormat_removeRule::undo()
{
    if (mStore && mCaptured) {
        mStore->addRule(mColumn, mRule, mIndex);
    }
}

//===================================================
// DACommandConditionalFormat_modifyRule
//===================================================
/**
 * @brief 构造修改规则命令（旧新对象记录）
 * @param store 条件格式子存储（非拥有）
 * @param column 列索引
 * @param index 规则位置
 * @param oldRule 旧规则
 * @param newRule 新规则
 * @param parent 父命令
 */
DACommandConditionalFormat_modifyRule::DACommandConditionalFormat_modifyRule(
    DATableConditionalFormatStore* store, int column, int index,
    const DATableConditionalFormatRule& oldRule, const DATableConditionalFormatRule& newRule, QUndoCommand* parent
)
    : QUndoCommand(parent), mStore(store), mColumn(column), mIndex(index), mOldRule(oldRule), mNewRule(newRule)
{
    setText(QObject::tr("Modify Conditional Format Rule"));  // cn:修改条件格式规则
}

/**
 * @brief redo：应用新规则
 */
void DACommandConditionalFormat_modifyRule::redo()
{
    if (mStore) {
        mStore->updateRule(mColumn, mIndex, mNewRule);
    }
}

/**
 * @brief undo：还原旧规则
 */
void DACommandConditionalFormat_modifyRule::undo()
{
    if (mStore) {
        mStore->updateRule(mColumn, mIndex, mOldRule);
    }
}

//===================================================
// DACommandConditionalFormat_moveRule
//===================================================
/**
 * @brief 构造移动规则命令（上移/下移 = 优先级调整）
 * @param store 条件格式子存储（非拥有）
 * @param column 列索引
 * @param from 原位置
 * @param to 目标位置
 * @param parent 父命令
 */
DACommandConditionalFormat_moveRule::DACommandConditionalFormat_moveRule(
    DATableConditionalFormatStore* store, int column, int from, int to, QUndoCommand* parent
)
    : QUndoCommand(parent), mStore(store), mColumn(column), mFrom(from), mTo(to)
{
    setText(QObject::tr("Move Conditional Format Rule"));  // cn:移动条件格式规则
}

/**
 * @brief redo：from → to
 */
void DACommandConditionalFormat_moveRule::redo()
{
    if (mStore) {
        mStore->moveRule(mColumn, mFrom, mTo);
    }
}

/**
 * @brief undo：to → from
 */
void DACommandConditionalFormat_moveRule::undo()
{
    if (mStore) {
        mStore->moveRule(mColumn, mTo, mFrom);
    }
}

//===================================================
// DACommandConditionalFormat_clearColumn
//===================================================
/**
 * @brief 构造清除某列全部规则命令
 * @param store 条件格式子存储（非拥有）
 * @param column 列索引
 * @param parent 父命令
 */
DACommandConditionalFormat_clearColumn::DACommandConditionalFormat_clearColumn(
    DATableConditionalFormatStore* store, int column, QUndoCommand* parent
)
    : QUndoCommand(parent), mStore(store), mColumn(column)
{
    setText(QObject::tr("Clear Conditional Format Rules of Column"));  // cn:清除列条件格式规则
}

/**
 * @brief redo：捕获旧规则列表后清空该列（首次捕获，重做复用）
 */
void DACommandConditionalFormat_clearColumn::redo()
{
    if (!mStore) {
        return;
    }
    if (!mCaptured) {
        mOldRules = mStore->columnRules(mColumn);
        mCaptured = true;
    }
    mStore->clearColumn(mColumn);
}

/**
 * @brief undo：整体还原被清除的规则列表
 */
void DACommandConditionalFormat_clearColumn::undo()
{
    if (mStore && mCaptured) {
        mStore->setColumnRules(mColumn, mOldRules);
    }
}

}  // end of namespace DA
