#ifndef DAABSTRACTCHARTITEMSETTINGWIDGET_H
#define DAABSTRACTCHARTITEMSETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
// qwt
class QwtPlotItem;
class QwtPlot;
namespace DA
{
/**
 * @brief chart设置的基类封装了基本操作
 */
class DAGUI_API DAAbstractChartItemSettingWidget : public QWidget
{
    Q_OBJECT
public:
    DAAbstractChartItemSettingWidget(QWidget* parent = nullptr);
    ~DAAbstractChartItemSettingWidget();
    // 设置plotitem
    void setPlotItem(QwtPlotItem* item);
    QwtPlotItem* getPlotItem() const;

    /**
     * @brief 快捷转换为别的item
     * @return
     */
    template< typename T >
    T item_cast()
    {
        return dynamic_cast< T >(mPlotItem);
    }
protected slots:
    virtual void plotItemAttached(QwtPlotItem* plotItem, bool on);
    // 清空界面
    virtual void clear();

protected:
    QwtPlotItem* mPlotItem { nullptr };
    QPointer< QwtPlot > mPlot { nullptr };
};
}  // end DA

#endif  // DAABSTRACTCHARTITEMSETTINGWIDGET_H
