#ifndef DAABSTRACTCHART3DADDITEMWIDGET_H
#define DAABSTRACTCHART3DADDITEMWIDGET_H
#include "DAGuiAPI.h"
#include "qwt3d_plotitem.h"
#include "DAAbstractChartAddWidgetBase.h"
namespace DA
{
/**
 * @brief 创建Qwt3DPlotItem的窗口基类，DAChartAdd3D***Widget类的基类
 */
class DAGUI_API DAAbstractChart3DAddItemWidget : public DAAbstractChartAddWidgetBase
{
    Q_OBJECT
public:
    DAAbstractChart3DAddItemWidget(QWidget* par = nullptr);
    ~DAAbstractChart3DAddItemWidget();

public:
    // 创建3D绘图item，如果无法创建返回nullptr
    virtual Qwt3DPlotItem* create3DPlotItem() = 0;
};
}  // end DA
#endif  // DAABSTRACTCHART3DADDITEMWIDGET_H
