#ifndef DACOMMANDSTABLEVALIDATION_H
#define DACOMMANDSTABLEVALIDATION_H
#include <QUndoCommand>
#include <QList>
#include "DAGuiAPI.h"
#include "DATableValidationRule.h"

namespace DA
{
class DATableValidationStore;

/**
 * @brief 校验规则集合变更命令（005 T1.25，旧新对象记录范式，D4）
 *
 * 校验规则是元数据：命令记录整份规则列表的旧新对象（O(规则数)），不写 DataFrame。
 * 规则对话框「确定」后由页面 widget 对比差异构造（无差异不推空命令）；一次确定 =
 * 一条命令 = 一步撤销（与条件格式对话框的批量提交口径一致）。校验执行本身是只读
 * 操作，**不入撤销栈**（红线2），与本命令无关。
 */
class DAGUI_API DACommandValidationRule_setRules : public QUndoCommand
{
public:
    DACommandValidationRule_setRules(DATableValidationStore* store,
                                     const QList< DATableValidationRule >& oldRules,
                                     const QList< DATableValidationRule >& newRules,
                                     QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableValidationStore* mStore;  ///< 非拥有
    QList< DATableValidationRule > mOldRules;
    QList< DATableValidationRule > mNewRules;
};
}  // end of namespace DA
#endif  // DACOMMANDSTABLEVALIDATION_H
