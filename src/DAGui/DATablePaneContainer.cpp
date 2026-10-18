#include "DATablePaneContainer.h"
#include "DADataTableView.h"
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLayout>

namespace DA
{

/**
 * @brief 构造函数
 *
 * 水平布局、零边距零间距（006e 冻结窗格并排进来不能有缝）。
 * @param parent 父部件
 */
DATablePaneContainer::DATablePaneContainer(QWidget* parent) : QWidget(parent)
{
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
}

/**
 * @brief 运行时包装主窗格
 *
 * 进布局即重父化：ui->tableView 指针不变（006 计划 §4.4，APP/插件调用点零改动）。
 * setParent 会把部件置为隐藏，重挂后显式 show() 恢复。
 * @param mainView 主窗格（页面从 .ui 接管的原表格视图）
 */
void DATablePaneContainer::setMainView(DADataTableView* mainView)
{
    if (!mainView || mMainView == mainView) {
        return;
    }
    mMainView = mainView;
    layout()->addWidget(mainView);
    mainView->show();
}

/**
 * @brief 主窗格
 * @return 主窗格指针（未包装为 nullptr）
 */
DADataTableView* DATablePaneContainer::mainView() const
{
    return mMainView;
}

/**
 * @brief 冻结窗格
 * @return 本单恒 nullptr（006e 起懒建）
 */
DADataTableView* DATablePaneContainer::frozenView() const
{
    return nullptr;
}

/**
 * @brief 单窗格投影
 *
 * 1) 可见性：不在本窗格显示集合里的段一律隐藏（006e 再加"冻在另一个窗格"）；
 * 2) 顺序：目标列 visualIndex 依次挪到 0,1,2…（视觉槽位含隐藏段，不要手算目标 index）。
 * 未装模型/表头的窗格上 moveSection/setSectionHidden 是空操作，直接 return（防御）。
 * @param pane 目标窗格
 * @param paneVisibleOrdered 本窗格要显示的列（logical，按目标顺序）
 */
void DATablePaneContainer::projectPane(DADataTableView* pane, const QVector< int >& paneVisibleOrdered)
{
    if (!pane || !pane->model()) {
        return;
    }
    QHeaderView* hh = pane->horizontalHeader();
    if (!hh || hh->count() <= 0) {
        return;
    }
    QSet< int > showSet;
    showSet.reserve(paneVisibleOrdered.size());
    for (int logical : paneVisibleOrdered) {
        showSet.insert(logical);
    }
    for (int c = 0; c < hh->count(); ++c) {
        // QTableView 的列隐藏 API 是 setColumnHidden（转发到表头 setSectionHidden）
        pane->setColumnHidden(c, !showSet.contains(c));
    }
    for (int i = 0; i < paneVisibleOrdered.size(); ++i) {
        const int logical = paneVisibleOrdered.at(i);
        const int from    = hh->visualIndex(logical);
        if (from >= 0 && from != i) {
            hh->moveSection(from, i);
        }
    }
    pane->viewport()->update();
}

/**
 * @brief 列状态投影（单窗格）
 *
 * 缓存 ordered/hidden（006c 合并全局列序要用）。本窗格显示集合 = 全局列序里的
 * 非隐藏列（按全局列序）++ 尾部预留空白列（不在 ordered 里的段：列名为空、不参与
 * 列序/隐藏，恒可见、恒排数据列之后）。freezeCount 本单只有主窗格，冻结语义见
 * 006e（那里把冻结段切出去）。投影全程置守卫：投影里的 moveSection 自己也会
 * emit sectionMoved，006c 起靠它防回写死循环。
 * @param orderedLogical 全局列序（logical，含隐藏列）
 * @param hiddenLogical 用户隐藏集合
 * @param freezeCount 冻结列数（本容器内夹紧为 0）
 */
void DATablePaneContainer::applyColumnState(const QVector< int >& orderedLogical,
                                             const QSet< int >& hiddenLogical, int freezeCount)
{
    mOrderedLogical = orderedLogical;
    mHiddenLogical  = hiddenLogical;
    if (!mMainView) {
        return;
    }
    Q_UNUSED(freezeCount);  // 本单只有主窗格：冻结语义见 006e（那里把冻结段切出去）
    const int freeze = 0;
    Q_UNUSED(freeze);
    QVector< int > paneVisible;  // 本窗格显示集合 = 全局列序里的非隐藏列（按全局列序）
    paneVisible.reserve(orderedLogical.size());
    for (int logical : orderedLogical) {
        if (!hiddenLogical.contains(logical)) {
            paneVisible.append(logical);
        }
    }
    if (QHeaderView* hh = mMainView->horizontalHeader()) {
        for (int c = 0; c < hh->count(); ++c) {  // 尾部预留空白列：恒可见、恒排数据列之后
            if (!orderedLogical.contains(c)) {
                paneVisible.append(c);
            }
        }
    }
    mApplyingColumnState = true;
    projectPane(mMainView, paneVisible);
    mApplyingColumnState = false;
}

/**
 * @brief 生效冻结列数
 * @return 本单恒 0（006e 起真实值）
 */
int DATablePaneContainer::effectiveFrozenColumnCount() const
{
    return 0;
}

/**
 * @brief 最近一次投影的全局列序缓存
 * @return 全局列序（logical，含隐藏列）
 */
QVector< int > DATablePaneContainer::cachedOrderedLogical() const
{
    return mOrderedLogical;
}

/**
 * @brief 最近一次投影的隐藏集合缓存
 * @return 隐藏集合（logical）
 */
QSet< int > DATablePaneContainer::cachedHiddenLogical() const
{
    return mHiddenLogical;
}

/**
 * @brief 数据列全序
 * @return 最近一次投影的全局列序（logical，含隐藏列）
 */
QVector< int > DATablePaneContainer::orderedColumnsLogical() const
{
    return mOrderedLogical;
}

/**
 * @brief 可见列序
 *
 * 与 applyColumnState 里 paneVisible 的算法律同源：ordered 顺序剔掉 hidden。
 * 冻结区判定与「冻结到此列」都取这一口径，调用点不要重算。
 * @return 可见列序（logical，按视图顺序）
 */
QVector< int > DATablePaneContainer::visibleColumnOrderLogical() const
{
    QVector< int > res;
    res.reserve(mOrderedLogical.size());
    for (int logical : mOrderedLogical) {
        if (!mHiddenLogical.contains(logical)) {
            res.append(logical);
        }
    }
    return res;
}

}  // end of namespace DA
