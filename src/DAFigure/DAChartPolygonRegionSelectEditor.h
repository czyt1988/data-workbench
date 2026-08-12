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
    virtual QPainterPath getSelectRegion() const override;
    // 设置选区
    virtual void setSelectRegion(const QPainterPath& shape) override;
    // 设置选择模式
    virtual void setSelectionMode(const SelectionMode& selectionMode) override;
    // rtti
    virtual int rtti() const override;
    // 清理数据
    void clear();
    // 取消
    virtual bool cancel() override;
    // 获取选框绘制的item
    virtual QwtPlotItem* takeItem() override;
private Q_SLOTS:
    void onItemAttached(QwtPlotItem* item, bool on);

protected:
    virtual bool mousePressEvent(const QMouseEvent* e) override;
    virtual bool mouseMoveEvent(const QMouseEvent* e) override;
    virtual bool keyPressEvent(const QKeyEvent* e) override;
    bool completeRegion();
    bool backspaceRegion();
};
}  // End Of Namespace DA
#endif  // DACHARTPOLYGONREGIONSELECTEDITOR_H
