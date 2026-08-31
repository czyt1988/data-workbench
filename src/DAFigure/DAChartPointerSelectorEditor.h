#ifndef DACHARTPOINTERSELECTOREDITOR_H
#define DACHARTPOINTERSELECTOREDITOR_H
#include "DAFigureAPI.h"
#include <QPointer>
#include <QPointF>
#include "DAAbstractChartEditor.h"
#include "DAChartElementHitTester.h"
class QwtPlotItem;

namespace DA
{
class DAFigurePointerSelectorOverlay;

/**
 * @brief 指针选择器（chart 级）
 *
 * 在激活 plot 的 canvas 上做 plotitem 命中测试与拖动：
 * - 左键点击：命中 item 则选中，未命中则通知空白点击（由 overlay 决定行为）
 * - 左键按住已选中的可移动 item：进入拖动，实时平移预览
 * - 释放：若发生位移，请求宿主提交 undo 命令
 *
 * 与创建型编辑器不同，指针模式是持久会话，不发 finishedEdit，
 * 生命周期由 DAFigurePointerSelectorOverlay 管理
 */
class DAFIGURE_API DAChartPointerSelectorEditor : public DAAbstractChartEditor
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAChartPointerSelectorEditor)
    friend class DAFigurePointerSelectorOverlay;  // overlay需转发鼠标事件到protected虚函数
public:
    DAChartPointerSelectorEditor(QwtPlot* parent);
    ~DAChartPointerSelectorEditor() override;
    int rtti() const override;
    // 交出编辑产出的item，指针模式不产出item，恒返回nullptr
    QwtPlotItem* takeItem() override;
    // 当前选中的item（可能为nullptr）
    QwtPlotItem* currentItem() const;
    // 程序化设置选中item（不发选中信号）
    void setCurrentItem(QwtPlotItem* item);

Q_SIGNALS:
    /**
     * @brief canvas 内 item 被点击选中（或空白点击，item 为 nullptr）
     *
     * 由 overlay 统一转发为 DAFigureElementSelection
     */
    void itemClicked(DA::DAChartPointerSelectorEditor* editor, QwtPlotItem* item);
    /**
     * @brief item 开始被拖动，宿主需开始捕获鼠标
     */
    void itemDragBegan();
    /**
     * @brief 拖动结束，宿主需释放鼠标
     *
     * @param item 拖动的item
     * @param oldGeo 拖动前几何
     * @param newGeo 拖动后几何（位移为零时 newGeo==oldGeo）
     */
    void itemDragEnded(QwtPlotItem* item,
                       const DA::DAChartElementHitTester::ItemGeometry& oldGeo,
                       const DA::DAChartElementHitTester::ItemGeometry& newGeo);
    /**
     * @brief item 位置在拖动过程中变化，选中框需要重绘
     */
    void itemGeometryChanged();

protected:
    bool mousePressEvent(const QMouseEvent* e) override;
    bool mouseMoveEvent(const QMouseEvent* e) override;
    bool mouseReleaseEvent(const QMouseEvent* e) override;

private:
    // 计算两次canvas位置的位移（数据坐标）
    QPointF dataDelta(const QPoint& fromCanvasPos, const QPoint& toCanvasPos) const;
};
}  // namespace DA
#endif  // DACHARTPOINTERSELECTOREDITOR_H
