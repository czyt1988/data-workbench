#ifndef DACHARTPOLYGONREGIONSELECTEDITOR_H
#define DACHARTPOLYGONREGIONSELECTEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractRegionSelectEditor.h"
#include "DAChartSelectRegionShapeItem.h"
#include <QPolygonF>
namespace DA
{
class DAFIGURE_API DAChartPolygonRegionSelectEditor : public DAAbstractRegionSelectEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartPolygonRegionSelectEditor)
public:
    explicit DAChartPolygonRegionSelectEditor(QwtPlot* parent);
    virtual ~DAChartPolygonRegionSelectEditor();
    // 获取选择的数据区域
    virtual QPainterPath getSelectRegion() const;
    // 设置选区
    virtual void setSelectRegion(const QPainterPath& shape);
    // 设置选择模式
    virtual void setSelectionMode(const SelectionMode& selectionMode);
    // rtti
    virtual int rtti() const;
    // 清理数据
    void clear();
    // 取消
    virtual bool cancel() override;
    // 获取选框绘制的item
    virtual QwtPlotItem* takeItem() override;
private slots:
    void onItemAttached(QwtPlotItem* item, bool on);

protected:
    bool mousePressEvent(const QMouseEvent* e);
    bool mouseMoveEvent(const QMouseEvent* e);
    bool keyPressEvent(const QKeyEvent* e);
    bool completeRegion();
    bool backspaceRegion();
};
}  // End Of Namespace DA
#endif  // DACHARTPOLYGONREGIONSELECTEDITOR_H
