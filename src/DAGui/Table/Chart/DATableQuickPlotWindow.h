#ifndef DATABLEQUICKPLOTWINDOW_H
#define DATABLEQUICKPLOTWINDOW_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include <QPointer>
#include <QWidget>
#include <functional>

class QStackedWidget;

namespace DA
{
class DAChartWidget;
class DAChart3DWidget;

/**
 * @brief 表内快速绘图的绘图区（v4）
 *
 * 只承载图表的 2D/3D 堆叠区：图型、角色/聚合等设置项与「更新为当前选区」「刷新」
 * 「转入绘图区」等操作入口全部在右侧「设置」Dock 的 DATableQuickPlotSettingWidget，
 * 本类只负责取数绘制、把状态回推给设置页、并把设置页的编辑写回渲染。
 *
 * 生命周期约定（实施方案 §5.3）：
 * - 每个表格页最多一个实例，再次触发复用同一实例；
 * - 绑定触发时的选区，不随表里选区自动跟随；
 * - 不写工程数据、不进撤销栈；转入绘图区后由宿主关闭。
 */
class DAGUI_API DATableQuickPlotWindow : public QWidget
{
    Q_OBJECT
public:
    explicit DATableQuickPlotWindow(QWidget* parent);
    ~DATableQuickPlotWindow() override;

    // 贴入框架时的推荐尺寸（宿主 attach 用）
    static QSize recommendedSize();
    // 用请求打开/刷新内容；首次调用即显示（位置与尺寸由框架负责）
    // preferredType 非 None 且当前选区支持时按该图型出图，否则用规划默认图型
    void showForRequest(const DATableChartRequest& request,
                        DATableChartType preferredType = DATableChartType::None);
    // 按「更新为当前选区」的连续性规则换成新选区（保持图型，不支持才回落）
    void updateRequest(const DATableChartRequest& request);
    // 数据已变化：显示可点击刷新的提示（不自动重画）
    void notifyDataChanged();
    // 当前绑定的请求/规格
    const DATableChartRequest& request() const { return mRequest; }
    const DATableChartSpec& spec() const { return mSpec; }
    // 是否已进入手动配置模式（设置页换过列）
    bool isManualMode() const { return mManual; }
    // 提示文案（无法出图/错误），供宿主在状态栏或消息区展示
    QString lastMessage() const { return mLastMessage; }

    // ===== 设置页（右侧 Dock）驱动入口 =====

    // 当前请求下可选图型与默认规格（设置页图型下拉的可用状态）
    DATableChartPlan currentPlan() const;
    // 设置页图型下拉：切图型（当前选区不支持时忽略）
    void setChartType(DATableChartType type);
    // 设置页角色编辑：用新的请求/规格重绘（进入手动配置模式）
    void applyEditedContext(const DATableChartRequest& request, const DATableChartSpec& spec);
    // 设置页「更新为当前选区」：请求宿主回填表格页当前选区
    void requestUpdateToCurrentSelection();
    // 设置页「刷新」：按当前数据立即重画
    void refresh();
    // 设置页「转入绘图区」
    void requestTransfer(bool appendToCurrent);

    // 只读状态（设置页显示）
    QString noticeText() const { return mNoticeText; }
    QString errorText() const { return mErrorText; }
    // 列摘要 +（手动配置）后缀：子窗口标题与设置页标题共用
    QString displayTitle() const;
    // 「追加到当前图表」是否可用（无提供者时按可用处理，转入失败会给出可读原因）
    bool isAppendToCurrentAvailable() const;

    // 「追加到当前图表」是否可用（由宿主注入：取决于绘图区有没有当前图表）
    using AppendAvailabilityProvider = std::function< bool() >;
    void setAppendAvailabilityProvider(const AppendAvailabilityProvider& provider);

    // 无法出图原因的中文/英文展示文案
    static QString rejectReasonText(DATableChartRejectReason reason);
    // 图型显示名（柱状图按规格区分计数/聚合文案）
    static QString chartTypeText(DATableChartType type);
    static QString chartTypeText(DATableChartType type, const DATableChartSpec& spec);
    // 图型对选区的要求（图型下拉置灰时的悬浮提示）
    static QString chartTypeRequirementText(DATableChartType type);
    // 聚合口径显示名
    static QString aggregationText(DATableChartAggregation aggregation);

Q_SIGNALS:
    // 列名摘要变化（框架标题栏显示用）
    void titleChanged(const QString& title);
    // 用户点「更新为当前选区」：宿主回填表格页当前选区
    void currentSelectionRequested();
    // 用户要求转入绘图区（append=false 新建图，true 追加到当前 chart）
    void transferRequested(const DA::DATableChartRequest& request, const DA::DATableChartSpec& spec, bool appendToCurrent);
    // 窗口请求关闭（Esc）
    void closeRequested();
    // 请求/规格/手动模式变化：设置页需要整体同步（图型下拉 + 角色行）
    void contextChanged();
    // 提示/错误/数据变化：设置页只刷新状态文字与按钮
    void statusChanged();
    // 用户点击了绘图区（含图表画布/3D 视图）：宿主把右侧「设置」Dock 抬到顶层
    void activated();
    // 绘图区被隐藏（关闭/切到别的表格页）：宿主解绑设置页
    void hidden();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUi();
    void renderNow();
    void scheduleRender();
    void setNotice(const QString& notice);
    void setError(const QString& error);
    void switchToType(DATableChartType type);
    void ensureChartForType();
    DAChart3DWidget* ensureChart3D();

private:
    DATableChartRequest mRequest;  ///< 当前渲染请求（可能含设置页追加的列）
    DATableChartSpec mSpec;
    QString mNoticeText;  ///< 采样受限/Top N 截断等只读提示（设置页显示）
    QString mErrorText;   ///< 无法出图的错误文案（设置页显示）
    QString mLastMessage;
    QString mPendingNotice;
    bool mDataChanged { false };
    bool mManual { false };
    bool mRenderPending { false };
    bool mActivationFilterInstalled { false };

    QStackedWidget* mStack { nullptr };
    QPointer< DAChartWidget > mChart2D;
    DAChart3DWidget* mChart3D { nullptr };  ///< 懒创建，由 mStack 拥有
    AppendAvailabilityProvider mAppendAvailability;
};

}  // namespace DA

#endif  // DATABLEQUICKPLOTWINDOW_H
