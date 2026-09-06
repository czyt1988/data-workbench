#include "DAPyWorkFlowOperateWidget.h"
// qt
#include <QAction>
#include <QActionGroup>
#include <QImage>
#include <QDebug>
#include "DALogCategory.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QPointer>
// ADS
#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
// workflow
#include "DAPyWorkFlowGraphicsView.h"
#include "DAPyWorkFlowGraphicsScene.h"
#include "DAPyWorkFlowManager.h"
#include "DAPyWorkFlow.h"
#include "DAGraphicsPixmapItem.h"
//
#include "DAPyWorkFlowEditWidget.h"
#include "DAPyWorkFlowEditWidgetDockWidget.h"
#include "DADockAreaTabPosition.h"
#include "DAPyNodeGraphicsItem.h"
#include "Commands/DACommandsForWorkFlow.h"

namespace DA
{

class DAPyWorkFlowOperateWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowOperateWidget)
public:
    PrivateData(DAPyWorkFlowOperateWidget* p);
    // 根据工作流编辑窗口获取其 dock
    ads::CDockWidget* dockOfWorkflow(DAPyWorkFlowEditWidget* wfe) const;
    // 根据 dock 获取其工作流编辑窗口
    DAPyWorkFlowEditWidget* workflowOfDock(ads::CDockWidget* dock) const;
    // 获取新工作流应加入的 dock area（取嵌套管理器内已有工作流所在 area，否则 nullptr）
    ads::CDockAreaWidget* targetAreaForNewWorkflow() const;

public:
    bool mIsShowGrid { true };
    QColor mDefaultTextColor { Qt::black };
    QFont mDefaultFont;
    bool mIsDestroying { false };
    bool mOnlyOneWorkflow { false };    ///< 设置只允许一个工作流
    bool mEnableWorkflowLink { true };  ///< 是否允许工作流连接
    ads::CDockManager* mDockManager { nullptr };                   ///< 嵌套停靠管理器
    QList< DAPyWorkFlowEditWidget* > mWorkflows;                   ///< 插入顺序，作为 index 基础
    QHash< DAPyWorkFlowEditWidget*, ads::CDockWidget* > mWfToDock;  ///< 工作流 -> dock
    QPointer< DAPyWorkFlowEditWidget > mCurrentWorkFlow;           ///< 当前激活工作流
    bool mSuppressCurrentChanged { false };                        ///< 创建/加载期间抑制 currentWorkFlowWidgetChanged
    QAction* mActionCopy { nullptr };
    QAction* mActionCut { nullptr };
    QAction* mActionPaste { nullptr };
    QAction* mActionDelete { nullptr };              ///< 删除选中
    QAction* mActionCancel { nullptr };              ///< 取消动作
    QAction* mActionSelectAll { nullptr };           ///< 全选
    QAction* mActionZoomIn { nullptr };              ///< 放大
    QAction* mActionZoomOut { nullptr };             ///< 缩小
    QAction* mActionZoomFit { nullptr };             ///< 全部显示
    QAction* actionViewCrossLineMarker { nullptr };  ///< 视图的十字标记线
    QAction* actionViewHLineMarker { nullptr };      ///< 视图的水平标记线
    QAction* actionViewVLineMarker { nullptr };      ///< 视图的垂直标记线
    QAction* actionViewNoneMarker { nullptr };       ///< 无标记线
    QActionGroup* actionGroupViewLineMarkers { nullptr };
};

DAPyWorkFlowOperateWidget::PrivateData::PrivateData(DAPyWorkFlowOperateWidget* p) : q_ptr(p)
{
}

ads::CDockWidget* DAPyWorkFlowOperateWidget::PrivateData::dockOfWorkflow(DAPyWorkFlowEditWidget* wfe) const
{
    return mWfToDock.value(wfe, nullptr);
}

DAPyWorkFlowEditWidget* DAPyWorkFlowOperateWidget::PrivateData::workflowOfDock(ads::CDockWidget* dock) const
{
    if (!dock) {
        return nullptr;
    }
    if (DAPyWorkFlowEditWidgetDockWidget* ed = qobject_cast< DAPyWorkFlowEditWidgetDockWidget* >(dock->widget())) {
        return ed->getEditWidget();
    }
    return nullptr;
}

ads::CDockAreaWidget* DAPyWorkFlowOperateWidget::PrivateData::targetAreaForNewWorkflow() const
{
    // 关键：不能用 mDockManager->focusedDockWidget()——FocusHighlighting 下嵌套管理器的焦点
    // 控制器与顶层管理器共享 window 属性（DockFocusController.cpp onApplicationFocusChanged 不
    // 校验 dock 所属管理器），用户点过顶层 dock 后 nested->focusedDockWidget() 会返回顶层 dock，
    // 用它作 target 会让新工作流被加到顶层中心区（逃逸出 DAPyWorkFlowOperateWidget）。
    // 改为从本嵌套管理器已有的工作流 dock 取 area，确保新工作流落在嵌套管理器内。
    for (int i = mWorkflows.size() - 1; i >= 0; --i) {
        if (ads::CDockWidget* d = mWfToDock.value(mWorkflows.at(i), nullptr)) {
            if (ads::CDockAreaWidget* a = d->dockAreaWidget()) {
                return a;
            }
        }
    }
    return nullptr;  // 首个工作流：在容器根创建 area
}

//===================================================
// DAPyWorkFlowOperateWidget
//===================================================
DAPyWorkFlowOperateWidget::DAPyWorkFlowOperateWidget(QWidget* parent)
    : DAAbstractOperateWidget(parent), DA_PIMPL_CONSTRUCT
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    d_ptr->mDockManager = new ads::CDockManager(this);
    lay->addWidget(d_ptr->mDockManager);
    // 禁止工作流 dock 浮动为独立窗口（全局锁，对所有当前及后续 dock 生效），保留分屏/并栏/拖拽
    d_ptr->mDockManager->lockDockWidgetFeaturesGlobally(ads::CDockWidget::DockWidgetFloatable);
    // 嵌套停靠区聚焦改变：onFocusedDockChanged 内部会过滤掉非本管理器的 dock，
    // 规避 FocusHighlighting 下嵌套焦点控制器跨管理器回调顶层 dock 的问题
    connect(d_ptr->mDockManager, &ads::CDockManager::focusedDockWidgetChanged,
            this, &DAPyWorkFlowOperateWidget::onFocusedDockChanged);
    initActions();
}

DAPyWorkFlowOperateWidget::~DAPyWorkFlowOperateWidget()
{
    d_ptr->mIsDestroying = true;
    // 断开所有子对象到 this 的信号连接，防止析构期间信号发给已析构对象
    const auto allChildren = findChildren< QObject* >();
    for (auto* obj : allChildren) {
        obj->disconnect(this);
    }
    // mDockManager 作为本部件的子对象，由 Qt 自动销毁；工作流 dock 禁止浮动，无浮动窗口需额外清理
}

/**
 * @brief 创建工作流管理器
 *
 * 子类可覆写此方法以注入自定义workflow类型（如DADataWorkFlow）。
 * 此函数会在@ref appendWorkflow 中调用。
 *
 * @return 新创建的DAPyWorkFlowManager实例指针
 */
DAPyWorkFlowManager* DAPyWorkFlowOperateWidget::createManager()
{
    return new DAPyWorkFlowManager();
}

/**
 * @brief 添加一个工作流编辑窗口
 *
 * 基于嵌套 ads::CDockManager 管理：DAPyWorkFlowEditWidget 包成
 * DAPyWorkFlowEditWidgetDockWidget 后由 ads::CDockWidget 包装加入停靠区。
 * id 非空时用作工作流持久 id 与 dock objectName，供工程反序列化时 restoreState
 * 按 objectName 匹配恢复布局。
 *
 * 此函数发射信号workflowCreated（先），也会触发currentWorkFlowWidgetChanged（后）
 * @param name 工作流名称
 * @param id 持久 id，非空时覆盖工作流默认 id（供工程反序列化恢复布局）
 */
DAPyWorkFlowEditWidget* DAPyWorkFlowOperateWidget::appendWorkflow(const QString& name, const QString& id)
{
    DA_D(d);
    if (isOnlyOneWorkflow() && d->mWorkflows.size() >= 1) {
        return nullptr;
    }
    DAPyWorkFlowEditWidget* wfe = new DAPyWorkFlowEditWidget(this);
    if (!id.isEmpty()) {
        // 工程反序列化时恢复持久 id，同时作为 dock objectName 供 restoreState 匹配
        wfe->setWorkFlowId(id);
    }
    DAPyWorkFlowManager* mgr = createManager();
    mgr->setParent(wfe);  // Manager 生命周期绑定到 EditWidget
    wfe->setManager(mgr);
    // 把undo添加进去
    wfe->setEnableShowGrid(d->mIsShowGrid);
    wfe->setDefaultTextColor(d->mDefaultTextColor);
    wfe->setDefaultTextFont(d->mDefaultFont);
    DAPyWorkFlowGraphicsScene* scene = wfe->getWorkFlowGraphicsScene();
    // 同步状态
    scene->setIgnoreLinkEvent(!isEnableWorkflowLink());

    connect(wfe, &DAPyWorkFlowEditWidget::selectNodeItemChanged, this, &DAPyWorkFlowOperateWidget::selectNodeItemChanged);
    connect(wfe, &DAPyWorkFlowEditWidget::sceneActionActived, this, &DAPyWorkFlowOperateWidget::sceneActionActived);
    connect(wfe, &DAPyWorkFlowEditWidget::sceneActionDeactived, this, &DAPyWorkFlowOperateWidget::sceneActionDeactived);
    connect(scene, &DAPyWorkFlowGraphicsScene::selectionChanged, this, &DAPyWorkFlowOperateWidget::onSelectionChanged);
    connect(scene, &DAPyWorkFlowGraphicsScene::itemsAdded, this, &DAPyWorkFlowOperateWidget::onSceneItemsAdded);
    connect(scene, &DAPyWorkFlowGraphicsScene::itemsRemoved, this, &DAPyWorkFlowOperateWidget::onSceneItemsRemoved);
    connect(wfe, &DAPyWorkFlowEditWidget::startExecute, this, [ this, wfe ]() { emit workflowStartExecute(wfe); });
    connect(wfe, &DAPyWorkFlowEditWidget::nodeExecuteFinished, this, [ this, wfe ](const DA::DAPyNode& n, bool state) {
        emit nodeExecuteFinished(wfe, n, state);
    });
    connect(wfe, &DAPyWorkFlowEditWidget::finished, this, [ this, wfe ](bool s) { emit workflowFinished(wfe, s); });
    // 信号转发：工作流标题改变 → 同步 dock 标签
    connect(wfe, &DAPyWorkFlowEditWidget::windowTitleChanged, this, &DAPyWorkFlowOperateWidget::onWorkflowTitleChanged);

    // 封装为 DAPyWorkFlowEditWidgetDockWidget（纯 QWidget，遵循项目约定，不继承 ads::CDockWidget）
    DAPyWorkFlowEditWidgetDockWidget* editDock = new DAPyWorkFlowEditWidgetDockWidget(wfe);
    // 包装进 ads::CDockWidget（使用 manager 构造以走管理器组件工厂，避免已弃用的两参构造）
    ads::CDockWidget* dock = new ads::CDockWidget(d->mDockManager, name);
    dock->setObjectName(wfe->getWorkFlowId());
    dock->setWidget(editDock, ads::CDockWidget::ForceNoScrollArea);
    // 关闭按钮触发 closeRequested 而非自动隐藏，便于弹确认框后再删除
    dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
    connect(dock, &ads::CDockWidget::closeRequested, this, [ this, wfe ]() {
        onWorkflowCloseRequested(wfe);
    });
    // 记录映射
    d->mWfToDock[ wfe ] = dock;
    d->mWorkflows.append(wfe);
    // 抑制聚焦改变，避免 addDockWidget 触发 currentWorkFlowWidgetChanged（保留原不变量：创建只发 workflowCreated）
    d->mSuppressCurrentChanged = true;
    ads::CDockAreaWidget* area = d->targetAreaForNewWorkflow();
    if (area) {
        // 默认以标签形式加入当前聚焦 dock 所在 area
        d->mDockManager->addDockWidgetTabToArea(dock, area);
    } else {
        // 首个工作流：在容器根创建 dock area
        d->mDockManager->addDockWidget(ads::CenterDockWidgetArea, dock);
    }
    d->mSuppressCurrentChanged = false;
    // 把名字保存到DAPyWorkFlowEditWidget中，在DAProject保存的时候会用到
    wfe->setWindowTitle(name);
    emit workflowCreated(wfe);
    // 新建即激活（替代旧 ui->tabWidget->setCurrentIndex）
    setCurrentWorkflowWidget(wfe);

    return wfe;
}

/**
 * @brief 创建一个新的工作流窗口
 * @note 此函数带有交互
 * @return
 */
DAPyWorkFlowEditWidget* DAPyWorkFlowOperateWidget::appendWorkflowWithDialog()
{
    bool ok = false;
    QString text = QInputDialog::getText(this, tr("Title of new workflow"),  // cn:新工作流标题
                                         tr("Title:"),                       // cn:标题:
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || text.isEmpty()) {
        return nullptr;
    }
    return appendWorkflow(text);
}

/**
 * @brief 获取当前工作流的索引
 * @return
 */
int DAPyWorkFlowOperateWidget::getCurrentWorkflowIndex() const
{
    return d_ptr->mWorkflows.indexOf(d_ptr->mCurrentWorkFlow);
}

/**
 * @brief 设置当前的工作流
 * @param index
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflow(int index)
{
    setCurrentWorkflowWidget(d_ptr->mWorkflows.value(index, nullptr));
}

/**
 * @brief 获取当前工作流管理器
 *
 * @return 当前工作流编辑器中的Manager指针，无活动工作流时返回nullptr
 */
DAPyWorkFlowManager* DAPyWorkFlowOperateWidget::getCurrentManager() const
{
    if (auto w = getCurrentWorkFlowWidget()) {
        return w->getManager();
    }
    return nullptr;
}

/**
 * @brief 获取当前的工作流（兼容方法）
 *
 * 内部通过getCurrentManager()获取Manager后返回其workflow。
 * 供DAAbstractNodePlugin等外部调用。
 *
 * @return 当前DAPyWorkFlow
 */
DAPyWorkFlow DAPyWorkFlowOperateWidget::getCurrentWorkflow() const
{
    if (auto mgr = getCurrentManager()) {
        return mgr->getWorkflow();
    }
    return DAPyWorkFlow();
}

/**
 * @brief 设置当前的页面
 *
 * raise() 把 dock 置为当前标签（浮动则 raise 窗口，已禁浮动故仅标签）。
 * raise 可能不触发 focusedDockWidgetChanged（如同 area 内已是当前），主动同步 mCurrentWorkFlow。
 * @param wf
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflowWidget(DAPyWorkFlowEditWidget* wf)
{
    if (!wf) {
        return;
    }
    ads::CDockWidget* dock = d_ptr->dockOfWorkflow(wf);
    if (!dock) {
        return;
    }
    dock->raise();
    if (d_ptr->mCurrentWorkFlow == wf) {
        return;
    }
    d_ptr->mCurrentWorkFlow = wf;
    if (auto un = wf->getUndoStack()) {
        if (!un->isActive()) {
            un->setActive(true);
        }
    }
    syncLineMarkerActionForView(wf->getWorkFlowGraphicsView());
    emit currentWorkFlowWidgetChanged(wf);
}

/**
 * @brief 获取当前选中的工作流窗口
 * @return
 */
DAPyWorkFlowEditWidget* DAPyWorkFlowOperateWidget::getCurrentWorkFlowWidget() const
{
    // 返回追踪的当前工作流（QPointer 在工作流销毁后自动置空）
    if (d_ptr->mCurrentWorkFlow) {
        return d_ptr->mCurrentWorkFlow;
    }
    // 回退到最近创建的工作流（不使用 focusedDockWidget，见 targetAreaForNewWorkflow 注释）
    if (!d_ptr->mWorkflows.isEmpty()) {
        return d_ptr->mWorkflows.constLast();
    }
    return nullptr;
}

void DAPyWorkFlowOperateWidget::setCurrentWorkflowName(const QString& name)
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (w) {
        // setWindowTitle → windowTitleChanged → onWorkflowTitleChanged 同步 dock 标签
        w->setWindowTitle(name);
    }
}

/**
 * @brief 获取所有的工作流编辑窗口
 * @return
 */
QList< DAPyWorkFlowEditWidget* > DAPyWorkFlowOperateWidget::getAllWorkFlowWidgets() const
{
    return d_ptr->mWorkflows;
}

/**
 * @brief 获取scene
 * @return
 */
DAPyWorkFlowGraphicsScene* DAPyWorkFlowOperateWidget::getCurrentWorkFlowScene() const
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        return nullptr;
    }
    return w->getWorkFlowGraphicsScene();
}

/**
 * @brief 获取所有的工作流窗口
 * @return
 */
QList< DAPyWorkFlowGraphicsScene* > DAPyWorkFlowOperateWidget::getAllWorkFlowScene() const
{
    QList< DAPyWorkFlowGraphicsScene* > res;
    for (DAPyWorkFlowEditWidget* we : std::as_const(d_ptr->mWorkflows)) {
        if (we) {
            if (DAPyWorkFlowGraphicsScene* sc = we->getWorkFlowGraphicsScene()) {
                res.append(sc);
            }
        }
    }
    return res;
}

/**
 * @brief 获取当前视图
 * @return
 */
DAPyWorkFlowGraphicsView* DAPyWorkFlowOperateWidget::getCurrentWorkFlowView() const
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        return nullptr;
    }
    return w->getWorkFlowGraphicsView();
}

/**
 * @brief 获取工作流窗口
 * @param index
 * @return 如果超出索引范围返回nullptr
 */
DAPyWorkFlowEditWidget* DAPyWorkFlowOperateWidget::getWorkFlowWidget(int index) const
{
    return d_ptr->mWorkflows.value(index, nullptr);
}

/**
 * @brief 按 id 查找工作流窗口
 * @param id 工作流持久 id
 * @return 匹配的工作流编辑窗口，未找到返回 nullptr
 */
DAPyWorkFlowEditWidget* DAPyWorkFlowOperateWidget::findWorkFlowWidget(const QString& id) const
{
    if (id.isEmpty()) {
        return nullptr;
    }
    for (DAPyWorkFlowEditWidget* wfe : std::as_const(d_ptr->mWorkflows)) {
        if (wfe && wfe->getWorkFlowId() == id) {
            return wfe;
        }
    }
    return nullptr;
}

/**
 * @brief 获取工作流窗口的名称
 * @param index
 * @return
 */
QString DAPyWorkFlowOperateWidget::getWorkFlowWidgetName(int index) const
{
    DAPyWorkFlowEditWidget* w = getWorkFlowWidget(index);
    return w ? w->windowTitle() : QString();
}

/**
 * @brief 给工作流重命名
 * @param index
 * @param name
 */
void DAPyWorkFlowOperateWidget::renameWorkFlowWidget(int index, const QString& name)
{
    DAPyWorkFlowEditWidget* w = getWorkFlowWidget(index);
    if (w) {
        // setWindowTitle → windowTitleChanged → onWorkflowTitleChanged 同步 dock 标签
        w->setWindowTitle(name);
    }
}

/**
 * @brief 获取编辑窗口数量
 * @return
 */
int DAPyWorkFlowOperateWidget::count() const
{
    return d_ptr->mWorkflows.size();
}

/**
 * @brief 实际移除工作流（不发确认框，不发 workflowRemoving）
 *
 * 供 removeWorkflow / onWorkflowCloseRequested / clear 共用，承担 dock 拆除与映射清理。
 * @param wfe 工作流编辑窗口
 * @param deleteWidget 是否级联删除工作流及其封装，默认 true
 */
void DAPyWorkFlowOperateWidget::removeWorkflowNoConfirm(DAPyWorkFlowEditWidget* wfe, bool deleteWidget)
{
    if (!wfe) {
        return;
    }
    ads::CDockWidget* dock = d_ptr->dockOfWorkflow(wfe);
    if (!dock) {
        return;  // 不由本部件管理
    }
    d_ptr->mWorkflows.removeAll(wfe);
    d_ptr->mWfToDock.remove(wfe);
    if (d_ptr->mCurrentWorkFlow == wfe) {
        d_ptr->mCurrentWorkFlow = nullptr;  // 回退交由 getCurrentWorkFlowWidget
    }
    if (d_ptr->mDockManager) {
        d_ptr->mDockManager->removeDockWidget(dock);
    }
    if (deleteWidget) {
        // 级联删除 editDock + wfe，延迟到事件循环（与原 deleteLater 语义一致）
        dock->deleteLater();
    }
}

/**
 * @brief 移除工作流
 * @param index
 */
void DAPyWorkFlowOperateWidget::removeWorkflow(int index)
{
    DAPyWorkFlowEditWidget* wfe = getWorkFlowWidget(index);
    if (nullptr == wfe) {
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::question(
        this,
        tr("Question"),                                                        // cn:疑问
        tr("Confirm to delete workflow:%1").arg(getWorkFlowWidgetName(index))  // cn:是否确认删除工作流:%1
    );
    if (btn != QMessageBox::Yes) {
        return;
    }
    // 发射移除信号
    emit workflowRemoving(wfe);
    removeWorkflowNoConfirm(wfe, true);
}

/**
 * @brief 激活当前的回退功能
 */
void DAPyWorkFlowOperateWidget::setUndoStackActive()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (w) {
        w->setUndoStackActive();
    }
}

/**
 * @brief 是否显示网格
 * @return
 */
bool DAPyWorkFlowOperateWidget::isCurrentWorkflowShowGrid() const
{
    DAPyWorkFlowGraphicsScene* sc = getCurrentWorkFlowScene();
    if (!sc) {
        return false;
    }
    return sc->isShowGridLine();
}

/**
 * @brief 获取undostack
 * @return
 */
QUndoStack* DAPyWorkFlowOperateWidget::getUndoStack()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (w) {
        return w->getUndoStack();
    }
    return nullptr;
}

void DAPyWorkFlowOperateWidget::addBackgroundPixmap(const QString& pixmapPath)
{
    DAPyWorkFlowGraphicsScene* s = getCurrentWorkFlowScene();
    if (nullptr == s) {
        return;
    }
    QImage img(pixmapPath);
    QPixmap px;
    px.convertFromImage(img);
    DAGraphicsPixmapItem* item = s->setBackgroundPixmap(px);
    item->setSelectable(true);
    item->setMoveable(true);
}

void DAPyWorkFlowOperateWidget::setBackgroundPixmapLock(bool on)
{
    DAPyWorkFlowGraphicsScene* s = getCurrentWorkFlowScene();
    if (nullptr == s) {
        return;
    }
    DAGraphicsPixmapItem* item = s->getBackgroundPixmapItem();
    if (nullptr == item) {
        return;
    }
    item->setSelectable(!on);
    item->setMoveable(!on);
}

void DAPyWorkFlowOperateWidget::setSelectTextColor(const QColor& color)
{
    DAPyWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectTextColor(color);
    }
}

void DAPyWorkFlowOperateWidget::setSelectShapeBackgroundBrush(const QBrush& b)
{
    DAPyWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectShapeBackgroundBrush(b);
    }
}

void DAPyWorkFlowOperateWidget::setSelectShapeBorderPen(const QPen& v)
{
    DAPyWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectShapeBorderPen(v);
    }
}

void DAPyWorkFlowOperateWidget::setSelectTextFont(const QFont& f)
{
    DAPyWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectTextItemFont(f);
    }
}

/**
 * @brief 设置当前工作流的网格显示与否
 * @param on
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflowShowGrid(bool on)
{
    DAPyWorkFlowGraphicsScene* sc = getCurrentWorkFlowScene();
    if (!sc) {
        return;
    }
    sc->showGridLine(on);
    sc->update();
    d_ptr->mIsShowGrid = on;  // 记录最后的状态
}

/**
 * @brief 设置当前工作流锁定
 * @param on
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflowReadOnly(bool on)
{
    DAPyWorkFlowGraphicsScene* sc = getCurrentWorkFlowScene();
    if (!sc) {
        return;
    }
    sc->setReadOnly(on);
}

/**
 * @brief 设置当前工作流全部显示
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflowWholeView()
{
    DAPyWorkFlowGraphicsView* view = getCurrentWorkFlowView();
    if (!view) {
        daWarning << tr("Missing view");  // cn:缺少视图
        return;
    }
    view->zoomFit();
}

/**
 * @brief 放大
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflowZoomIn()
{
    DAPyWorkFlowGraphicsView* view = getCurrentWorkFlowView();
    if (!view) {
        daWarning << tr("Missing view");  // cn:缺少视图
        return;
    }
    view->zoomIn();
}

/**
 * @brief 缩小
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflowZoomOut()
{
    DAPyWorkFlowGraphicsView* view = getCurrentWorkFlowView();
    if (!view) {
        daWarning << tr("Missing view");  // cn:缺少视图
        return;
    }
    view->zoomOut();
}

/**
 * @brief 全选
 */
void DAPyWorkFlowOperateWidget::setCurrentWorkflowSelectAll()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    w->selectAll();
}

/**
 * @brief 运行工作流
 */
void DAPyWorkFlowOperateWidget::runCurrentWorkFlow()
{
    DAPyWorkFlowManager* mgr = getCurrentManager();
    if (nullptr == mgr) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    if (!mgr->executeWorkflow()) {
        daCritical << tr("Workflow execution failed");  // cn:工作流执行失败
    }
}

/**
 * @brief 终止当前工作流
 */
void DAPyWorkFlowOperateWidget::terminateCurrentWorkFlow()
{
    DAPyWorkFlowManager* mgr = getCurrentManager();
    if (nullptr == mgr) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    // TODO: 工作流终止功能还未实现
    daWarning << tr("Workflow termination has not been implemented yet");  // cn:工作流终止功能尚未实现
}

/**
 * @brief 复制当前选中的items
 */
void DAPyWorkFlowOperateWidget::copyCurrentSelectItems()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    w->copySelectItems();
}

/**
 * @brief 剪切当前选中的items
 */
void DAPyWorkFlowOperateWidget::cutCurrentSelectItems()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    w->cutSelectItems();
}

/**
 * @brief ctrl+v动作
 */
void DAPyWorkFlowOperateWidget::pasteFromClipBoard()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    w->pasteToViewCenter();
}

/**
 * @brief 删除当前的item
 */
void DAPyWorkFlowOperateWidget::removeCurrentSelectItems()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    w->removeSelectItems();
}

/**
 * @brief 当前的wf执行取消动作
 */
void DAPyWorkFlowOperateWidget::cancelCurrent()
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return;
    }
    w->cancel();
}

/**
 * @brief 设置是否允许连接
 * @param on
 */
void DAPyWorkFlowOperateWidget::setEnableWorkflowLink(bool on)
{
    d_ptr->mEnableWorkflowLink = on;
    iteratorScene([ on ](DAPyWorkFlowGraphicsScene* sc) -> bool {
        sc->setIgnoreLinkEvent(!on);
        return true;
    });
}

/**
 * @brief 是否允许连接
 * @return
 */
bool DAPyWorkFlowOperateWidget::isEnableWorkflowLink() const
{
    return d_ptr->mEnableWorkflowLink;
}

/**
 * @brief 文本字体
 * @param c
 */
QFont DAPyWorkFlowOperateWidget::getDefaultTextFont() const
{
    return d_ptr->mDefaultFont;
}
/**
 * @brief 设置文本字体
 * @param c
 */
void DAPyWorkFlowOperateWidget::setDefaultTextFont(const QFont& f)
{
    d_ptr->mDefaultFont = f;
    iteratorScene([ f ](DAPyWorkFlowGraphicsScene* sc) -> bool {
        sc->setDefaultTextFont(f);
        return true;
    });
}
/**
 * @brief 文本颜色
 * @param c
 */
QColor DAPyWorkFlowOperateWidget::getDefaultTextColor() const
{
    return d_ptr->mDefaultTextColor;
}
/**
 * @brief 设置默认的文本颜色
 * @param c
 */
void DAPyWorkFlowOperateWidget::setDefaultTextColor(const QColor& c)
{
    d_ptr->mDefaultTextColor = c;
    iteratorScene([ c ](DAPyWorkFlowGraphicsScene* sc) -> bool {
        sc->setDefaultTextColor(c);
        return true;
    });
}

/**
 * @brief 嵌套停靠区聚焦 dock 改变
 *
 * FocusHighlighting 下嵌套管理器的 CDockFocusController 与顶层管理器共享 window 属性，
 * 用户聚焦顶层 dock（如数据操作）时本信号也会被回调到顶层 dock。这里通过
 * nowDock->dockManager() 过滤，只处理属于本嵌套管理器的工作流 dock，避免
 * mCurrentWorkFlow 被误置空/误切换。
 * @param oldDock 旧聚焦 dock
 * @param nowDock 新聚焦 dock
 */
void DAPyWorkFlowOperateWidget::onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock)
{
    Q_UNUSED(oldDock);
    if (d_ptr->mIsDestroying || d_ptr->mSuppressCurrentChanged) {
        return;
    }
    // 过滤掉非本嵌套管理器的 dock（顶层 dock 的跨管理器回调）
    if (nowDock && nowDock->dockManager() != d_ptr->mDockManager) {
        return;
    }
    DAPyWorkFlowEditWidget* wfe = d_ptr->workflowOfDock(nowDock);
    if (d_ptr->mCurrentWorkFlow == wfe) {
        return;  // 未变化，避免重复发射
    }
    d_ptr->mCurrentWorkFlow = wfe;
    if (!wfe) {
        return;
    }
    if (auto un = wfe->getUndoStack()) {
        if (!un->isActive()) {
            un->setActive(true);
        }
    }
    syncLineMarkerActionForView(wfe->getWorkFlowGraphicsView());
    emit currentWorkFlowWidgetChanged(wfe);
}

/**
 * @brief dock 关闭请求处理（经 closeRequested 信号触发）
 * @param wfe 对应的工作流编辑窗口
 */
void DAPyWorkFlowOperateWidget::onWorkflowCloseRequested(DAPyWorkFlowEditWidget* wfe)
{
    if (!wfe) {
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::question(this,
                                                            tr("Question"),                          // cn:疑问
                                                            tr("Confirm to close workflow"));  // cn:是否确认关闭工作流
    if (QMessageBox::Yes != btn) {
        return;
    }
    emit workflowRemoving(wfe);
    removeWorkflowNoConfirm(wfe, true);
}

/**
 * @brief 工作流标题改变槽函数
 * @param t
 */
void DAPyWorkFlowOperateWidget::onWorkflowTitleChanged(const QString& t)
{
    DAPyWorkFlowEditWidget* wfe = qobject_cast< DAPyWorkFlowEditWidget* >(sender());
    if (!wfe) {
        return;
    }
    if (ads::CDockWidget* dock = d_ptr->dockOfWorkflow(wfe)) {
        // 设置 dock 窗口标题会触发 WindowTitleChange 事件，ADS 据此更新标签文本
        dock->setWindowTitle(t);
    }
}

/**
 * @brief 把当前视图的标记线样式同步到 line-marker action group 的选中状态
 * @param view 当前工作流视图，空则跳过
 */
void DAPyWorkFlowOperateWidget::syncLineMarkerActionForView(DAPyWorkFlowGraphicsView* view)
{
    if (!view) {
        return;
    }
    auto markerStyle = view->getCurrentMarkerStyle();
    switch (markerStyle) {
    case DAGraphicsViewOverlayMouseMarker::CrossLine:
        d_ptr->actionViewCrossLineMarker->setChecked(true);
        break;
    case DAGraphicsViewOverlayMouseMarker::VLine:
        d_ptr->actionViewVLineMarker->setChecked(true);
        break;
    case DAGraphicsViewOverlayMouseMarker::HLine:
        d_ptr->actionViewHLineMarker->setChecked(true);
        break;
    case DAGraphicsViewOverlayMouseMarker::NoMarkerStyle:
        d_ptr->actionViewNoneMarker->setChecked(true);
        break;
    default:
        break;
    }
}

/**
 * @brief 场景条目选择变化触发的槽
 */
void DAPyWorkFlowOperateWidget::onSelectionChanged()
{
    if (d_ptr->mIsDestroying) {
        return;
    }
    DAPyWorkFlowGraphicsScene* scene = getCurrentWorkFlowScene();
    if (nullptr == scene) {
        return;
    }
    QList< QGraphicsItem* > sits = scene->selectedItems();
    if (sits.isEmpty()) {
        return;
    }
    emit selectionItemChanged(sits.last());
}

void DAPyWorkFlowOperateWidget::onSceneItemsAdded(const QList< QGraphicsItem* >& its)
{
    DAGraphicsScene* sc = qobject_cast< DAGraphicsScene* >(sender());
    if (sc) {
        emit itemsAdded(sc, its);
    }
}

void DAPyWorkFlowOperateWidget::onSceneItemsRemoved(const QList< QGraphicsItem* >& its)
{
    DAGraphicsScene* sc = qobject_cast< DAGraphicsScene* >(sender());
    if (sc) {
        emit itemsRemoved(sc, its);
    }
}

void DAPyWorkFlowOperateWidget::onActionGroupViewLineMarkersTriggered(QAction* act)
{
    if (act == d_ptr->actionViewCrossLineMarker) {
        setCurrentViewLineMarker(act->isChecked() ? DAGraphicsViewOverlayMouseMarker::CrossLine
                                                  : DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
    } else if (act == d_ptr->actionViewHLineMarker) {
        setCurrentViewLineMarker(act->isChecked() ? DAGraphicsViewOverlayMouseMarker::HLine
                                                  : DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
    } else if (act == d_ptr->actionViewVLineMarker) {
        setCurrentViewLineMarker(act->isChecked() ? DAGraphicsViewOverlayMouseMarker::VLine
                                                  : DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
    } else if (act == d_ptr->actionViewNoneMarker) {
        setCurrentViewLineMarker(DAGraphicsViewOverlayMouseMarker::NoMarkerStyle);
    }
}

QList< DAGraphicsStandardTextItem* > DAPyWorkFlowOperateWidget::getSelectTextItems()
{
    QList< DAGraphicsStandardTextItem* > res;
    DAPyWorkFlowGraphicsScene* secen = getCurrentWorkFlowScene();
    if (nullptr == secen) {
        return res;
    }
    QList< QGraphicsItem* > its = secen->selectedItems();
    if (its.size() == 0) {
        return res;
    }
    for (QGraphicsItem* item : std::as_const(its)) {
        if (DAGraphicsStandardTextItem* textItem = dynamic_cast< DAGraphicsStandardTextItem* >(item)) {
            res.append(textItem);
        }
    }
    return res;
}

void DAPyWorkFlowOperateWidget::initActions()
{
    DA_D(d);

    d->mActionCopy = new QAction(this);
    d->mActionCopy->setObjectName(QStringLiteral("actionCopyToDAPyWorkFlowOperateWidget"));
    d->mActionCopy->setIcon(QIcon(QStringLiteral(":/DAGui/icon/copy.svg")));
    d->mActionCopy->setShortcuts(QKeySequence::Copy);
    connect(d->mActionCopy, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::copyCurrentSelectItems);

    d->mActionCut = new QAction(this);
    d->mActionCut->setObjectName(QStringLiteral("actionCutToDAPyWorkFlowOperateWidget"));
    d->mActionCut->setIcon(QIcon(QStringLiteral(":/DAGui/icon/cut.svg")));
    d->mActionCut->setShortcuts(QKeySequence::Cut);
    connect(d->mActionCut, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::cutCurrentSelectItems);

    d->mActionPaste = new QAction(this);
    d->mActionPaste->setObjectName(QStringLiteral("actionPasteToDAPyWorkFlowOperateWidget"));
    d->mActionPaste->setIcon(QIcon(QStringLiteral(":/DAGui/icon/paste.svg")));
    d->mActionPaste->setShortcuts(QKeySequence::Paste);
    connect(d->mActionPaste, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::pasteFromClipBoard);

    d->mActionDelete = new QAction(this);
    d->mActionDelete->setObjectName(QStringLiteral("actionDeleteToDAPyWorkFlowOperateWidget"));
    d->mActionDelete->setIcon(QIcon(QStringLiteral(":/DAGui/icon/delete.svg")));
    d->mActionDelete->setShortcuts(QKeySequence::Delete);
    connect(d->mActionDelete, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::removeCurrentSelectItems);

    d->mActionCancel = new QAction(this);
    d->mActionCancel->setObjectName(QStringLiteral("actionCancelToDAPyWorkFlowOperateWidget"));
    d->mActionCancel->setIcon(QIcon(QStringLiteral(":/DAGui/icon/cancel.svg")));
    d->mActionCancel->setShortcuts(QKeySequence::Cancel);
    connect(d->mActionCancel, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::cancelCurrent);

    d->mActionSelectAll = new QAction(this);
    d->mActionSelectAll->setObjectName(QStringLiteral("actionSelectAllToDAPyWorkFlowOperateWidget"));
    d->mActionSelectAll->setIcon(QIcon(QStringLiteral(":/DAGui/icon/select-all.svg")));
    d->mActionSelectAll->setShortcuts(QKeySequence::SelectAll);
    connect(d->mActionSelectAll, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::setCurrentWorkflowSelectAll);

    d->mActionZoomIn = new QAction(this);
    d->mActionZoomIn->setObjectName(QStringLiteral("actionZoomInToDAPyWorkFlowOperateWidget"));
    d->mActionZoomIn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/zoomIn.svg")));
    d->mActionZoomIn->setShortcuts({ QKeySequence(QKeySequence::ZoomIn), QKeySequence(QStringLiteral("CTRL+=")) });
    connect(d->mActionZoomIn, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::setCurrentWorkflowZoomIn);

    d->mActionZoomOut = new QAction(this);
    d->mActionZoomOut->setObjectName(QStringLiteral("actionZoomOutToDAPyWorkFlowOperateWidget"));
    d->mActionZoomOut->setIcon(QIcon(QStringLiteral(":/DAGui/icon/zoomOut.svg")));
    d->mActionZoomOut->setShortcuts(QKeySequence::ZoomOut);
    connect(d->mActionZoomOut, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::setCurrentWorkflowZoomOut);

    // 缩放到适合屏幕
    d->mActionZoomFit = new QAction(this);
    d->mActionZoomFit->setObjectName(QStringLiteral("actionZoomFullToDAPyWorkFlowOperateWidget"));
    d->mActionZoomFit->setIcon(QIcon(QStringLiteral(":/DAGui/icon/viewAll.svg")));
    d->mActionZoomFit->setShortcut(QKeySequence(QStringLiteral("CTRL+0")));
    connect(d->mActionZoomFit, &QAction::triggered, this, &DAPyWorkFlowOperateWidget::setCurrentWorkflowWholeView);

    // 十字标记线
    d->actionViewCrossLineMarker = new QAction(this);
    d->actionViewCrossLineMarker->setCheckable(true);
    d->actionViewCrossLineMarker->setChecked(false);
    d->actionViewCrossLineMarker->setObjectName(QStringLiteral("actionViewCrossLineMarker"));
    d->actionViewCrossLineMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-corss-marker.svg")));

    // 水平标记线
    d->actionViewHLineMarker = new QAction(this);
    d->actionViewHLineMarker->setCheckable(true);
    d->actionViewHLineMarker->setChecked(false);
    d->actionViewHLineMarker->setObjectName(QStringLiteral("actionViewHLineMarker"));
    d->actionViewHLineMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-hline-marker.svg")));

    // 竖直标记线
    d->actionViewVLineMarker = new QAction(this);
    d->actionViewVLineMarker->setCheckable(true);
    d->actionViewVLineMarker->setChecked(false);
    d->actionViewVLineMarker->setObjectName(QStringLiteral("actionViewVLineMarker"));
    d->actionViewVLineMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-vline-marker.svg")));

    // 无标记线
    d->actionViewNoneMarker = new QAction(this);
    d->actionViewNoneMarker->setCheckable(true);
    d->actionViewNoneMarker->setChecked(false);
    d->actionViewNoneMarker->setObjectName(QStringLiteral("actionViewNoneMarker"));
    d->actionViewNoneMarker->setIcon(QIcon(QStringLiteral(":/DAGui/icon/view-none-marker.svg")));

    d->actionGroupViewLineMarkers = new QActionGroup(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    d->actionGroupViewLineMarkers->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
#endif
    d->actionGroupViewLineMarkers->setExclusive(true);
    d->actionGroupViewLineMarkers->addAction(d->actionViewCrossLineMarker);
    d->actionGroupViewLineMarkers->addAction(d->actionViewHLineMarker);
    d->actionGroupViewLineMarkers->addAction(d->actionViewVLineMarker);
    d->actionGroupViewLineMarkers->addAction(d->actionViewNoneMarker);

    connect(d->actionGroupViewLineMarkers,
            &QActionGroup::triggered,
            this,
            &DAPyWorkFlowOperateWidget::onActionGroupViewLineMarkersTriggered);

    addAction(d->mActionCopy);
    addAction(d->mActionCut);
    addAction(d->mActionPaste);
    addAction(d->mActionDelete);
    addAction(d->mActionCancel);
    addAction(d->mActionSelectAll);
    addAction(d->mActionZoomIn);
    addAction(d->mActionZoomOut);
    addAction(d->mActionZoomFit);
    addAction(d->actionViewCrossLineMarker);
    addAction(d->actionViewHLineMarker);
    addAction(d->actionViewVLineMarker);
    addAction(d->actionViewNoneMarker);
    retranslateUi();
}

void DAPyWorkFlowOperateWidget::retranslateUi()
{
    DA_D(d);
    d->mActionCopy->setText(tr("Copy"));                                   // cn:复制
    d->mActionCopy->setStatusTip(tr("Copy"));                              // cn:复制
    d->mActionCut->setText(tr("Cut"));                                     // cn:剪切
    d->mActionCut->setStatusTip(tr("Cut"));                                // cn:剪切
    d->mActionPaste->setText(tr("Paste"));                                 // cn:粘贴
    d->mActionPaste->setStatusTip(tr("Paste"));                            // cn:粘贴
    d->mActionDelete->setText(tr("Delete"));                               // cn:删除
    d->mActionDelete->setStatusTip(tr("Delete"));                          // cn:删除
    d->mActionCancel->setText(tr("Cancel"));                               // cn:取消
    d->mActionCancel->setStatusTip(tr("Cancel"));                          // cn:取消
    d->mActionSelectAll->setText(tr("Select All"));                        // cn:全选
    d->mActionSelectAll->setStatusTip(tr("Select all items"));             // cn:全选所有图元
    d->mActionZoomIn->setText(tr("Zoom In"));                              // cn:放大
    d->mActionZoomIn->setStatusTip(tr("Zoom in graphics view"));           // cn:放大画布
    d->mActionZoomOut->setText(tr("Zoom Out"));                            // cn:缩小
    d->mActionZoomOut->setStatusTip(tr("Zoom out graphics view"));         // cn:缩小画布
    d->mActionZoomFit->setText(tr("Zoom to Fit"));                         // cn:适合屏幕
    d->mActionZoomFit->setStatusTip(tr("Zoom to fit screen size"));        // cn:缩放到适合屏幕大小
    d->actionViewCrossLineMarker->setText(tr("Cross Line Marker"));        // cn:十字标记线
    d->actionViewCrossLineMarker->setStatusTip(tr("Cross Line Marker"));   // cn:十字标记线
    d->actionViewHLineMarker->setText(tr("Horizontal Line Marker"));       // cn:水平标记线
    d->actionViewHLineMarker->setStatusTip(tr("Horizontal Line Marker"));  // cn:水平标记线
    d->actionViewVLineMarker->setText(tr("Vertical Line Marker"));         // cn:垂直标记线
    d->actionViewVLineMarker->setStatusTip(tr("Vertical Line Marker"));    // cn:垂直标记线
    d->actionViewNoneMarker->setText(tr("None Marker"));                   // cn:无标记线
    d->actionViewNoneMarker->setStatusTip(tr("None Marker"));              // cn:无标记线
}

bool DAPyWorkFlowOperateWidget::isOnlyOneWorkflow() const
{
    return d_ptr->mOnlyOneWorkflow;
}

void DAPyWorkFlowOperateWidget::setOnlyOneWorkflow(bool v)
{
    d_ptr->mOnlyOneWorkflow = v;
}

/**
 * @brief 获取窗口内置的action，一般这个函数用来把action设置到工具栏或者菜单中
 * @param act
 * @return
 */
QAction* DAPyWorkFlowOperateWidget::getInnerAction(DAPyWorkFlowOperateWidget::InnerActions act)
{
    switch (act) {
    case ActionCopy:
        return d_ptr->mActionCopy;
    case ActionCut:
        return d_ptr->mActionCut;
    case ActionPaste:
        return d_ptr->mActionPaste;
    case ActionDelete:
        return d_ptr->mActionDelete;
    case ActionCancel:
        return d_ptr->mActionCancel;
    case ActionSelectAll:
        return d_ptr->mActionSelectAll;
    case ActionZoomIn:
        return d_ptr->mActionZoomIn;
    case ActionZoomOut:
        return d_ptr->mActionZoomOut;
    case ActionZoomFit:
        return d_ptr->mActionZoomFit;
    case ActionCrossLineMarker:
        return d_ptr->actionViewCrossLineMarker;
    case ActionHLineMarker:
        return d_ptr->actionViewHLineMarker;
    case ActionVLineMarker:
        return d_ptr->actionViewVLineMarker;
    case ActionNoneMarker:
        return d_ptr->actionViewNoneMarker;
    default:
        break;
    }
    return nullptr;
}

/**
 * @brief 迭代场景操作
 * @param fp 函数指：bool(DAPyWorkFlowGraphicsScene*)，返回false代表迭代结束，返回true，代表迭代继续
 * @sa FpScenesOpt
 */
void DAPyWorkFlowOperateWidget::iteratorScene(FpScenesOpt fp)
{
    const QList< DAPyWorkFlowGraphicsScene* > secens = getAllWorkFlowScene();
    for (DAPyWorkFlowGraphicsScene* sc : secens) {
        if (!fp(sc)) {
            return;
        }
    }
}

/**
 * @brief 设置当前视图的标记线
 * @param s
 */
void DAPyWorkFlowOperateWidget::setCurrentViewLineMarker(DAGraphicsViewOverlayMouseMarker::MarkerStyle s)
{
    DAPyWorkFlowGraphicsView* v = getCurrentWorkFlowView();
    if (!v) {
        return;
    }
    v->setViewMarkerStyle(s);
}

QActionGroup* DAPyWorkFlowOperateWidget::getLineMarkerActionGroup() const
{
    return d_ptr->actionGroupViewLineMarkers;
}

/**
 * @brief 设置鼠标动作
 *
 * 一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
 * @param mf 鼠标动作
 * @param continuous 是否连续执行
 */
bool DAPyWorkFlowOperateWidget::setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::SceneActionFlag mf)
{
    DAPyWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        daWarning << tr("No active workflow detected");  // cn:未检测到激活的工作流
        return false;
    }
    w->setPreDefineSceneAction(mf);
    return true;
}

/**
 * @brief 清空
 * @note 此函数会发射@ref workflowClearing 信号，不会逐个发射 workflowRemoving
 */
void DAPyWorkFlowOperateWidget::clear()
{
    emit workflowClearing();
    // 拷贝后遍历，removeWorkflowNoConfirm 会修改 mWorkflows
    const QList< DAPyWorkFlowEditWidget* > wfes = d_ptr->mWorkflows;
    for (DAPyWorkFlowEditWidget* wfe : std::as_const(wfes)) {
        removeWorkflowNoConfirm(wfe, true);
    }
}

/**
 * @brief 保存嵌套停靠区布局（供工程序列化）
 *
 * 顶层 ads::CDockManager::saveState 不会捕获嵌套管理器的布局，故需单独保存
 * @return 布局状态字节数组，无停靠区时返回空
 */
QByteArray DAPyWorkFlowOperateWidget::saveWorkFlowLayout() const
{
    if (!d_ptr->mDockManager) {
        return QByteArray();
    }
    return d_ptr->mDockManager->saveState();
}

/**
 * @brief 恢复嵌套停靠区布局（供工程反序列化）
 *
 * 调用前需先按保存顺序 appendWorkflow(name, id) 重建所有工作流 dock（objectName=id），
 * restoreState 按 objectName 重新挂接布局。state 为空或匹配失败返回 false（保持默认标签顺序）
 * @param state saveWorkFlowLayout 返回的状态
 * @return 恢复成功返回 true
 */
bool DAPyWorkFlowOperateWidget::restoreWorkFlowLayout(const QByteArray& state)
{
    if (!d_ptr->mDockManager || state.isEmpty()) {
        return false;
    }
    return d_ptr->mDockManager->restoreState(state);
}

/**
 * @brief 设置内部嵌套停靠区标签页方位
 *
 * 通过 DA::DADockAreaTabPosition 实现：重定位已有停靠区并绑定 dockAreaCreated
 * 信号，运行期新建的工作流页停靠区（含拖拽拆分、布局恢复）自动应用该方位。
 * @param atBottom true=底部，false=顶部
 */
void DAPyWorkFlowOperateWidget::setInnerDockTabsAtBottom(bool atBottom)
{
    DA::DADockAreaTabPosition::applyToDockManager(d_ptr->mDockManager, atBottom);
}

/**
 * @brief 获取所有工作流的名字
 * @return
 */
QList< QString > DAPyWorkFlowOperateWidget::getAllWorkflowNames() const
{
    QList< QString > names;
    for (DAPyWorkFlowEditWidget* wfe : std::as_const(d_ptr->mWorkflows)) {
        if (wfe) {
            names.append(wfe->windowTitle());
        }
    }
    return names;
}

}
