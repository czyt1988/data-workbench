#include "DAFigureElementSelection.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "qwt_scale_widget.h"
#include "qwt3d_plotitem.h"
namespace DA
{

/**
 * @brief 默认构造函数
 */
DAFigureElementSelection::DAFigureElementSelection()
{
}

/**
 * @brief 构造函数，选择2D绘图
 * @param fig 关联的FigureWidget
 * @param p 选中的QwtPlot
 * @param col 选择的列
 */
DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, SelectionColumns col)
    : figureWidget(fig), plot(p), selectionType(SelectPlot), selectionColumn(col)
{
}

/**
 * @brief 构造函数，选择2D绘图项
 * @param fig 关联的FigureWidget
 * @param p 选中的QwtPlot
 * @param item 选中的绘图项
 * @param col 选择的列
 */
DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, QwtPlotItem* item, SelectionColumns col)
    : figureWidget(fig), plot(p), plotItem(item), selectionType(SelectPlotItem), selectionColumn(col)
{
}

/**
 * @brief 构造函数，选择坐标轴
 * @param fig 关联的FigureWidget
 * @param p 选中的QwtPlot
 * @param sw 选中的坐标轴控件
 * @param axis 坐标轴ID
 * @param col 选择的列
 */
DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, QwtScaleWidget* sw, int axis, SelectionColumns col)
    : figureWidget(fig), plot(p), scaleWidget(sw), axisId(axis), selectionType(SelectScaleWidget), selectionColumn(col)
{
}

/**
 * @brief 构造函数，选择3D绘图
 * @param fig 关联的FigureWidget
 * @param p3d 选中的3D绘图控件
 * @param col 选择的列
 */
DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, DAChart3DWidget* p3d, SelectionColumns col)
    : figureWidget(fig), plot3D(p3d), selectionType(SelectPlot3D), selectionColumn(col)
{
}

/**
 * @brief 构造函数，选择3D绘图项
 * @param fig 关联的FigureWidget
 * @param p3d 选中的3D绘图控件
 * @param item 选中的3D绘图项
 * @param col 选择的列
 */
DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, DAChart3DWidget* p3d, Qwt3DPlotItem* item, SelectionColumns col)
    : figureWidget(fig), plot3D(p3d), plot3DItem(item), selectionType(SelectPlot3DItem), selectionColumn(col)
{
}

/**
 * @brief 构造函数，选择3D坐标轴
 * @param fig 关联的FigureWidget
 * @param p3d 选中的3D绘图控件
 * @param axis3D 3D坐标轴ID
 * @param col 选择的列
 */
DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, DAChart3DWidget* p3d, int axis3D, SelectionColumns col)
    : figureWidget(fig), plot3D(p3d), axis3DId(axis3D), selectionType(SelectPlot3DAxis), selectionColumn(col)
{
}

/**
 * @brief 判断是否选中了2D绘图
 * @return 如果选中了2D绘图返回true，否则返回false
 */
bool DAFigureElementSelection::isSelectedPlot() const
{
    return selectionType == SelectPlot;
}

/**
 * @brief 判断是否选中了坐标轴控件
 * @return 如果选中了坐标轴控件返回true，否则返回false
 */
bool DAFigureElementSelection::isSelectedScaleWidget() const
{
    return selectionType == SelectScaleWidget;
}

/**
 * @brief 判断是否选中了2D绘图项
 * @return 如果选中了2D绘图项返回true，否则返回false
 */
bool DAFigureElementSelection::isSelectedPlotItem() const
{
    return selectionType == SelectPlotItem;
}

/**
 * @brief 判断是否选中了3D绘图
 * @return 如果选中了3D绘图返回true，否则返回false
 */
bool DAFigureElementSelection::isSelectedPlot3D() const
{
    return selectionType == SelectPlot3D;
}

/**
 * @brief 判断是否选中了3D绘图项
 * @return 如果选中了3D绘图项返回true，否则返回false
 */
bool DAFigureElementSelection::isSelectedPlot3DItem() const
{
    return selectionType == SelectPlot3DItem;
}

/**
 * @brief 判断是否选中了3D坐标轴
 * @return 如果选中了3D坐标轴返回true，否则返回false
 */
bool DAFigureElementSelection::isSelectedPlot3DAxis() const
{
    return selectionType == SelectPlot3DAxis;
}

}  // end namespace DA

DA_AUTO_REGISTER_META_TYPE(DA::DAFigureElementSelection)
