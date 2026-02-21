#ifndef DAABSTRACTPLOTADDITEMWIDGET_H
#define DAABSTRACTPLOTADDITEMWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "plot/QImPlotItemNode.h"
#include "DAData.h"
namespace DA
{
class DADataManager;
/**
 * @brief 创建QImPlotItemNode的窗口基类，DAChartAdd***Widget类的基类
 */
class DAGUI_API DAAbstractPlotAddItemWidget : public QWidget
{
    Q_OBJECT
public:
    DAAbstractPlotAddItemWidget(QWidget* par = nullptr);
    ~DAAbstractPlotAddItemWidget();

public:
    /**
     * @brief 创建QIM::QImPlotItemNode
     * @return 如果无法创建，返回nullptr
     */
    virtual QIM::QImPlotItemNode* createPlotItem() = 0;

    // 设置datamanager，会触发dataManagerChanged信号
    virtual void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
Q_SIGNALS:
    /**
     * @brief dataManager发生改变的信号
     * @param dmgr
     */
    void dataManagerChanged(DA::DADataManager* dmgr);

private:
    DADataManager* mDataManager { nullptr };
};
}  // end DA

#endif  // DAABSTRACTCHARTADDITEMWIDGET_H
