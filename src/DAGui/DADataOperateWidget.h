#ifndef DADATAOPERATEWIDGET_H
#define DADATAOPERATEWIDGET_H
#include <QWidget>
#include <QMap>
#include "DAGuiAPI.h"
#include "DAAbstractOperateWidget.h"
#include "DAData.h"

#include "DADataManager.h"
class QUndoStack;
namespace Ui
{
class DADataOperateWidget;
}
namespace DA
{
class DADataOperatePageWidget;
class DADataOperateOfDataFrameWidget;
class DADataManager;
/**
 * @brief 数据操作窗口，负责数据区域操作
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
private Q_SLOTS:
    // 数据删除对应的槽
    void onDataRemoved(const DA::DAData& d, int index);
    // 数据删除对应的槽
    void onDataChanged(const DA::DAData& d, DADataManager::ChangeType t);
    // tab标签切换
    void onTabWidgetCurrentChanged(int index);
    // tab关闭请求
    void onTabWidgetCloseRequested(int index);

private:
    void showDataframeData(const DA::DAData& d);
    void init();

private:
    Ui::DADataOperateWidget* ui;
};
}  // end of namespace DA
#endif  // DADATAOPERATEWIDGET_H
