#ifndef DACOMMANDSTABLESTYLE_H
#define DACOMMANDSTABLESTYLE_H
#include <QUndoCommand>
#include <QList>
#include <QPair>
#include "DAGuiAPI.h"
#include "DATableCellStyle.h"

namespace DA
{
class DATableStyleManager;

/**
 * @brief 表格样式变更撤销命令
 *
 * 支持批量变更：选中区多个单元格/行/列一次性处理，undo 栈只增 1 步。
 * 每条变更记录目标层级、键、旧样式、新样式、是否合并。
 */
class DAGUI_API DACommandTableStyle : public QUndoCommand
{
public:
    enum StyleTarget
    {
        Cell,
        Row,
        Column
    };
    struct ChangeRecord
    {
        StyleTarget target;
        int key1;  // Cell:row, Row:row, Column:col
        int key2;  // Cell:col, Row/Column:unused(0)
        DATableCellStyle oldStyle;
        DATableCellStyle newStyle;
        bool merge;
    };

    DACommandTableStyle(DATableStyleManager* mgr, QUndoCommand* parent = nullptr);

    // 追加一条变更记录
    void addChange(StyleTarget target, int key1, int key2,
                   const DATableCellStyle& oldStyle,
                   const DATableCellStyle& newStyle,
                   bool merge);

    void redo() override;
    void undo() override;

private:
    void applyRecords(bool isRedo);
    DATableStyleManager* mMgr;
    QList< ChangeRecord > mRecords;
};
}  // end of namespace DA
#endif  // DACOMMANDSTABLESTYLE_H
