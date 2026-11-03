#ifndef DATABLEQUICKPLOTWINDOW_H
#define DATABLEQUICKPLOTWINDOW_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include <QWidget>
#include <QPointer>
#include <functional>

class QLabel;
class QComboBox;
class QToolButton;
class QPushButton;
class QAction;

namespace DA
{
class DAChartWidget;

/**
 * @brief 表内快速绘图内容部件
 *
 * 表格页快速绘图的全部交互内容：图型切换、聚合口径、X 轴选择、
 * 「更新为当前选区」「转入绘图区」与提示；由 DATableChildWindowContainer
 * 贴入 DATableChildWindow 框架承载（框架负责拖动移动、手柄缩放、最大化、
 * 关闭——本类不再自带标题栏与定位逻辑，标题经 titleChanged 交给框架显示）。
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
    void showForRequest(const DATableChartRequest& request);
    // 按「更新为当前选区」的连续性规则换成新选区（保持图型，不支持才回落）
    void updateRequest(const DATableChartRequest& request);
    // 数据已变化：显示可点击刷新的提示（不自动重画）
    void notifyDataChanged();
    // 当前绑定的请求/规格
    const DATableChartRequest& request() const { return mRequest; }
    const DATableChartSpec& spec() const { return mSpec; }
    // 提示文案（无法出图/错误），供宿主在状态栏或消息区展示
    QString lastMessage() const { return mLastMessage; }
    // 「追加到当前图表」是否可用（由宿主注入：取决于绘图区有没有当前图表）
    using AppendAvailabilityProvider = std::function< bool() >;
    void setAppendAvailabilityProvider(const AppendAvailabilityProvider& provider);

    // 无法出图原因的中文/英文展示文案
    static QString rejectReasonText(DATableChartRejectReason reason);
    // 图型显示名
    static QString chartTypeText(DATableChartType type);
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

protected:
    void keyPressEvent(QKeyEvent* event) override;

private Q_SLOTS:
    void onChartTypeClicked(int typeValue);
    void onAggregationChanged(int index);
    void onXAxisChanged(int index);
    void onUpdateClicked();
    void onRefreshClicked();
    void onTransferClicked();
    void onTransferNewFigure();
    void onTransferAppendCurrent();

private:
    void buildUi();
    void rebuildTypeButtons();
    void refreshOptionWidgets();
    void renderNow();
    void scheduleRender();
    void setNotice(const QString& notice);
    void setError(const QString& error);
    // 绑定列的名称摘要（titleChanged 载荷）
    QString columnSummary() const;
    void switchToType(DATableChartType type);

private:
    DATableChartRequest mRequest;
    DATableChartSpec mSpec;
    QString mLastMessage;
    QString mPendingNotice;
    bool mDataChanged { false };
    bool mIgnoreTypeSignals { false };
    bool mRenderPending { false };

    QWidget* mTypeBar { nullptr };
    QLabel* mAggregationLabel { nullptr };
    QComboBox* mAggregationCombo { nullptr };
    QLabel* mXAxisLabel { nullptr };
    QComboBox* mXAxisCombo { nullptr };
    QPointer< DAChartWidget > mChart;
    QLabel* mNoticeLabel { nullptr };
    QLabel* mErrorLabel { nullptr };
    QPushButton* mRefreshButton { nullptr };
    QToolButton* mTransferButton { nullptr };
    QAction* mAppendAction { nullptr };
    AppendAvailabilityProvider mAppendAvailability;
};

}  // namespace DA

#endif  // DATABLEQUICKPLOTWINDOW_H
