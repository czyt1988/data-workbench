#ifndef DATABLENOTECONTROLLER_H
#define DATABLENOTECONTROLLER_H
#include "DAGuiAPI.h"
#include <QObject>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;

/**
 * @brief 单元格批注控制器（008 期3）
 *
 * 批注的编辑弹层定位、增删改、清空。构造注入上下文与选区服务，不持窗口指针；
 * 命令经上下文的 pushCommand 入页撤销栈（push 即激活撤销栈）。
 * 批注不进 DataFrame（红线3）：只写批注子存储，变更经协调器订阅的
 * notesChanged 重绘视口角标。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableNoteController : public QObject
{
    Q_OBJECT
public:
    explicit DATableNoteController(const DATablePageContext& ctx, const DATableSelectionService& sel, QObject* parent = nullptr);

public:
    // 打开当前单元格的批注编辑弹层（无批注=插入，有批注=编辑且弹层内可删除）
    void showNoteEditorForCurrentCell();
    // 删除当前单元格批注（命令可撤销）；无当前格或无批注返回 false
    bool deleteNoteAtCurrentCell();
    // 清除本表全部批注（一次命令，可撤销）；返回清除的批注数
    int clearAllNotes();

private:
    // 对给定格弹出编辑弹层并落地结果（插入/编辑共用：保存→增/改命令，删除→删除命令）
    void editNoteAtCell(int actualRow, int actualCol);

private:
    const DATablePageContext& mCtx;   ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
};
}  // end of namespace DA
#endif  // DATABLENOTECONTROLLER_H
