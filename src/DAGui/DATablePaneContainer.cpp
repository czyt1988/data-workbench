#include "DATablePaneContainer.h"
#include "DADataHeaderView.h"
#include "DADataTableView.h"
#include <QEvent>
#include <QHBoxLayout>
#include <QLayout>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

namespace DA
{

/**
 * @brief 构造函数
 *
 * 水平布局、零边距零间距（冻结窗格并排不能有缝）。
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
 * setParent 会把部件置为隐藏，重挂后显式 show() 恢复。主窗格横向滚动条
 * 装事件过滤器（底部等高占位随其可见性变化）。
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
    if (QScrollBar* hb = mMainView->horizontalScrollBar()) {
        hb->installEventFilter(this);
    }
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
 * @return 未冻结过时 nullptr；冻结过则恒返回窗格（hide 折叠不销毁）
 */
DADataTableView* DATablePaneContainer::frozenView() const
{
    return mFrozenView;
}

/**
 * @brief 单窗格投影
 *
 * 1) 可见性：不在本窗格显示集合里的段一律隐藏（用户隐藏列两边都隐藏；
 *    冻结窗格不显示主区列，反之亦然）；2) 顺序：目标列 visualIndex 依次挪到
 *    0,1,2…（视觉槽位含隐藏段，不要手算目标 index）。未装模型/表头的窗格上
 *    moveSection/setSectionHidden 是空操作，直接 return（懒建顺序的前提）。
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
 * @brief 懒建冻结窗格
 *
 * 冻结侧容器竖排布局 [frozen][bottomSpacer]（零边距零间距，底部等高占位把
 * 冻结窗格视口压到与主窗格视口等高），插入主窗格左侧（布局 [行号][冻结列][主区]）。
 * 滚动条策略 AlwaysOff（纵向滚动走同步、横向冻结区不滚）。
 * 懒建完成后 emit frozenViewCreated——页面同步 configurePaneView 装配表头/模型/
 * 委托/接线，容器随后才能投影（未装配的窗格上投影是空操作）。
 */
void DATablePaneContainer::ensureFrozenView()
{
    if (mFrozenView) {
        return;
    }
    mFrozenSide = new QWidget(this);
    QVBoxLayout* vlay = new QVBoxLayout(mFrozenSide);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);
    mFrozenView = new DADataTableView(mFrozenSide);
    mFrozenView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mFrozenView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mBottomSpacer = new QWidget(mFrozenSide);
    mBottomSpacer->setFixedHeight(0);
    vlay->addWidget(mFrozenView);
    vlay->addWidget(mBottomSpacer);
    // 冻结侧插到主窗格之前（左侧）
    if (QHBoxLayout* hlay = qobject_cast< QHBoxLayout* >(layout())) {
        hlay->insertWidget(0, mFrozenSide);
    }
    // 纵向滚动同步（双向，守卫防乒乓）
    connect(mFrozenView->verticalScrollBar(), &QScrollBar::valueChanged, this, &DATablePaneContainer::syncScroll);
    if (mMainView) {
        connect(mMainView->verticalScrollBar(), &QScrollBar::valueChanged, this, &DATablePaneContainer::syncScroll);
    }
    Q_EMIT frozenViewCreated(mFrozenView);
}

/**
 * @brief 列状态投影（双窗格）
 *
 * 缓存 ordered/hidden/frozen（006c 合并全局列序、冻结区判定用）。冻结集合 =
 * ordered 中前 freeze 个非隐藏列，其余非隐藏列归主区；用户隐藏列两边都隐藏。
 * 主窗格显示集合额外追加尾部预留空白列（不在 ordered 里的段：恒可见、恒排末尾，
 * 不参与列序/隐藏）；冻结窗格不显示空白列。投影全程置守卫：moveSection 自己
 * 也会 emit sectionMoved，不屏蔽会"程序化重投影 → 又生成一条列序命令"死循环。
 * @param orderedLogical 全局列序（logical，含隐藏列）
 * @param hiddenLogical 用户隐藏集合
 * @param freezeCount 冻结的可见列数
 */
void DATablePaneContainer::applyColumnState(const QVector< int >& orderedLogical,
                                             const QSet< int >& hiddenLogical, int freezeCount)
{
    mOrderedLogical = orderedLogical;
    mHiddenLogical  = hiddenLogical;
    const int freeze = qBound(0, freezeCount, orderedLogical.size());
    QVector< int > frozenSet, mainSet;
    mFrozenLogical.clear();
    for (int logical : orderedLogical) {
        if (hiddenLogical.contains(logical)) {
            continue;  // 用户隐藏列两边都隐藏
        }
        if (frozenSet.size() < freeze) {
            frozenSet.append(logical);
            mFrozenLogical.insert(logical);
        } else {
            mainSet.append(logical);
        }
    }
    mEffectiveFrozen = frozenSet.size();
    if (mEffectiveFrozen > 0) {
        ensureFrozenView();  // 懒建 → frozenViewCreated（页面同步装配）→ 之后投影才不是空操作
    } else if (mFrozenView) {
        mFrozenSide->hide();  // 只隐藏不销毁（解冻再冻结走"复用已有窗格"路径）
    }
    QVector< int > mainVisible = mainSet;
    if (mMainView) {
        if (QHeaderView* hh = mMainView->horizontalHeader()) {
            for (int c = 0; c < hh->count(); ++c) {  // 尾部预留空白列恒可见、恒排数据列之后
                if (!orderedLogical.contains(c)) {
                    mainVisible.append(c);
                }
            }
        }
    }
    mApplyingColumnState = true;
    if (mEffectiveFrozen > 0 && mFrozenView) {
        mFrozenSide->show();
        projectPane(mFrozenView, frozenSet);
    }
    projectPane(mMainView, mainVisible);
    mApplyingColumnState = false;
    // 行头归属 + 冻结窗格宽（幂等；冻结数为 0 时隐藏行头分支无效）
    if (mFrozenView) {
        mFrozenView->verticalHeader()->setVisible(mEffectiveFrozen > 0);
        mFrozenView->setVisible(mEffectiveFrozen > 0);
    }
    if (mMainView) {
        mMainView->verticalHeader()->setVisible(mEffectiveFrozen <= 0);
    }
    updateFrozenGeometry();
    // 冻结窗格刚建/刚显示时行头与段宽要在布局稳定后才有终值——补一次延迟重算
    QTimer::singleShot(0, this, [ this ]() { updateFrozenGeometry(); });
}

/**
 * @brief 显示/尺寸变化后重算冻结几何（行头宽随布局变化，006e）
 */
void DATablePaneContainer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateFrozenGeometry();
}

/**
 * @brief 尺寸变化后重算冻结几何（主窗格横向滚动条显隐随之变化）
 */
void DATablePaneContainer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateFrozenGeometry();
}

/**
 * @brief 生效冻结列数
 * @return 本轮投影实际切进冻结窗格的列数
 */
int DATablePaneContainer::effectiveFrozenColumnCount() const
{
    return mEffectiveFrozen;
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
 * 与 applyColumnState 的分割法律同源：ordered 顺序剔掉 hidden。
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

/**
 * @brief 本轮投影切进冻结窗格的列集合
 * @return 冻结列集合（logical）
 */
QSet< int > DATablePaneContainer::frozenColumnsLogical() const
{
    return mFrozenLogical;
}

/**
 * @brief 指定窗格的可见数据列读回
 *
 * 按 visualIndex 升序收集该窗格当前可见且在缓存 ordered 里的列
 * （过滤尾部预留空白列与隐藏列）。
 * @param pane 目标窗格
 * @return 可见数据列（logical，按当前视图顺序）
 */
QVector< int > DATablePaneContainer::visibleDataColumnsOf(DADataTableView* pane) const
{
    QVector< int > res;
    if (!pane) {
        return res;
    }
    QHeaderView* hh = pane->horizontalHeader();
    if (!hh) {
        return res;
    }
    for (int v = 0; v < hh->count(); ++v) {
        const int logical = hh->logicalIndex(v);
        if (logical < 0 || hh->isSectionHidden(logical) || !mOrderedLogical.contains(logical)) {
            continue;
        }
        res.append(logical);
    }
    return res;
}

/**
 * @brief 窗格表头 sectionMoved 落库槽（006c，006e 扩双窗格）
 *
 * 合并成全局列序：冻结区可见列（冻结窗格按 visualIndex 升序）++ 主区可见列 ++
 * 用户隐藏列（保持旧全局列序里的相对顺序），最后补齐漏掉的列（防御）。
 * 程序化投影里的 moveSection 自己也会 emit sectionMoved——被 mApplyingColumnState
 * 守卫挡住（不回写，防死循环）。
 */
void DATablePaneContainer::onPaneSectionMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex)
{
    Q_UNUSED(logicalIndex);
    Q_UNUSED(oldVisualIndex);
    Q_UNUSED(newVisualIndex);
    if (mApplyingColumnState || !mMainView) {
        return;  // 程序化投影不回写
    }
    QVector< int > merged;
    if (mFrozenView && mFrozenSide && mFrozenSide->isVisible()) {
        merged << visibleDataColumnsOf(mFrozenView);  // 冻结区可见列插到最前
    }
    merged << visibleDataColumnsOf(mMainView);
    for (int logical : mOrderedLogical) {
        if (mHiddenLogical.contains(logical)) {
            merged.append(logical);  // 隐藏列钉住旧相对顺序
        }
    }
    // 补齐：漏掉的列按旧全局列序追加（防御：绝不让某列从列序里消失）
    for (int logical : mOrderedLogical) {
        if (!merged.contains(logical)) {
            merged.append(logical);
        }
    }
    if (merged.isEmpty()) {
        return;
    }
    Q_EMIT columnOrderChanged(merged);
}

/**
 * @brief 事件过滤：主窗格横向滚动条显隐/尺寸变化 → 重算底部等高占位
 */
bool DATablePaneContainer::eventFilter(QObject* watched, QEvent* event)
{
    if (mMainView && watched == mMainView->horizontalScrollBar()
        && (event->type() == QEvent::Show || event->type() == QEvent::Hide || event->type() == QEvent::Resize)) {
        updateFrozenBottomSpacer();
    }
    return QWidget::eventFilter(watched, event);
}

/**
 * @brief 重算冻结窗格宽度与底部等高占位（幂等）
 *
 * 列宽拖拽、冻结数变化、行头宽变化、窗口缩放后各调一次。
 */
void DATablePaneContainer::updateFrozenGeometry()
{
    if (!mFrozenView || !mMainView) {
        return;
    }
    updateFrozenWidth();
    updateFrozenBottomSpacer();
}

/**
 * @brief 冻结窗格宽度
 *
 * = Σ(冻结列 sectionSize) + 冻结窗格行头宽 + 2×frameWidth + 2px 余量
 * （余量是 AlwaysOff 下"最后一列不被裁"的保险；行头宽取归属方——冻结窗格）。
 */
void DATablePaneContainer::updateFrozenWidth()
{
    if (!mFrozenView || !mFrozenView->isVisible()) {
        return;
    }
    QHeaderView* fh = mFrozenView->horizontalHeader();
    if (!fh) {
        return;
    }
    int w = 0;
    const auto frozenList = mFrozenLogical.values();
    for (int logical : frozenList) {
        w += fh->sectionSize(logical);
    }
    if (QHeaderView* vh = mFrozenView->verticalHeader()) {
        w += vh->width();
    }
    w += 2 * mFrozenView->frameWidth() + 2;
    mFrozenView->setFixedWidth(w);
}

/**
 * @brief 底部等高占位高度
 *
 * = 主窗格横向滚动条可见时的高度、隐藏时 0。冻结窗格横向滚动条 AlwaysOff，
 * 其视口比主窗格高一个滚动条；占位把冻结窗格高度压到与主窗格视口等高，
 * 保证数据末尾一屏不错行（不占用 setViewportMargins——会被
 * QTableView::updateGeometries() 覆盖）。
 */
void DATablePaneContainer::updateFrozenBottomSpacer()
{
    if (!mBottomSpacer || !mMainView) {
        return;
    }
    QScrollBar* hb = mMainView->horizontalScrollBar();
    mBottomSpacer->setFixedHeight((hb && hb->isVisible()) ? hb->height() : 0);
}

/**
 * @brief 纵向滚动同步（双向）
 *
 * 守卫期间不再转发（防乒乓）；只挡重入，不限制方向。冻结窗格上的滚轮经
 * QAbstractScrollArea::wheelEvent 转发给纵向滚动条（隐藏的滚动条也接受
 * setValue），滚轮路径同样生效。
 */
void DATablePaneContainer::syncScroll(int value)
{
    if (mSyncingScroll || !mMainView || !mFrozenView) {
        return;
    }
    QScrollBar* from = qobject_cast< QScrollBar* >(sender());
    if (!from) {
        return;
    }
    QScrollBar* to = (from == mMainView->verticalScrollBar()) ? mFrozenView->verticalScrollBar()
                                                             : mMainView->verticalScrollBar();
    mSyncingScroll = true;
    if (to && to->value() != value) {
        to->setValue(value);
    }
    mSyncingScroll = false;
}

}  // end of namespace DA
