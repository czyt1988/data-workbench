#ifndef DACHARTELLIPSEREGIONSELECTEDITOR_H
#define DACHARTELLIPSEREGIONSELECTEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractRegionSelectEditor.h"
#include "DAChartSelectRegionShapeItem.h"
namespace DA
{
DA_IMPL_FORWARD_DECL(DAChartEllipseRegionSelectEditor)

/**
 * @brief 椭圆选区
 */
class DAFIGURE_API DAChartEllipseRegionSelectEditor : public DAAbstractRegionSelectEditor
{
    Q_OBJECT
    DA_IMPL(DAChartEllipseRegionSelectEditor)
public:
    DAChartEllipseRegionSelectEditor(QwtPlot* parent);
    virtual ~DAChartEllipseRegionSelectEditor();
    //获取选择的数据区域
    virtual QPainterPath getSelectRegion() const;
    //设置选区
    virtual void setSelectRegion(const QPainterPath& shape);
    //设置选择模式
    virtual void setSelectionMode(const SelectionMode& selectionMode);
    // rtti
    virtual int rtti() const;
    //清理数据
    void clear();
private slots:
    void onItemAttached(QwtPlotItem* item, bool on);

protected:
    bool mousePressEvent(const QMouseEvent* e);
    bool mouseMovedEvent(const QMouseEvent* e);
    bool mouseReleasedEvent(const QMouseEvent* e);
};
}  // End Of Namespace DA
#endif  // DACHARTELLIPSEREGIONSELECTEDITOR_H
