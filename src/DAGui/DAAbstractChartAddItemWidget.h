#ifndef DAABSTRACTCHARTADDITEMWIDGET_H
#define DAABSTRACTCHARTADDITEMWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "qwt_plot_item.h"
#include "DAData.h"
namespace DA
{
class DADataManager;
/**
 * @brief 创建QwtPlotItem的窗口基类，DAChartAdd***Widget类的基类
 */
class DAGUI_API DAAbstractChartAddItemWidget : public QWidget
{
	Q_OBJECT
public:
	DAAbstractChartAddItemWidget(QWidget* par = nullptr);
	~DAAbstractChartAddItemWidget();

public:
	/**
	 * @brief 创建QwtPlotItem
	 * @return 如果无法创建，返回nullptr
	 */
	virtual QwtPlotItem* createPlotItem() = 0;
    // 设置datamanager，会触发dataManagerChanged信号
    void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
    // 设置当前的data
    void setCurrentData(const DAData& d);
    const DAData& getCurrentData() const;
    // 更新数据接口
    virtual void updateData();
Q_SIGNALS:
    /**
     * @brief dataManager发生改变的信号
     * @param dmgr
     */
    void dataManagerChanged(DADataManager* dmgr);
    /**
     * @brief 当前数据发生了改变
     * @param d
     */
    void currentDataChanged(const DAData& d);

private:
    DADataManager* mDataManager { nullptr };
    DAData mData;
};
}  // end DA

#endif  // DAABSTRACTCHARTADDITEMWIDGET_H
