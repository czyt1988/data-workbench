#ifndef DACHARTELLIPSEREGIONSELECTEDITOR_H
#define DACHARTELLIPSEREGIONSELECTEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractRegionSelectEditor.h"
#include "DAChartSelectRegionShapeItem.h"
namespace DA
{

/**
 * @brief 椭圆选区
 */
class DAFIGURE_API DAChartEllipseRegionSelectEditor : public DAAbstractRegionSelectEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartEllipseRegionSelectEditor)
public:
    explicit DAChartEllipseRegionSelectEditor(QwtPlot* parent);
    virtual ~DAChartEllipseRegionSelectEditor();
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
    DAChartSelectRegionShapeItem* takeItem();
private slots:
    void onItemAttached(QwtPlotItem* item, bool on);

protected:
    bool mousePressEvent(const QMouseEvent* e);
    bool mouseMoveEvent(const QMouseEvent* e);
    bool mouseReleaseEvent(const QMouseEvent* e);
};
}  // End Of Namespace DA
#endif  // DACHARTELLIPSEREGIONSELECTEDITOR_H
