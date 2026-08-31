#include "DAFigurePointerSelectorOverlay.h"
#include "DAChartPointerSelectorEditor.h"
#include "DAChartElementHitTester.h"
#include "DAFigureWidget.h"
#include "DAFigureChartEditorWidgetOverlay.h"
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
// Qt
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
// qwt
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_item.h"
#include "qwt_scale_widget.h"
#include "qwt_text_label.h"

namespace DA
{
//===================================================
// 调试开关
//===================================================
#ifndef DAFigurePointerSelectorOverlay_DebugPrint
#define DAFigurePointerSelectorOverlay_DebugPrint 0
#endif

/**
 * @brief 选中框样式常量
 *
 * 橙红 #CE6043 来自项目语义色板（icon-ui-design-guide.md），
 * 与激活plot的蓝色控制线（基类 drawControlLine）形成视觉区分
 */
static const QColor c_pointer_selection_color = QColor(206, 96, 67);   // #CE6043
static const QColor c_pointer_handle_color    = QColor(255, 255, 255);  // 手柄填充白

class DAFigurePointerSelectorOverlay::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigurePointerSelectorOverlay)
public:
    PrivateData(DAFigurePointerSelectorOverlay* p);
    // figure坐标→目标widget坐标
    QPoint mapToWidget(const QWidget* w, const QPoint& figPos) const;
    // 清除轴高亮
    void clearScaleHighlight();
    // 清除editor的选中item
    void clearEditorSelection();

public:
    QPointer< DAChartPointerSelectorEditor > mActiveEditor;  ///< 当前激活plot的editor
    QwtPlotItem* mSelectedItem { nullptr };                  ///< 当前选中的item（生命周期由plot管理）
    QPointer< QwtScaleWidget > mSelectedScale;               ///< 当前选中的轴widget
    QPointer< DAFigureWidget > mFigureWidget;                ///< 宿主绘图窗口
    QPoint mLastFigureMousePos;                              ///< 最近的figure坐标（绘制用）
    bool mDragging { false };                                ///< item拖动态（grabMouse配对标志）
};

DAFigurePointerSelectorOverlay::PrivateData::PrivateData(DAFigurePointerSelectorOverlay* p) : q_ptr(p)
{
}

/**
 * @brief figure坐标 → 目标widget本地坐标
 */
QPoint DAFigurePointerSelectorOverlay::PrivateData::mapToWidget(const QWidget* w, const QPoint& figPos) const
{
    if (!w) {
        return QPoint();
    }
    return w->mapFromGlobal(q_ptr->mapToGlobal(figPos));
}

/**
 * @brief 清除轴选中高亮
 */
void DAFigurePointerSelectorOverlay::PrivateData::clearScaleHighlight()
{
    if (mSelectedScale) {
        mSelectedScale->setSelected(false);
        mSelectedScale = nullptr;
    }
}

/**
 * @brief 清除editor持有的选中item
 */
void DAFigurePointerSelectorOverlay::PrivateData::clearEditorSelection()
{
    if (mActiveEditor) {
        mActiveEditor->setCurrentItem(nullptr);
    }
    mSelectedItem = nullptr;
}

//===================================================
// DAFigurePointerSelectorOverlay
//===================================================

/**
 * @brief 构造函数
 *
 * 关闭子图尺寸调整（指针模式不拖子图框），保留激活plot切换能力
 * @param fig 关联的QwtFigure
 * @param figureWidget 宿主DAFigureWidget
 */
DAFigurePointerSelectorOverlay::DAFigurePointerSelectorOverlay(QwtFigure* fig, DAFigureWidget* figureWidget)
    : DAFigureWidgetOverlay(fig), DA_PIMPL_CONSTRUCT
{
    d_ptr->mFigureWidget = figureWidget;
    setBuiltInFunctionsEnable(QwtFigureWidgetOverlay::FunResizePlot, false);
    connect(this, &DAFigureWidgetOverlay::activeWidgetChanged, this, &DAFigurePointerSelectorOverlay::onActiveWidgetChanged);
    // 监听所有plot的itemAttached，item被删除时自动清除失效选中
    const QList< QwtPlot* > plots = fig->allAxes(true);
    for (QwtPlot* p : plots) {
        connect(p, &QwtPlot::itemAttached, this, &DAFigurePointerSelectorOverlay::onItemAttached);
    }
    // overlay本身NoFocus，键盘事件由宿主DAFigureWidget转发，安装事件过滤器拦截Esc/Delete
    if (figureWidget) {
        figureWidget->installEventFilter(this);
    }
    // 指针模式光标
    setCursor(Qt::ArrowCursor);
}

/**
 * @brief 析构函数
 */
DAFigurePointerSelectorOverlay::~DAFigurePointerSelectorOverlay()
{
    // 清理editor
    if (d_ptr->mActiveEditor) {
        d_ptr->mActiveEditor->deleteLater();
    }
    d_ptr->clearScaleHighlight();
}

/**
 * @brief 获取当前选中的item
 * @return 选中的item，无选中返回nullptr
 */
QwtPlotItem* DAFigurePointerSelectorOverlay::selectedItem() const
{
    return d_ptr->mSelectedItem;
}

/**
 * @brief 获取当前选中的轴widget
 * @return 选中的轴widget，无选中返回nullptr
 */
QwtScaleWidget* DAFigurePointerSelectorOverlay::selectedScaleWidget() const
{
    return d_ptr->mSelectedScale;
}

/**
 * @brief 激活plot切换：清除选中状态
 *
 * 指针工具的选中不跨plot保持（与Origin行为一致）
 */
void DAFigurePointerSelectorOverlay::onActiveWidgetChanged(QWidget* oldActive, QWidget* newActive)
{
    Q_UNUSED(oldActive);
    Q_UNUSED(newActive);
    clearSelection();
    updateOverlay();
}

/**
 * @brief editor的item点击结果处理
 *
 * item命中 → 记录选中并发射elementSelected(SelectPlotItem)；
 * 空白点击 → 清除选中并发射elementSelected(SelectPlot)
 * @param editor 来源editor
 * @param item 命中的item，nullptr表示空白
 */
void DAFigurePointerSelectorOverlay::onItemClicked(DA::DAChartPointerSelectorEditor* editor, QwtPlotItem* item)
{
    Q_UNUSED(editor);
    DA_D(d);
    QwtPlot* plot = currentActivePlot();
    if (item) {
        d->clearScaleHighlight();
        d->mSelectedItem = item;
        DAFigureElementSelection sel(d->mFigureWidget, plot, item, DAFigureElementSelection::ColumnName);
        Q_EMIT elementSelected(sel);
    } else {
        d->mSelectedItem = nullptr;
        // 空白点击：属性面板切回chart整体设置
        DAFigureElementSelection sel(d->mFigureWidget, plot, DAFigureElementSelection::ColumnName);
        Q_EMIT elementSelected(sel);
    }
    updateOverlay();
}

/**
 * @brief item拖动开始：抓取鼠标
 */
void DAFigurePointerSelectorOverlay::onItemDragBegan()
{
    grabMouse();
    d_ptr->mDragging = true;
}

/**
 * @brief item拖动结束：释放鼠标并请求undo命令
 */
void DAFigurePointerSelectorOverlay::onItemDragEnded(QwtPlotItem* item,
                                                     const DA::DAChartElementHitTester::ItemGeometry& oldGeo,
                                                     const DA::DAChartElementHitTester::ItemGeometry& newGeo)
{
    releaseMouse();
    d_ptr->mDragging = false;
    if (item && oldGeo.valid && newGeo.valid && oldGeo.p1 != newGeo.p1) {
        Q_EMIT requestMoveItemPosition(item, oldGeo, newGeo);
    }
    updateOverlay();
}

/**
 * @brief plot的item attach/detatch监听
 *
 * item被detach（删除）时清除失效选中，避免悬垂引用
 */
void DAFigurePointerSelectorOverlay::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on && d_ptr->mSelectedItem == item) {
        d_ptr->mSelectedItem = nullptr;
        updateOverlay();
    }
}

/**
 * @brief 鼠标按下：plot级命中分派
 *
 * 先让基类完成激活plot切换；再对激活plot做区域命中：
 * 标题/footer → SelectPlot；轴 → 选中轴；canvas → 转发给editor
 * @param me figure坐标鼠标事件
 */
void DAFigurePointerSelectorOverlay::mousePressEvent(QMouseEvent* me)
{
    DA_D(d);
    d->mLastFigureMousePos = compat::eventPos(me);
#if DAFigurePointerSelectorOverlay_DebugPrint
    qDebug() << "DAFigurePointerSelectorOverlay::mousePressEvent" << d->mLastFigureMousePos;
#endif
    if (me->button() != Qt::LeftButton) {
        DAFigureWidgetOverlay::mousePressEvent(me);
        return;
    }
    // 先让基类处理激活plot切换（点击其他子图会切换激活）
    DAFigureWidgetOverlay::mousePressEvent(me);
    if (me->isAccepted()) {
        // 基类消费了事件（如发生了激活切换），本次点击仅用于切换，不选择元素
        return;
    }
    QwtPlot* plot = currentActivePlot();
    if (!plot) {
        return;
    }
    const QPoint plotLocal = d->mapToWidget(plot, d->mLastFigureMousePos);
    const DAChartElementHitTester::PlotHitResult hit
        = DAChartElementHitTester::hitTestPlot(plot, plotLocal);
    switch (hit.type) {
    case DAChartElementHitTester::HitTitle:
    case DAChartElementHitTester::HitFooter: {
        // 标题/footer → 选中宿主plot
        d->clearScaleHighlight();
        d->clearEditorSelection();
        DAFigureElementSelection sel(d->mFigureWidget, plot, DAFigureElementSelection::ColumnName);
        Q_EMIT elementSelected(sel);
        updateOverlay();
        me->accept();
    } break;
    case DAChartElementHitTester::HitAxis: {
        // 坐标轴 → 轴选中 + 内建高亮
        d->clearEditorSelection();
        d->clearScaleHighlight();
        d->mSelectedScale = hit.axisWidget;
        d->mSelectedScale->setSelected(true);
        DAFigureElementSelection sel(
            d->mFigureWidget, plot, hit.axisWidget, hit.axisId, DAFigureElementSelection::ColumnName);
        Q_EMIT elementSelected(sel);
        updateOverlay();
        me->accept();
    } break;
    case DAChartElementHitTester::HitCanvas: {
        // canvas → 转发给editor做item命中
        d->clearScaleHighlight();
        DAChartPointerSelectorEditor* editor = ensureEditor(plot);
        if (!editor) {
            return;
        }
        const QPoint canvasPos = d->mapToWidget(plot->canvas(), d->mLastFigureMousePos);
        QMouseEvent mappedEvent(
            QEvent::MouseButtonPress, canvasPos, canvasPos, compat::eventGlobalPos(me), me->button(), me->buttons(), me->modifiers()
        );
        editor->mousePressEvent(&mappedEvent);
        me->accept();
    } break;
    default:
        break;
    }
}

/**
 * @brief 鼠标移动：拖动态转发给editor
 */
void DAFigurePointerSelectorOverlay::mouseMoveEvent(QMouseEvent* me)
{
    DA_D(d);
    d->mLastFigureMousePos = compat::eventPos(me);
    if (d->mDragging && d->mActiveEditor) {
        QwtPlot* plot = currentActivePlot();
        if (plot && plot->canvas()) {
            const QPoint canvasPos = d->mapToWidget(plot->canvas(), d->mLastFigureMousePos);
            QMouseEvent mappedEvent(
                QEvent::MouseMove, canvasPos, canvasPos, compat::eventGlobalPos(me), me->button(), me->buttons(), me->modifiers()
            );
            d->mActiveEditor->mouseMoveEvent(&mappedEvent);
            updateOverlay();
        }
        me->accept();
        return;
    }
    DAFigureWidgetOverlay::mouseMoveEvent(me);
}

/**
 * @brief 鼠标释放：拖动态转发给editor
 */
void DAFigurePointerSelectorOverlay::mouseReleaseEvent(QMouseEvent* me)
{
    DA_D(d);
    d->mLastFigureMousePos = compat::eventPos(me);
    if (d->mDragging && d->mActiveEditor) {
        QwtPlot* plot = currentActivePlot();
        if (plot && plot->canvas()) {
            const QPoint canvasPos = d->mapToWidget(plot->canvas(), d->mLastFigureMousePos);
            QMouseEvent mappedEvent(
                QEvent::MouseButtonRelease,
                canvasPos,
                canvasPos,
                compat::eventGlobalPos(me),
                me->button(),
                me->buttons(),
                me->modifiers()
            );
            d->mActiveEditor->mouseReleaseEvent(&mappedEvent);
        }
        me->accept();
        return;
    }
    DAFigureWidgetOverlay::mouseReleaseEvent(me);
}

/**
 * @brief 键盘事件：Esc退出指针模式，Delete删除选中item
 */
void DAFigurePointerSelectorOverlay::keyPressEvent(QKeyEvent* ke)
{
    DA_D(d);
    switch (ke->key()) {
    case Qt::Key_Escape: {
        clearSelection();
        updateOverlay();
        Q_EMIT finished(true);
        ke->accept();
    } break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace: {
        if (d->mSelectedItem) {
            QwtPlotItem* item = d->mSelectedItem;
            d->mSelectedItem  = nullptr;
            Q_EMIT requestRemoveItem(item);
            updateOverlay();
            ke->accept();
        }
    } break;
    default:
        DAFigureWidgetOverlay::keyPressEvent(ke);
        break;
    }
}

/**
 * @brief 事件过滤器：拦截宿主DAFigureWidget的键盘事件
 *
 * overlay本身NoFocus收不到键盘事件，Esc/Delete经宿主窗口转发到此
 */
bool DAFigurePointerSelectorOverlay::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress && d_ptr->mFigureWidget == obj) {
        QKeyEvent* ke = static_cast< QKeyEvent* >(event);
        const int key = ke->key();
        if (Qt::Key_Escape == key || Qt::Key_Delete == key || Qt::Key_Backspace == key) {
            keyPressEvent(ke);
            if (ke->isAccepted()) {
                return true;
            }
        }
    }
    return DAFigureWidgetOverlay::eventFilter(obj, event);
}

/**
 * @brief 覆盖层内容绘制
 *
 * 基类绘制激活plot蓝框；本类追加选中元素框。
 * 绘制统一在drawOverlay中进行（QwtWidgetOverlay的paintEvent会调用它）
 * @param p painter
 */
void DAFigurePointerSelectorOverlay::drawOverlay(QPainter* p) const
{
    DAFigureWidgetOverlay::drawOverlay(p);
    drawItemSelectionRect(p);
}

//===================================================
// 私有辅助
//===================================================

/**
 * @brief 懒创建当前激活plot的editor
 * @param plot 激活的plot
 * @return editor指针，失败返回nullptr
 */
DAChartPointerSelectorEditor* DAFigurePointerSelectorOverlay::ensureEditor(QwtPlot* plot)
{
    DA_D(d);
    if (!plot) {
        return nullptr;
    }
    if (d->mActiveEditor && d->mActiveEditor->plot() == plot) {
        return d->mActiveEditor;
    }
    // 切换了plot，销毁旧editor
    if (d->mActiveEditor) {
        disconnect(d->mActiveEditor, nullptr, this, nullptr);
        d->mActiveEditor->deleteLater();
        d->mActiveEditor = nullptr;
        d->mSelectedItem = nullptr;
    }
    DAChartPointerSelectorEditor* editor = new DAChartPointerSelectorEditor(plot);
    editor->setEnabled(true);
    connect(editor, &DAChartPointerSelectorEditor::itemClicked, this, &DAFigurePointerSelectorOverlay::onItemClicked);
    connect(editor, &DAChartPointerSelectorEditor::itemDragBegan, this, &DAFigurePointerSelectorOverlay::onItemDragBegan);
    connect(
        editor, &DAChartPointerSelectorEditor::itemDragEnded, this, &DAFigurePointerSelectorOverlay::onItemDragEnded);
    connect(editor,
            &DAChartPointerSelectorEditor::itemGeometryChanged,
            this,
            &DAFigurePointerSelectorOverlay::updateOverlay);
    d->mActiveEditor = editor;
    return editor;
}

/**
 * @brief 清除所有选中状态
 */
void DAFigurePointerSelectorOverlay::clearSelection()
{
    DA_D(d);
    d->clearScaleHighlight();
    d->clearEditorSelection();
}

/**
 * @brief 绘制选中item的选中框
 *
 * 选中框为 canvas 像素矩形经 mapRectTo 映射到 figure 坐标，
 * 橙红虚线边框 + 半透明填充 + 四角白色手柄方块
 * @param p painter
 */
void DAFigurePointerSelectorOverlay::drawItemSelectionRect(QPainter* p) const
{
    DA_DC(d);
    if (!d->mSelectedItem) {
        return;
    }
    QwtPlotItem* item = d->mSelectedItem;
    QwtPlot* plot     = item->plot();
    if (!plot || !plot->canvas()) {
        return;
    }
    const QRect canvasRect = DAChartElementHitTester::itemSelectionRect(plot, item);
    if (canvasRect.isNull()) {
        return;
    }
    // canvas坐标 → figure坐标
    const QRect figRect = DAFigureChartEditorWidgetOverlay::mapRectTo(plot->canvas(), figure(), canvasRect);
    // 半透明填充
    QColor fillColor = c_pointer_selection_color;
    fillColor.setAlpha(40);
    p->fillRect(figRect, fillColor);
    // 虚线边框
    QPen pen(c_pointer_selection_color, 1.5, Qt::DashLine);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRect(figRect);
    // 四角手柄
    const int handleSize = 7;
    p->setPen(QPen(c_pointer_selection_color, 1));
    p->setBrush(c_pointer_handle_color);
    p->drawRect(QRect(figRect.topLeft() - QPoint(1, 1), QSize(handleSize, handleSize)));
    p->drawRect(QRect(QPoint(figRect.right() - handleSize + 2, figRect.top() - 1), QSize(handleSize, handleSize)));
    p->drawRect(QRect(QPoint(figRect.left() - 1, figRect.bottom() - handleSize + 2), QSize(handleSize, handleSize)));
    p->drawRect(QRect(QPoint(figRect.right() - handleSize + 2, figRect.bottom() - handleSize + 2), QSize(handleSize, handleSize)));
}

}  // namespace DA
