#ifndef DACOMMANDSTABLECOLUMNVIEW_H
#define DACOMMANDSTABLECOLUMNVIEW_H
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QUndoCommand>
#include "DACallBackInterface.h"
#include "DAGuiAPI.h"

namespace DA
{
class DATableViewStateStore;

/**
 * @brief 列视图（列序/隐藏/冻结列数）变更撤销命令族（006c，旧新值记录范式，D4）
 *
 * 列视图是元数据——命令只写 DATableViewStateStore 与视图，不写 DataFrame。
 * 每条命令持有真源值旧/新值，redo 写新值、undo 写旧值，随后 callback() 重投影。
 * 全部经 DADataOperatePageWidget::push 入页撤销栈；文本用 QObject::tr
 * （与 002 的 Sort rows 同上下文）。不继承 DACommandWithRedoCount——值没有被
 * 前置应用，push 的首次 redo 就要写 store。
 */

/**
 * @brief 列序变更命令：redo 写新列序，undo 写旧列序
 */
class DAGUI_API DACommandTableColumnOrder : public QUndoCommand, public DACallBackInterface
{
public:
    DACommandTableColumnOrder(DATableViewStateStore* store,
                              const QList< QPair< QString, int > >& oldOrder,
                              const QList< QPair< QString, int > >& newOrder,
                              CallBack projectCallback, QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableViewStateStore* mStore { nullptr };  ///< 非拥有
    QList< QPair< QString, int > > mOldOrder;
    QList< QPair< QString, int > > mNewOrder;
};

/**
 * @brief 隐藏集合变更命令：redo 写新集合，undo 写旧集合（整体替换，非增量）
 */
class DAGUI_API DACommandTableColumnVisibility : public QUndoCommand, public DACallBackInterface
{
public:
    DACommandTableColumnVisibility(DATableViewStateStore* store,
                                   const QSet< QPair< QString, int > >& oldHidden,
                                   const QSet< QPair< QString, int > >& newHidden,
                                   CallBack projectCallback,
                                   const QString& commandText = QString(),
                                   QUndoCommand* parent                  = nullptr);
    void redo() override;
    void undo() override;

private:
    void applyHidden(const QSet< QPair< QString, int > >& hidden);
    DATableViewStateStore* mStore { nullptr };  ///< 非拥有
    QSet< QPair< QString, int > > mOldHidden;
    QSet< QPair< QString, int > > mNewHidden;
};

/**
 * @brief 冻结列数变更命令：redo 写新计数，undo 写旧计数
 */
class DAGUI_API DACommandTableFreezeColumns : public QUndoCommand, public DACallBackInterface
{
public:
    DACommandTableFreezeColumns(DATableViewStateStore* store, int oldCount, int newCount,
                                CallBack projectCallback, QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;

private:
    DATableViewStateStore* mStore { nullptr };  ///< 非拥有
    int mOldCount;
    int mNewCount;
};

/**
 * @brief 列视图批量变更父命令：多个子命令合成一个撤销单元（006d 对话框确定用）
 *
 * redo/undo 用 QUndoCommand 默认实现（遍历子命令）；子命令在构造时以 parent
 * 挂入，各自持有投影回调。
 */
class DAGUI_API DACommandTableColumnViewBatch : public QUndoCommand
{
public:
    explicit DACommandTableColumnViewBatch(const QString& text, QUndoCommand* parent = nullptr);
};
}  // end of namespace DA
#endif  // DACOMMANDSTABLECOLUMNVIEW_H
