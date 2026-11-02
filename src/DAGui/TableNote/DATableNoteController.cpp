#include "DATableNoteController.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
// qt
#include <QAbstractItemModel>
#include <QCursor>
#include <QDateTime>
#include <QDialog>
#include <QPoint>
#include <QRect>
// DAGui
#include "DADataTableView.h"
#include "Models/DAAbstractCacheWindowTableModel.h"
#include "Commands/DACommandsTableStyle.h"
#include "DATableNoteEditDialog.h"
#include "DATableNoteStore.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

namespace DA
{
//===================================================
// DATableNoteController
//===================================================

DATableNoteController::DATableNoteController(const DATablePageContext& ctx, const DATableSelectionService& sel, QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel)
{
}

/**
 * @brief 打开当前单元格的批注编辑弹层（无批注=插入，有批注=编辑且弹层内可删除）
 */
void DATableNoteController::showNoteEditorForCurrentCell()
{
    if (!mCtx.noteStore()) {
        return;
    }
    int row = -1, col = -1;
    if (!mSel.currentActualCell(&row, &col)) {
        daWarning << tr("Please select a cell to annotate");  // cn:请先选中要批注的单元格
        return;
    }
    editNoteAtCell(row, col);
}

/**
 * @brief 对给定格弹出编辑弹层并落地结果（插入/编辑共用）
 *
 * 弹层跟随单元格右下角定位（先 showActualRow 保证行在滑动窗内，visualRect 转全局
 * 坐标，出屏由弹层回夹）。保存 → 新增/编辑命令（旧新条目含时间戳记录，D4）；
 * 弹层内点删除或文本清空 → 删除命令；文本未变不推命令。批注不进 DataFrame（红线3）。
 * @param actualRow 数据源绝对行
 * @param actualCol 列位置
 */
void DATableNoteController::editNoteAtCell(int actualRow, int actualCol)
{
    DATableNoteStore* noteStore = mCtx.noteStore();
    if (!noteStore) {
        return;
    }
    // 弹层父部件取主窗格（等价旧页内弹层的堆叠关系；popupNear 手动定位）
    DATableNoteEditDialog dlg(mCtx.mainView());
    const DATableNoteStore::Note oldNote = noteStore->note(actualRow, actualCol);
    dlg.setNoteText(oldNote.text);
    dlg.setExistingNote(!oldNote.text.isEmpty());
    // 弹层位置跟随单元格右下角
    QPoint corner;
    if (DADataTableView* tv = mCtx.mainView()) {
        tv->showActualRow(actualRow);
        QAbstractItemModel* m = tv->model();
        int startRow          = 0;
        if (auto* cm = qobject_cast< DAAbstractCacheWindowTableModel* >(m)) {
            startRow = cm->getCacheWindowStartRow();
        }
        const QModelIndex idx = m ? m->index(actualRow - startRow, actualCol) : QModelIndex();
        if (idx.isValid()) {
            const QRect vr = tv->visualRect(idx);
            corner         = tv->viewport()->mapToGlobal(vr.bottomRight());
        }
    }
    if (corner.isNull()) {
        corner = QCursor::pos();
    }
    dlg.popupNear(corner);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    if (dlg.isDeleteRequested()) {
        if (oldNote.text.isEmpty()) {
            return;  // 本来就没有批注
        }
        if (mCtx.pushCommand) {
            mCtx.pushCommand(new DACommandTableNote_set(noteStore, actualRow, actualCol, oldNote, DATableNoteStore::Note()));
        }
        return;
    }
    const QString newText = dlg.noteText().trimmed();
    if (newText == oldNote.text) {
        return;  // 无变化不推命令
    }
    if (newText.isEmpty()) {
        // 文本清空 = 删除批注（批注没有"空内容"状态）
        if (oldNote.text.isEmpty()) {
            return;
        }
        if (mCtx.pushCommand) {
            mCtx.pushCommand(new DACommandTableNote_set(noteStore, actualRow, actualCol, oldNote, DATableNoteStore::Note()));
        }
        return;
    }
    DATableNoteStore::Note newNote;
    newNote.text      = newText;
    const QDateTime now = QDateTime::currentDateTime();
    newNote.createdAt = oldNote.text.isEmpty() ? now : oldNote.createdAt;
    newNote.updatedAt = now;
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableNote_set(noteStore, actualRow, actualCol, oldNote, newNote));
    }
}

/**
 * @brief 删除当前单元格批注（命令可撤销）
 * @return 成功入栈返回 true；无当前格或无批注返回 false
 */
bool DATableNoteController::deleteNoteAtCurrentCell()
{
    DATableNoteStore* noteStore = mCtx.noteStore();
    if (!noteStore) {
        return false;
    }
    int row = -1, col = -1;
    if (!mSel.currentActualCell(&row, &col)) {
        daWarning << tr("Please select a cell whose note is to be deleted");  // cn:请先选中要删除批注的单元格
        return false;
    }
    const DATableNoteStore::Note oldNote = noteStore->note(row, col);
    if (oldNote.text.isEmpty()) {
        daInfo << tr("The selected cell has no note");  // cn:选中单元格没有批注
        return false;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableNote_set(noteStore, row, col, oldNote, DATableNoteStore::Note()));
    }
    return true;
}

/**
 * @brief 清除本表全部批注（一次命令，可撤销）
 * @return 清除的批注数（无批注返回 0，不推空命令）
 */
int DATableNoteController::clearAllNotes()
{
    DATableNoteStore* noteStore = mCtx.noteStore();
    if (!noteStore) {
        return 0;
    }
    const int n = noteStore->count();
    if (n == 0) {
        daInfo << tr("This table has no notes");  // cn:本表没有批注
        return 0;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableNote_clearAll(noteStore));
    }
    return n;
}
}  // end of namespace DA
