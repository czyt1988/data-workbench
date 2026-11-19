#include "DASignalAnalysisSettingContext.h"
// Qt
#include <QDebug>
#include <QUndoStack>
#include <QWidget>
// DAGui
#include "Chart/Setting3D/DAChart3DSettingWidget.h"
#include "Common/Settings/DASettingContainerWidget.h"
#include "DAChart3DWidget.h"
#include "DAChartManageWidget.h"
#include "DAChartOperateWidget.h"
#include "DAChartSettingWidget.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "qwt3d_waterfall.h"
// DAInterface
#include "DADockingAreaInterface.h"
// 插件
#include "DASignalAnalysisUI.h"
#include "Page/DASignalAnalysisPage.h"
#include "Page/DASignalSettingTargets.h"

namespace DA
{
/**
 * @brief 构造函数：保存插件 UI 指针（不持有所有权，用 QPointer 跟踪其生命周期）
 * @param ui 插件界面层（提供页面/当前视图/上下文标签与底部工具栏开关）
 * @param parent 父 QObject
 */
DASignalAnalysisSettingContext::DASignalAnalysisSettingContext(DASignalAnalysisUI* ui, QObject* parent)
    : QObject(parent), mUI(ui)
{
}

/**
 * @brief 析构函数：断开页面切信号与当前视图子图切换的信号连接
 *
 * 提供者被注销/销毁时，dock 内容可能已经或即将销毁，因此这里只断连接，
 * 不再回写界面状态（失活时的界面还原由 deactivateSettingContext() 完成）。
 */
DASignalAnalysisSettingContext::~DASignalAnalysisSettingContext()
{
    if (mUI) {
        if (DASignalAnalysisPage* page = mUI->page()) {
            QObject::disconnect(page, &DASignalAnalysisPage::currentSignalChanged, this, nullptr);
        }
    }
    unbindCurrentView();
}

/**
 * @brief 该 dock 成为激活页时接管设置上下文
 *
 * 行为（与 T7/T8 团队规格 §4 一致）：
 * 1. 页面未创建时直接返回（页面惰性创建，未打开过信号分析就没有上下文可接管）；
 * 2. 唤起右侧设置停靠窗口（raiseDockByWidget(设置容器)）；
 * 3. 按当前子图切换设置页并设置 target（2D → Chart 页 + setPlot；3D → 3D 页 + setPlot3D）；
 * 4. 图元树接管为全部信号 figure，当前 figure 为当前信号的 figure；
 * 5. 显示插件 Ribbon 上下文标签与窗口底部播放工具栏；
 * 6. 绑定页面切信号信号与当前视图的子图切换信号（先断后连，重复激活不重复连接）；
 * 7. 置激活标志。
 *
 * 幂等：宿主可能因为嵌套焦点变化对同一提供者重复调用，重复执行只会刷新 target。
 */
void DASignalAnalysisSettingContext::activateSettingContext()
{
    if (!mUI) {
        qWarning() << "DASignalAnalysisSettingContext::activateSettingContext: ui is null";
        return;
    }
    DASignalAnalysisPage* page = mUI->page();
    if (nullptr == page) {
        qWarning() << "DASignalAnalysisSettingContext::activateSettingContext: page is not created";
        return;
    }
    DADockingAreaInterface* dockingArea = mUI->getDockingArea();
    if (nullptr == dockingArea) {
        qWarning() << "DASignalAnalysisSettingContext::activateSettingContext: docking area is null";
        return;
    }
    // 1) 唤起右侧设置 dock（设置容器所在窗口）
    if (DASettingContainerWidget* container = dockingArea->getSettingContainerWidget()) {
        dockingArea->raiseDockByWidget(container);
    }
    // 2) 设置面板切页并设置 target；图元树接管信号 figure
    applySettingTarget();
    updateChartManageTarget();
    // 3) 显示只在插件上下文出现的 UI：Ribbon 上下文标签 + 窗口底部工具栏
    mUI->setSignalContextCategoryVisible(true);
    mUI->setSignalToolbarVisible(true);
    // 4) 绑定页面切信号与当前视图子图切换（先断后连，保证重复激活只有一条连接）
    QObject::disconnect(page, &DASignalAnalysisPage::currentSignalChanged, this, nullptr);
    QObject::connect(page,
                     &DASignalAnalysisPage::currentSignalChanged,
                     this,
                     &DASignalAnalysisSettingContext::onPageCurrentSignalChanged);
    bindCurrentView();
    mActive = true;
}

/**
 * @brief 失活：隐藏插件上下文 UI，并把设置面板/图元树还原给宿主绘图区
 *
 * 行为（与团队规格 §4 一致）：
 * 1. 隐藏 Ribbon 上下文标签与窗口底部工具栏；
 * 2. 图元树恢复宿主绘图操作窗口（setChartOperateWidget 会清空并以宿主 figure 列表重建）；
 * 3. 设置面板恢复为 Chart 页，target 为宿主当前 figure 的当前 2D 子图（取不到则 setPlot(nullptr)）；
 * 4. 结束信号 figure 上可能仍激活的图表编辑器（选区/标记由内置「绘图操作」触发）；
 * 5. 解绑页面/视图信号，清激活标志；
 * 6. 通知宿主刷新绘图上下文（此时注册表已清空当前 provider，宿主会恢复绘图区动作状态）。
 *
 * 幂等：未激活时调用直接返回，不触碰界面。
 */
void DASignalAnalysisSettingContext::deactivateSettingContext()
{
    if (!mActive) {
        return;
    }
    // 1) 隐藏插件上下文 UI
    if (mUI) {
        // D6：页面失活即停止播放（只挂在 provider 失活；setSignalToolbarVisible(false) 也会在
        // 激活流程里被调用，不能在那里停）。工具 dock（设置/峰值/日志）激活不会失活本 provider，
        // 因此打开峰值窗口不会误停播放。
        mUI->stopPlayback();
        mUI->setSignalContextCategoryVisible(false);
        mUI->setSignalToolbarVisible(false);
    }
    DADockingAreaInterface* dockingArea = (mUI ? mUI->getDockingArea() : nullptr);
    if (nullptr == dockingArea) {
        qWarning() << "DASignalAnalysisSettingContext::deactivateSettingContext: docking area is null";
    } else {
        // 2) 图元树恢复宿主绘图操作窗口
        if (DAChartManageWidget* chartManage = dockingArea->getChartManageWidget()) {
            chartManage->setChartOperateWidget(dockingArea->getChartOperateWidget());
        }
        // 3) 设置面板恢复宿主当前 figure 的当前 2D 子图
        if (DASettingContainerWidget* container = dockingArea->getSettingContainerWidget()) {
            container->showChartSettingWidget();
            DAChartWidget* hostChart = nullptr;
            if (DAChartOperateWidget* chartOperate = dockingArea->getChartOperateWidget()) {
                if (DAFigureWidget* hostFigure = chartOperate->getCurrentFigure()) {
                    hostChart = hostFigure->getCurrentChart();
                }
            }
            if (DAChartSettingWidget* chartSetting = container->getChartSettingWidget()) {
                chartSetting->setPlot(hostChart);
            }
            // D1-10：3D 设置页的 target 同样要复位。失活后 3D 页仍指向信号视图的三维瀑布子图，
            // 而该子图会随页面销毁；宿主绘图区的「3D 设置」不该停在一个插件对象上。
            if (DAChart3DSettingWidget* chart3DSetting = container->getChart3DSettingWidget()) {
                chart3DSetting->setPlot3D(nullptr);
            }
        }
    }
    // 4) 结束信号 figure 上可能仍激活的图表编辑器：内置「绘图操作」的选区/标记工具
    //    以 beginChartEditor 进入编辑态，失活后不再有对应该 figure 的结束入口，需在此收尾
    if (DASignalAnalysisView* view = currentView()) {
        if (DAFigureWidget* figure = view->figure()) {
            if (figure->isChartEditorActive()) {
                figure->endChartEditor();
            }
        }
    }
    // 5) 解绑信号
    if (mUI) {
        if (DASignalAnalysisPage* page = mUI->page()) {
            QObject::disconnect(page, &DASignalAnalysisPage::currentSignalChanged, this, nullptr);
        }
    }
    unbindCurrentView();
    // 图元树已被恢复成宿主绘图区目标（setChartOperateWidget 重建了管理树）：
    // 缓存作废，下次激活必须重新 setFigures 注入信号 figure 列表（D1-09）
    mFigureListCache.clear();
    mActive = false;
    // 6) 通知宿主刷新绘图上下文：注销/卸载路径不走 APP 的固定窗口路由，需要显式通知
    //    （注册表已清空当前 provider，宿主据此恢复绘图区动作状态并收起内置「绘图操作」标签）
    if (mUI) {
        if (DADockingAreaInterface* dockingArea = mUI->getDockingArea()) {
            dockingArea->notifySettingContextChanged();
        }
    }
}

/**
 * @brief 本上下文的撤销栈：当前信号视图 figure 的 undo stack
 * @return 无页面/无当前视图/无 figure 时返回 nullptr（宿主据此不切换字体颜色等状态）
 */
QUndoStack* DASignalAnalysisSettingContext::contextUndoStack() const
{
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        return nullptr;
    }
    DAFigureWidget* figure = view->figure();
    if (nullptr == figure) {
        return nullptr;
    }
    return figure->getUndoStack();
}

/**
 * @brief 本上下文的绘图操作目标：当前信号视图的 figure
 *
 * 宿主在内置「绘图操作」上下文标签显示期间把绘图域动作（网格/缩放/拾取/主题/导出/
 * 选区/标记等）路由到该 figure；返回 nullptr 表示当前没有可操作的信号图，宿主会
 * 隐藏内置「绘图操作」上下文标签并禁用相关动作。
 * @return 当前信号 figure；页面未创建或无当前视图时返回 nullptr
 */
DAFigureWidget* DASignalAnalysisSettingContext::chartContextFigure() const
{
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        return nullptr;
    }
    return view->figure();
}

/**
 * @brief 激活状态下刷新设置面板 target 与图元树当前 figure
 *
 * 只重做 activateSettingContext() 的第 2 步（设置页 target 与图元树），
 * 不重复 raise 设置 dock，也不改上下文标签/底部工具栏显隐。
 * 结束后通知宿主刷新绘图上下文：信号/子图切换会改变 chartContextFigure() 与
 * 内置「绘图操作」动作的勾选状态，宿主据此重新回灌 Ribbon 并同步动作可用性。
 */
void DASignalAnalysisSettingContext::refreshTarget()
{
    if (!mActive || !mUI) {
        return;
    }
    applySettingTarget();
    updateChartManageTarget();
    if (DADockingAreaInterface* dockingArea = mUI->getDockingArea()) {
        dockingArea->notifySettingContextChanged();
    }
}

/**
 * @brief 把右侧 3D 设置面板切到当前三维瀑布图元
 *
 * 丝带「信号显示 → 瀑布图设置 → 瀑布设置…」的唯一落点：显示 3D 设置页、
 * target 设为当前 3D 子图，并在图元下拉里选中瀑布图元（详细样式都在这里改，
 * 丝带只留色带/dB/高亮三个常用项）。图元尚未生成（时频分析未完成）时不做任何事，
 * 由调用方给出提示。
 */
void DASignalAnalysisSettingContext::showWaterfallItemSetting()
{
    if (!mUI) {
        return;
    }
    DADockingAreaInterface* dockingArea = mUI->getDockingArea();
    if (nullptr == dockingArea) {
        return;
    }
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        return;
    }
    Qwt3DWaterfall* waterfallItem = view->waterfall3DItem();
    if (nullptr == waterfallItem) {
        return;  // 图元未生成：没有可定位的对象
    }
    DASettingContainerWidget* container = dockingArea->getSettingContainerWidget();
    if (nullptr == container) {
        return;
    }
    container->showChart3DSettingWidget();
    if (DAChart3DSettingWidget* chart3DSetting = container->getChart3DSettingWidget()) {
        chart3DSetting->setPlot3D(view->current3DChart());
        chart3DSetting->setCurrentPlot3DItem(waterfallItem);
    }
}

/**
 * @brief 是否处于激活状态
 * @return 已激活返回 true
 */
bool DASignalAnalysisSettingContext::isActive() const
{
    return mActive;
}

/**
 * @brief 信号标签页切换：结束旧 figure 的图表编辑器、重新绑定新视图并刷新设置 target
 *
 * 页面切信号后当前视图对象整体变化，旧的 currentSubChartChanged 连接必须解到新视图上，
 * 否则切子图时设置面板 target 不再跟随。若旧信号 figure 上仍有内置「绘图操作」的
 * 选区/标记编辑器处于激活态，先结束它，避免用户切走后旧 figure 停留在编辑态。
 */
void DASignalAnalysisSettingContext::onPageCurrentSignalChanged()
{
    if (!mActive) {
        return;
    }
    if (mView) {
        if (DAFigureWidget* oldFigure = mView->figure()) {
            if (oldFigure->isChartEditorActive()) {
                oldFigure->endChartEditor();
            }
        }
    }
    bindCurrentView();
    refreshTarget();
}

/**
 * @brief 当前 2D/3D 子图变化：刷新设置面板 target（2D ↔ 3D 页切换）
 */
void DASignalAnalysisSettingContext::onViewSubChartChanged()
{
    if (!mActive) {
        return;
    }
    refreshTarget();
}

/**
 * @brief 绑定当前信号视图的子图切换信号（重绑前先断开旧连接）
 */
void DASignalAnalysisSettingContext::bindCurrentView()
{
    unbindCurrentView();
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        return;
    }
    mView = view;
    mViewConnection = QObject::connect(view,
                                       &DASignalAnalysisView::currentSubChartChanged,
                                       this,
                                       &DASignalAnalysisSettingContext::onViewSubChartChanged);
}

/**
 * @brief 断开当前信号视图的子图切换信号并清空视图指针
 */
void DASignalAnalysisSettingContext::unbindCurrentView()
{
    if (mViewConnection) {
        QObject::disconnect(mViewConnection);
        mViewConnection = QMetaObject::Connection();
    }
    mView.clear();
}

/**
 * @brief 按当前子图类型切换设置页并设置 target
 *
 * - 当前为 3D 子图：显示 3D 设置页并把 target 设为 current3DChart()；
 * - 否则显示 2D 绘图设置页并把 target 设为 currentChart()；
 * - 页面还没有信号（无当前视图）：2D 设置页 + 空 target，**并把 3D 页 target 也置空**，
 *   避免继续编辑宿主绘图区/已销毁的信号子图（D1-10）。
 *
 * 页面类型判定抽成纯函数 daSignalSettingPage()，便于单测（不依赖 ADS 停靠结构）。
 */
void DASignalAnalysisSettingContext::applySettingTarget()
{
    if (!mUI) {
        return;
    }
    DADockingAreaInterface* dockingArea = mUI->getDockingArea();
    if (nullptr == dockingArea) {
        return;
    }
    DASettingContainerWidget* container = dockingArea->getSettingContainerWidget();
    if (nullptr == container) {
        return;
    }
    DASignalAnalysisView* view     = currentView();
    const DASignalSettingPage page = daSignalSettingPage(view);
    if (DASignalSettingPage::Chart3D == page) {
        container->showChart3DSettingWidget();
        if (DAChart3DSettingWidget* chart3DSetting = container->getChart3DSettingWidget()) {
            chart3DSetting->setPlot3D(view->current3DChart());
        }
        return;
    }
    container->showChartSettingWidget();
    if (DAChartSettingWidget* chartSetting = container->getChartSettingWidget()) {
        chartSetting->setPlot(DASignalSettingPage::Chart2D == page ? view->currentChart() : nullptr);
    }
    if (DASignalSettingPage::None == page) {
        // 无当前视图：3D 设置页 target 一并复位（D1-10）
        if (DAChart3DSettingWidget* chart3DSetting = container->getChart3DSettingWidget()) {
            chart3DSetting->setPlot3D(nullptr);
        }
    }
}

/**
 * @brief 图元树目标设为全部信号 figure，当前 figure 为当前信号的 figure
 *
 * setFigures 走 DAChartManageWidget 的外部模式：它会在外部模式置位、清空 combo 与树视图
 * 并逐个重建（旧树视图 deleteLater + 新对象），因此**只在 figure 列表真的变化时**才调用。
 * 切信号/点一下子图都会走 refreshTarget，列表通常不变，此时只 setCurrentFigure，
 * 既保留树的展开/选中状态，也避免每次点击都重建整棵树（D1-09）。
 * 失活时由 deactivateSettingContext() 用 setChartOperateWidget 恢复宿主目标（缓存同时作废）。
 */
void DASignalAnalysisSettingContext::updateChartManageTarget()
{
    if (!mUI) {
        return;
    }
    DADockingAreaInterface* dockingArea = mUI->getDockingArea();
    if (nullptr == dockingArea) {
        return;
    }
    DAChartManageWidget* chartManage = dockingArea->getChartManageWidget();
    if (nullptr == chartManage) {
        return;
    }
    DASignalAnalysisView* view    = currentView();
    DAFigureWidget* currentFigure = (view ? view->figure() : nullptr);
    const QList< DAFigureWidget* > figures = mUI->signalFigures();
    if (currentFigure && daSameFigureList(figures, mFigureListCache)) {
        // 列表同构：只切当前项，不重建 combo/树视图
        chartManage->setCurrentFigure(currentFigure);
        return;
    }
    chartManage->setFigures(figures, currentFigure);
    mFigureListCache = figures;
}

/**
 * @brief 取当前信号视图（页面未创建/无信号时返回 nullptr）
 * @return 当前信号对应的视图
 */
DASignalAnalysisView* DASignalAnalysisSettingContext::currentView() const
{
    if (!mUI) {
        return nullptr;
    }
    return mUI->currentView();
}

}  // namespace DA
