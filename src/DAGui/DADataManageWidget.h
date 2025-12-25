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
    DADataManageWidget(QWidget* parent = nullptr);
    ~DADataManageWidget();
    // 设置DADataManager
    void setDataManager(DADataManager* dmgr);
    // 获取mgr
    DADataManager* getDataManager() const;
    // 获取选择的数据，如果没有数据，返回一个空的data
    DAData getCurrentSelectData() const;
    // 获取选中的数据
    QList< DAData > getAllSelectDatas() const;
public slots:
    // 移除选中的数据
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

private:
    Ui::DADataManageWidget* ui;
    DADataManager* mDataManager;  ///< 数据管理器
};
}  // end of namespace DA
#endif  // DADATAMANAGEWIDGET_H
