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
// DAInterface
#include "DADockingAreaInterface.h"
// 插件
#include "DASignalAnalysisUI.h"
#include "Page/DASignalAnalysisPage.h"

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
 * 4. 解绑页面/视图信号，清激活标志。
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
        }
    }
    // 4) 解绑信号
    if (mUI) {
        if (DASignalAnalysisPage* page = mUI->page()) {
            QObject::disconnect(page, &DASignalAnalysisPage::currentSignalChanged, this, nullptr);
        }
    }
    unbindCurrentView();
    mActive = false;
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
 * @brief 激活状态下刷新设置面板 target 与图元树当前 figure
 *
 * 只重做 activateSettingContext() 的第 2 步（设置页 target 与图元树），
 * 不重复 raise 设置 dock，也不改上下文标签/底部工具栏显隐。
 */
void DASignalAnalysisSettingContext::refreshTarget()
{
    if (!mActive || !mUI) {
        return;
    }
    applySettingTarget();
    updateChartManageTarget();
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
 * @brief 信号标签页切换：重新绑定新视图并刷新设置 target
 *
 * 页面切信号后当前视图对象整体变化，旧的 currentSubChartChanged 连接必须解到新视图上，
 * 否则切子图时设置面板 target 不再跟随。
 */
void DASignalAnalysisSettingContext::onPageCurrentSignalChanged()
{
    if (!mActive) {
        return;
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
 * - 页面还没有信号（无当前视图）：2D 设置页 + 空 target，避免继续编辑宿主绘图区子图。
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
    DASignalAnalysisView* view = currentView();
    if (nullptr == view) {
        container->showChartSettingWidget();
        if (DAChartSettingWidget* chartSetting = container->getChartSettingWidget()) {
            chartSetting->setPlot(nullptr);
        }
        return;
    }
    if (view->is3DChartActive()) {
        container->showChart3DSettingWidget();
        if (DAChart3DSettingWidget* chart3DSetting = container->getChart3DSettingWidget()) {
            chart3DSetting->setPlot3D(view->current3DChart());
        }
    } else {
        container->showChartSettingWidget();
        if (DAChartSettingWidget* chartSetting = container->getChartSettingWidget()) {
            chartSetting->setPlot(view->currentChart());
        }
    }
}

/**
 * @brief 图元树目标设为全部信号 figure，当前 figure 为当前信号的 figure
 *
 * setFigures 走 DAChartManageWidget 的外部模式：不依赖宿主 DAChartOperateWidget，
 * 失活时由 deactivateSettingContext() 用 setChartOperateWidget 恢复宿主目标。
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
    DASignalAnalysisView* view = currentView();
    chartManage->setFigures(mUI->signalFigures(), (view ? view->figure() : nullptr));
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
