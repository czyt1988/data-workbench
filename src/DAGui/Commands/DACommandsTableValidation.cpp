#include "DACommandsTableValidation.h"
#include "DATableValidationStore.h"
#include <QObject>

namespace DA
{

/**
 * @brief 构造
 * @param store 校验规则子存储（非拥有，会话级比页撤销栈长寿——调用方保证页面存活期内有效）
 * @param oldRules 变更前规则列表（快照）
 * @param newRules 变更后规则列表
 * @param parent 父命令
 */
DACommandValidationRule_setRules::DACommandValidationRule_setRules(DATableValidationStore* store,
                                                                   const QList< DATableValidationRule >& oldRules,
                                                                   const QList< DATableValidationRule >& newRules,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mStore(store), mOldRules(oldRules), mNewRules(newRules)
{
    setText(QObject::tr("edit validation rules"));  // cn:编辑校验规则
}

/**
 * @brief redo：应用新规则列表
 */
void DACommandValidationRule_setRules::redo()
{
    if (mStore) {
        mStore->setRules(mNewRules);
    }
}

/**
 * @brief undo：还原旧规则列表
 */
void DACommandValidationRule_setRules::undo()
{
    if (mStore) {
        mStore->setRules(mOldRules);
    }
}

}  // end of namespace DA
