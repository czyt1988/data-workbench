// DABadgeAutoHideTab.cpp
#include "DABadgeAutoHideTab.h"
// Qt
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QSet>
// ADS
#include "DockWidget.h"
// DA
#include "DAMessageLogQueue.h"

namespace DA
{
//===================================================
// DABadgeAutoHideTab
//===================================================

DABadgeAutoHideTab::DABadgeAutoHideTab(QWidget* parent) : Super(parent)
{
}

DABadgeAutoHideTab::~DABadgeAutoHideTab()
{
}

/**
 * @brief 徽章启用名单（静态，按 dock objectName）
 * @return
 * @note 仅主线程（APP 构建 docking 区阶段）读写，无需加锁
 */
static QSet< QString >& badgeEnabledDockNames()
{
    static QSet< QString > s_names;
    return s_names;
}

/**
 * @brief 启用/关闭指定 dock objectName 的徽章能力
 * @param dockObjectName dock 的 objectName
 * @param on 是否启用
 * @note 必须在 dock 创建前注册（tab 在 dock 创建时由工厂生成）
 */
void DABadgeAutoHideTab::setBadgeEnabled(const QString& dockObjectName, bool on)
{
    if (on) {
        badgeEnabledDockNames().insert(dockObjectName);
    } else {
        badgeEnabledDockNames().remove(dockObjectName);
    }
}

/**
 * @brief 查询指定 dock objectName 的徽章能力是否启用
 * @param dockObjectName dock 的 objectName
 * @return
 */
bool DABadgeAutoHideTab::isBadgeEnabled(const QString& dockObjectName)
{
    return badgeEnabledDockNames().contains(dockObjectName);
}

/**
 * @brief 当前徽章未读数
 * @return
 */
int DABadgeAutoHideTab::badgeCount() const
{
    return mBadgeCount;
}

/**
 * @brief 设置徽章未读数
 * @param count 未读数（负数钳 0）
 */
void DABadgeAutoHideTab::setBadgeCount(int count)
{
    count = qMax(0, count);
    if (mBadgeCount == count) {
        return;
    }
    mBadgeCount = count;
    Q_EMIT badgeCountChanged(mBadgeCount);
    update();
}

/**
 * @brief 警告 + 错误（含致命）的累计推送总数
 *
 * 徽章只对警告/错误级消息计数（普通 info 不打扰用户），
 * 未读数 = 该累计数 - 已读水位。
 * @return
 */
static qint64 warnOrErrorPushedTotal()
{
    const DAMessageLogQueue& queue = DAMessageLogQueue::instance();
    return queue.totalWarningPushed() + queue.totalCriticalPushed();
}

/**
 * @brief 首帧绘制时尝试激活徽章
 *
 * ADS 工厂创建侧边栏 tab 时拿不到 dock 指针（传 nullptr），因此延后到
 * tab 首帧绘制（此时 dockWidget() 已绑定）按名单判定；
 * dock 尚未绑定时保持未尝试状态，等待下一帧。
 */
void DABadgeAutoHideTab::tryActivateBadge()
{
    ads::CDockWidget* dw = dockWidget();
    if (nullptr == dw) {
        return;  // 尚未绑定 dock，等待下一帧
    }
    mBadgeTried = true;
    if (!isBadgeEnabled(dw->objectName())) {
        return;  // 未命中名单：不激活，绘制行为与基类完全一致
    }
    mBadgeActive   = true;
    mReadWatermark = warnOrErrorPushedTotal();
    // 有新消息（1s 聚合信号）→ 刷新未读数
    connect(&DAMessageLogQueue::instance(), &DAMessageLogQueue::messageQueueAppended, this,
            &DABadgeAutoHideTab::onMessageQueueAppended);
    // auto-hide 面板展开（dock 变为可见）→ 用户看到日志，清零已读
    connect(dw, &ads::CDockWidget::visibilityChanged, this, [ this ](bool visible) {
        if (visible) {
            markAsRead();
        }
    });
}

/**
 * @brief 标记当前累计数为已读（清零徽章）
 */
void DABadgeAutoHideTab::markAsRead()
{
    mReadWatermark = warnOrErrorPushedTotal();
    setBadgeCount(0);
}

/**
 * @brief 消息队列有新消息到达，刷新未读数
 *
 * 未读数 = 警告/错误累计数 - 已读水位（info/debug 不计入）；
 * 面板展开期间到达的消息即时视为已读。
 */
void DABadgeAutoHideTab::onMessageQueueAppended()
{
    if (nullptr != dockWidget() && dockWidget()->isVisible()) {
        markAsRead();  // 面板展开：用户正在看日志，即时已读
        return;
    }
    const qint64 total = warnOrErrorPushedTotal();
    if (total > mReadWatermark) {
        setBadgeCount(static_cast< int >(total - mReadWatermark));
    }
}

/**
 * @brief 绘制：先基类（背景/图标/旋转文字），再按需叠加徽章
 */
void DABadgeAutoHideTab::paintEvent(QPaintEvent* event)
{
    Super::paintEvent(event);
    if (!mBadgeTried) {
        tryActivateBadge();
    }
    if (!mBadgeActive || mBadgeCount <= 0) {
        return;
    }
    QPainter painter(this);
    drawBadge(painter);
}

/**
 * @brief 在 tab 右上角绘制"胶囊底 + 数字"徽章
 *
 * 配色遵循 DESIGN.md 语义色板：橙红 #CE6043（需注意/未读）+ 白字；
 * 数字超过 99（badgeCountLimit）显示 99+；位置固定在矩形右上角，
 * 垂直/水平方向 tab 均适用。
 */
void DABadgeAutoHideTab::drawBadge(QPainter& painter)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    // 胶囊高度基于缩小的粗体字号（tab 字号过小时保底 6.5pt）
    QFont f = font();
    f.setBold(true);
    f.setPointSizeF(qMax(6.5, (f.pointSizeF() > 0 ? f.pointSizeF() : 9.0) * 0.8));
    const QFontMetrics fm(f);
    const int count = mBadgeCount;
    const QString text = (count > badgeCountLimit()) ? (QString::number(badgeCountLimit()) + QLatin1Char('+'))
                                                     : QString::number(count);
    const int padX   = 4;
    const int height = fm.height() + 2;
    const int width  = qMax(height, fm.horizontalAdvance(text) + 2 * padX);
    // 右上角锚定，留 1px 边距避免贴边裁切
    QRectF badgeRect(rect().right() - width - 1, rect().top() + 1, width, height);
    // 橙红胶囊底（DESIGN.md Danger Red #CE6043）
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0xCE, 0x60, 0x43));
    painter.drawRoundedRect(badgeRect, height / 2.0, height / 2.0);
    // 白色数字
    painter.setPen(QColor(0xFF, 0xFF, 0xFF));
    painter.setFont(f);
    painter.drawText(badgeRect, Qt::AlignCenter, text);
    painter.restore();
}
}  // namespace DA
