#include "Common/DAStatusBarWidget.h"
#include <QStyle>
#include <QApplication>
#include <QPainter>
#include <QEvent>
#include <QFrame>
#include <QToolButton>
#include <QAction>
namespace DA
{
class DAStatusBarWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAStatusBarWidget)
public:
	PrivateData(DAStatusBarWidget* p);
	void setupUI(DAStatusBarWidget* par);
	// 按「调用方请求 + 进度条显示 + 动作可见」重算取消按钮显隐
	void updateProgressCancelVisibility();
	// 消息窗口组件
	QLabel* m_messageLabel;
	QTimer* m_messageTimer;
	QString m_currentMessage;
	QToolButton* m_switchButtonWorkflow { nullptr };
	QToolButton* m_switchButtonData { nullptr };
	QToolButton* m_switchButtonChart { nullptr };
	// 进度窗口组件
	QProgressBar* m_progressBar;
	QLabel* m_progressTextLabel;  ///< 进度文本组件
	QToolButton* m_progressCancelButton { nullptr };  ///< 进度条右侧取消/停止按钮（宿主创建，随进度条显隐）
	QAction* m_progressCancelAction { nullptr };      ///< 取消按钮绑定的外部动作（不拥有）
	bool m_progressCancelRequested { false };         ///< 调用方是否请求显示取消按钮（防别的调用方误触）
	// 布局
	QHBoxLayout* m_layout;
	// 额外永久部件（007 状态栏统计；本类不持有不删除）
	QWidget* m_extraPermanentWidget { nullptr };

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
	// 创建按钮组
	m_switchButtonWorkflow = new QToolButton(par);
	m_switchButtonWorkflow->setIcon(QIcon(":/DAGui/icon/workflow.svg"));
	m_switchButtonWorkflow->setAutoRaise(true);
	m_switchButtonWorkflow->setText(tr("Workflow"));  // cn:工作流
	m_switchButtonWorkflow->setMaximumWidth(50);
	m_switchButtonWorkflow->setToolTip(tr("Switch to Workflow Mode"));  // cn:切换为工作流模式

	m_switchButtonData = new QToolButton(par);
	m_switchButtonData->setIcon(QIcon(":/DAGui/icon/data.svg"));
	m_switchButtonData->setAutoRaise(true);
	m_switchButtonData->setText(tr("Data"));  // cn:数据
	m_switchButtonData->setMaximumWidth(50);
	m_switchButtonData->setToolTip(tr("Switch to Data Mode"));  // cn:切换为数据模式

	m_switchButtonChart = new QToolButton(par);
	m_switchButtonChart->setIcon(QIcon(":/DAGui/icon/chart.svg"));
	m_switchButtonChart->setAutoRaise(true);
	m_switchButtonChart->setText(tr("Chart"));  // cn:图表
	m_switchButtonChart->setMaximumWidth(50);
	m_switchButtonChart->setToolTip(tr("Switch to Chart Mode"));  // cn:切换为图表模式

	// 创建消息标签

	// 创建消息标签
	m_messageLabel = new QLabel(par);
	m_messageLabel->setMinimumWidth(200);
	m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_messageLabel->setFrameStyle(QFrame::NoFrame);
	// m_messageLabel->setLineWidth(1);
	// m_messageLabel->setMidLineWidth(0);
	m_messageLabel->hide();

	// 创建消息定时器
	m_messageTimer = new QTimer(par);
	m_messageTimer->setSingleShot(true);
	par->connect(m_messageTimer, &QTimer::timeout, par, &DAStatusBarWidget::onMessageTimeout);

	// 创建进度条
	m_progressBar = new QProgressBar(par);
	m_progressBar->setMinimumWidth(150);
	m_progressBar->setMaximumWidth(200);
	// 固定进度条高度：状态栏高度由既有常驻按钮决定，进度条显隐不得把状态栏撑高
	m_progressBar->setFixedHeight(16);
	m_progressBar->setTextVisible(true);
	m_progressBar->setAlignment(Qt::AlignCenter);
	m_progressBar->setRange(0, 100);
	m_progressBar->hide();  // 默认隐藏

	// 创建进度文本标签（放在进度条右侧）
	m_progressTextLabel = new QLabel(par);
	m_progressTextLabel->setMinimumWidth(80);
	m_progressTextLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_progressTextLabel->setFrameStyle(QFrame::NoFrame);
	// m_progressTextLabel->setLineWidth(1);
	// m_progressTextLabel->setMidLineWidth(0);
	m_progressTextLabel->hide();  // 默认隐藏

	// 创建淡出动画
	m_fadeAnimation = new QPropertyAnimation(m_messageLabel, "windowOpacity", par);
	m_fadeAnimation->setDuration(500);
	m_fadeAnimation->setStartValue(1.0);
	m_fadeAnimation->setEndValue(0.0);
	connect(m_fadeAnimation, &QPropertyAnimation::finished, m_messageLabel, &QLabel::hide);

	// 将组件添加到布局
	m_layout->addWidget(m_switchButtonWorkflow);
	m_layout->addWidget(m_switchButtonData);
	m_layout->addWidget(m_switchButtonChart);
	m_layout->addWidget(m_messageLabel);
	m_layout->addWidget(m_progressBar, 0, Qt::AlignRight);
	m_layout->addWidget(m_progressTextLabel, 0, Qt::AlignRight);
	m_layout->addStretch();

	// 设置widget
	par->setLayout(m_layout);
	par->setMaximumHeight(20);
}

/**
 * @brief 取消按钮显隐 = 调用方请求显示 + 进度条显示 + 动作本身可见
 *
 * 用 isHidden() 而不是 isVisible()：进度条/按钮是状态栏的子控件，
 * 主窗口隐藏时 isVisible() 会全为 false，但显隐状态不应因此丢失。
 */
void DAStatusBarWidget::PrivateData::updateProgressCancelVisibility()
{
	if (!m_progressCancelButton) {
		return;
	}
	const bool barShown      = !m_progressBar->isHidden();
	const bool actionVisible = m_progressCancelAction ? m_progressCancelAction->isVisible() : false;
	m_progressCancelButton->setVisible(m_progressCancelRequested && barShown && actionVisible);
}

//----------------------------------------------------
// DAStatusBarWidget
//----------------------------------------------------

DAStatusBarWidget::DAStatusBarWidget(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
	d_ptr->setupUI(this);
	// 设置尺寸策略，横向为扩展
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	updateWidgetStyle();
	initConnect();
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

void DA::DAStatusBarWidget::initConnect()
{
	connect(d_ptr->m_switchButtonWorkflow, &QToolButton::clicked, [ this ]() {
		Q_EMIT requestSwitch(DA::DAWorkbenchFeatureType::Workflow);
	});
	connect(d_ptr->m_switchButtonData, &QToolButton::clicked, [ this ]() {
		Q_EMIT requestSwitch(DA::DAWorkbenchFeatureType::Data);
	});
	connect(d_ptr->m_switchButtonChart, &QToolButton::clicked, [ this ]() {
		Q_EMIT requestSwitch(DA::DAWorkbenchFeatureType::Chart);
	});
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
	// 取消按钮只在调用方请求显示时出现；别的调用方显示进度条不会带出本按钮
	d->updateProgressCancelVisibility();

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
	d->updateProgressCancelVisibility();
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

bool DAStatusBarWidget::isBusy() const
{
	DA_DC(d);
	return (d->m_progressBar->minimum() == 0 && d->m_progressBar->minimum() == d->m_progressBar->maximum());
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

/**
 * @brief 设置进度条右侧的取消/停止按钮（按钮随进度条显示/隐藏）
 *
 * 只接受调用方的 QAction（本类不拥有动作）：按钮外观/文本/可用性全部跟随动作，
 * 插件卸载时传 nullptr 解绑即可。按钮固定 18×18、图标 14×14，避免改变状态栏高度。
 * @param action 绑定的动作，nullptr 表示解绑并销毁按钮
 */
void DAStatusBarWidget::setProgressCancelAction(QAction* action)
{
	DA_D(d);
	if (d->m_progressCancelAction == action) {
		return;
	}
	// 先销毁旧按钮（先断开默认动作再删除，避免它继续引用下面的新动作/空动作）
	if (d->m_progressCancelButton) {
		QToolButton* oldButton = d->m_progressCancelButton;
		d->m_progressCancelButton = nullptr;
		d->m_layout->removeWidget(oldButton);
		oldButton->setDefaultAction(nullptr);
		oldButton->deleteLater();
	}
	d->m_progressCancelAction = action;
	if (!action) {
		d->m_progressCancelRequested = false;
		return;
	}
	QToolButton* button = new QToolButton(this);
	button->setObjectName(QStringLiteral("da-statusbar-progress-cancel"));
	button->setAutoRaise(true);
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setIconSize(QSize(14, 14));
	// 固定尺寸 + 固定垂直策略：按钮显隐不参与状态栏高度计算
	button->setFixedSize(18, 18);
	button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	button->setDefaultAction(action);
	button->setVisible(false);
	// 插到进度文本之后、末尾 stretch 之前 = 紧贴进度条右侧
	const int progressTextIndex = d->m_layout->indexOf(d->m_progressTextLabel);
	d->m_layout->insertWidget(progressTextIndex + 1, button);
	d->m_progressCancelButton = button;
	// 动作可能先于状态栏销毁（插件卸载/动作管理器清理）：动作销毁时同步摘除按钮
	connect(action, &QObject::destroyed, this, [ this, button ]() {
		DA_D(d);
		if (d->m_progressCancelButton == button) {
			d->m_progressCancelButton    = nullptr;
			d->m_progressCancelAction    = nullptr;
			d->m_progressCancelRequested = false;
		}
		d->m_layout->removeWidget(button);
		button->setDefaultAction(nullptr);
		button->deleteLater();
	});
	// 进度条已在显示（例如宿主先开进度条、后绑定动作）时按请求状态补上按钮
	d->updateProgressCancelVisibility();
}

/**
 * @brief 显示/隐藏进度条旁的取消按钮
 *
 * 只有调用方自己显示进度时才置 true；别的插件调用 showProgressBar() 时不会带出本按钮。
 * @param visible 是否请求显示取消按钮
 */
void DAStatusBarWidget::setProgressCancelVisible(bool visible)
{
	DA_D(d);
	d->m_progressCancelRequested = visible;
	d->updateProgressCancelVisibility();
}

/**
 * @brief 当前绑定的进度条取消动作（未绑定返回 nullptr）
 */
QAction* DAStatusBarWidget::progressCancelAction() const
{
	return d_ptr->m_progressCancelAction;
}

QString DAStatusBarWidget::getCurrentMessage() const
{
	return d_ptr->m_currentMessage;
}

void DA::DAStatusBarWidget::setSwitchButtonVisible(DA::DAWorkbenchFeatureType type, bool visible)
{
	switch (type) {
	case DA::DAWorkbenchFeatureType::Chart:
		d_ptr->m_switchButtonChart->setVisible(visible);
		break;
	case DA::DAWorkbenchFeatureType::Data:
		d_ptr->m_switchButtonData->setVisible(visible);
		break;
	case DA::DAWorkbenchFeatureType::Workflow:
		d_ptr->m_switchButtonWorkflow->setVisible(visible);
		break;
	case DA::DAWorkbenchFeatureType::All:
		d_ptr->m_switchButtonChart->setVisible(visible);
		d_ptr->m_switchButtonData->setVisible(visible);
		d_ptr->m_switchButtonWorkflow->setVisible(visible);
		break;
	default:
		break;
	}
}

/**
 * @brief 设置额外永久部件（007：状态栏统计部件挂载点）
 *
 * 部件由调用方持有，本类不 delete；加在 stretch 之后 = 贴状态栏最右。
 * @param w 部件（nullptr 只摘除）
 */
void DAStatusBarWidget::setExtraPermanentWidget(QWidget* w)
{
	if (d_ptr->m_extraPermanentWidget == w) {
		return;
	}
	if (d_ptr->m_extraPermanentWidget) {
		d_ptr->m_layout->removeWidget(d_ptr->m_extraPermanentWidget);
		d_ptr->m_extraPermanentWidget->setParent(nullptr);
	}
	d_ptr->m_extraPermanentWidget = w;
	if (w) {
		d_ptr->m_layout->addWidget(w);
	}
}

/**
 * @brief 当前挂载的额外永久部件
 * @return 部件指针（未挂载为 nullptr）
 */
QWidget* DAStatusBarWidget::extraPermanentWidget() const
{
	return d_ptr->m_extraPermanentWidget;
}

bool DA::DAStatusBarWidget::isSwitchButtonVisible(DA::DAWorkbenchFeatureType type) const
{
	switch (type) {
	case DA::DAWorkbenchFeatureType::Chart:
		return d_ptr->m_switchButtonChart->isVisible();
	case DA::DAWorkbenchFeatureType::Data:
		return d_ptr->m_switchButtonData->isVisible();
	case DA::DAWorkbenchFeatureType::Workflow:
		return d_ptr->m_switchButtonWorkflow->isVisible();
	case DA::DAWorkbenchFeatureType::All:
		return d_ptr->m_switchButtonChart->isVisible() && d_ptr->m_switchButtonData->isVisible()
			   && d_ptr->m_switchButtonWorkflow->isVisible();
	default:
		return false;
	}
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
