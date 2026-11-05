#ifndef DATABLEQUICKPLOTCONTROLLER_H
#define DATABLEQUICKPLOTCONTROLLER_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include <QObject>
#include <QRect>
#include <functional>

class QString;

namespace DA
{
class DATableChildWindow;
class DATableChildWindowContainer;
class DATablePageContext;
class DATableQuickPlotWindow;
class DATableSelectionService;

/**
 * @brief 表内快速绘图控制器（008 期3）
 *
 * 选区→请求、快速绘图子窗口生命周期、转入绘图区处理器注入、关闭。
 * 构造注入上下文与选区服务；子窗口容器懒建（宿主=上下文的 hostWidget，
 * 允许区域=主视口），浮窗全程夹取在主视口内。
 * 生命周期：宿主页面构造并持有（QObject 父子持有快速绘图窗与子窗口容器）。
 */
class DAGUI_API DATableQuickPlotController : public QObject
{
    Q_OBJECT
public:
    // 转入绘图区的处理器（APP 能拿到 DAChartOperateWidget，由 APP 注入）
    using QuickPlotTransferHandler =
        std::function< bool(const DA::DATableChartRequest&, const DA::DATableChartSpec&, bool /*append*/, QString* /*error*/) >;
    // 「追加到当前图表」是否可用（APP 依绘图区当前图表注入；决定浮窗菜单项灰显）
    using QuickPlotAppendAvailabilityProvider = std::function< bool() >;

    explicit DATableQuickPlotController(const DATablePageContext& ctx, const DATableSelectionService& sel, QObject* parent = nullptr);

public:
    // 用当前（有序）选区构造快速绘图请求；列角色走 DATableChartPlanner
    DA::DATableChartRequest buildQuickPlotRequest() const;
    // 按右键列构造请求：该列在多选内时用整个选区，否则只用该列
    DA::DATableChartRequest buildQuickPlotRequestForColumn(int logicalIndex) const;
    // 按右键列规划可选图型（右键子菜单用）
    DA::DATableChartPlan quickPlotPlanForColumn(int logicalIndex) const;
    // 用当前选区打开/刷新快速绘图子窗口；无法出图时返回 false（已给用户可见提示）
    // preferredType 非 None 且当前选区支持时按该图型出图（右键子菜单点击路径）
    bool showQuickPlot(DA::DATableChartType preferredType = DA::DATableChartType::None);
    // 关闭快速绘图子窗口（实例保留复用）
    void closeQuickPlot();
    // 快速绘图子窗口是否正在显示
    bool isQuickPlotVisible() const;
    // 转入绘图区处理器（APP 注入）
    void setTransferHandler(const QuickPlotTransferHandler& handler);
    // 「追加到当前图表」可用性提供者（APP 注入）
    void setAppendAvailabilityProvider(const QuickPlotAppendAvailabilityProvider& provider);
    // 数据变更通知（宿主页面的 dataModified 信号接线到这里，转发给浮窗重取数）
    void notifyDataChanged();
    // 页面析构前调用：断开子窗口容器的 boundsProvider 回调
    //（回调经上下文读主视口，页面析构期间被监听部件的 Hide 事件仍会触发
    // eventFilter → clampAllWindows → 回调，须先于上下文销毁断开，否则 use-after-free）
    void detach();

private:
    // 按数据列清单构造请求（含整表列清单）
    DA::DATableChartRequest buildRequestFromDataColumns(const QList< int >& dataColumns) const;
    // 懒建表格子窗口容器（宿主=hostWidget；允许区域=主视口）
    DATableChildWindowContainer* tableChildWindowContainer();
    // 子窗口允许区域（主视口在宿主部件坐标系下的矩形；无视图回退窗格容器）
    QRect tableChildWindowBounds() const;

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableQuickPlotWindow* mQuickPlotWindow { nullptr };       ///< 快速绘图内容部件（懒建，单实例）
    DATableChildWindow* mQuickPlotFrame { nullptr };             ///< 快速绘图子窗口框架（容器承载）
    DATableChildWindowContainer* mChildWindowContainer { nullptr };  ///< 表格子窗口容器（懒建）
    QuickPlotTransferHandler mTransfer;                          ///< 转入绘图区处理器（APP 注入）
    QuickPlotAppendAvailabilityProvider mAppendAvailable;         ///< 追加可用性（APP 注入）
};
}  // end of namespace DA
#endif  // DATABLEQUICKPLOTCONTROLLER_H
