#ifndef DAFIGUREELEMENTSELECTION_H
#define DAFIGUREELEMENTSELECTION_H
#include "DAFigureAPI.h"
#include <QMetaType>
class QwtPlot;
class QwtPlotItem;
class QwtScaleWidget;
namespace DA
{
class DAFigureWidget;
/**
 * @brief 用于存放绘图元素
 *
 *   主要用于在绘图窗口选中绘图元素后，能携带各个层级的绘图元素传递给其它窗口
 *
 *   例如选中绘图的一个曲线，可以携带figure窗口，曲线对应的宿主plot窗口，寄生绘图窗口，以及曲线自身的plotitem指针
 */
class DAFIGURE_API DAFigureElementSelection
{
public:
    /**
     * @brief 选中类型
     */
    enum SelectionType
    {
        SelectNone,         ///< 无
        SelectPlot,         ///< plot,此时figureWidget、plot指针有效
        SelectScaleWidget,  ///< 刻度，此时figureWidget、plot、scaleWidget、axisId有效
        SelectPlotItem,     ///< plotitem，此时figureWidget、plot、plotItem有效
    };

public:
    DAFigureElementSelection();
    DAFigureElementSelection(DAFigureWidget* fig,QwtPlot* p);
    DAFigureElementSelection(DAFigureWidget* fig,QwtPlot* p,QwtPlotItem* item);
    DAFigureElementSelection(DAFigureWidget* fig,QwtPlot* p,QwtScaleWidget* sw,int axis);
    DAFigureWidget* figureWidget { nullptr };
    QwtPlot* plot { nullptr };
    QwtPlotItem* plotItem { nullptr };
    QwtScaleWidget* scaleWidget { nullptr };
    int axisId;
    SelectionType selectionType { SelectNone };
};
}
Q_DECLARE_METATYPE(DA::DAFigureElementSelection)
#endif  // DAFIGUREELEMENTSELECTION_H
