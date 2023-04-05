#ifndef DACHARTPOLYGONREGIONSELECTEDITOR_H
#define DACHARTPOLYGONREGIONSELECTEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractRegionSelectEditor.h"
#include "DAChartSelectRegionShapeItem.h"
#include <QPolygonF>
namespace DA
{
DA_IMPL_FORWARD_DECL(DAChartPolygonRegionSelectEditor)
class DAFIGURE_API DAChartPolygonRegionSelectEditor : public DAAbstractRegionSelectEditor
{
    Q_OBJECT
    DA_IMPL(DAChartPolygonRegionSelectEditor)
public:
    DAChartPolygonRegionSelectEditor(QwtPlot* parent);
    virtual ~DAChartPolygonRegionSelectEditor();
    //获取选择的数据区域
    virtual QPainterPath getSelectRegion() const;
    //设置选区
    virtual void setSelectRegion(const QPainterPath& shape);
    //设置选择模式
    virtual void setSelectionMode(const SelectionMode& selectionMode);
    // rtti
    virtual int rtti() const;
private slots:
    void onItemAttached(QwtPlotItem* item, bool on);

protected:
    bool mousePressEvent(const QMouseEvent* e);
    bool mouseMovedEvent(const QMouseEvent* e);
    bool keyPressEvent(const QKeyEvent* e);
    bool completeRegion();
    bool backspaceRegion();
};
}  // End Of Namespace DA
#endif  // DACHARTPOLYGONREGIONSELECTEDITOR_H
