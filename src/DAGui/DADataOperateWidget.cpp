#include "DADataOperateWidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPointer>
#include "DALogCategory.h"
// ADS
#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
// api
#include "DADataManager.h"
#include "DADataOperatePageWidget.h"
#include <QUndoStack>
// table style registry
#include "DATableStyleRegistry.h"
// widget
#include "DADataOperateOfDataFrameWidget.h"
// py
#include "DADataPyObject.h"
// pybind11 <-> Qt 类型转换（本 cpp 出现 QString 作为参数传给 Python 可调用对象等场景）
#include "DAPybind11QtCaster.hpp"
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

namespace DA
{

class DADataOperateWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DADataOperateWidget)
public:
    PrivateData(DADataOperateWidget* p);
    // 根据 page 获取其 dock
    ads::CDockWidget* dockOfPage(DADataOperatePageWidget* page) const;
    // 根据 dock 获取其 page
    DADataOperatePageWidget* pageOfDock(ads::CDockWidget* dock) const;
    // 获取新数据页应加入的 dock area（取嵌套管理器内已有 page 所在 area，否则 nullptr）
    ads::CDockAreaWidget* targetAreaForNewData() const;

public:
    QMap< DA::DAData, QPointer< QWidget > > _dataToWidget;  ///< 记录数据对应的窗口
    DADataManager* _dataManager { nullptr };
    DATableStyleRegistry* _styleRegistry { nullptr };  ///< 表格样式会话级注册表，随数据存在
    QMetaObject::Connection _currentHeaderConn;  ///< 当前 DataFrame 窗口的表头点击连接
    ads::CDockManager* _dockManager { nullptr };  ///< 嵌套停靠管理器
    QList< DADataOperatePageWidget* > _pages;  ///< 插入顺序，作为 index 基础
    QHash< DADataOperatePageWidget*, ads::CDockWidget* > _pageToDock;  ///< page -> dock
    QPointer< DADataOperatePageWidget > _currentPage;  ///< 当前激活 page
    bool _suppressCurrentChanged { false };  ///< 创建/加载期间抑制 currentDataTableWidgetChanged
};

DADataOperateWidget::PrivateData::PrivateData(DADataOperateWidget* p) : q_ptr(p)
{
}

/**
 * @brief 根据 page 获取其 dock
 */
ads::CDockWidget* DADataOperateWidget::PrivateData::dockOfPage(DADataOperatePageWidget* page) const
{
    return _pageToDock.value(page, nullptr);
}

/**
 * @brief 根据 dock 获取其 page
 *
 * dock->widget() 即为包装进来的 DADataOperateOfDataFrameWidget（纯 QWidget）。
 */
DADataOperatePageWidget* DADataOperateWidget::PrivateData::pageOfDock(ads::CDockWidget* dock) const
{
    if (!dock) {
        return nullptr;
    }
    return qobject_cast< DADataOperatePageWidget* >(dock->widget());
}

/**
 * @brief 获取新数据页应加入的 dock area
 *
 * 关键：不能用 _dockManager->focusedDockWidget()——FocusHighlighting 下嵌套管理器的焦点
 * 控制器与顶层管理器共享 window 属性（DockFocusController.cpp onApplicationFocusChanged 不
 * 校验 dock 所属管理器），用户点过顶层 dock 后 nested->focusedDockWidget() 会返回顶层 dock，
 * 用它作 target 会让新数据页被加到顶层中心区（逃逸出 DADataOperateWidget）。
 * 改为从本嵌套管理器已有的 page dock 取 area，确保新页落在嵌套管理器内。
 */
ads::CDockAreaWidget* DADataOperateWidget::PrivateData::targetAreaForNewData() const
{
    for (int i = _pages.size() - 1; i >= 0; --i) {
        if (ads::CDockWidget* d = _pageToDock.value(_pages.at(i), nullptr)) {
            if (ads::CDockAreaWidget* a = d->dockAreaWidget()) {
                return a;
            }
        }
    }
    return nullptr;  // 首个数据页：在容器根创建 area
}

//===================================================
// DADataOperateWidget
//===================================================
DADataOperateWidget::DADataOperateWidget(DADataManager* mgr, QWidget* parent)
    : DAAbstractOperateWidget(parent), DA_PIMPL_CONSTRUCT
{
    init();
    setDataManager(mgr);
}

DADataOperateWidget::DADataOperateWidget(QWidget* parent)
    : DAAbstractOperateWidget(parent), DA_PIMPL_CONSTRUCT
{
    init();
}

void DADataOperateWidget::init()
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    d_ptr->_dockManager = new ads::CDockManager(this);
    lay->addWidget(d_ptr->_dockManager);
    // 禁止数据页 dock 浮动为独立窗口（全局锁，对所有当前及后续 dock 生效），保留分屏/并栏/拖拽
    d_ptr->_dockManager->lockDockWidgetFeaturesGlobally(ads::CDockWidget::DockWidgetFloatable);
    // 嵌套停靠区聚焦改变：onFocusedDockChanged 内部会过滤掉非本管理器的 dock，
    // 规避 FocusHighlighting 下嵌套焦点控制器跨管理器回调顶层 dock 的问题
    connect(d_ptr->_dockManager, &ads::CDockManager::focusedDockWidgetChanged,
            this, &DADataOperateWidget::onFocusedDockChanged);
}

DADataOperateWidget::~DADataOperateWidget()
{
    // _dockManager 作为本部件的子对象，由 Qt 自动销毁；数据页 dock 禁止浮动，无浮动窗口需额外清理
}

void DADataOperateWidget::setDataManager(DADataManager* mgr)
{
    d_ptr->_dataManager = mgr;
    // 样式注册表随数据管理器存在，连接 datasCleared 信号统一清空
    if (!d_ptr->_styleRegistry) {
        d_ptr->_styleRegistry = new DATableStyleRegistry(mgr, this);
    }
    connect(mgr, &DADataManager::dataRemoved, this, &DADataOperateWidget::onDataRemoved);
    connect(mgr, &DADataManager::dataChanged, this, &DADataOperateWidget::onDataChanged);
}

DADataManager* DADataOperateWidget::getDataManger() const
{
    return d_ptr->_dataManager;
}

/**
 * @brief 当前显示的窗口
 * @return
 */
QWidget* DADataOperateWidget::currentWidget() const
{
    return d_ptr->_currentPage.data();
}
/**
 * @brief 当前显示的DataFrame窗口，如果不是DataFrame窗口，返回nullptr
 * @return
 */
DADataOperateOfDataFrameWidget* DADataOperateWidget::getCurrentDataFrameWidget() const
{
    return qobject_cast< DADataOperateOfDataFrameWidget* >(currentWidget());
}

/**
 * @brief 获取所有已打开的 DataFrame 操作窗口
 *
 * 遍历 _dataToWidget，返回其中所有 DADataOperateOfDataFrameWidget 实例。
 * 供工程序列化遍历表格样式使用。
 * @return DataFrame 操作窗口列表
 */
QList< DADataOperateOfDataFrameWidget* > DADataOperateWidget::getAllDataFrameWidgets() const
{
    QList< DADataOperateOfDataFrameWidget* > res;
    for (auto it = d_ptr->_dataToWidget.begin(); it != d_ptr->_dataToWidget.end(); ++it) {
        DADataOperateOfDataFrameWidget* w = qobject_cast< DADataOperateOfDataFrameWidget* >(it.value());
        if (w) {
            res.append(w);
        }
    }
    return res;
}

/**
 * @brief 按 DAData 精确查找已打开的 DataFrame 窗口
 *
 * 供样式加载按 id 回填 registry 后，若该数据已打开则刷新表格视图。
 * @param d 数据
 * @return 已打开返回窗口指针，否则 nullptr
 */
DADataOperateOfDataFrameWidget* DADataOperateWidget::findDataFrameWidget(const DAData& d) const
{
    auto ite = d_ptr->_dataToWidget.find(d);
    if (ite == d_ptr->_dataToWidget.end()) {
        return nullptr;
    }
    return qobject_cast< DADataOperateOfDataFrameWidget* >(ite.value().data());
}

/**
 * @brief 按插入顺序返回已打开数据列表
 *
 * 供工程序列化遍历数据页（类比 DAChartOperateWidget::getFigureList）。
 * @return 已打开数据列表（插入顺序）
 */
QList< DAData > DADataOperateWidget::getOpenedDataList() const
{
    QList< DAData > res;
    for (DADataOperatePageWidget* page : std::as_const(d_ptr->_pages)) {
        if (DADataOperateOfDataFrameWidget* dfw = qobject_cast< DADataOperateOfDataFrameWidget* >(page)) {
            res.append(dfw->data());
        }
    }
    return res;
}

/**
 * @brief 表格样式会话级注册表
 *
 * 样式生命周期脱离单个 widget，随数据存在。widget 构造时从注册表借用 manager。
 * @return 注册表指针
 */
DATableStyleRegistry* DADataOperateWidget::styleRegistry() const
{
    return d_ptr->_styleRegistry;
}

/**
 * @brief 获取当前正在操作的数据
 * @return
 */
DAData DADataOperateWidget::getCurrentOperateData() const
{
    DADataOperateOfDataFrameWidget* dfw = getCurrentDataFrameWidget();
    if (!dfw) {
        return DAData();
    }
    return dfw->data();
}

/**
 * @brief 获取当前已经选中的列索引
 * @return
 */
QList< int > DADataOperateWidget::getCurrentOperateDataSelectedColumns() const
{
    DADataOperateOfDataFrameWidget* dfw = getCurrentDataFrameWidget();
    if (!dfw) {
        return QList< int >();
    }
    return dfw->getSelectedDataframeCoumns();
}

QUndoStack* DADataOperateWidget::getUndoStack()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameWidget();
    if (w) {
        return w->getUndoStack();
    }
    return nullptr;
}

void DADataOperateWidget::refreshCurrentOperateTableView()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameWidget();
    if (w) {
        w->refreshTable();
    }
}

/**
 * @brief 确保当前窗口的列名可见，可搭配showData函数后使用
 * @param colName
 */
void DADataOperateWidget::ensureCurrentTableColumnVisible(const QString& colName, bool selectCol)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameWidget();
    if (!w) {
        return;
    }
    w->ensureColumnVisible(colName, selectCol);
}

/**
 * @brief 获取当前选中的Dataframe,如果用户在选中了列，返回选中的列索引
 * @return
 */
std::pair< DAData, QList< int > > DADataOperateWidget::getCurrentOperateDataInfo() const
{
    std::pair< DAData, QList< int > > res;
    DADataOperateOfDataFrameWidget* dfw = getCurrentDataFrameWidget();
    if (!dfw) {
        return res;
    }
    res.first  = dfw->data();
    res.second = dfw->getSelectedDataframeCoumns();
    return res;
}

/**
 * @brief 保存嵌套停靠区布局（供工程序列化）
 *
 * 顶层 ads::CDockManager::saveState 不会捕获嵌套管理器的布局，故需单独保存
 * @return 布局状态字节数组，无停靠区时返回空
 */
QByteArray DADataOperateWidget::saveDataLayout() const
{
    if (!d_ptr->_dockManager) {
        return QByteArray();
    }
    return d_ptr->_dockManager->saveState();
}

/**
 * @brief 恢复嵌套停靠区布局（供工程反序列化）
 *
 * 调用前需先按保存顺序 showData 重建所有数据页 dock（objectName=data id），
 * restoreState 按 objectName 重新挂接布局。state 为空或匹配失败返回 false（保持默认标签顺序）
 * @param state saveDataLayout 返回的状态
 * @return 恢复成功返回 true
 */
bool DADataOperateWidget::restoreDataLayout(const QByteArray& state)
{
    if (!d_ptr->_dockManager || state.isEmpty()) {
        return false;
    }
    return d_ptr->_dockManager->restoreState(state);
}

/**
 * @brief 显示数据，如果数据已经有，唤起对应的dock，如果没有，则创建一个数据页
 * @param d
 */
void DADataOperateWidget::showData(const DA::DAData& d)
{
    switch (d.getDataType()) {
    case DAAbstractData::TypePythonDataFrame:
        showDataframeData(d);
        break;
    default:
        break;
    }
}

/**
 * @brief 删除dock窗口，同时删除dock和其对应的widget
 * @param w
 * @return 成功删除返回true
 */
bool DADataOperateWidget::removeTabWidget(QWidget* w)
{
    DADataOperatePageWidget* page = qobject_cast< DADataOperatePageWidget* >(w);
    if (!page) {
        return false;
    }
    ads::CDockWidget* dock = d_ptr->dockOfPage(page);
    if (!dock) {
        daCritical << tr("removing a widget that does not exist in the dock");  // cn:正在移除一个不存在的窗口
        return false;
    }
    emit dataTableRemoving(page);
    d_ptr->_pages.removeAll(page);
    d_ptr->_pageToDock.remove(page);
    if (d_ptr->_currentPage == page) {
        d_ptr->_currentPage = nullptr;
    }
    // 移除_dataToWidget记录
    for (auto i = d_ptr->_dataToWidget.begin(); i != d_ptr->_dataToWidget.end();) {
        if (i.value() == w) {
            i = d_ptr->_dataToWidget.erase(i);
        } else {
            ++i;
        }
    }
    if (d_ptr->_dockManager) {
        d_ptr->_dockManager->removeDockWidget(dock);
    }
    // 级联删除 page（page 为 dock->widget()，setWidget 时 reparent 进 dock），延迟到事件循环
    dock->deleteLater();
    return true;
}

/**
 * @brief 清除操作
 */
void DADataOperateWidget::clear()
{
    // 清空栈
    if (auto undostack = getUndoStack()) {
        undostack->clear();
    }
    // 断开当前表头点击连接
    if (d_ptr->_currentHeaderConn) {
        disconnect(d_ptr->_currentHeaderConn);
        d_ptr->_currentHeaderConn = QMetaObject::Connection();
    }
    // 移除所有 dock（不发 dataTableRemoving，保持原 clear 不发信号的语义）
    const QList< DADataOperatePageWidget* > pages = d_ptr->_pages;
    for (DADataOperatePageWidget* page : std::as_const(pages)) {
        if (ads::CDockWidget* dock = d_ptr->_pageToDock.value(page, nullptr)) {
            if (d_ptr->_dockManager) {
                d_ptr->_dockManager->removeDockWidget(dock);
            }
            dock->deleteLater();
        }
    }
    d_ptr->_pages.clear();
    d_ptr->_pageToDock.clear();
    d_ptr->_dataToWidget.clear();
    d_ptr->_currentPage = nullptr;
    // 数据清除
    getDataManger()->clear();
}

/**
 * @brief 数据删除过程触发的槽
 * @param d
 * @param index
 */
void DADataOperateWidget::onDataRemoved(const DA::DAData& d, int index)
{
    Q_UNUSED(index);
    auto ite = d_ptr->_dataToWidget.find(d);
    if (ite == d_ptr->_dataToWidget.end()) {
        return;
    }
    DADataOperatePageWidget* page = qobject_cast< DADataOperatePageWidget* >(ite.value().data());
    if (!page) {
        return;
    }
    ads::CDockWidget* dock = d_ptr->dockOfPage(page);
    if (!dock) {
        return;
    }
    // 标记已删除（追加到当前 dock 标题，匹配原 setTabText 语义）
    QString title = dock->windowTitle();
    title += tr("[deleted]");  // cn:[已删除]
    dock->setWindowTitle(title);
}

/**
 * @brief 变量信息改变
 * @param d
 * @param t
 */
void DADataOperateWidget::onDataChanged(const DA::DAData& d, DADataManager::ChangeType t)
{
    auto ite = d_ptr->_dataToWidget.find(d);
    if (ite == d_ptr->_dataToWidget.end()) {
        return;
    }
    DADataOperatePageWidget* page = qobject_cast< DADataOperatePageWidget* >(ite.value().data());
    if (!page) {
        return;
    }
    ads::CDockWidget* dock = d_ptr->dockOfPage(page);
    switch (t) {
    case DADataManager::ChangeName:
        if (dock) {
            dock->setWindowTitle(d.getName());
        }
        break;
    case DADataManager::ChangeDescribe:
        if (dock) {
            dock->setToolTip(d.getDescribe());
        }
        break;
    case DADataManager::ChangeValue: {
        // 值发生了变化，要刷新界面
        DADataOperateOfDataFrameWidget* dfWidget = qobject_cast< DADataOperateOfDataFrameWidget* >(page);
        if (dfWidget) {
            dfWidget->refreshTable();
        }
    }
    default:
        break;
    }
}

/**
 * @brief 嵌套停靠区聚焦 dock 改变
 *
 * FocusHighlighting 下嵌套管理器的 CDockFocusController 与顶层管理器共享 window 属性，
 * 用户聚焦顶层 dock（如工作流操作）时本信号也会被回调到顶层 dock。这里通过
 * nowDock->dockManager() 过滤，只处理属于本嵌套管理器的数据页 dock，避免 currentPage
 * 被误置空/误切换。焦点离开所有 dock 时保留 _currentPage，匹配 QTabWidget"有页即有当前"语义。
 * @param oldDock 旧聚焦 dock
 * @param nowDock 新聚焦 dock
 */
void DADataOperateWidget::onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock)
{
    Q_UNUSED(oldDock);
    if (d_ptr->_suppressCurrentChanged) {
        return;
    }
    // 过滤掉非本嵌套管理器的 dock（顶层 dock 的跨管理器回调）
    if (nowDock && nowDock->dockManager() != d_ptr->_dockManager) {
        return;
    }
    DADataOperatePageWidget* page = d_ptr->pageOfDock(nowDock);
    if (!page) {
        // 焦点离开所有 dock，保留 _currentPage
        return;
    }
    setCurrentPage(page);
}

/**
 * @brief dock 关闭请求处理（经 closeRequested 信号触发）
 * @param page 对应的数据页
 */
void DADataOperateWidget::onDataCloseRequested(DADataOperatePageWidget* page)
{
    if (!page) {
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::question(this,
                                                             tr("Question"),                             // cn:询问
                                                             tr("Whether to close the data table widget"));  // cn:是否关闭数据表窗口
    if (QMessageBox::Yes != btn) {
        return;
    }
    removeTabWidget(page);
}

/**
 * @brief 同步当前页
 *
 * 激活 undo 栈 + 重连表头点击 + 发射 currentDataTableWidgetChanged。
 * 带 _currentPage==page 提前返回，避免 onFocusedDockChanged 与 showData 双路径重复发射。
 * @param page 当前页
 */
void DADataOperateWidget::setCurrentPage(DADataOperatePageWidget* page)
{
    if (d_ptr->_currentPage == page) {
        return;
    }
    d_ptr->_currentPage = page;
    // 断开旧的表头点击连接
    if (d_ptr->_currentHeaderConn) {
        disconnect(d_ptr->_currentHeaderConn);
        d_ptr->_currentHeaderConn = QMetaObject::Connection();
    }
    if (!page) {
        emit currentDataTableWidgetChanged(nullptr, -1);
        return;
    }
    // 激活undostack
    page->activeUndoStack();
    // 连接当前 DataFrame 窗口的表头点击信号，转发出去
    if (DADataOperateOfDataFrameWidget* d = qobject_cast< DADataOperateOfDataFrameWidget* >(page)) {
        d_ptr->_currentHeaderConn = connect(d, &DADataOperateOfDataFrameWidget::columnHeaderClicked,
                                            this, &DADataOperateWidget::currentDataFrameColumnHeaderClicked);
    }
    emit currentDataTableWidgetChanged(page, d_ptr->_pages.indexOf(page));
}

void DADataOperateWidget::showDataframeData(const DA::DAData& d)
{
    // 先查找是否已经存在对应窗口
    DADataOperateOfDataFrameWidget* w =
        qobject_cast< DADataOperateOfDataFrameWidget* >(d_ptr->_dataToWidget.value(d, nullptr).data());
    if (nullptr == w) {
        // 没有就创建，传入样式注册表以借用会话级 manager
        w = new DADataOperateOfDataFrameWidget(d, d_ptr->_styleRegistry, d_ptr->_dockManager);
        emit dataTableCreated(w);
        // 记录窗口
        d_ptr->_dataToWidget[ d ] = w;
        // 包装进 ads::CDockWidget（使用 manager 构造以走管理器组件工厂，避免已弃用的两参构造）
        ads::CDockWidget* dock = new ads::CDockWidget(d_ptr->_dockManager, d.getName());
        // 稳定 objectName = data id，供 restoreState 按 objectName 匹配布局
        dock->setObjectName(QString::number(d.id()));
        dock->setWidget(w, ads::CDockWidget::ForceNoScrollArea);
        // 关闭按钮触发 closeRequested 而非自动隐藏，便于弹确认框后再删除
        dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
        dock->setToolTip(d.getDescribe());
        connect(dock, &ads::CDockWidget::closeRequested, this, [ this, w ]() {
            onDataCloseRequested(w);
        });
        // 记录映射
        d_ptr->_pageToDock[ w ] = dock;
        d_ptr->_pages.append(w);
        // 抑制聚焦改变，避免 addDockWidget 触发 currentDataTableWidgetChanged
        d_ptr->_suppressCurrentChanged = true;
        ads::CDockAreaWidget* area = d_ptr->targetAreaForNewData();
        if (area) {
            // 默认以标签形式加入当前聚焦 dock 所在 area
            d_ptr->_dockManager->addDockWidgetTabToArea(dock, area);
        } else {
            // 首个数据页：在容器根创建 dock area
            d_ptr->_dockManager->addDockWidget(ads::CenterDockWidgetArea, dock);
        }
        d_ptr->_suppressCurrentChanged = false;
        // 主动同步当前页（addDockWidget 可能不触发 focusedDockWidgetChanged）
        setCurrentPage(w);
    } else {
        // 已存在，唤起对应的 dock
        ads::CDockWidget* dock = d_ptr->dockOfPage(w);
        if (dock) {
            dock->setWindowTitle(d.getName());
            dock->setToolTip(d.getDescribe());
            dock->raise();  // 标签则置为当前，浮动则 raise 窗口
        }
        // raise 可能不触发 focusedDockWidgetChanged，主动同步
        setCurrentPage(w);
    }
}

}  // namespace DA
