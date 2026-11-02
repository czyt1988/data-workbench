#ifndef DATABLEFINDCONTROLLER_H
#define DATABLEFINDCONTROLLER_H
#include "DAGuiAPI.h"
#include "Dialog/DADialogFindReplace.h"
#include <QPair>
#include <QVector>
#include <optional>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;

/**
 * @brief 查找/替换控制器（008 期3）
 *
 * 查找/替换状态机（命中集、当前序号、请求串号）、浮动条生命周期、扫描
 * （去抖在对话框侧）、循环导航、替换（含超阈值不可撤销降级路径）。
 * 构造注入上下文、选区服务与编辑后效协调器，不持窗口指针。
 * 命中灌入瞬态高亮层 Source::Find（Hit/CurrentHit 两级，来源隔离）；
 * 替换命令的后效经协调器 commit（值变更：受影响列统计失效+计算列标脏）。
 * 生命周期：宿主页面构造并持有（QObject 父子持有浮动条）。
 */
class DAGUI_API DATableFindController : public QObject
{
    Q_OBJECT
public:
    explicit DATableFindController(const DATablePageContext& ctx,
                                   const DATableSelectionService& sel,
                                   DATableMutationCoordinator* mutation,
                                   QObject* parent = nullptr);

public:
    // 打开查找/替换浮动条（懒建，非模态）；expandReplace=true 展开替换行（Ctrl+H 语义）；
    // prefillFromCurrentCell=true（右键菜单入口）时查找框预填当前单格显示文本
    void showFindBar(bool expandReplace, bool prefillFromCurrentCell = false);
    // 关闭浮动条（清高亮经 closed → onDialogClosed）
    void closeFindBar();
    // 当前查找命中数（已装载高亮，≤上限）与当前命中序号（-1=无）
    int hitCount() const;
    int currentHitIndex() const;
    // 选区变化且查找条处于「当前选区」范围模式时重启去抖重扫
    //（页面选区变化信号接线用；范围基准随选区更新）
    void restartDebounceIfVisible();

Q_SIGNALS:
    // 替换已生效（含超阈值不可撤销降级路径）；changedCells=变更格数；
    // 页面转发给 APP（setDirty）
    void replaceApplied(int changedCells);

private Q_SLOTS:
    // 查找条：扫描请求（去抖到期）、上一个/下一个导航、替换当前、全部替换、关闭清高亮
    void onSearchRequested();
    void onPrevRequested();
    void onNextRequested();
    void onReplaceRequested();
    void onReplaceAllRequested();
    void onDialogClosed();

private:
    // 执行一次查找扫描（同步 + 忙等待光标 + 请求序号丢弃过期结果，硬约束②）
    void runScan();
    // 导航到第 idx 条命中（循环）：更新 Level::CurrentHit + 定位（select 随范围模式）
    void gotoHit(int idx);
    // 解析扫描范围：全表 → nullopt；选区 → 行/列包围盒（数据源绝对坐标）
    std::optional< QPair< int, int > > scanRowRange(bool selectionOnly) const;
    std::optional< QPair< int, int > > scanColRange(bool selectionOnly) const;
    // 更新计数标签（含截断提示）
    void updateCountLabel();
    // 应用替换：all=false 替换当前命中这一格，true 替换扫描范围全部；
    // 变更格数超阈值时提示不可撤销并走降级路径（不入栈，§4.11）
    void applyReplace(bool all);

private:
    const DATablePageContext& mCtx;   ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableMutationCoordinator* mMutation { nullptr };  ///< 编辑后效协调器（宿主持有，非拥有）

    DADialogFindReplace* mFindDialog { nullptr };  ///< 非模态浮动查找/替换条（懒建）
    QVector< QPair< int, int > > mHits;  ///< 当前命中集（数据源绝对行列，行主序，≤上限）
    int mHitIndex { -1 };                 ///< 当前命中序号（-1=无当前项）
    int mTotalHits { 0 };                 ///< 真实命中总数（截断时 > mHits.size()）
    int mRequestSerial { 0 };             ///< 扫描请求序号（硬约束②：结果返回时非最新即丢弃）
    DADialogFindReplace::FindOptions mLastOptions;  ///< 最近一次扫描的参数（范围/导航语义用）
};
}  // end of namespace DA
#endif  // DATABLEFINDCONTROLLER_H
