#ifndef DATABLESORTCONTROLLER_H
#define DATABLESORTCONTROLLER_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QPair>
#include <QVector>

class QUndoCommand;

namespace DA
{
class DATablePageContext;

/**
 * @brief 排序控制器（008 期3）
 *
 * 多列排序规格（列逻辑索引+方向，顺序即优先级；不持久化 R13）、
 * 排序动作统一入口（守卫→构造规格→命令入栈）、选区置换重映射、
 * 表头排序角标。构造注入上下文，不持窗口指针。
 * 排序不改变列值集合 → 条件格式列统计不失效（003 性能决策）；
 * 计算列不标脏（R13 行顺序变化不影响列值）。
 * 排序是破坏式重排：exec/undo 都经上下文 notifyDataModified 标脏。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableSortController : public QObject
{
    Q_OBJECT
public:
    explicit DATableSortController(const DATablePageContext& ctx, QObject* parent = nullptr);

public:
    // 当前多列排序规格
    QVector< QPair< int, Qt::SortOrder > > sortSpec() const;
    // 按列排序（重置规格为「仅该列」）：只读/惰性源 daWarning 提示一次；拾取模式下不执行
    void sortColumn(int logicalIndex, Qt::SortOrder order);
    // 追加为次级排序列（保留既有规格；该列已在规格中则切换其方向）
    void addSecondarySortColumn(int logicalIndex, Qt::SortOrder order);
    // 清除排序规格并刷新全部窗格角标（插删列/换数据集后列位置失效时用；数据顺序不变）
    void clearSpec();

private:
    // 排序动作统一入口（菜单驱动）：守卫（拾取模式/边界/只读/空表）→ 构造规格 → applySpec；
    // addKey=false 重置为「仅该列」，true 保留既有规格追加/切换该列
    bool applySortAction(int logicalIndex, Qt::SortOrder order, bool addKey);
    // 应用排序规格：构造 DACommandDataFrame_sortRows、接线样式/模型/选区/视口回调并 push；
    // 算子失败（如混合类型不可比较）提示并保持原规格，返回 false
    bool applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& newSpec);
    // 按最近应用的置换修正选区（选中行随排序跟随；游程压缩，巨大散落选区降级）
    void remapSelectionByPermutation(const QVector< int >& appliedPerm);
    // 刷新全部窗格表头的排序角标
    void refreshHeaders();

private:
    const DATablePageContext& mCtx;  ///< 依赖注入的上下文（构造注入，非拥有）
    QVector< QPair< int, Qt::SortOrder > > mSortSpec;  ///< 多列排序规格（不持久化 R13）
};
}  // end of namespace DA
#endif  // DATABLESORTCONTROLLER_H
