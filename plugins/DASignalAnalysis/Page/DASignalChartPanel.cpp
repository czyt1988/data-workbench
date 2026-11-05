#include "DASignalChartPanel.h"
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include "DAChartWidget.h"

namespace DA
{
DASignalChartPanel::DASignalChartPanel(const QString& title, QWidget* parent) : QWidget(parent), mTitle(title)
{
    mLayout      = new QVBoxLayout(this);
    mLayout->setContentsMargins(2, 2, 2, 2);
    mLayout->setSpacing(2);

    mTitleLabel = new QLabel(this);
    QFont f     = mTitleLabel->font();
    f.setBold(true);
    mTitleLabel->setFont(f);
    mTitleLayout = new QHBoxLayout;
    mTitleLayout->addWidget(mTitleLabel);
    mTitleLayout->addStretch();
    mLayout->addLayout(mTitleLayout);

    mChart = new DAChartWidget(this);
    mLayout->addWidget(mChart, 1);
    retranslateUi();
}

DASignalChartPanel::~DASignalChartPanel()
{
}

DAChartWidget* DASignalChartPanel::chart() const
{
    return mChart;
}

void DASignalChartPanel::setTitle(const QString& title)
{
    mTitle = title;
    retranslateUi();
}

QString DASignalChartPanel::title() const
{
    return mTitle;
}

void DASignalChartPanel::addTitleBarWidget(QWidget* w)
{
    if (w) {
        mTitleLayout->addWidget(w);
    }
}

void DASignalChartPanel::setBottomWidget(QWidget* w)
{
    if (w) {
        mLayout->addWidget(w);
    }
}

void DASignalChartPanel::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

void DASignalChartPanel::retranslateUi()
{
    mTitleLabel->setText(mTitle);
}
}  // namespace DA
