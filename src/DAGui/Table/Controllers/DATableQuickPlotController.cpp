#include "Table/Controllers/DATableQuickPlotController.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Core/DATableSelectionService.h"
// qt
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QWidget>
// DAGui
#include "Table/Views/DADataTableView.h"
#include "Table/Views/DATablePaneContainer.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "Table/Chart/DATableQuickPlotWindow.h"
#include "Table/Views/DATableChildWindow.h"
#include "Table/Views/DATableChildWindowContainer.h"
// DAData
#include "DATableDataSource.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 忙等待光标（构造请求与规划期间取数，RAII 恢复）
#include "DAWaitCursorScoped.h"

namespace DA
{
//===================================================
// DATableQuickPlotController
//===================================================

DATableQuickPlotController::DATableQuickPlotController(const DATablePageContext& ctx, const DATableSelectionService& sel, QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel)
{
}

/**
 * @brief 用当前（有序）选区构造快速绘图请求
 *
 * 有序选区经选区服务（视图顺序、隐藏列跳过）；列角色与基数判定走
 * DATableChartPlanner；惰性源标记随数据源探得。
 * @return 快速绘图请求
 */
DA::DATableChartRequest DATableQuickPlotController::buildQuickPlotRequest() const
{
    DA::DATableChartRequest request;
    request.data = mCtx.data();
    DAData dataCopy            = mCtx.data();  // 值语义共享底层指针，取非 const 接口
    DATableDataSource* source  = dataCopy.tableSource();
    if (source) {
        request.rowCount = source->tableRowCount();
        request.lazy     = source->isLazyLoaded();
    }
    const DAData d = mCtx.data();
    const QList< int > columns = mSel.selectedDataColumnsInViewOrder(d.isDataFrame());
    for (int col : columns) {
        int occurrence = 0;
        if (source && col >= 0 && static_cast< std::size_t >(col) < source->tableColumnCount()) {
            const QString name = source->tableColumnName(static_cast< std::size_t >(col));
            for (int c = 0; c < col; ++c) {
                if (source->tableColumnName(static_cast< std::size_t >(c)) == name) {
                    ++occurrence;
                }
            }
        }
        request.columns.append(DA::DATableChartPlanner::makeColumn(mCtx.data(), col, occurrence));
    }
    return request;
}

/**
 * @brief 用当前选区打开/刷新快速绘图子窗口
 *
 * 没有可画图型时子窗口不出现，只在消息区给出原因（实施方案 §3.2 第 1 步）。
 * 内容部件贴入表格子窗口容器（DATableChildWindow 框架承载）：可拖动移动、
 * 8 向手柄缩放、最大化/还原，全程夹取在主视口内——不压表头与滚动条。
 */
bool DATableQuickPlotController::showQuickPlot()
{
    // 构造请求与规划期间会探测列角色/基数（惰性源要取数），给一个等待光标（实施方案 §8.4）
    DAWaitCursorScoped waitCursor;
    const DA::DATableChartRequest request = buildQuickPlotRequest();
    const DA::DATableChartPlan plan       = DA::DATableChartPlanner::plan(request);
    if (!plan.isPlottable()) {
        daWarning << DA::DATableQuickPlotWindow::rejectReasonText(plan.reject);
        return false;
    }

    if (!mQuickPlotWindow) {
        mQuickPlotWindow = new DA::DATableQuickPlotWindow(mCtx.hostWidget());
        mQuickPlotWindow->setAppendAvailabilityProvider([ this ]() {
            return mAppendAvailable ? mAppendAvailable() : false;
        });
        connect(mQuickPlotWindow, &DA::DATableQuickPlotWindow::closeRequested, this, &DATableQuickPlotController::closeQuickPlot);
        connect(mQuickPlotWindow, &DA::DATableQuickPlotWindow::currentSelectionRequested, this, [ this ]() {
            if (mQuickPlotWindow) {
                mQuickPlotWindow->updateRequest(buildQuickPlotRequest());
            }
        });
        connect(mQuickPlotWindow, &DA::DATableQuickPlotWindow::transferRequested, this,
                [ this ](const DA::DATableChartRequest& req, const DA::DATableChartSpec& spec, bool append) {
                    if (!mTransfer) {
                        daWarning << tr("The chart area is not available");  // cn:绘图区不可用
                        return;
                    }
                    QString error;
                    if (!mTransfer(req, spec, append, &error)) {
                        daWarning << (error.isEmpty() ? tr("Failed to transfer to the chart area") : error);  // cn:转入绘图区失败
                        return;
                    }
                    daInfo << tr("The chart has been transferred to the chart area");  // cn:已转入绘图区
                    closeQuickPlot();
                });
        // 贴入表格子窗口容器（Excel 式浮层）：首次贴在主视口右上角附近；
        // 标题栏由框架绘制，列名摘要经 titleChanged 同步
        const QSize preferred = DA::DATableQuickPlotWindow::recommendedSize();
        const QRect bounds    = tableChildWindowBounds();
        const QPoint preferredPos(bounds.right() - preferred.width() + 1 - 12, bounds.top() + 12);
        mQuickPlotFrame = tableChildWindowContainer()->attach(mQuickPlotWindow, QString(), preferred, preferredPos);
        if (mQuickPlotFrame) {
            connect(mQuickPlotWindow, &DA::DATableQuickPlotWindow::titleChanged, this, [ this ](const QString& title) {
                if (mQuickPlotFrame) {
                    mQuickPlotFrame->setTitle(title);
                }
            });
        }
    }

    // 再次打开保持用户上次移动/缩放出的位置与大小，只夹回当前允许区域
    if (mQuickPlotFrame) {
        mQuickPlotFrame->setVisible(true);
        mQuickPlotFrame->raise();
        mQuickPlotFrame->clampIntoBounds();
    }
    mQuickPlotWindow->showForRequest(request);
    return mQuickPlotWindow->isVisible();
}

/**
 * @brief 懒建表格子窗口容器
 *
 * 宿主=hostWidget；允许区域=主视口（tableChildWindowBounds）；监听主视口与窗格容器
 * 的移动/缩放/显隐（滚动条显隐、冻结分栏变化、页面缩放都会改变允许区域），
 * 变化后容器自动把全部子窗口夹回允许区域。
 */
DATableChildWindowContainer* DATableQuickPlotController::tableChildWindowContainer()
{
    if (!mChildWindowContainer) {
        mChildWindowContainer = new DA::DATableChildWindowContainer(mCtx.hostWidget(), this);
        mChildWindowContainer->setBoundsProvider([ this ]() { return tableChildWindowBounds(); });
        if (DADataTableView* view = mCtx.mainView()) {
            if (QWidget* vp = view->viewport()) {
                mChildWindowContainer->watchGeometrySource(vp);
            }
        }
        if (DATablePaneContainer* pc = mCtx.paneContainer()) {
            mChildWindowContainer->watchGeometrySource(pc);
        }
    }
    return mChildWindowContainer;
}

/**
 * @brief 子窗口允许区域（宿主部件坐标系）
 *
 * 主视口 = 数据网格可视区（不含行头/列表头/滚动条）：子窗口夹取其中，
 * 表头排序与滚动条交互永远不会被浮窗压住；取不到视图时回退窗格容器矩形。
 */
QRect DATableQuickPlotController::tableChildWindowBounds() const
{
    QWidget* host = mCtx.hostWidget();
    if (DADataTableView* view = mCtx.mainView()) {
        if (QWidget* vp = view->viewport()) {
            return QRect(vp->mapTo(host, QPoint(0, 0)), vp->size());
        }
    }
    if (DATablePaneContainer* pc = mCtx.paneContainer()) {
        return QRect(pc->mapTo(host, QPoint(0, 0)), pc->size());
    }
    return host ? QRect(QPoint(0, 0), host->size()) : QRect();
}

/**
 * @brief 关闭快速绘图子窗口（实例保留复用）
 */
void DATableQuickPlotController::closeQuickPlot()
{
    // 先隐藏承载框架（只藏内容会剩一个空标题栏）
    if (mQuickPlotFrame) {
        mQuickPlotFrame->hide();
    } else if (mQuickPlotWindow) {
        mQuickPlotWindow->hide();
    }
}

/**
 * @brief 快速绘图浮窗是否正在显示
 */
bool DATableQuickPlotController::isQuickPlotVisible() const
{
    return mQuickPlotWindow && mQuickPlotWindow->isVisible();
}

/**
 * @brief 设置转入绘图区的处理器（APP 注入：APP 才能拿到 DAChartOperateWidget）
 */
void DATableQuickPlotController::setTransferHandler(const QuickPlotTransferHandler& handler)
{
    mTransfer = handler;
}

/**
 * @brief 设置「追加到当前图表」的可用性提供者（APP 依绘图区当前图表注入）
 */
void DATableQuickPlotController::setAppendAvailabilityProvider(const QuickPlotAppendAvailabilityProvider& provider)
{
    mAppendAvailable = provider;
}

/**
 * @brief 数据变更通知（宿主页面的 dataModified 信号接线到这里）
 *
 * 浮窗已创建时转发（内容部件重取数据刷新图表）；未创建时无操作。
 */
void DATableQuickPlotController::notifyDataChanged()
{
    if (mQuickPlotWindow) {
        mQuickPlotWindow->notifyDataChanged();
    }
}

/**
 * @brief 页面析构前断开子窗口容器与宿主的全部依赖（use-after-free 防护）
 *
 * boundsProvider 经上下文读主视口；页面析构期间（delete ui 之后、~QObject 阶段）
 * 被监听部件的 Hide 事件仍会触发 eventFilter → clampAllWindows → 回调，必须先于
 * 上下文销毁断开（DATableQuickPlotTest 在页面析构路径偶现 0xC0000005 的教训）。
 * detachFromHost（dev 0c87de48）：事件过滤与允许区域提供者也一并解除——
 * ~QWidget 期间子窗口隐藏抛 Hide 事件 → 回调读已析构 QWidget 部分的 size() 崩溃。
 */
void DATableQuickPlotController::detach()
{
    if (mChildWindowContainer) {
        mChildWindowContainer->detachFromHost();
    }
}
}  // end of namespace DA
