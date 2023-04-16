#ifndef DADATAMANAGEWIDGET_H
#define DADATAMANAGEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataManager.h"
class QAction;
class QActionGroup;

namespace Ui
{
class DADataManageWidget;
}

namespace DA
{

/**
 * @brief 数据管理窗口
 */
class DAGUI_API DADataManageWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 视图模式
     */
    enum DataViewMode
    {
        ViewDataInTable,  ///< 以表格的方式显示
        ViewDataInTree    ///< 以树的方式显示
    };
    Q_ENUM(DataViewMode)
public:
    DADataManageWidget(QWidget* parent = nullptr);
    ~DADataManageWidget();
    //设置DADataManager
    void setDataManager(DADataManager* dmgr);
    //获取mgr
    DADataManager* getDataManager() const;
    //获取选择的数据，如果没有数据，返回一个空的data
    DAData getOneSelectData() const;
    //获取选中的数据
    QList< DAData > getSelectDatas() const;
    //判断当前是否为table模式
    bool isTableView() const;
    //判断当前是否为tree模式
    bool isTreeView() const;
    //当前的显示模式
    DataViewMode getCurrentDataViewMode() const;
    //添加数据文件夹
    void addDataFolder();

public slots:
    //移除选中的数据
    void removeSelectData();

protected:
    void changeEvent(QEvent* e);
    void retranslateUi();
signals:
    /**
     * @brief 变量双击
     * @param data
     */
    void dataDbClicked(const DA::DAData& data);
    /**
     * @brief 当显示模式改变的时候发射此信号
     * @param v
     */
    void dataViewModeChanged(DADataManageWidget::DataViewMode v);
private slots:
    void onActionTableViewTriggered(bool on);
    void onActionTreeViewTriggered(bool on);

private:
    Ui::DADataManageWidget* ui;
    DADataManager* mDataManager;  ///< 数据管理器
    QAction* mActionViewDataListByTable;
    QAction* mActionViewDataListByTree;
    QActionGroup* mActionGroup;
};
}  // end of namespace DA
#endif  // DADATAMANAGEWIDGET_H
