#ifndef DAFigurePointerSelectorOverlay_H
#define DAFigurePointerSelectorOverlay_H
#include "DAFigureWidgetOverlay.h"
#include "DAFigureAPI.h"
#include "DAChartElementHitTester.h"
#include "DAFigureElementSelection.h"
#include <QPointer>
class QMouseEvent;
class QKeyEvent;
class QPainter;
class QwtPlot;
class QwtPlotItem;
class QwtScaleWidget;
namespace DA
{
class DAChartPointerSelectorEditor;
class DAFigureWidget;

/**
 * @brief 指针选择工具的figure级覆盖层
 *
 * 在整个figure范围内做元素命中分派：
 * - 标题/footer → 选中宿主plot
 * - 坐标轴 → 选中轴（QwtScaleWidget::setSelected 内建高亮）
 * - canvas → 委托 DAChartPointerSelectorEditor 做 item 命中与拖动
 *
 * 选中状态会绘制橙红虚线选中框（canvas item）或轴高亮，
 * 并通过 elementSelected 信号把 DAFigureElementSelection 发给宿主
 * （DAFigureWidget 转发给 DAAppController 联动属性面板）。
 *
 * Esc 结束指针模式（finished 信号走 endChartEditor 销毁流程），
 * Delete 删除选中的 plotitem（经 requestRemoveItem 由宿主走 undo 栈）
 */
class DAFIGURE_API DAFigurePointerSelectorOverlay : public DAFigureWidgetOverlay
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigurePointerSelectorOverlay)
public:
    /**
     * @brief 构造函数
     * @param fig 关联的QwtFigure
     * @param figureWidget 宿主DAFigureWidget（用于构造DAFigureElementSelection），允许为nullptr
     */
    DAFigurePointerSelectorOverlay(QwtFigure* fig, DAFigureWidget* figureWidget = nullptr);
    ~DAFigurePointerSelectorOverlay() override;
    // 当前选中的item（可能为nullptr）
    QwtPlotItem* selectedItem() const;
    // 当前选中的轴widget（可能为nullptr）
    QwtScaleWidget* selectedScaleWidget() const;

Q_SIGNALS:
    /**
     * @brief 元素被选中（含空白点击，此时为 SelectPlot）
     *
     * 由 DAFigureWidget 转发为 figureElementClicked
     */
    void elementSelected(const DA::DAFigureElementSelection& sel);
    /**
     * @brief 请求宿主删除选中的item（走undo栈）
     */
    void requestRemoveItem(QwtPlotItem* item);
    /**
     * @brief 请求宿主提交item位置移动命令（走undo栈）
     */
    void requestMoveItemPosition(QwtPlotItem* item,
                                 const DA::DAChartElementHitTester::ItemGeometry& oldGeo,
                                 const DA::DAChartElementHitTester::ItemGeometry& newGeo);

private Q_SLOTS:
    void onActiveWidgetChanged(QWidget* oldActive, QWidget* newActive);
    void onItemClicked(DA::DAChartPointerSelectorEditor* editor, QwtPlotItem* item);
    void onItemDragBegan();
    void onItemDragEnded(QwtPlotItem* item,
                         const DA::DAChartElementHitTester::ItemGeometry& oldGeo,
                         const DA::DAChartElementHitTester::ItemGeometry& newGeo);
    // 监听plot的itemAttached，detach时清除失效选中
    void onItemAttached(QwtPlotItem* item, bool on);

protected:
    void mousePressEvent(QMouseEvent* me) override;
    void mouseMoveEvent(QMouseEvent* me) override;
    void mouseReleaseEvent(QMouseEvent* me) override;
    void keyPressEvent(QKeyEvent* ke) override;
    void drawOverlay(QPainter* p) const override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    // 获取当前激活plot的editor（懒创建）
    DAChartPointerSelectorEditor* ensureEditor(QwtPlot* plot);
    // 清除所有选中状态（轴高亮/选中item）
    void clearSelection();
    // 绘制选中item的选中框（figure坐标）
    void drawItemSelectionRect(QPainter* p) const;
};
}  // namespace DA
#endif  // DAFigurePointerSelectorOverlay_H
