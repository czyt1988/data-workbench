#ifndef DADATAOPERATEWIDGET_H
#define DADATAOPERATEWIDGET_H
#include <QWidget>
#include <QMap>
#include "DAGuiAPI.h"
#include "DAData.h"

#include "DADataManager.h"
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
class DAGUI_API DADataOperateWidget : public QWidget
{
    Q_OBJECT

public:
    DADataOperateWidget(DADataManager* mgr, QWidget* parent = nullptr);
    DADataOperateWidget(QWidget* parent = nullptr);
    ~DADataOperateWidget();

    //
    void setDataManager(DADataManager* mgr);
    //当前显示的窗口
    QWidget* currentWidget() const;

    //当前显示的DataFrame窗口，如果不是DataFrame窗口，返回nullptr
    DADataOperateOfDataFrameWidget* getCurrentDataFrameWidget() const;
public slots:
    //显示一个数据
    void showData(const DA::DAData& d);
    //删除tab窗口，同时删除tab标签和上次tab对应的widget
    bool removeTabWidget(QWidget* w);
signals:
    /**
     * @brief 数据操作页面添加信号
     * @param page
     */
    void pageAdded(DA::DADataOperatePageWidget* page);
    /**
     * @brief 页面删除，此时可以操作此指针，此信号之后指针销毁
     * @param page
     */
    void pageBeginRemove(DA::DADataOperatePageWidget* page);
private slots:
    //数据删除对应的槽
    void onDataRemoved(const DA::DAData& d, int index);
    //数据删除对应的槽
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
    QMap< DA::DAData, QWidget* > _dataToWidget;  ///< 记录数据对应的窗口
    DADataManager* _dataManager;
};
}  // end of namespace DA
#endif  // DADATAOPERATEWIDGET_H
