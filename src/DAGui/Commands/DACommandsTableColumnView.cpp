#include "DACommandsTableColumnView.h"
#include "DATableViewStateStore.h"

namespace DA
{
namespace
{
/**
 * @brief 集合差集：from -= to（QSet 无 subtract，Qt5/6 通吃的写法）
 */
void subtractSet(QSet< QPair< QString, int > >& from, const QSet< QPair< QString, int > >& to)
{
    for (const QPair< QString, int >& v : to) {
        from.remove(v);
    }
}

/**
 * @brief 列表转集合（冻结命令按集合语义比较差异）
 */
QSet< QPair< QString, int > > toIdentSet(const QList< QPair< QString, int > >& list)
{
    QSet< QPair< QString, int > > res;
    res.reserve(list.size());
    for (const QPair< QString, int >& v : list) {
        res.insert(v);
    }
    return res;
}
}  // namespace

/**
 * @brief 列序变更命令构造
 *
 * @param store 视图状态子存储（非拥有）
 * @param oldOrder 旧列序身份表
 * @param newOrder 新列序身份表
 * @param projectCallback 重投影回调（页面接线为 applyViewStateFromStore）
 * @param parent 父命令（批量时挂入）
 */
DACommandTableColumnOrder::DACommandTableColumnOrder(DATableViewStateStore* store,
                                                     const QList< QPair< QString, int > >& oldOrder,
                                                     const QList< QPair< QString, int > >& newOrder,
                                                     CallBack projectCallback, QUndoCommand* parent)
    : QUndoCommand(parent), DACallBackInterface(), mStore(store), mOldOrder(oldOrder), mNewOrder(newOrder)
{
    setCallBack(projectCallback);
    setText(QObject::tr("Reorder columns"));  // cn:重排列序
}

/**
 * @brief redo：写新列序并重投影
 */
void DACommandTableColumnOrder::redo()
{
    if (mStore) {
        mStore->setColumnOrder(mNewOrder);
    }
    callback();
}

/**
 * @brief undo：写旧列序并重投影
 */
void DACommandTableColumnOrder::undo()
{
    if (mStore) {
        mStore->setColumnOrder(mOldOrder);
    }
    callback();
}

/**
 * @brief 隐藏集合变更命令构造
 *
 * 文案：commandText 非空时用它；newHidden 为空 → "Show all columns"；
 * 比 oldHidden 恰好多一项 → "Hide column \"%1\""；其余 → Show all columns（兜底）。
 * @param store 视图状态子存储（非拥有）
 * @param oldHidden 旧隐藏集合
 * @param newHidden 新隐藏集合
 * @param projectCallback 重投影回调
 * @param commandText 调用方指定的命令文本（空则按集合差异推导）
 * @param parent 父命令（批量时挂入）
 */
DACommandTableColumnVisibility::DACommandTableColumnVisibility(DATableViewStateStore* store,
                                                               const QSet< QPair< QString, int > >& oldHidden,
                                                               const QSet< QPair< QString, int > >& newHidden,
                                                               CallBack projectCallback,
                                                               const QString& commandText,
                                                               QUndoCommand* parent)
    : QUndoCommand(parent), DACallBackInterface(), mStore(store), mOldHidden(oldHidden), mNewHidden(newHidden)
{
    setCallBack(projectCallback);
    if (!commandText.isEmpty()) {
        setText(commandText);
        return;
    }
    if (newHidden.isEmpty()) {
        setText(QObject::tr("Show all columns"));  // cn:显示所有列
    } else if (newHidden.size() == oldHidden.size() + 1) {
        // 找出新增的那一项（隐藏单列的最常见路径）
        QSet< QPair< QString, int > > added = newHidden;
        subtractSet(added, oldHidden);
        if (added.size() == 1) {
            setText(QObject::tr("Hide column \"%1\"").arg(added.constBegin()->first));  // cn:隐藏列"%1"
        } else {
            setText(QObject::tr("Show all columns"));  // cn:显示所有列
        }
    } else {
        setText(QObject::tr("Show all columns"));  // cn:显示所有列
    }
}

/**
 * @brief redo：写新隐藏集合并重投影
 */
void DACommandTableColumnVisibility::redo()
{
    applyHidden(mNewHidden);
    callback();
}

/**
 * @brief undo：写旧隐藏集合并重投影
 */
void DACommandTableColumnVisibility::undo()
{
    applyHidden(mOldHidden);
    callback();
}

/**
 * @brief 整体替换隐藏集合（先清掉不再隐藏的条目，再加入目标集合）
 *
 * setColumnHidden(name, occ, false) 会从集合移除条目（006a 语义），先清后写即可
 * 得到与目标集合完全一致的状态。
 * @param hidden 目标隐藏集合
 */
void DACommandTableColumnVisibility::applyHidden(const QSet< QPair< QString, int > >& hidden)
{
    if (!mStore) {
        return;
    }
    const QList< QPair< QString, int > > current = mStore->hiddenColumns();  // 局部 const，防 COW 深拷贝
    for (const QPair< QString, int >& ident : current) {
        if (!hidden.contains(ident)) {
            mStore->setColumnHidden(ident.first, ident.second, false);
        }
    }
    for (const QPair< QString, int >& ident : hidden) {
        mStore->setColumnHidden(ident.first, ident.second, true);
    }
}

/**
 * @brief 冻结列变更命令构造
 *
 * 文本按新旧集合差异推导：清空 → Unfreeze all columns；只多一项 → Freeze column "%1"；
 * 只少一项 → Unfreeze column "%1"；其余 → Freeze/Unfreeze columns。
 * @param store 视图状态子存储（非拥有）
 * @param oldFrozen 旧冻结列列表（有序）
 * @param newFrozen 新冻结列列表（有序）
 * @param projectCallback 重投影回调
 * @param commandText 调用方指定的命令文本（空则按差异推导）
 * @param parent 父命令（批量时挂入）
 */
DACommandTableFreezeColumns::DACommandTableFreezeColumns(DATableViewStateStore* store,
                                                         const QList< QPair< QString, int > >& oldFrozen,
                                                         const QList< QPair< QString, int > >& newFrozen,
                                                         CallBack projectCallback,
                                                         const QString& commandText,
                                                         QUndoCommand* parent)
    : QUndoCommand(parent)
    , DACallBackInterface()
    , mStore(store)
    , mOldFrozen(oldFrozen)
    , mNewFrozen(newFrozen)
{
    setCallBack(projectCallback);
    if (!commandText.isEmpty()) {
        setText(commandText);
        return;
    }
    const QSet< QPair< QString, int > > oldSet = toIdentSet(oldFrozen);
    const QSet< QPair< QString, int > > newSet = toIdentSet(newFrozen);
    QSet< QPair< QString, int > > added   = newSet;
    QSet< QPair< QString, int > > removed = oldSet;
    subtractSet(added, oldSet);
    subtractSet(removed, newSet);
    if (added.size() == 1 && removed.isEmpty()) {
        setText(QObject::tr("Freeze column \"%1\"").arg(added.constBegin()->first));  // cn:冻结列"%1"
    } else if (removed.size() == 1 && added.isEmpty()) {
        setText(QObject::tr("Unfreeze column \"%1\"").arg(removed.constBegin()->first));  // cn:取消冻结列"%1"
    } else if (newSet.isEmpty() && !oldSet.isEmpty()) {
        setText(QObject::tr("Unfreeze all columns"));  // cn:取消全部冻结
    } else if (added.size() >= removed.size()) {
        setText(QObject::tr("Freeze columns"));  // cn:冻结列
    } else {
        setText(QObject::tr("Unfreeze columns"));  // cn:取消冻结列
    }
}

/**
 * @brief redo：写新冻结列列表并重投影
 */
void DACommandTableFreezeColumns::redo()
{
    if (mStore) {
        mStore->setFrozenColumns(mNewFrozen);
    }
    callback();
}

/**
 * @brief undo：写旧冻结列列表并重投影
 */
void DACommandTableFreezeColumns::undo()
{
    if (mStore) {
        mStore->setFrozenColumns(mOldFrozen);
    }
    callback();
}

/**
 * @brief 列视图批量变更父命令构造
 *
 * 子命令以本对象为 parent 挂入（构造时），redo/undo 用 QUndoCommand 默认实现。
 * @param text 命令文本（撤销面板显示）
 * @param parent 父命令
 */
DACommandTableColumnViewBatch::DACommandTableColumnViewBatch(const QString& text, QUndoCommand* parent)
    : QUndoCommand(parent)
{
    setText(text);
}
}  // end of namespace DA
