#ifndef DABADGEAUTOHIDETAB_H
#define DABADGEAUTOHIDETAB_H
#include "DAGuiAPI.h"
// Qt
#include <QString>
// ADS
#include "AutoHideTab.h"

QT_FORWARD_DECLARE_CLASS(QPainter)

namespace DA
{
/**
 * @brief 带未读计数徽章的 ADS 侧边栏选项卡
 *
 * 继承 ads::CAutoHideTab，在基类绘制之上叠加"红点 + 未读数"徽章
 * （橙红 #CE6043 胶囊底 + 白字，超过 99 显示 99+）。
 *
 * 徽章能力按 dock objectName 静态注册启用（@ref setBadgeEnabled，须在 dock 创建前注册）：
 * ADS 创建侧边栏 tab 时传给工厂的 dock 指针为 nullptr，无法在构造期区分 dock，
 * 因此延迟到 tab 首帧绘制、绑定 dock 后按名单激活；未命中名单时绘制行为与基类完全一致。
 *
 * 未读数只统计警告/错误级消息（QtWarningMsg/QtCriticalMsg/QtFatalMsg），
 * 普通 info/debug 不打扰用户；未读数 = 累计告警数 - 已读水位，
 * auto-hide 面板展开（dock 可见）时清零水位。
 */
class DAGUI_API DABadgeAutoHideTab : public ads::CAutoHideTab
{
    Q_OBJECT
    Q_PROPERTY(int badgeCount READ badgeCount WRITE setBadgeCount NOTIFY badgeCountChanged)

public:
    using Super = ads::CAutoHideTab;
    explicit DABadgeAutoHideTab(QWidget* parent = nullptr);
    ~DABadgeAutoHideTab() override;

    // 当前徽章未读数（0 = 不绘制徽章）
    int badgeCount() const;

    // 设置徽章未读数（负数钳 0，本类不封顶，封顶仅影响绘制文本 99+）
    void setBadgeCount(int count);

    // 徽章计数上限（超过显示 99+）
    static constexpr int badgeCountLimit() { return 99; }

    // 启用/关闭指定 dock objectName 的徽章能力（须在 dock 创建前注册）
    static void setBadgeEnabled(const QString& dockObjectName, bool on = true);

    // 查询指定 dock objectName 的徽章能力是否启用
    static bool isBadgeEnabled(const QString& dockObjectName);

protected:
    virtual void paintEvent(QPaintEvent* event) override;

private Q_SLOTS:
    // 消息队列有新消息到达（DAMessageLogQueue 的 1s 聚合信号）
    void onMessageQueueAppended();

private:
    // 绑定 dock 后按名单尝试激活徽章（首帧触发，dock 未绑定则等待下一帧）
    void tryActivateBadge();
    // 标记当前累计数为已读（清零徽章）
    void markAsRead();
    // 在 tab 右上角绘制徽章（胶囊底 + 数字）
    void drawBadge(QPainter& painter);

Q_SIGNALS:
    // 徽章未读数变化
    void badgeCountChanged(int count);

private:
    bool mBadgeTried { false };    ///< 是否已尝试激活徽章（命中名单与否只判定一次）
    bool mBadgeActive { false };  ///< 徽章是否激活（命中名单并完成信号连接）
    int mBadgeCount { 0 };        ///< 当前未读数
    qint64 mReadWatermark { 0 };  ///< 已读水位（DAMessageLogQueue 累计推送总数）
};
}  // namespace DA
#endif  // DABADGEAUTOHIDETAB_H
