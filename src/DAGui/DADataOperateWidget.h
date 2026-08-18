#ifndef DADATAOPERATEWIDGET_H
#define DADATAOPERATEWIDGET_H
#include <QWidget>
#include <QMap>
#include <QList>
#include <QByteArray>
#include "DAGuiAPI.h"
#include "DAAbstractOperateWidget.h"
#include "DAData.h"

#include "DADataManager.h"
class QUndoStack;
namespace ads
{
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}
namespace DA
{
class DADataOperatePageWidget;
class DADataOperateOfDataFrameWidget;
class DADataManager;
class DATableStyleRegistry;
/**
 * @brief 数据操作窗口，负责数据区域操作
 *
 * 基于 Qt-Advanced-Docking-System (ADS) 的 dockindock 嵌套模式实现：
 * 内部持有一个嵌套的 ads::CDockManager，每个 DADataOperateOfDataFrameWidget
 * （纯 QWidget）由 ads::CDockWidget 包装加入停靠区。数据页 dock 无法逃逸到 app
 * 顶层管理器，满足"只能在 DADataOperateWidget 里布局"。默认以标签形式加入当前
 * 聚焦 dock area，用户可自由拖拽分屏/并栏（禁浮动，保留 movable）。
 */
class DAGUI_API DADataOperateWidget : public DAAbstractOperateWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADataOperateWidget)
public:
    DADataOperateWidget(DADataManager* mgr, QWidget* parent = nullptr);
    DADataOperateWidget(QWidget* parent = nullptr);
    ~DADataOperateWidget();

    //
    void setDataManager(DADataManager* mgr);
    DADataManager* getDataManger() const;
    // 当前显示的窗口
    QWidget* currentWidget() const;

    // 当前显示的DataFrame窗口，如果不是DataFrame窗口，返回nullptr
    DADataOperateOfDataFrameWidget* getCurrentDataFrameWidget() const;
    // 获取所有已打开的 DataFrame 操作窗口（供序列化遍历样式）
    QList< DADataOperateOfDataFrameWidget* > getAllDataFrameWidgets() const;
    // 按 DAData 精确查找已打开的 DataFrame 窗口（无则 nullptr），供样式加载按 id 回填后刷新
    DADataOperateOfDataFrameWidget* findDataFrameWidget(const DAData& d) const;
    // 按插入顺序返回已打开数据列表（供工程序列化遍历，类比 DAChartOperateWidget::getFigureList）
    QList< DAData > getOpenedDataList() const;
    // 表格样式会话级注册表（样式生命周期脱离单个 widget，随数据存在）
    DATableStyleRegistry* styleRegistry() const;
    // 获取当前操作的表，这个表就是当前打开的表格
    DAData getCurrentOperateData() const;
    // 获取当前正在操作数据选中的列
    QList< int > getCurrentOperateDataSelectedColumns() const;
    // getCurrentOperateData和getCurrentOperateDataSelectedColumns的组合
    std::pair< DAData, QList< int > > getCurrentOperateDataInfo() const;
    // 获取QUndoStack
    QUndoStack* getUndoStack() override;
    // 刷新当前操作窗口的表格，这个一般是删除数据或数据变更后调用
    void refreshCurrentOperateTableView();
    // 确保当前窗口的列名可见，可搭配showData函数后使用
    void ensureCurrentTableColumnVisible(const QString& colName, bool selectCol = true);
    // 保存嵌套停靠区布局（供工程序列化），无停靠区时返回空
    QByteArray saveDataLayout() const;
    // 恢复嵌套停靠区布局（供工程反序列化），state 为空或失败返回 false
    bool restoreDataLayout(const QByteArray& state);
public Q_SLOTS:
    // 显示一个数据
    void showData(const DA::DAData& d);
    // 删除tab窗口，同时删除tab标签和上次tab对应的widget
    bool removeTabWidget(QWidget* w);
    // 清除操作
    void clear();
Q_SIGNALS:
    /**
     * @brief 数据操作页面添加信号
     * @param page
     */
    void dataTableCreated(DA::DADataOperatePageWidget* page);
    /**
     * @brief 页面删除，此时可以操作此指针，此信号之后指针销毁
     * @param page
     */
    void dataTableRemoving(DA::DADataOperatePageWidget* page);
    /**
     * @brief 当前的页面发生了改变
     * @param w
     * @param index
     */
    void currentDataTableWidgetChanged(DA::DADataOperatePageWidget* page, int index);
    /**
     * @brief 当前 DataFrame 窗口的表格表头被点击
     * @param logicalIndex 列逻辑索引
     * @note 仅在当前 dock 为 DataFrame 窗口时转发，用于"选择序列"窗口拾取列
     */
    void currentDataFrameColumnHeaderClicked(int logicalIndex);
private Q_SLOTS:
    // 数据删除对应的槽
    void onDataRemoved(const DA::DAData& d, int index);
    // 数据删除对应的槽
    void onDataChanged(const DA::DAData& d, DADataManager::ChangeType t);
    // 嵌套停靠区聚焦 dock 改变（过滤掉非本管理器的顶层 dock）
    void onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock);
    // dock 关闭请求处理（经 closeRequested 信号触发，传入对应的 page）
    void onDataCloseRequested(DA::DADataOperatePageWidget* page);

private:
    // 同步当前页：激活 undo 栈 + 重连表头点击 + 发射 currentDataTableWidgetChanged
    void setCurrentPage(DADataOperatePageWidget* page);
    void showDataframeData(const DA::DAData& d);
    void init();

private:
    DADataOperateWidget(const DADataOperateWidget&) = delete;
    DADataOperateWidget& operator=(const DADataOperateWidget&) = delete;
};
}  // end of namespace DA
#endif  // DADATAOPERATEWIDGET_H
