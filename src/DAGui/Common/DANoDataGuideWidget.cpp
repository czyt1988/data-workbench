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
 * @brief 构建界面：标题 + 说明 + 两个入口按钮
 *
 * 版式与信号分析页空状态一致（标题放大加粗、说明用中性色、两个入口按钮居中）。
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
    for (QPushButton* btn : { mOpenProjectButton, mImportDataButton }) {
        btn->setMinimumHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
    }
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(12);
    buttonLayout->addStretch();
    buttonLayout->addWidget(mOpenProjectButton);
    buttonLayout->addWidget(mImportDataButton);
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
 */
void DANoDataGuideWidget::retranslateUi()
{
    mTitleLabel->setText(tr("No data loaded"));  // cn:尚未载入数据
    if (ChartOperateRole == mRole) {
        mHintLabel->setText(tr("Open an existing project or import data, then create a chart to start plotting."));
        // cn:打开已有工程或导入数据，然后新建绘图开始绘图。
    } else {
        mHintLabel->setText(tr("Open an existing project or import data to start working with data."));
        // cn:打开已有工程或导入数据，开始处理数据。
    }
    mOpenProjectButton->setText(tr("Open Existing Project"));  // cn:打开已有工程
    mImportDataButton->setText(tr("Import Data"));             // cn:导入数据
}
}  // namespace DA
