#ifndef DACHARTRECTREGIONSELECTEDITOR_H
#define DACHARTRECTREGIONSELECTEDITOR_H
#include "DAFigureAPI.h"
#include "DAAbstractRegionSelectEditor.h"
#include "DAChartSelectRegionShapeItem.h"
class QKeyEvent;
namespace DA
{
class DAChartSelectRegionShapeItem;
/**
 * @brief 用于给图标添加矩形选框的事件过滤器
 */
class DAFIGURE_API DAChartRectRegionSelectEditor : public DAAbstractRegionSelectEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartRectRegionSelectEditor)
public:
    explicit DAChartRectRegionSelectEditor(QwtPlot* parent);
    virtual ~DAChartRectRegionSelectEditor();
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
    bool keyPressEvent(const QKeyEvent* e);
    bool keyReleaseEvent(const QKeyEvent* e);
};
}  // End Of Namespace DA
#endif  // SARECTSELECTEDITOR_H
