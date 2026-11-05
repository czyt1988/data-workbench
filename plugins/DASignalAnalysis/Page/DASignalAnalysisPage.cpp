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
