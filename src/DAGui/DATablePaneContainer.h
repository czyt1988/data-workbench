#ifndef DATABLEPANECONTAINER_H
#define DATABLEPANECONTAINER_H
#include "DAGuiAPI.h"
#include <QSet>
#include <QVector>
#include <QWidget>

class QHeaderView;

namespace DA
{
class DADataTableView;

/**
 * @brief 数据表格列轴窗格容器（006 计划 WP5）
 *
 * 包装主窗格并懒建冻结窗格（freezeCount > 0 时），把页面算出的列视图状态投影到
 * 各窗格：冻结列在左、主区在右，行头归属冻结窗格 → 布局 [行号][冻结列][主区]。
 * 容器不持模型/委托/store——页面是唯一真源操作方；懒建时序必须是：
 * emit frozenViewCreated（页面同步 configurePaneView 装配）→ 容器投影 → show()。
 */
class DAGUI_API DATablePaneContainer : public QWidget
{
    Q_OBJECT
public:
    explicit DATablePaneContainer(QWidget* parent = nullptr);

    // 运行时包装：主窗格重父化到本容器布局（ui->tableView 指针不变）
    void setMainView(DADataTableView* mainView);
    DADataTableView* mainView() const;
    // 冻结窗格（未懒建时 nullptr）
    DADataTableView* frozenView() const;

    // 列状态投影：ordered = 全部数据列显示顺序（含隐藏列，logical 升序表）；
    // hidden = 用户隐藏集合；freezeCount = 冻结的可见列数（容器内夹紧到可见列数）
    void applyColumnState(const QVector< int >& orderedLogical, const QSet< int >& hiddenLogical, int freezeCount);
    // 生效冻结列数（= 本轮投影实际切进冻结窗格的列数）
    int effectiveFrozenColumnCount() const;
    // 最近一次投影的全局列序 / 隐藏集合缓存（006c 合并全局列序用）
    QVector< int > cachedOrderedLogical() const;
    QSet< int > cachedHiddenLogical() const;
    // 只读口径 getter（006d「冻结到此列」/ 006e/007 冻结区判定共用，调用点不要重算"列序剔隐藏"）
    QVector< int > orderedColumnsLogical() const;     // 数据列全序（logical，含隐藏列）
    QVector< int > visibleColumnOrderLogical() const; // 可见列序（logical，按视图顺序剔除隐藏列）
    // 本轮投影切进冻结窗格的列集合（logical）
    QSet< int > frozenColumnsLogical() const;

    // 重算冻结窗格宽度与底部等高占位（列宽/行头宽/滚动条可见性变化后调用，幂等）
    void updateFrozenGeometry();

public Q_SLOTS:
    // 窗格表头 sectionMoved 落库槽（006c）：读回发起窗格可见列新顺序 →
    // 合并全局列序（冻结区可见列 ++ 主区可见列 ++ 隐藏列保持旧相对顺序）→
    // emit columnOrderChanged 交页面入栈；投影期间（mApplyingColumnState）不回写
    void onPaneSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);

Q_SIGNALS:
    // 冻结窗格懒建完成（006e：此时窗格已进布局但尚未装配，页面在槽里 configurePaneView）
    void frozenViewCreated(DA::DADataTableView* frozenView);
    // 拖动落库后的全局列序（006c：页面入栈后统一重投影）
    void columnOrderChanged(const QVector< int >& newGlobalOrderLogical);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // 单窗格投影：可见性（不在显示集合里的段隐藏）+ 顺序（目标列 visualIndex 依次挪到 0,1,2…）
    void projectPane(DADataTableView* pane, const QVector< int >& paneVisibleOrdered);
    // 懒建冻结窗格（竖排布局 [frozen][bottomSpacer] + 滚动同步接线）
    void ensureFrozenView();
    // 纵向滚动同步（双向，守卫防乒乓）
    void syncScroll(int value);
    // 冻结窗格宽度 = Σ冻结列宽 + 行头宽 + 2×frame + 2px 余量
    void updateFrozenWidth();
    // 底部等高占位高度 = 主窗格横向滚动条可见时的高度（隐藏时 0）
    void updateFrozenBottomSpacer();
    // 指定窗格的可见数据列按 visualIndex 升序读回（过滤尾部预留空白列）
    QVector< int > visibleDataColumnsOf(DADataTableView* pane) const;

private:
    DADataTableView* mMainView { nullptr };   ///< 非拥有（页面持有，本容器重父化到自己的布局）
    DADataTableView* mFrozenView { nullptr }; ///< 冻结窗格（容器拥有，hide 折叠不销毁）
    QWidget* mFrozenSide { nullptr };         ///< 冻结侧容器（[frozen][bottomSpacer] 竖排零边距）
    QWidget* mBottomSpacer { nullptr };       ///< 底部等高占位（对齐主窗格横向滚动条高度）
    QVector< int > mOrderedLogical;           ///< 全局列序缓存
    QSet< int > mHiddenLogical;               ///< 隐藏集合缓存
    QSet< int > mFrozenLogical;               ///< 本轮投影的冻结列集合
    int mEffectiveFrozen { 0 };               ///< 本轮投影实际冻结列数
    bool mApplyingColumnState { false };      ///< 投影守卫（006c 屏蔽 sectionMoved 回写）
    bool mSyncingScroll { false };            ///< 滚动同步守卫（防乒乓）
};
}  // end of namespace DA
#endif  // DATABLEPANECONTAINER_H
