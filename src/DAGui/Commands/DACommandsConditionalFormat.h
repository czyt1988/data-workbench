#ifndef DACOMMANDSCONDITIONALFORMAT_H
#define DACOMMANDSCONDITIONALFORMAT_H
#include <QUndoCommand>
#include <QVector>
#include "DAGuiAPI.h"
#include "DATableConditionalFormatRule.h"

namespace DA
{
class DATableConditionalFormatStore;

/**
 * @brief 条件格式规则变更撤销命令族（003 T1.5，旧新对象记录范式，D4）
 *
 * 条件格式是元数据：命令记录的是"规则定义"的旧新对象，不写 DataFrame（红线3）。
 * 全部经 DADataOperatePageWidget::push 入页撤销栈。规则增/删/改/移动各一条命令，
 * 多列批量提交（对话框确定）由调用方用父命令聚合（QUndoCommand 子命令机制）。
 */

/**
 * @brief 新增规则命令：redo 在 column 的 index 处插入，undo 移除
 */
class DAGUI_API DACommandConditionalFormat_addRule : public QUndoCommand
{
public:
    DACommandConditionalFormat_addRule(DATableConditionalFormatStore* store,
                                       int column,
                                       const DATableConditionalFormatRule& rule,
                                       int index                                       = -1,
                                       QUndoCommand* parent                              = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableConditionalFormatStore* mStore;  ///< 非拥有
    int mColumn;
    int mIndex;  ///< 实际插入位置（构造时 -1 归一化为末尾）
    DATableConditionalFormatRule mRule;
};

/**
 * @brief 删除规则命令：redo 移除 index 处规则，undo 原位插回（记录被删对象）
 */
class DAGUI_API DACommandConditionalFormat_removeRule : public QUndoCommand
{
public:
    DACommandConditionalFormat_removeRule(DATableConditionalFormatStore* store,
                                          int column,
                                          int index,
                                          QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableConditionalFormatStore* mStore;  ///< 非拥有
    int mColumn;
    int mIndex;
    DATableConditionalFormatRule mRule;  ///< 被删规则（首次 redo 时从 store 捕获）
    bool mCaptured { false };
};

/**
 * @brief 修改规则命令：记录旧新规则对象，redo 应用新、undo 还原旧
 */
class DAGUI_API DACommandConditionalFormat_modifyRule : public QUndoCommand
{
public:
    DACommandConditionalFormat_modifyRule(DATableConditionalFormatStore* store,
                                          int column,
                                          int index,
                                          const DATableConditionalFormatRule& oldRule,
                                          const DATableConditionalFormatRule& newRule,
                                          QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableConditionalFormatStore* mStore;  ///< 非拥有
    int mColumn;
    int mIndex;
    DATableConditionalFormatRule mOldRule;
    DATableConditionalFormatRule mNewRule;
};

/**
 * @brief 移动规则命令（上移/下移 = 优先级调整）：记录移动前后索引
 */
class DAGUI_API DACommandConditionalFormat_moveRule : public QUndoCommand
{
public:
    DACommandConditionalFormat_moveRule(DATableConditionalFormatStore* store,
                                        int column,
                                        int from,
                                        int to,
                                        QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableConditionalFormatStore* mStore;  ///< 非拥有
    int mColumn;
    int mFrom;
    int mTo;
};

/**
 * @brief 清除某列全部规则命令：记录旧规则列表，redo 清空，undo 整体还原
 */
class DAGUI_API DACommandConditionalFormat_clearColumn : public QUndoCommand
{
public:
    DACommandConditionalFormat_clearColumn(DATableConditionalFormatStore* store,
                                           int column,
                                           QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableConditionalFormatStore* mStore;  ///< 非拥有
    int mColumn;
    QVector< DATableConditionalFormatRule > mOldRules;  ///< 被清除的规则列表（首次 redo 捕获）
    bool mCaptured { false };
};
}  // end of namespace DA
#endif  // DACOMMANDSCONDITIONALFORMAT_H
