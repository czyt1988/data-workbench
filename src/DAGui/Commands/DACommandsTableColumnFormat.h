#ifndef DACOMMANDTABLECOLUMNFORMAT_H
#define DACOMMANDTABLECOLUMNFORMAT_H
#include <QUndoCommand>
#include <QList>
#include "DAGuiAPI.h"
#include "DATableDisplayFormat.h"

namespace DA
{
class DATableStyleManager;

/**
 * @brief 表格列显示格式变更撤销命令
 *
 * 支持批量变更：选中区多列一次性处理，undo 栈只增 1 步。
 * 每条记录列号、旧格式、新格式。redo 应用新格式，undo 恢复旧格式；
 * 目标格式 invalid 时清除该列格式。
 */
class DAGUI_API DACommandTableColumnFormat : public QUndoCommand
{
public:
    struct ChangeRecord
    {
        int col;
        DATableDisplayFormat oldFmt;
        DATableDisplayFormat newFmt;
    };

    DACommandTableColumnFormat(DATableStyleManager* mgr, QUndoCommand* parent = nullptr);

    // 追加一条变更记录
    void addChange(int col, const DATableDisplayFormat& oldFmt, const DATableDisplayFormat& newFmt);

    // 是否没有变更记录（用于避免推送空命令到 undo 栈）
    bool isEmpty() const;

    void redo() override;
    void undo() override;

private:
    void applyRecords(bool isRedo);
    DATableStyleManager* mMgr;
    QList< ChangeRecord > mRecords;
};
}  // end of namespace DA
#endif  // DACOMMANDTABLECOLUMNFORMAT_H
