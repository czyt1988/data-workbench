#include "DAChartOperateWidget.h"
// stl
#include <memory>
#include <utility>
// Qt
#include <QUndoStack>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPointer>
#include <QDebug>
// ADS
#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
// DAFigure
#include "DAFigureFactory.h"
#include "DAFigureDockWidget.h"
namespace DA
{
int g_figure_cnt = 0;  ///< 绘图的数量，仅限当前程序创建计数
//==============================================================
// DAChartOperateWidgetPrivate
//==============================================================
class DAChartOperateWidgetPrivate
{
    DA_IMPL_PUBLIC(DAChartOperateWidget)
public:
    DAChartOperateWidgetPrivate(DAChartOperateWidget* p);
    // 根据 figure 获取其 dock
    ads::CDockWidget* dockOfFigure(DAFigureWidget* fig) const;
    // 根据 dock 获取其 figure
    DAFigureWidget* figureOfDock(ads::CDockWidget* dock) const;
    // 获取新 figure 应加入的 dock area（取嵌套管理器内已有 figure 所在 area，否则 nullptr）
    ads::CDockAreaWidget* targetAreaForNewFigure() const;

public:
    std::unique_ptr< DAFigureFactory > mFigureFactory;
    ads::CDockManager* mDockManager { nullptr };                       ///< 嵌套停靠管理器
    QList< DAFigureWidget* > mFigures;                                 ///< 插入顺序，作为 index 基础
    QHash< DAFigureWidget*, ads::CDockWidget* > mFigToDock;            ///< figure -> dock
    QHash< DAFigureWidget*, DAFigureDockWidget* > mFigToFigureDock;    ///< figure -> figureDock
    QPointer< DAFigureWidget > mCurrentFigure;                         ///< 当前激活 figure
    bool mSuppressCurrentChanged { false };                            ///< 创建/加载期间抑制 currentFigureChanged
};

DAChartOperateWidgetPrivate::DAChartOperateWidgetPrivate(DAChartOperateWidget* p) : q_ptr(p)
{
    mFigureFactory = std::make_unique< DAFigureFactory >();
}

ads::CDockWidget* DAChartOperateWidgetPrivate::dockOfFigure(DAFigureWidget* fig) const
{
    return mFigToDock.value(fig, nullptr);
}

DAFigureWidget* DAChartOperateWidgetPrivate::figureOfDock(ads::CDockWidget* dock) const
{
    if (!dock) {
        return nullptr;
    }
    if (DAFigureDockWidget* fd = qobject_cast< DAFigureDockWidget* >(dock->widget())) {
        return fd->getFigureWidget();
    }
    return nullptr;
}

ads::CDockAreaWidget* DAChartOperateWidgetPrivate::targetAreaForNewFigure() const
{
    // 关键：不能用 mDockManager->focusedDockWidget()——FocusHighlighting 下嵌套管理器的焦点
    // 控制器与顶层管理器共享 window 属性（DockFocusController.cpp onApplicationFocusChanged 不
    // 校验 dock 所属管理器），用户点过顶层 dock 后 nested->focusedDockWidget() 会返回顶层 dock，
    // 用它作 target 会让新 figure 被加到顶层中心区（逃逸出 DAChartOperateWidget）。
    // 改为从本嵌套管理器已有的 figure dock 取 area，确保新 figure 落在嵌套管理器内。
    for (int i = mFigures.size() - 1; i >= 0; --i) {
        if (ads::CDockWidget* d = mFigToDock.value(mFigures.at(i), nullptr)) {
            if (ads::CDockAreaWidget* a = d->dockAreaWidget()) {
                return a;
            }
        }
    }
    return nullptr;  // 首个 figure：在容器根创建 area
}

//===================================================
// DAChartOperateWidget
//===================================================
DAChartOperateWidget::DAChartOperateWidget(QWidget* parent)
    : DAAbstractOperateWidget(parent), d_ptr(new DAChartOperateWidgetPrivate(this))
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    d_ptr->mDockManager = new ads::CDockManager(this);
    lay->addWidget(d_ptr->mDockManager);
    // 禁止 figure dock 浮动为独立窗口（全局锁，对所有当前及后续 dock 生效），保留分屏/并栏/拖拽
    d_ptr->mDockManager->lockDockWidgetFeaturesGlobally(ads::CDockWidget::DockWidgetFloatable);
    // 嵌套停靠区聚焦改变：onFocusedDockChanged 内部会过滤掉非本管理器的 dock，
    // 规避 FocusHighlighting 下嵌套焦点控制器跨管理器回调顶层 dock 的问题
    connect(d_ptr->mDockManager, &ads::CDockManager::focusedDockWidgetChanged,
            this, &DAChartOperateWidget::onFocusedDockChanged);
}

DAChartOperateWidget::~DAChartOperateWidget()
{
    // mDockManager 作为本部件的子对象，由 Qt 自动销毁；figure dock 禁止浮动，无浮动窗口需额外清理
}

// 获取窗口数量
int DAChartOperateWidget::getFigureCount() const
{
    return d_ptr->mFigures.size();
}

/**
 * @brief 安装FigureFactory，DAChartOperateWidget负责工厂的销毁
 *
 * 工厂在createFigure中调用，在某些情况下，用户可以临时设置一个新工厂，生成一个特殊的figure，再设置回默认的工厂
 * @sa takeFactory
 * @param factory
 */
void DAChartOperateWidget::setupFigureFactory(DAFigureFactory* factory)
{
    d_ptr->mFigureFactory.reset(factory);
}

/**
 * @brief 拿出之前的工厂
 * @note 此函数调用后必须@ref setupFigureFactory 否则会异常
 * @return
 */
DAFigureFactory* DAChartOperateWidget::takeFactory()
{
    return d_ptr->mFigureFactory.release();
}

/**
 * @brief 获取工厂
 * @return
 */
DAFigureFactory* DAChartOperateWidget::getFigureFactory() const
{
    return d_ptr->mFigureFactory.get();
}

/**
 * @brief 创建一个绘图
 *
 * 基于嵌套 ads::CDockManager 管理：DAFigureWidget 包成 DAFigureDockWidget 后由
 * ads::CDockWidget 包装加入停靠区。id 非空时用作 figure 持久 id 与 dock objectName，
 * 供工程反序列化时 restoreState 按 objectName 匹配恢复布局。
 *
 * @note 重载此函数，如果没有调用DAChartOperateWidget::createFigure，必须调用initFigureConnect(fig);来初始化创建的fig，同时也要发射信号figureCreated
 * @param name 绘图名称，为空时自动生成
 * @param id 持久 id，非空时覆盖 figure 默认 id（供工程反序列化恢复布局）
 * @return
 */
DAFigureWidget* DAChartOperateWidget::createFigure(const QString& name, const QString& id)
{
    ++g_figure_cnt;
    QString t = name;
    if (name.isEmpty()) {
        t = tr("figure-%1").arg(g_figure_cnt);  // cn:图-%1
    }
    DAFigureWidget* fig = getFigureFactory()->createFigure();
    fig->setWindowTitle(t);
    if (!id.isEmpty()) {
        // 工程反序列化时恢复持久 id，同时作为 dock objectName 供 restoreState 匹配
        fig->setFigureId(id);
    }
    // 封装为 DAFigureDockWidget（纯 QWidget，遵循项目约定，不继承 ads::CDockWidget）
    DAFigureDockWidget* figureDock = new DAFigureDockWidget(fig);
    // 包装进 ads::CDockWidget（使用 manager 构造以走管理器组件工厂，避免已弃用的两参构造）
    ads::CDockWidget* dock = new ads::CDockWidget(d_ptr->mDockManager, t);
    dock->setObjectName(fig->getFigureId());
    dock->setWidget(figureDock, ads::CDockWidget::ForceNoScrollArea);
    // 关闭按钮触发 closeRequested 而非自动隐藏，便于弹确认框后再删除
    dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
    connect(dock, &ads::CDockWidget::closeRequested, this, [this, fig]() {
        onFigureCloseRequested(fig);
    });
    // 记录映射
    d_ptr->mFigToDock[fig]        = dock;
    d_ptr->mFigToFigureDock[fig]  = figureDock;
    d_ptr->mFigures.append(fig);
    // 抑制聚焦改变，避免 addDockWidget 触发 currentFigureChanged（保留原不变量：创建只发 figureCreated）
    d_ptr->mSuppressCurrentChanged = true;
    ads::CDockAreaWidget* area = d_ptr->targetAreaForNewFigure();
    if (area) {
        // 默认以标签形式加入当前聚焦 dock 所在 area
        d_ptr->mDockManager->addDockWidgetTabToArea(dock, area);
    } else {
        // 首个 figure：在容器根创建 dock area
        d_ptr->mDockManager->addDockWidget(ads::CenterDockWidgetArea, dock);
    }
    d_ptr->mSuppressCurrentChanged = false;
    initFigureConnect(fig);
    Q_EMIT figureCreated(fig);
    return fig;
}

/**
 * @brief 获取所有的绘图
 * @return
 */
QList< DAFigureWidget* > DAChartOperateWidget::getFigureList() const
{
    return d_ptr->mFigures;
}

/**
 * @brief 获取当前的fig，如果没有返回nullptr
 * @return
 */
DAFigureWidget* DAChartOperateWidget::getCurrentFigure() const
{
    // 返回追踪的当前 figure（QPointer 在 figure 销毁后自动置空）
    if (d_ptr->mCurrentFigure) {
        return d_ptr->mCurrentFigure;
    }
    // 回退到最近创建的 figure（不使用 focusedDockWidget，见 targetAreaForNewFigure 注释）
    if (!d_ptr->mFigures.isEmpty()) {
        return d_ptr->mFigures.constLast();
    }
    return nullptr;
}

/**
 * @brief like matlab/matplotlib gcf api
 * @return
 */
DAFigureWidget* DAChartOperateWidget::gcf() const
{
    return getCurrentFigure();
}

/**
 * @brief 把绘图设置为当前绘图
 * @param index
 */
void DAChartOperateWidget::setCurrentFigure(int index)
{
    setCurrentFigure(d_ptr->mFigures.value(index, nullptr));
}

/**
 * @brief 把绘图设置为当前绘图
 * @param fig
 */
void DAChartOperateWidget::setCurrentFigure(DAFigureWidget* fig)
{
    if (!fig) {
        return;
    }
    ads::CDockWidget* dock = d_ptr->dockOfFigure(fig);
    if (!dock) {
        return;
    }
    dock->raise();  // 标签则置为当前，浮动则 raise 窗口
    // raise 可能不触发 focusedDockWidgetChanged（如同 area 内已是当前），主动同步
    if (d_ptr->mCurrentFigure != fig) {
        d_ptr->mCurrentFigure = fig;
        if (QUndoStack* un = fig->getUndoStack()) {
            if (!un->isActive()) {
                un->setActive(true);
            }
        }
        Q_EMIT currentFigureChanged(fig, d_ptr->mFigures.indexOf(fig));
    }
}

/**
 * @brief 根据索引获取fig
 * @param index
 * @return
 */
DAFigureWidget* DAChartOperateWidget::getFigure(int index) const
{
    return d_ptr->mFigures.value(index, nullptr);
}

/**
 * @brief 查找绘图
 * @param id
 * @return
 */
DAFigureWidget* DAChartOperateWidget::findFigure(const QString& id) const
{
    for (DAFigureWidget* fig : std::as_const(d_ptr->mFigures)) {
        if (fig->getFigureId() == id) {
            return fig;
        }
    }
    return nullptr;
}

/**
 * @brief 获取fig的命名
 * @param index
 * @return
 */
QString DAChartOperateWidget::getFigureName(int index) const
{
    DAFigureWidget* fig = getFigure(index);
    return fig ? fig->windowTitle() : QString();
}

QString DAChartOperateWidget::getFigureName(DAFigureWidget* f) const
{
    return f ? f->windowTitle() : QString();
}

/**
 * @brief 设置绘图名称
 * @param index
 * @param name
 */
void DAChartOperateWidget::setFigureName(int index, const QString& name)
{
    setFigureName(getFigure(index), name);
}

void DAChartOperateWidget::setFigureName(DAFigureWidget* f, const QString& name)
{
    if (!f) {
        return;
    }
    // 设置 figure 窗口标题 → windowTitleChanged → onFigureTitleChanged 同步 dock 标签 + 发射 figureTitleChanged
    f->setWindowTitle(name);
}

/**
 * @brief 获取fig在DAChartOperateWidget的索引（创建/插入顺序）
 * @param f
 * @return
 */
int DAChartOperateWidget::getFigureIndex(DAFigureWidget* f) const
{
    return d_ptr->mFigures.indexOf(f);
}

/**
 * @brief  删除窗口
 * @param f
 * @param deleteFigure 如果为true，将会把窗口也删除，默认为true
 */
void DAChartOperateWidget::removeFigure(DAFigureWidget* f, bool deleteFigure)
{
    if (!f) {
        return;
    }
    ads::CDockWidget* dock = d_ptr->dockOfFigure(f);
    if (!dock) {
        return;  // 不由本部件管理
    }
    Q_EMIT figureRemoving(f);  // 不变量：移除前发射，供监听者在此期间访问 figure
    d_ptr->mFigures.removeAll(f);
    d_ptr->mFigToFigureDock.remove(f);
    d_ptr->mFigToDock.remove(f);
    if (d_ptr->mCurrentFigure == f) {
        d_ptr->mCurrentFigure = nullptr;
    }
    if (d_ptr->mDockManager) {
        d_ptr->mDockManager->removeDockWidget(dock);
    }
    if (deleteFigure) {
        // 级联删除 figureDock + fig，延迟到事件循环（与原 f->deleteLater() 语义一致）
        dock->deleteLater();
    }
}

/**
 * @brief 获取当前的chart，如果没有返回nullptr
 * @return
 */
DAChartWidget* DAChartOperateWidget::getCurrentChart() const
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        return fig->getCurrentChart();
    }
    return nullptr;
}

/**
 * @brief like matlab/matplotlib gca api
 * @return
 */
DAChartWidget* DAChartOperateWidget::gca() const
{
    return getCurrentChart();
}

/**
 * @brief 获取当前选中绘图的所有图表
 * @return
 */
QList< DAChartWidget* > DAChartOperateWidget::getAllCharts() const
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        return fig->getCharts();
    }
    return QList< DAChartWidget* >();
}

/**
 * @brief 获取当前选中绘图的所有图表
 * @return
 */
QList< DAChartWidget* > DAChartOperateWidget::gcas() const
{
    return getAllCharts();
}

QUndoStack* DAChartOperateWidget::getUndoStack()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        return fig->getUndoStack();
    }
    return nullptr;
}

/**
 * @brief 保存嵌套停靠区布局（供工程序列化）
 *
 * 顶层 ads::CDockManager::saveState 不会捕获嵌套管理器的布局，故需单独保存
 * @return 布局状态字节数组，无停靠区时返回空
 */
QByteArray DAChartOperateWidget::saveChartLayout() const
{
    if (!d_ptr->mDockManager) {
        return QByteArray();
    }
    return d_ptr->mDockManager->saveState();
}

/**
 * @brief 恢复嵌套停靠区布局（供工程反序列化）
 *
 * 调用前需先按保存顺序 createFigure(name, id) 重建所有 figure dock（objectName=figureId），
 * restoreState 按 objectName 重新挂接布局。state 为空或匹配失败返回 false（保持默认标签顺序）
 * @param state saveChartLayout 返回的状态
 * @return 恢复成功返回 true
 */
bool DAChartOperateWidget::restoreChartLayout(const QByteArray& state)
{
    if (!d_ptr->mDockManager || state.isEmpty()) {
        return false;
    }
    return d_ptr->mDockManager->restoreState(state);
}

/**
 * @brief 清除所有绘图
 */
void DAChartOperateWidget::clear()
{
    // 拷贝后遍历，removeFigure 会修改 mFigures
    const QList< DAFigureWidget* > figs = d_ptr->mFigures;
    for (DAFigureWidget* fig : std::as_const(figs)) {
        removeFigure(fig, true);
    }
}

/**
 * @brief 初始化figure的连接
 *
 * 这个函数用于重载createFigure函数时创建fig后绑定槽函数到DAChartOperateWidget用
 * @param fig
 */
void DAChartOperateWidget::initFigureConnect(DAFigureWidget* fig)
{
    // 信号转发：figure 标题改变 → 同步 dock 标签 + 发射 figureTitleChanged
    connect(fig, &DAFigureWidget::windowTitleChanged, this, &DAChartOperateWidget::onFigureTitleChanged);
}

/**
 * @brief 嵌套停靠区聚焦 dock 改变
 *
 * FocusHighlighting 下嵌套管理器的 CDockFocusController 与顶层管理器共享 window 属性，
 * 用户聚焦顶层 dock（如工作流操作）时本信号也会被回调到顶层 dock。这里通过
 * nowDock->dockManager() 过滤，只处理属于本嵌套管理器的 figure dock，避免 currentFigure
 * 被误置空/误切换。
 * @param oldDock 旧聚焦 dock
 * @param nowDock 新聚焦 dock
 */
void DAChartOperateWidget::onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock)
{
    Q_UNUSED(oldDock);
    if (d_ptr->mSuppressCurrentChanged) {
        return;
    }
    // 过滤掉非本嵌套管理器的 dock（顶层 dock 的跨管理器回调）
    if (nowDock && nowDock->dockManager() != d_ptr->mDockManager) {
        return;
    }
    DAFigureWidget* fig = d_ptr->figureOfDock(nowDock);
    if (d_ptr->mCurrentFigure == fig) {
        return;  // 未变化，避免重复发射
    }
    d_ptr->mCurrentFigure = fig;
    if (!fig) {
        return;
    }
    if (QUndoStack* un = fig->getUndoStack()) {
        if (!un->isActive()) {
            un->setActive(true);
        }
    }
    Q_EMIT currentFigureChanged(fig, d_ptr->mFigures.indexOf(fig));
}

/**
 * @brief dock 关闭请求处理（经 closeRequested 信号触发）
 * @param fig 对应的 figure
 */
void DAChartOperateWidget::onFigureCloseRequested(DAFigureWidget* fig)
{
    if (!fig) {
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::question(this,
                                                            tr("Question"),                             // cn:询问
                                                            tr("Whether to close the figure widget"));  // cn:是否关闭绘图窗口
    if (QMessageBox::Yes != btn) {
        return;
    }
    removeFigure(fig, true);
}

/**
 * @brief 绘图的标题改变槽函数
 * @param t
 */
void DAChartOperateWidget::onFigureTitleChanged(const QString& t)
{
    DAFigureWidget* fig = qobject_cast< DAFigureWidget* >(sender());
    if (!fig) {
        return;
    }
    if (ads::CDockWidget* dock = d_ptr->dockOfFigure(fig)) {
        // 设置 dock 窗口标题会触发 WindowTitleChange 事件，ADS 据此更新标签文本
        dock->setWindowTitle(t);
    }
    Q_EMIT figureTitleChanged(fig, t);
}

}  // end DA
