#include "DATablePaneContainer.h"
#include "DADataHeaderView.h"
#include "DADataTableView.h"
#include <QEvent>
#include <QHBoxLayout>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

namespace DA
{

namespace
{
// 分界条宽度（像素）
constexpr int c_separatorWidth = 5;
// 分界条拖动的末冻结列宽上限（像素，防呆）
constexpr int c_boundaryMaxColumnWidth = 4096;

/**
 * @brief 冻结分界条（水平布局 [冻结侧][分界条][主窗格] 中的把手）
 *
 * 只做事件转发与自绘，拖动状态机在容器（beginFrozenBoundaryDrag / dragFrozenBoundary /
 * endFrozenBoundaryDrag）。常态画 1px 中灰竖线（DESIGN.md 分隔线色），hover/拖动时
 * 加粗为主题高亮色。
 */
class FrozenPaneSeparator : public QWidget
{
public:
    explicit FrozenPaneSeparator(DATablePaneContainer* owner, QWidget* parent = nullptr)
        : QWidget(parent), mOwner(owner)
    {
        setFixedWidth(c_separatorWidth);
        setCursor(Qt::SizeHorCursor);
        setMouseTracking(true);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        const QColor line = (mHovered || mDragging) ? palette().color(QPalette::Highlight)
                                                    : QColor(0x72, 0x72, 0x72);
        p.setPen(QPen(line, (mHovered || mDragging) ? 2 : 1));
        p.drawLine(width() / 2, 0, width() / 2, height());
    }

    void enterEvent(QEvent* event) override
    {
        QWidget::enterEvent(event);
        mHovered = true;
        update();
    }

    void leaveEvent(QEvent* event) override
    {
        QWidget::leaveEvent(event);
        mHovered = false;
        update();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && mOwner->beginFrozenBoundaryDrag()) {
            mDragging = true;
            mPressGlobalX = globalPosOf(event).x();
            update();
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (mDragging) {
            mOwner->dragFrozenBoundary(globalPosOf(event).x() - mPressGlobalX);
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (mDragging && event->button() == Qt::LeftButton) {
            mDragging = false;
            mOwner->endFrozenBoundaryDrag();
            update();
        }
        QWidget::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            mOwner->requestFrozenBoundaryAutoFit();
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    // 鼠标事件的全局坐标（Qt5/Qt6 兼容，与 DADataHeaderView 同法）
    static QPoint globalPosOf(const QMouseEvent* event)
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return event->globalPos();
#else
        return event->globalPosition().toPoint();
#endif
    }

    DATablePaneContainer* mOwner;  ///< 非拥有（事件转发的目标容器）
    int mPressGlobalX { 0 };       ///< 拖动起始全局 x
    bool mHovered { false };       ///< hover 态（加粗高亮反馈）
    bool mDragging { false };      ///< 拖动态（与容器拖动状态机同步）
};
}  // namespace

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
    // 分界条（冻结/主区把手，拖动改末冻结列宽，Excel 语义）
    mSeparator = new FrozenPaneSeparator(this);
    // 布局 [冻结侧][分界条][主窗格]：冻结侧与分界条插到主窗格之前
    if (QHBoxLayout* hlay = qobject_cast< QHBoxLayout* >(layout())) {
        hlay->insertWidget(0, mFrozenSide);
        hlay->insertWidget(1, mSeparator);
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
 * 传入的冻结列列表（可来自列序任意位置；隐藏列仍占冻结身份，只是不显示，
 * 主区列不会补位）。其余未隐藏列归主区，顺序按全局列序。用户隐藏列两边都隐藏。
 * 全部冻结列被隐藏时冻结窗格整体隐藏（冻结身份保留，任一冻结列恢复可见即恢复
 * 窗格）。主窗格显示集合额外追加尾部预留空白列（不在 ordered 里的段：恒可见、
 * 恒排末尾，不参与列序/隐藏）；冻结窗格不显示空白列。投影全程置守卫：
 * moveSection 自己也会 emit sectionMoved，不屏蔽会"程序化重投影 → 又生成一条
 * 列序命令"死循环。
 * @param orderedLogical 全局列序（logical，含隐藏列）
 * @param hiddenLogical 用户隐藏集合
 * @param frozenOrderLogical 冻结列（logical，有序 = 冻结区显示顺序）
 */
void DATablePaneContainer::applyColumnState(const QVector< int >& orderedLogical,
                                             const QSet< int >& hiddenLogical,
                                             const QVector< int >& frozenOrderLogical)
{
    mOrderedLogical = orderedLogical;
    mHiddenLogical  = hiddenLogical;
    QVector< int > frozenSet;  // 冻结窗格要显示的可见冻结列（保持冻结区顺序）
    mFrozenOrderLogical.clear();
    mFrozenLogical.clear();
    mFrozenOrderLogical.reserve(frozenOrderLogical.size());
    for (int logical : frozenOrderLogical) {
        if (!orderedLogical.contains(logical)) {
            continue;  // 防御：不在全局列序里的冻结列跳过
        }
        mFrozenOrderLogical.append(logical);
        mFrozenLogical.insert(logical);
        if (!hiddenLogical.contains(logical)) {
            frozenSet.append(logical);  // 只有可见的冻结列进冻结窗格
        }
    }
    QVector< int > mainSet;
    mainSet.reserve(orderedLogical.size());
    for (int logical : orderedLogical) {
        if (!mFrozenLogical.contains(logical) && !hiddenLogical.contains(logical)) {
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
    // 行头归属 + 冻结窗格宽（幂等；无可见冻结列时不显示冻结窗格与分界条）
    if (mFrozenView) {
        mFrozenView->verticalHeader()->setVisible(mEffectiveFrozen > 0);
        mFrozenView->setVisible(mEffectiveFrozen > 0);
    }
    if (mSeparator) {
        mSeparator->setVisible(mEffectiveFrozen > 0);
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
 * 冻结区判定与定位都取这一口径，调用点不要重算。
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
 * @brief 本轮投影的冻结列（有序 = 冻结区显示顺序）
 * @return 冻结列（logical，含被隐藏的冻结列）
 */
QVector< int > DATablePaneContainer::frozenColumnOrderLogical() const
{
    return mFrozenOrderLogical;
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
 * 按拖动来源分流：
 * - 冻结窗格：只重排冻结区内部顺序（冻结列之间的先后），隐藏的冻结列按旧相对
 *   顺序追加，emit frozenOrderChanged；
 * - 主窗格：合并全局列序——冻结列**钉在全局列序原位**（冻结不改变列序，解冻
 *   即回原位），主区拖动后的可见顺序依次填入非冻结槽位，隐藏的非冻结列追加
 *   末尾，最后补齐漏列（防御），emit columnOrderChanged。
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
    QHeaderView* movedHeader = qobject_cast< QHeaderView* >(sender());
    // —— 冻结窗格拖动：只重排冻结区内部顺序 ——
    if (mFrozenView && movedHeader == mFrozenView->horizontalHeader()) {
        QVector< int > newFrozenOrder = visibleDataColumnsOf(mFrozenView);
        if (newFrozenOrder.isEmpty()) {
            return;
        }
        for (int logical : std::as_const(mFrozenOrderLogical)) {
            if (!newFrozenOrder.contains(logical)) {
                newFrozenOrder.append(logical);  // 隐藏的冻结列按旧相对顺序追加
            }
        }
        if (newFrozenOrder != mFrozenOrderLogical) {
            Q_EMIT frozenOrderChanged(newFrozenOrder);
        }
        return;
    }
    // —— 主窗格拖动：合并全局列序 ——
    const QVector< int > mainVisible = visibleDataColumnsOf(mMainView);
    QVector< int > merged;
    merged.reserve(mOrderedLogical.size());
    int mi = 0;
    for (int logical : std::as_const(mOrderedLogical)) {
        if (mFrozenLogical.contains(logical)) {
            merged.append(logical);  // 冻结列钉在全局列序原位
        } else if (!mHiddenLogical.contains(logical)) {
            if (mi < mainVisible.size()) {
                merged.append(mainVisible.at(mi++));  // 主区拖动后的顺序填非冻结槽位
            } else {
                merged.append(logical);  // 防御
            }
        }
    }
    for (int logical : std::as_const(mOrderedLogical)) {
        if (!mFrozenLogical.contains(logical) && mHiddenLogical.contains(logical)) {
            merged.append(logical);  // 隐藏的非冻结列钉住旧相对顺序（追加末尾）
        }
    }
    // 补齐：漏掉的列按旧全局列序追加（防御：绝不让某列从列序里消失）
    for (int logical : std::as_const(mOrderedLogical)) {
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
 * 列宽拖拽、冻结列变化、行头宽变化、窗口缩放后各调一次。
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
 * 冻结侧容器必须锁到同一宽度：水平布局各项 stretch 全 0 时剩余空间在可伸展项间
 * 均分，侧容器保持 Preferred 会被拉宽到半个窗口——冻结窗格右侧留一大片空白
 * （现场「冻结一列仍有一段宽空白、拖分界空白不消」的根因）。
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
    for (int logical : std::as_const(mFrozenOrderLogical)) {
        w += fh->sectionSize(logical);
    }
    if (QHeaderView* vh = mFrozenView->verticalHeader()) {
        w += vh->width();
    }
    w += 2 * mFrozenView->frameWidth() + 2;
    mFrozenView->setFixedWidth(w);
    if (mFrozenSide) {
        mFrozenSide->setFixedWidth(w);
    }
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

/**
 * @brief 视图顺序最后一个可见冻结列
 *
 * 从冻结区显示顺序（mFrozenOrderLogical）末尾往前找第一个未被隐藏的列。
 * 末位冻结列可能被隐藏——分界条要对最后一个看得见的冻结列生效；全部冻结列
 * 隐藏时窗格已隐藏，本函数不会被触达。
 * @return 逻辑列号；无可见冻结列返回 -1
 */
int DATablePaneContainer::lastFrozenLogical() const
{
    if (!mFrozenView) {
        return -1;
    }
    for (int i = mFrozenOrderLogical.size() - 1; i >= 0; --i) {
        const int logical = mFrozenOrderLogical.at(i);
        if (!mHiddenLogical.contains(logical)) {
            return logical;
        }
    }
    return -1;
}

/**
 * @brief 分界条按下：进入拖动态
 *
 * 记录末冻结列起始宽（宽上限防呆；双击自适应在拖动前可能把列拉得很宽）。
 * @return 成功进入拖动态返回 true（无冻结列/读不到宽度返回 false）
 */
bool DATablePaneContainer::beginFrozenBoundaryDrag()
{
    const int logical = lastFrozenLogical();
    if (logical < 0 || !mFrozenView) {
        return false;
    }
    QHeaderView* fh = mFrozenView->horizontalHeader();
    if (!fh) {
        return false;
    }
    mBoundaryDragStartWidth = fh->sectionSize(logical);
    return mBoundaryDragStartWidth > 0;
}

/**
 * @brief 分界条位移：改末冻结列宽
 *
 * 实时 resizeSection（冻结窗格宽经既有 sectionResized → updateFrozenGeometry 链跟随），
 * 夹紧到表头最小段宽；此处不发落库信号——释放时 endFrozenBoundaryDrag 统一提交。
 * @param deltaX 相对按下位置的水平位移（像素，右拖变宽）
 */
void DATablePaneContainer::dragFrozenBoundary(int deltaX)
{
    if (mBoundaryDragStartWidth <= 0 || !mFrozenView) {
        return;
    }
    const int logical = lastFrozenLogical();
    if (logical < 0) {
        return;
    }
    QHeaderView* fh = mFrozenView->horizontalHeader();
    if (!fh) {
        return;
    }
    const int w = qBound(fh->minimumSectionSize(),
                         mBoundaryDragStartWidth + deltaX,
                         c_boundaryMaxColumnWidth);
    if (w != fh->sectionSize(logical)) {
        fh->resizeSection(logical, w);
    }
}

/**
 * @brief 分界条释放：提交末冻结列新宽
 *
 * 发 frozenBoundaryAutoFitRequested 的姊妹信号 frozenBoundaryResized 交页面写
 * store（分界条路径不经表头拖拽采集闸门——合成鼠标事件的 underMouse 判定在
 * 测试与自动化场景不可靠，列宽落库由本信号唯一承载）。
 */
void DATablePaneContainer::endFrozenBoundaryDrag()
{
    if (mBoundaryDragStartWidth <= 0 || !mFrozenView) {
        mBoundaryDragStartWidth = -1;
        return;
    }
    const int startWidth  = mBoundaryDragStartWidth;
    mBoundaryDragStartWidth = -1;
    const int logical = lastFrozenLogical();
    if (logical < 0) {
        return;
    }
    QHeaderView* fh = mFrozenView->horizontalHeader();
    if (!fh) {
        return;
    }
    const int w = fh->sectionSize(logical);
    if (w > 0 && w != startWidth) {
        Q_EMIT frozenBoundaryResized(logical, w);
    }
}

/**
 * @brief 双击分界条 → 请求末冻结列内容自适应
 */
void DATablePaneContainer::requestFrozenBoundaryAutoFit()
{
    const int logical = lastFrozenLogical();
    if (logical >= 0) {
        Q_EMIT frozenBoundaryAutoFitRequested(logical);
    }
}

}  // end of namespace DA
