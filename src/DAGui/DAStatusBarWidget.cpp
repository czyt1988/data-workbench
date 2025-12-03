#include "DAStatusBarWidget.h"
#include <QStyle>
#include <QApplication>
#include <QPainter>
#include <QEvent>
#include <QFrame>

namespace DA
{
class DAStatusBarWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAStatusBarWidget)
public:
    PrivateData(DAStatusBarWidget* p);
    void setupUI(DAStatusBarWidget* par);
    // 消息窗口组件
    QLabel* m_messageLabel;
    QTimer* m_messageTimer;
    QString m_currentMessage;

    // 进度窗口组件
    QProgressBar* m_progressBar;
    QLabel* m_progressTextLabel;  ///< 进度文本组件
    // 布局
    QHBoxLayout* m_layout;

    // 动画效果
    QPropertyAnimation* m_fadeAnimation;
};

DAStatusBarWidget::PrivateData::PrivateData(DAStatusBarWidget* p) : q_ptr(p)
{
}

void DAStatusBarWidget::PrivateData::setupUI(DAStatusBarWidget* par)
{
    // 创建主布局
    m_layout = new QHBoxLayout(par);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(10);

    // 创建消息标签
    m_messageLabel = new QLabel(par);
    m_messageLabel->setMinimumWidth(200);
    m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_messageLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_messageLabel->setLineWidth(1);
    m_messageLabel->setMidLineWidth(0);
    m_messageLabel->hide();

    // 创建消息定时器
    m_messageTimer = new QTimer(par);
    m_messageTimer->setSingleShot(true);
    par->connect(m_messageTimer, &QTimer::timeout, par, &DAStatusBarWidget::onMessageTimeout);

    // 创建进度条
    m_progressBar = new QProgressBar(par);
    m_progressBar->setMinimumWidth(150);
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setTextVisible(true);
    m_progressBar->setAlignment(Qt::AlignCenter);
    m_progressBar->setRange(0, 100);
    m_progressBar->hide();  // 默认隐藏

    // 创建进度文本标签（放在进度条右侧）
    m_progressTextLabel = new QLabel(par);
    m_progressTextLabel->setMinimumWidth(80);
    m_progressTextLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_progressTextLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_progressTextLabel->setLineWidth(1);
    m_progressTextLabel->setMidLineWidth(0);
    m_progressTextLabel->hide();  // 默认隐藏

    // 创建淡出动画
    m_fadeAnimation = new QPropertyAnimation(m_messageLabel, "windowOpacity", par);
    m_fadeAnimation->setDuration(500);
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.0);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, m_messageLabel, &QLabel::hide);

    // 将组件添加到布局
    m_layout->addWidget(m_messageLabel);
    m_layout->addWidget(m_progressBar, 0, Qt::AlignRight);
    m_layout->addWidget(m_progressTextLabel, 0, Qt::AlignRight);
    m_layout->addStretch();

    // 设置widget
    par->setLayout(m_layout);
}

//----------------------------------------------------
// DAStatusBarWidget
//----------------------------------------------------

DAStatusBarWidget::DAStatusBarWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->setupUI(this);
    updateWidgetStyle();
}

DAStatusBarWidget::~DAStatusBarWidget()
{
}

void DAStatusBarWidget::updateWidgetStyle()
{
    DA_D(d);
    // 使用QPalette和样式属性来设置外观，不使用样式表

    // 设置消息标签的调色板
    QPalette messagePalette = d->m_messageLabel->palette();
    messagePalette.setColor(QPalette::WindowText, Qt::black);
    messagePalette.setColor(QPalette::Window, QColor(240, 240, 240));
    d->m_messageLabel->setPalette(messagePalette);
    d->m_messageLabel->setAutoFillBackground(true);

    // 设置进度条的调色板
    QPalette progressPalette = d->m_progressBar->palette();
    progressPalette.setColor(QPalette::Highlight, QColor(76, 175, 80));  // 绿色
    progressPalette.setColor(QPalette::HighlightedText, Qt::black);
    progressPalette.setColor(QPalette::Base, QColor(240, 240, 240));
    progressPalette.setColor(QPalette::Text, Qt::black);
    d->m_progressBar->setPalette(progressPalette);

    // 设置整体背景
    QPalette widgetPalette = palette();
    widgetPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    setPalette(widgetPalette);
    setAutoFillBackground(true);
}

void DAStatusBarWidget::showMessage(const QString& message, int timeout)
{
    DA_D(d);
    // 停止之前的定时器
    if (d->m_messageTimer->isActive()) {
        d->m_messageTimer->stop();
    }

    // 停止淡出动画
    if (d->m_fadeAnimation->state() == QPropertyAnimation::Running) {
        d->m_fadeAnimation->stop();
    }

    // 保存消息
    d->m_currentMessage = message;

    // 显示消息
    d->m_messageLabel->setText(message);
    d->m_messageLabel->setWindowOpacity(1.0);  // 确保完全不透明
    d->m_messageLabel->show();

    // 启动定时器
    if (timeout > 0) {
        startMessageTimer(timeout);
    }
}

void DAStatusBarWidget::clearMessage()
{
    DA_D(d);
    d->m_currentMessage.clear();
    d->m_messageLabel->clear();
    d->m_messageLabel->hide();

    if (d->m_messageTimer->isActive()) {
        d->m_messageTimer->stop();
    }

    if (d->m_fadeAnimation->state() == QPropertyAnimation::Running) {
        d->m_fadeAnimation->stop();
    }
}

void DAStatusBarWidget::startMessageTimer(int timeout)
{
    d_ptr->m_messageTimer->start(timeout);
}

void DAStatusBarWidget::onMessageTimeout()
{
    // 使用淡出动画隐藏消息
    fadeOutMessage();
}

void DAStatusBarWidget::fadeOutMessage()
{
    if (d_ptr->m_messageLabel->isVisible()) {
        d_ptr->m_fadeAnimation->start();
    }
}

void DAStatusBarWidget::showProgressBar()
{
    DA_D(d);
    d->m_progressBar->show();

    // 如果有进度文本，则显示进度文本标签
    if (!(d->m_progressTextLabel->text().isEmpty())) {
        d->m_progressTextLabel->show();
    }
}

void DAStatusBarWidget::hideProgressBar()
{
    DA_D(d);
    d->m_progressBar->hide();
    d->m_progressTextLabel->hide();  // 同时隐藏进度文本标签
}

/**
 * @brief 默认范围0~100，如果设置超出范围值，进度会变为reset状态
 * @param value
 */
void DAStatusBarWidget::setProgress(int value)
{
    DA_D(d);
    if (!(d->m_progressBar->isVisible())) {
        showProgressBar();
    }
    // 设置确定进度
    d->m_progressBar->setValue(qBound(0, value, 100));
}

void DAStatusBarWidget::setBusy(bool busy)
{
    DA_D(d);
    if (busy) {
        if (!(d->m_progressBar->isVisible())) {
            showProgressBar();
        }
        // 设置为繁忙（不确定进度）模式
        d->m_progressBar->setRange(0, 0);  // 设置为0,0会显示繁忙动画（默认样式）
    } else {
        d->m_progressBar->setRange(0, 100);
        d->m_progressBar->setValue(0);
    }
}

void DAStatusBarWidget::resetProgress()
{
    DA_D(d);
    d->m_progressBar->setValue(0);
}

void DAStatusBarWidget::setProgressText(const QString& text)
{
    DA_D(d);
    if (!text.isEmpty()) {
        d->m_progressTextLabel->setText(text);
        if (d->m_progressBar->isVisible() && !d->m_progressTextLabel->isVisible()) {
            d->m_progressTextLabel->show();
        }
    } else {
        clearProgressText();
    }
}

void DAStatusBarWidget::clearProgressText()
{
    DA_D(d);
    d->m_progressTextLabel->clear();
    d->m_progressTextLabel->hide();
}

QString DAStatusBarWidget::progressText() const
{
    return d_ptr->m_progressTextLabel->text();
}

bool DAStatusBarWidget::isProgressBarVisible() const
{
    return d_ptr->m_progressBar->isVisible();
}

QString DAStatusBarWidget::getCurrentMessage() const
{
    return d_ptr->m_currentMessage;
}

void DAStatusBarWidget::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);

    // 当系统主题变化时，更新样式
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange
        || event->type() == QEvent::ThemeChange) {
        updateWidgetStyle();
    }
}

}
