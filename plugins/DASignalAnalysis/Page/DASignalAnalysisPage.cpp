#include "DASignalAnalysisPage.h"
// Qt
#include <QEvent>
#include <QLabel>
#include <QPushButton>
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
 */
void DASignalAnalysisPage::addSignal(const DASignalPtr& signal)
{
    if (!signal) {
        return;
    }
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
