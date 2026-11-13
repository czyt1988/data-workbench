#include "DASignalAnalysisPage.h"
// Qt
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QTabBar>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QDebug>
#include "DASignalAnalysisView.h"

namespace DA
{
/**
 * @brief 构造函数
 * @param par 父窗口
 */
DASignalAnalysisPage::DASignalAnalysisPage(QWidget* par) : QWidget(par)
{
    buildUI();
    retranslateUi();
}

/**
 * @brief 析构函数
 */
DASignalAnalysisPage::~DASignalAnalysisPage()
{
}

/**
 * @brief 获取信号标签栏
 */
QTabBar* DASignalAnalysisPage::signalTabBar() const
{
    return mTabBar;
}

/**
 * @brief 获取信号视图堆叠容器
 */
QStackedWidget* DASignalAnalysisPage::signalStack() const
{
    return mSignalStack;
}

/**
 * @brief 添加一个信号：新增标签页与完整信号视图
 *
 * 视图必须在这里真正创建并登记到 mViews：外部（DASignalAnalysisUI）通过
 * viewForSignal() 取视图来连接包络/频谱/热图请求，缺了它整个页面只剩空白。
 * 同名信号在这里给显示名追加 " (2)"/" (3)" 序号（BLK-02）：重复导入同一文件/同一列
 * 不再出现两个同名标签页，存档 signal-name 也随之唯一。
 */
void DASignalAnalysisPage::addSignal(const DASignalPtr& signal)
{
    if (!signal) {
        return;
    }
    // 显示名去重（纯数据格式化，不用 tr）：重名时改写信号名，标签页/存档/日志一致
    signal->setName(uniqueDisplayName(signal->name(), nullptr));
    mSignals.append(signal);
    const int idx = mTabBar->addTab(signal->name());
    mTabBar->setTabToolTip(idx, signal->source() ? signal->source()->sourceDescription() : QString());
    // 每个信号一个完整视图（波形 + 频谱 + 时频），按信号指针登记供结果回填与视图查找
    DASignalAnalysisView* view = new DASignalAnalysisView(signal, mSignalStack);
    mSignalStack->addWidget(view);
    mViews.insert(signal.get(), view);
    mTabBar->setCurrentIndex(idx);
    mSignalStack->setCurrentIndex(idx);
    updateEmptyState();
    Q_EMIT currentSignalChanged();
}

/**
 * @brief 清空全部信号：逐个关闭标签页并通知外部清理缓存（工程加载前调用）
 */
void DASignalAnalysisPage::clearAll()
{
    while (!mSignals.isEmpty()) {
        removeSignalAt(mSignals.size() - 1);
    }
}

/**
 * @brief 关闭指定索引的信号：移除标签页、销毁视图并通知外部清理缓存
 *
 * 标签栏与视图堆叠按索引一一对应。移除期间屏蔽标签栏信号，避免 removeTab
 * 自动切换当前页的中间态让两个容器索引错位；移除后手动同步堆叠页当前索引。
 */
void DASignalAnalysisPage::removeSignalAt(int index)
{
    if (index < 0 || index >= mSignals.size()) {
        return;
    }
    const DASignalPtr signal   = mSignals.takeAt(index);
    DASignalAnalysisView* view = mViews.take(signal.get());
    const bool oldBlocked      = mTabBar->blockSignals(true);
    mTabBar->removeTab(index);
    mTabBar->blockSignals(oldBlocked);
    if (view) {
        mSignalStack->removeWidget(view);
        // 关闭动作来自标签栏信号链，延后销毁，避免在信号发射过程中析构视图
        view->deleteLater();
    }
    if (mSignalStack->count() > 0) {
        mSignalStack->setCurrentIndex(mTabBar->currentIndex());
    }
    updateEmptyState();
    Q_EMIT signalRemoved(signal);
    Q_EMIT currentSignalChanged();
}

/**
 * @brief 当前信号（无信号返回空指针）
 */
DASignalPtr DASignalAnalysisPage::currentSignal() const
{
    const int idx = mTabBar->currentIndex();
    if (idx < 0 || idx >= mSignals.size()) {
        return DASignalPtr();
    }
    return mSignals.at(idx);
}

/**
 * @brief 全部信号
 */
QList< DASignalPtr > DASignalAnalysisPage::allSignals() const
{
    return mSignals;
}

/**
 * @brief 信号数量
 */
int DASignalAnalysisPage::signalCount() const
{
    return mSignals.size();
}

/**
 * @brief 某信号对应的视图（无对应视图返回 nullptr）
 */
DASignalAnalysisView* DASignalAnalysisPage::viewForSignal(const DASignal* sig) const
{
    return mViews.value(sig, nullptr);
}

/**
 * @brief 某信号在标签栏中的索引（未找到返回 -1）
 */
int DASignalAnalysisPage::indexOfSignal(const DASignal* sig) const
{
    for (int i = 0; i < mSignals.size(); ++i) {
        if (mSignals.at(i).get() == sig) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 用新信号替换指定索引的信号（重新指定来源用，评审 U16）
 *
 * 旧视图延后销毁（可能在信号发射链上），新视图按同一索引插入，保持标签顺序不变；
 * 旧信号通过 signalRemoved 通知外部清理其参数/频谱/峰值缓存。
 */
bool DASignalAnalysisPage::replaceSignalAt(int index, const DASignalPtr& signal)
{
    if (!signal || index < 0 || index >= mSignals.size()) {
        return false;
    }
    const DASignalPtr oldSignal   = mSignals.at(index);
    DASignalAnalysisView* oldView = mViews.take(oldSignal.get());
    if (oldView) {
        mSignalStack->removeWidget(oldView);
        oldView->deleteLater();
    }
    // 替换进来的信号也做显示名去重（排除被替换的旧信号本身）
    signal->setName(uniqueDisplayName(signal->name(), oldSignal.get()));
    mSignals[ index ] = signal;
    DASignalAnalysisView* view = new DASignalAnalysisView(signal, mSignalStack);
    mSignalStack->insertWidget(index, view);
    mViews.insert(signal.get(), view);
    const bool oldBlocked = mTabBar->blockSignals(true);
    mTabBar->setTabText(index, signal->name());
    mTabBar->setTabToolTip(index, signal->source() ? signal->source()->sourceDescription() : QString());
    mTabBar->setCurrentIndex(index);
    mTabBar->blockSignals(oldBlocked);
    mSignalStack->setCurrentWidget(view);
    updateEmptyState();
    Q_EMIT signalRemoved(oldSignal);  // 外部清理旧信号的缓存
    Q_EMIT currentSignalChanged();
    return true;
}

/**
 * @brief 同名信号显示名去重（BLK-02）
 *
 * 与现有信号重名时追加 " (2)"/" (3)" 序号（纯数据格式化，不翻译）：信号名来自文件名或
 * 「数据帧名.列名」，重复导入同一文件/同一列即重名；去重后标签页、figures.xml 的
 * signal-name 与日志提示保持唯一。@p except 用于替换场景（旧信号即将移走，不参与比对）。
 * @param name 待去重的显示名
 * @param except 比对时排除的信号（替换指定索引时传旧信号，其余传 nullptr）
 * @return 不与现有信号冲突的显示名
 */
QString DASignalAnalysisPage::uniqueDisplayName(const QString& name, const DASignal* except) const
{
    QSet< QString > usedNames;
    for (const DASignalPtr& sig : mSignals) {
        if (sig && sig.get() != except) {
            usedNames.insert(sig->name());
        }
    }
    if (!usedNames.contains(name)) {
        return name;
    }
    int suffix = 2;
    while (usedNames.contains(QStringLiteral("%1 (%2)").arg(name).arg(suffix))) {
        ++suffix;
    }
    return QStringLiteral("%1 (%2)").arg(name).arg(suffix);
}

/**
 * @brief 构建界面：根堆叠 = 空状态页 + 信号视图页（标签栏 + 视图堆叠）
 */
void DASignalAnalysisPage::buildUI()
{
    mRootStack = new QStackedWidget(this);

    // ---------- 空状态页 ----------
    mEmptyPage    = new QWidget(mRootStack);
    mEmptyTitle   = new QLabel(mEmptyPage);
    mEmptyHint    = new QLabel(mEmptyPage);
    mEmptyTitle->setAlignment(Qt::AlignCenter);
    mEmptyHint->setAlignment(Qt::AlignCenter);
    QFont titleFont = mEmptyTitle->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.35);
    titleFont.setBold(true);
    mEmptyTitle->setFont(titleFont);
    mEmptyHint->setWordWrap(true);
    // 提示文字用中性色（DESIGN.md §3.1：正文/次要文字 #727272）
    mEmptyHint->setStyleSheet(QStringLiteral("color:#727272;"));

    // 导入按钮组：空状态下唯一可操作的入口（D11：页面始终存在）
    mBtnImportAudio     = new QPushButton(mEmptyPage);
    mBtnImportDataFrame = new QPushButton(mEmptyPage);
    mBtnImportVideo     = new QPushButton(mEmptyPage);
    for (QPushButton* btn : { mBtnImportAudio, mBtnImportDataFrame, mBtnImportVideo }) {
        btn->setMinimumHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
    }
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(12);
    btnLayout->addStretch();
    btnLayout->addWidget(mBtnImportAudio);
    btnLayout->addWidget(mBtnImportDataFrame);
    btnLayout->addWidget(mBtnImportVideo);
    btnLayout->addStretch();

    QVBoxLayout* emptyLayout = new QVBoxLayout(mEmptyPage);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->addStretch(3);
    emptyLayout->addWidget(mEmptyTitle);
    emptyLayout->addSpacing(8);
    emptyLayout->addWidget(mEmptyHint);
    emptyLayout->addSpacing(20);
    emptyLayout->addLayout(btnLayout);
    emptyLayout->addStretch(4);

    connect(mBtnImportAudio, &QPushButton::clicked, this, &DASignalAnalysisPage::requestImportAudio);
    connect(mBtnImportDataFrame, &QPushButton::clicked, this, &DASignalAnalysisPage::requestImportDataFrame);
    connect(mBtnImportVideo, &QPushButton::clicked, this, &DASignalAnalysisPage::requestImportVideo);

    // ---------- 信号视图页 ----------
    mContentPage = new QWidget(mRootStack);
    mTabBar      = new QTabBar(mContentPage);
    mTabBar->setExpanding(false);
    mTabBar->setDrawBase(false);
    mTabBar->setMovable(false);
    mTabBar->setTabsClosable(true);
    mSignalStack = new QStackedWidget(mContentPage);
    QVBoxLayout* contentLayout = new QVBoxLayout(mContentPage);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(mTabBar);
    contentLayout->addWidget(mSignalStack, 1);
    connect(mTabBar, &QTabBar::currentChanged, mSignalStack, &QStackedWidget::setCurrentIndex);
    connect(mTabBar, &QTabBar::currentChanged, this, &DASignalAnalysisPage::currentSignalChanged);
    // 标签上的关闭按钮：移除信号与视图（页面不自行删除信号对象，由外部缓存失效后释放）
    connect(mTabBar, &QTabBar::tabCloseRequested, this, &DASignalAnalysisPage::removeSignalAt);

    mRootStack->addWidget(mEmptyPage);
    mRootStack->addWidget(mContentPage);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(mRootStack);

    updateEmptyState();
}

/**
 * @brief 按信号数量切换空状态页 / 信号视图页
 */
void DASignalAnalysisPage::updateEmptyState()
{
    const bool hasSignal = (mSignalStack->count() > 0);
    mRootStack->setCurrentWidget(hasSignal ? mContentPage : mEmptyPage);
}

/**
 * @brief 语言变更事件：页面自身处理，避免依赖宿主插件重译链路
 * @param e 事件
 */
void DASignalAnalysisPage::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

/**
 * @brief 重译界面文本
 */
void DASignalAnalysisPage::retranslateUi()
{
    mEmptyTitle->setText(tr("No signal loaded"));  // cn:尚未载入信号
    mEmptyHint->setText(tr("Import an audio file, a video, or columns from an existing data frame to begin analysis."));
    // cn:导入音频文件、视频，或从已有数据帧指定列，开始分析。
    mBtnImportAudio->setText(tr("Import Audio..."));            // cn:导入音频
    mBtnImportDataFrame->setText(tr("Import Data Frame..."));   // cn:导入数据帧
    mBtnImportVideo->setText(tr("Import Video..."));            // cn:导入视频
}
}  // namespace DA
