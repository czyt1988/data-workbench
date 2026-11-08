#ifndef DATABLEQUICKPLOTSETTINGWIDGET_H
#define DATABLEQUICKPLOTSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QPointer>
#include <QWidget>

class QAction;
class QComboBox;
class QLabel;
class QPushButton;
class QToolButton;

namespace DA
{
class DACollapseWidget;
class DATableChartRolePanel;
class DATableQuickPlotWindow;

/**
 * @brief 快速绘图的设置页（右侧「设置」Dock 内的一页）
 *
 * 快速绘图子窗口只保留绘图区，全部设置项集中到本页：
 * 图型下拉、按图型生成的角色设置（复用 DATableChartRolePanel）、
 * 「更新为当前选区」「刷新」「转入绘图区」与只读状态提示。
 *
 * 本页是单例（由 DASettingContainerWidget 持有），通过 setTarget() 绑定到某个表格页的
 * 快速绘图子窗口：绑定后与目标窗口双向同步（本页编辑立即回写目标并重绘；目标状态变化
 * 回推刷新本页）。没有打开快速绘图时显示空态提示。
 *
 * 分组折叠复用 DAWidgets 的 DACollapseWidget/DACollapseItem；
 * 角色行沿用 DATableChartRolePanel（按 12 种图型动态生成「角色 + 列下拉」，多序列可增删，
 * 无法用静态的 DAFormSpec 表达，故不走 DAPropertyFormWidget）。
 */
class DAGUI_API DATableQuickPlotSettingWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DATableQuickPlotSettingWidget(QWidget* parent = nullptr);
    ~DATableQuickPlotSettingWidget() override;

    // 绑定目标窗口（nullptr 表示解绑并显示空态）；重复绑定同一窗口会重新同步一次
    void setTarget(DATableQuickPlotWindow* window);
    // 当前绑定的目标窗口（未绑定返回 nullptr）
    DATableQuickPlotWindow* target() const;
    // 目标窗口关闭/销毁：仅当与当前目标一致时解绑（避免别的页面误清当前目标）
    void releaseTarget(DATableQuickPlotWindow* window);
    // 从目标窗口重新同步全部界面（宿主每次抬起本页时调用）
    void syncFromTarget();

private Q_SLOTS:
    // 图型下拉切换：回写目标窗口
    void onTypeComboChanged(int index);
    // 角色设置改动（列/聚合/分箱）：回写目标窗口并重绘
    void onRolePanelEdited();
    // 「更新为当前选区」
    void onUpdateToSelectionClicked();
    // 「刷新」
    void onRefreshClicked();
    // 「转入绘图区」
    void onTransferNewFigure();
    void onTransferAppendCurrent();
    // 目标窗口请求/规格变化（设置项需要整体同步）
    void onTargetContextChanged();
    // 目标窗口提示/错误/数据变化（只刷新状态文字与按钮）
    void onTargetStatusChanged();
    // 目标窗口销毁
    void onTargetDestroyed();

private:
    void buildUi();
    // 重建图型下拉条目（12 种图型，不满足当前选区的置灰并给出原因）
    void buildTypeComboItems();
    // 按目标窗口的规划刷新图型下拉的可用状态/文案与当前项
    void refreshTypeCombo();
    // 刷新标题摘要、提示/错误文字与按钮可用状态
    void refreshChrome();
    // 空态与内容态切换
    void refreshEmptyState();
    // 把目标窗口的请求/规格灌给角色设置面板
    void syncRolePanel();
    // 断开与当前目标的全部连接
    void disconnectTarget();

private:
    QPointer< DATableQuickPlotWindow > mTarget;  ///< 非拥有：由表格页的快速绘图控制器持有
    bool mIgnoreTypeSignals { false };           ///< 程序化改图型下拉时不回写目标
    bool mApplyingOwnEdit { false };             ///< 角色面板自身编辑回写期间不回灌角色行

    QLabel* mSummaryLabel { nullptr };   ///< 列摘要（与子窗口标题同一文案）
    QWidget* mBody { nullptr };          ///< 有目标时显示的主体
    QLabel* mEmptyLabel { nullptr };     ///< 无目标时的空态提示
    QComboBox* mTypeCombo { nullptr };
    DATableChartRolePanel* mRolePanel { nullptr };
    QLabel* mNoticeLabel { nullptr };    ///< 采样/数据变化等只读提示
    QLabel* mErrorLabel { nullptr };     ///< 无法出图的错误提示
    QPushButton* mUpdateButton { nullptr };
    QPushButton* mRefreshButton { nullptr };
    QToolButton* mTransferButton { nullptr };
    QAction* mAppendAction { nullptr };
};

}  // namespace DA

#endif  // DATABLEQUICKPLOTSETTINGWIDGET_H
