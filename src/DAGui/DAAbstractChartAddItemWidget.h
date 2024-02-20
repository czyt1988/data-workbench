#ifndef DAABSTRACTCHARTADDITEMWIDGET_H
#define DAABSTRACTCHARTADDITEMWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
#include "qwt_plot_item.h"
namespace DA
{
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
     * @brief 更新数据，可不实现
     */
    virtual void updateData();
};
}  // end DA

#endif  // DAABSTRACTCHARTADDITEMWIDGET_H
