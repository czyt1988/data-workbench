#ifndef DAABSTRACTCHARTADDITEMWIDGET_H
#define DAABSTRACTCHARTADDITEMWIDGET_H
#include <QWidget>
#include <QList>
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
    /**
     * @brief 创建一组QwtPlotItem（支持一次添加多条曲线等场景）
     *
     * 默认实现取 createPlotItem 的单个结果，多系列类型（如曲线的 1 个 x + 多个 y）重写本函数
     * @return 无法创建时返回空列表
     */
    virtual QList< QwtPlotItem* > createPlotItems();
    /**
     * @brief 获取坐标轴标题推荐（绘图添加后由上层设置到坐标轴）
     *
     * 默认实现返回 false 表示不提供标题
     * @param xTitle 输出：x 轴标题
     * @param yTitle 输出：y 轴标题
     * @return 是否提供标题
     */
    virtual bool getAxisTitleHint(QString& xTitle, QString& yTitle) const;
    // 设置datamanager，会触发dataManagerChanged信号
    virtual void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
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
};
}  // end DA

#endif  // DAABSTRACTCHARTADDITEMWIDGET_H
