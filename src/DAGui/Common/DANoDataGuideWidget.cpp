#include "Common/DANoDataGuideWidget.h"
// Qt
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace DA
{
DANoDataGuideWidget::DANoDataGuideWidget(Role role, QWidget* par) : QWidget(par), mRole(role)
{
    buildUI();
    retranslateUi();
}

DANoDataGuideWidget::~DANoDataGuideWidget() = default;

/**
 * @brief 所属操作窗口角色
 * @return 构造时传入的角色
 */
DANoDataGuideWidget::Role DANoDataGuideWidget::role() const
{
    return mRole;
}

/**
 * @brief 数据管理器里是否已有数据
 * @return 已有数据返回 true
 */
bool DANoDataGuideWidget::isDataAvailable() const
{
    return mDataAvailable;
}

/**
 * @brief 设置数据管理器是否已有数据
 *
 * 有数据和没数据时引导内容不同（入口按钮/提示文案），数据增删后由宿主调用。
 * @param available true=数据管理器里已有数据
 */
void DANoDataGuideWidget::setDataAvailable(bool available)
{
    if (mDataAvailable == available) {
        return;
    }
    mDataAvailable = available;
    updateContent();
}

/**
 * @brief 构建界面：标题 + 说明 + 入口按钮
 *
 * 版式与信号分析页空状态一致（标题放大加粗、说明用中性色、入口按钮居中）；
 * 具体显示哪些按钮由 updateContent() 按数据状态决定。
 */
void DANoDataGuideWidget::buildUI()
{
    mTitleLabel = new QLabel(this);
    mHintLabel  = new QLabel(this);
    mTitleLabel->setAlignment(Qt::AlignCenter);
    mHintLabel->setAlignment(Qt::AlignCenter);
    mHintLabel->setWordWrap(true);
    // 提示文字用中性色（DESIGN.md §3.1：次要文字 #727272）
    mHintLabel->setStyleSheet(QStringLiteral("color:#727272;"));
    QFont titleFont = mTitleLabel->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.35);
    titleFont.setBold(true);
    mTitleLabel->setFont(titleFont);

    mOpenProjectButton = new QPushButton(this);
    mImportDataButton  = new QPushButton(this);
    mAddFigureButton   = new QPushButton(this);
    for (QPushButton* btn : { mOpenProjectButton, mImportDataButton, mAddFigureButton }) {
        btn->setMinimumHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
    }
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(12);
    buttonLayout->addStretch();
    buttonLayout->addWidget(mOpenProjectButton);
    buttonLayout->addWidget(mImportDataButton);
    buttonLayout->addWidget(mAddFigureButton);
    buttonLayout->addStretch();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->addStretch(3);
    layout->addWidget(mTitleLabel);
    layout->addSpacing(8);
    layout->addWidget(mHintLabel);
    layout->addSpacing(20);
    layout->addLayout(buttonLayout);
    layout->addStretch(4);

    connect(mOpenProjectButton, &QPushButton::clicked, this, &DANoDataGuideWidget::openProjectRequested);
    connect(mImportDataButton, &QPushButton::clicked, this, &DANoDataGuideWidget::importDataRequested);
    connect(mAddFigureButton, &QPushButton::clicked, this, &DANoDataGuideWidget::addFigureRequested);
    updateContent();
}

/**
 * @brief 按「角色 + 数据是否可用」刷新标题、说明与按钮显隐
 *
 * - 没有数据：两个操作窗口都显示「打开已有工程」「导入数据」；
 * - 已有数据（数据操作窗口）：提示去数据管理区双击打开数据表；
 * - 已有数据（绘图操作窗口）：显示「添加绘图」。
 */
void DANoDataGuideWidget::updateContent()
{
    if (mDataAvailable) {
        if (ChartOperateRole == mRole) {
            mTitleLabel->setText(tr("No figure created"));  // cn:尚未创建绘图
            mHintLabel->setText(tr("Data is ready. Click Add Figure to start plotting."));
            // cn:数据已就绪，点击「添加绘图」新建绘图。
            mOpenProjectButton->setVisible(false);
            mImportDataButton->setVisible(false);
            mAddFigureButton->setVisible(true);
        } else {
            mTitleLabel->setText(tr("No data table open"));  // cn:尚未打开数据表
            mHintLabel->setText(tr("Double-click data in the Data Manager to open its table."));
            // cn:双击数据管理区的数据来打开数据表。
            mOpenProjectButton->setVisible(false);
            mImportDataButton->setVisible(false);
            mAddFigureButton->setVisible(false);
        }
        return;
    }
    mTitleLabel->setText(tr("No data loaded"));  // cn:尚未载入数据
    if (ChartOperateRole == mRole) {
        mHintLabel->setText(tr("Open an existing project or import data, then create a chart to start plotting."));
        // cn:打开已有工程或导入数据，然后新建绘图开始绘图。
    } else {
        mHintLabel->setText(tr("Open an existing project or import data to start working with data."));
        // cn:打开已有工程或导入数据，开始处理数据。
    }
    mOpenProjectButton->setVisible(true);
    mImportDataButton->setVisible(true);
    mAddFigureButton->setVisible(false);
}

/**
 * @brief 语言变更事件：页面自身处理，避免依赖宿主重译链路
 * @param e 事件
 */
void DANoDataGuideWidget::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

/**
 * @brief 重译界面文本
 *
 * 按钮文案先统一刷新，标题/说明与按钮显隐再由 updateContent() 按当前状态补齐。
 */
void DANoDataGuideWidget::retranslateUi()
{
    mOpenProjectButton->setText(tr("Open Existing Project"));  // cn:打开已有工程
    mImportDataButton->setText(tr("Import Data"));             // cn:导入数据
    mAddFigureButton->setText(tr("Add Figure"));               // cn:添加绘图
    updateContent();
}
}  // namespace DA
