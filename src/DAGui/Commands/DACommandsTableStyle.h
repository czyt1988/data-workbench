#ifndef DACOMMANDSTABLESTYLE_H
#define DACOMMANDSTABLESTYLE_H
#include <QUndoCommand>
#include <QList>
#include <QPair>
#include <QHash>
#include <QPointer>
#include "DAGuiAPI.h"
#include "DATableCellStyle.h"
#include "DATableNoteStore.h"

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

    // 是否没有变更记录（用于避免推送空命令到 undo 栈）
    bool isEmpty() const;

    void redo() override;
    void undo() override;

private:
    void applyRecords(bool isRedo);
    DATableStyleManager* mMgr;
    QList< ChangeRecord > mRecords;
};

/**
 * @brief 单元格批注增删改命令（005 T1.26，旧新文本记录同族，D4）
 *
 * 批注是元数据：命令记录 (row,col) 位置键上的旧新条目（含时间戳，undo 精确还原），
 * 不写 DataFrame（红线3：批注不进数据，不污染导出与工作流）。一条命令统一覆盖
 * 新增（无旧有新）、编辑（旧新都有）、删除（有旧无新）三种情形，文案按情形区分。
 * store 是会话级（比页撤销栈长寿），用 QPointer 防悬挂。
 */
class DAGUI_API DACommandTableNote_set : public QUndoCommand
{
public:
    // oldNote/newNote 的 text 为空表示该侧不存在批注（新增/删除情形）
    DACommandTableNote_set(DATableNoteStore* store,
                           int row,
                           int col,
                           const DATableNoteStore::Note& oldNote,
                           const DATableNoteStore::Note& newNote,
                           QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    void applyEntry(const DATableNoteStore::Note& note);
    QPointer< DATableNoteStore > mStore;
    int mRow;
    int mCol;
    DATableNoteStore::Note mOldNote;  ///< text 空 = 原先无批注
    DATableNoteStore::Note mNewNote;  ///< text 空 = 删除批注
};

/**
 * @brief 清除本表全部批注命令（005 T1.26，一次命令可撤销）
 *
 * 首次 redo 捕获整店快照，redo 清空、undo 整体还原（含时间戳）。
 */
class DAGUI_API DACommandTableNote_clearAll : public QUndoCommand
{
public:
    explicit DACommandTableNote_clearAll(DATableNoteStore* store, QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    QPointer< DATableNoteStore > mStore;
    QHash< DATableNoteStore::CellKey, DATableNoteStore::Note > mSnapshot;  ///< 被清除的整店快照（首次 redo 捕获）
    bool mCaptured { false };
};
}  // end of namespace DA
#endif  // DACOMMANDSTABLESTYLE_H
