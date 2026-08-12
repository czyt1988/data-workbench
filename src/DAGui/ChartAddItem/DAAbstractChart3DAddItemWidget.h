#ifndef DAABSTRACTCHART3DADDITEMWIDGET_H
#define DAABSTRACTCHART3DADDITEMWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "qwt3d_plotitem.h"
#include "DAData.h"
namespace DA
{
class DADataManager;

// 创建Qwt3DPlotItem的窗口基类，DAChartAdd3D***Widget类的基类
class DAGUI_API DAAbstractChart3DAddItemWidget : public QWidget
{
    Q_OBJECT
public:
    DAAbstractChart3DAddItemWidget(QWidget* par = nullptr);
    ~DAAbstractChart3DAddItemWidget();

public:
    // 创建3D绘图item，如果无法创建返回nullptr
    virtual Qwt3DPlotItem* create3DPlotItem() = 0;
    // 设置datamanager，会触发dataManagerChanged信号
    virtual void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
Q_SIGNALS:
    // dataManager发生改变
    void dataManagerChanged(DADataManager* dmgr);
    // 当前数据发生改变
    void currentDataChanged(const DAData& d);

private:
    DADataManager* mDataManager { nullptr };
};
}  // end DA
#endif  // DAABSTRACTCHART3DADDITEMWIDGET_H
