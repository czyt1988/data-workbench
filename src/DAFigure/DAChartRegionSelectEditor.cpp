#include "DAChartRegionSelectEditor.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"

namespace DA
{

/**
 * \if ENGLISH
 * @brief Private data class for DAChartRegionSelectEditor
 * @details Contains all the internal state and data members.
 * \endif
 *
 * \if CHINESE
 * @brief DAChartRegionSelectEditor 的私有数据类
 * @details 包含所有内部状态和数据成员。
 * \endif
 */
class DAChartRegionSelectEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartRegionSelectEditor)
public:
    ShapeType mShapeType { RectShape };
    std::unique_ptr< DARegionShapeStrategy > mStrategy;  ///< Cached strategy instance
    bool mIsStartDrawRegion { false };
    DAChartSelectRegionShapeItem* mTmpItem { nullptr };

    // Drag-based data
    QPointF mPressedPoint;
    QRectF mSelectedRect;
    bool mIsPlotEnableZoom { false };

    // Polygon data
    QPolygonF mPolygon;

    // Common data
    QPainterPath mLastPainterPath;

public:
    PrivateData(DAChartRegionSelectEditor* p) : q_ptr(p)
    {
    }

    ~PrivateData()
    {
        releaseTmpItem();
    }

    /**
     * \if ENGLISH
     * @brief Release and delete the temporary item
     * \endif
     *
     * \if CHINESE
     * @brief 释放并删除临时项
     * \endif
     */
    void releaseTmpItem()
    {
        if (mTmpItem) {
            mTmpItem->detach();
            delete mTmpItem;
            mTmpItem = nullptr;
        }
    }

    /**
     * \if ENGLISH
     * @brief Create the temporary item if not exists
     * \endif
     *
     * \if CHINESE
     * @brief 如果不存在则创建临时项
     * \endif
     */
    void createTmpItem()
    {
        if (!mTmpItem) {
            mTmpItem = new DAChartSelectRegionShapeItem("temp region");
            mTmpItem->attach(q_ptr->plot());
        }
    }

    /**
     * \if ENGLISH
     * @brief Check if two screen points are close to each other
     * @param p1 First point
     * @param p2 Second point
     * @param threshold Distance threshold in pixels
     * @return true if points are within threshold distance
     * \endif
     *
     * \if CHINESE
     * @brief 检查两个屏幕点是否接近
     * @param p1 第一个点
     * @param p2 第二个点
     * @param threshold 距离阈值（像素）
     * @return 如果点在阈值距离内则返回 true
     * \endif
     */
    static bool isPointClose(const QPoint& p1, const QPoint& p2, int threshold = 3)
    {
        int dx = p1.x() - p2.x();
        int dy = p1.y() - p2.y();
        return (dx * dx + dy * dy) < threshold * threshold;
    }
};

/**
 * \if ENGLISH
 * @brief Constructor with default shape type (Rect)
 * @param parent The parent QwtPlot widget
 * \endif
 *
 * \if CHINESE
 * @brief 使用默认形状类型（矩形）的构造函数
 * @param parent 父级 QwtPlot 控件
 * \endif
 */
DAChartRegionSelectEditor::DAChartRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    updateStrategy();
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartRegionSelectEditor::onItemAttached);
}

/**
 * \if ENGLISH
 * @brief Constructor with specified shape type
 * @param parent The parent QwtPlot widget
 * @param type The shape type to use
 * \endif
 *
 * \if CHINESE
 * @brief 使用指定形状类型的构造函数
 * @param parent 父级 QwtPlot 控件
 * @param type 要使用的形状类型
 * \endif
 */
DAChartRegionSelectEditor::DAChartRegionSelectEditor(QwtPlot* parent, ShapeType type)
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->mShapeType = type;
    updateStrategy();
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartRegionSelectEditor::onItemAttached);
}

DAChartRegionSelectEditor::~DAChartRegionSelectEditor()
{
}

/**
 * \if ENGLISH
 * @brief Set the shape type
 * @param type The shape type to use
 * \endif
 *
 * \if CHINESE
 * @brief 设置形状类型
 * @param type 要使用的形状类型
 * \endif
 */
void DAChartRegionSelectEditor::setShapeType(ShapeType type)
{
    if (d_ptr->mShapeType != type) {
        d_ptr->mShapeType = type;
        updateStrategy();
    }
}

/**
 * \if ENGLISH
 * @brief Get the current shape type
 * @return The current shape type
 * \endif
 *
 * \if CHINESE
 * @brief 获取当前形状类型
 * @return 当前形状类型
 * \endif
 */
DAChartRegionSelectEditor::ShapeType DAChartRegionSelectEditor::getShapeType() const
{
    return d_ptr->mShapeType;
}

/**
 * \if ENGLISH
 * @brief Get the selected region
 * @return The selected region as a QPainterPath
 * \endif
 *
 * \if CHINESE
 * @brief 获取选中的区域
 * @return 选中的区域，以 QPainterPath 形式返回
 * \endif
 */
QPainterPath DAChartRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->mLastPainterPath;
}

/**
 * \if ENGLISH
 * @brief Set the selection region
 * @param shape The shape to set as the selection region
 * \endif
 *
 * \if CHINESE
 * @brief 设置选区
 * @param shape 要设置为选区的形状
 * \endif
 */
void DAChartRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->mLastPainterPath = shape;
}

/**
 * \if ENGLISH
 * @brief Set the selection mode
 * @param selectionMode The selection mode to use
 * \endif
 *
 * \if CHINESE
 * @brief 设置选择模式
 * @param selectionMode 要使用的选择模式
 * \endif
 */
void DAChartRegionSelectEditor::setSelectionMode(const DAAbstractRegionSelectEditor::SelectionMode& selectionMode)
{
    DAAbstractRegionSelectEditor::setSelectionMode(selectionMode);
}

/**
 * \if ENGLISH
 * @brief Get the RTTI value for this editor
 * @return The RTTI value based on current shape type
 * \endif
 *
 * \if CHINESE
 * @brief 获取此编辑器的 RTTI 值
 * @return 基于当前形状类型的 RTTI 值
 * \endif
 */
int DAChartRegionSelectEditor::rtti() const
{
    return strategy()->rtti();
}

/**
 * \if ENGLISH
 * @brief Clear the current selection and reset state
 * \endif
 *
 * \if CHINESE
 * @brief 清除当前选区并重置状态
 * \endif
 */
void DAChartRegionSelectEditor::clear()
{
    d_ptr->releaseTmpItem();
    d_ptr->mSelectedRect    = QRectF();
    d_ptr->mPolygon         = QPolygonF();
    d_ptr->mLastPainterPath = QPainterPath();
}

/**
 * \if ENGLISH
 * @brief Cancel the current operation
 * @return true if cancellation was successful
 * \endif
 *
 * \if CHINESE
 * @brief 取消当前操作
 * @return 如果取消成功则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::cancel()
{
    clear();
    return true;
}

/**
 * \if ENGLISH
 * @brief Take ownership of the plot item
 * @return The plot item, ownership is transferred to caller
 * \endif
 *
 * \if CHINESE
 * @brief 获取绘图项的所有权
 * @return 绘图项，所有权转移给调用者
 * \endif
 */
QwtPlotItem* DAChartRegionSelectEditor::takeItem()
{
    QwtPlotItem* item = d_ptr->mTmpItem;
    d_ptr->mTmpItem   = nullptr;
    return item;
}

/**
 * \if ENGLISH
 * @brief Handle item attachment/detachment events
 * @param item The plot item
 * @param on true if attached, false if detached
 * \endif
 *
 * \if CHINESE
 * @brief 处理项附加/分离事件
 * @param item 绘图项
 * @param on 如果是附加则为 true，如果是分离则为 false
 * \endif
 */
void DAChartRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on) {
        if (item == d_ptr->mTmpItem) {
            d_ptr->mTmpItem = nullptr;
        }
    }
}

/**
 * \if ENGLISH
 * @brief Handle mouse press events
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理鼠标按下事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (strategy()->isDragBased()) {
        return handleDragMousePress(e);
    } else {
        return handlePolygonMousePress(e);
    }
}

/**
 * \if ENGLISH
 * @brief Handle mouse move events
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理鼠标移动事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::mouseMoveEvent(const QMouseEvent* e)
{
    if (strategy()->isDragBased()) {
        return handleDragMouseMove(e);
    } else {
        return handlePolygonMouseMove(e);
    }
}

/**
 * \if ENGLISH
 * @brief Handle mouse release events
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理鼠标释放事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    if (strategy()->isDragBased()) {
        return handleDragMouseRelease(e);
    }
    return false;
}

/**
 * \if ENGLISH
 * @brief Handle key press events
 * @param e The key event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理键盘按下事件
 * @param e 键盘事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::keyPressEvent(const QKeyEvent* e)
{
    if (!strategy()->isDragBased()) {
        if (Qt::Key_Enter == e->key() || Qt::Key_Return == e->key()) {
            return completePolygonRegion();
        } else if (Qt::Key_Backspace == e->key()) {
            return backspacePolygonRegion();
        }
    }
    return DAAbstractRegionSelectEditor::keyPressEvent(e);
}

/**
 * \if ENGLISH
 * @brief Handle mouse press for drag-based shapes
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理拖拽形状的鼠标按下事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::handleDragMousePress(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p = compat::eventPos(e);

    if (!d_ptr->mIsStartDrawRegion) {
        d_ptr->createTmpItem();
        DAChartWidget* chart     = qobject_cast< DAChartWidget* >(parent());
        d_ptr->mIsPlotEnableZoom = chart->isZoomEnabled();
        if (d_ptr->mIsPlotEnableZoom) {
            chart->enableZoom(false);
        }
    }

    d_ptr->mIsStartDrawRegion = true;
    d_ptr->mPressedPoint      = invTransform(p);

    if (getSelectionMode() == SingleSelection) {
        d_ptr->mLastPainterPath = QPainterPath();
    }

    Q_EMIT beginEdit();
    return true;
}

/**
 * \if ENGLISH
 * @brief Handle mouse move for drag-based shapes
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理拖拽形状的鼠标移动事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::handleDragMouseMove(const QMouseEvent* e)
{
    if (!d_ptr->mIsStartDrawRegion) {
        return false;
    }
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);

    d_ptr->mSelectedRect.setX(d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setY(d_ptr->mPressedPoint.y());
    d_ptr->mSelectedRect.setWidth(pf.x() - d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setHeight(pf.y() - d_ptr->mPressedPoint.y());

    strategy()->updateTmpItem(d_ptr->mTmpItem, QVariant::fromValue(d_ptr->mSelectedRect));

    return true;
}

/**
 * \if ENGLISH
 * @brief Handle mouse release for drag-based shapes
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理拖拽形状的鼠标释放事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::handleDragMouseRelease(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);

    if (pf == d_ptr->mPressedPoint) {
        d_ptr->releaseTmpItem();
        d_ptr->mIsStartDrawRegion = false;
        return true;
    }

    return completeDragRegion(pf);
}

/**
 * \if ENGLISH
 * @brief Handle mouse press for polygon shape
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理多边形形状的鼠标按下事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::handlePolygonMousePress(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button()) {
        return false;
    }

    QPoint p        = compat::eventPos(e);
    bool firstBegin = !d_ptr->mIsStartDrawRegion;

    if (Qt::LeftButton == e->button()) {
        if (!d_ptr->mIsStartDrawRegion) {
            d_ptr->mIsStartDrawRegion = true;
            d_ptr->createTmpItem();
        }

        QPointF pf = invTransform(p);

        if (d_ptr->mPolygon.size() > 1) {
            QPoint firstScreenPos = transform(d_ptr->mPolygon.first()).toPoint();
            if (PrivateData::isPointClose(p, firstScreenPos, 5)) {
                completePolygonRegion();
                return true;
            }
        }

        d_ptr->mPolygon.append(pf);
        if (d_ptr->mTmpItem) {
            d_ptr->mTmpItem->setPolygon(d_ptr->mPolygon);
        }

        if (firstBegin) {
            Q_EMIT beginEdit();
        }
    } else if (Qt::RightButton == e->button()) {
        if (d_ptr->mPolygon.size() > 0) {
            d_ptr->mPolygon.pop_back();
            if (d_ptr->mTmpItem) {
                d_ptr->mTmpItem->setPolygon(d_ptr->mPolygon);
            }
        }
    }
    return true;
}

/**
 * \if ENGLISH
 * @brief Handle mouse move for polygon shape
 * @param e The mouse event
 * @return true if the event was handled
 * \endif
 *
 * \if CHINESE
 * @brief 处理多边形形状的鼠标移动事件
 * @param e 鼠标事件
 * @return 如果事件被处理则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::handlePolygonMouseMove(const QMouseEvent* e)
{
    if (!d_ptr->mIsStartDrawRegion) {
        return false;
    }
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }

    QPoint p   = e->pos();
    QPointF pf = invTransform(p);

    QPolygonF tmp = d_ptr->mPolygon;
    tmp.append(pf);
    if (d_ptr->mTmpItem) {
        d_ptr->mTmpItem->setPolygon(tmp);
    }

    return DAAbstractRegionSelectEditor::mouseMoveEvent(e);
}

/**
 * \if ENGLISH
 * @brief Complete the drag-based region selection
 * @param endPoint The end point of the drag operation
 * @return true if completion was successful
 * \endif
 *
 * \if CHINESE
 * @brief 完成拖拽区域选择
 * @param endPoint 拖拽操作的终点
 * @return 如果完成成功则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::completeDragRegion(const QPointF& endPoint)
{
    d_ptr->mSelectedRect.setX(d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setY(d_ptr->mPressedPoint.y());
    d_ptr->mSelectedRect.setWidth(endPoint.x() - d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setHeight(endPoint.y() - d_ptr->mPressedPoint.y());

    QPainterPath painterPath = strategy()->createPath(QVariant::fromValue(d_ptr->mSelectedRect));

    mergeSelectionPath(painterPath);

    d_ptr->mIsStartDrawRegion = false;

    if (d_ptr->mIsPlotEnableZoom) {
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
        chart->enableZoom(true);
    }

    Q_EMIT finishSelection(d_ptr->mLastPainterPath);
    Q_EMIT finishedEdit(false);
    return true;
}

/**
 * \if ENGLISH
 * @brief Complete the polygon region selection
 * @return true if completion was successful
 * \endif
 *
 * \if CHINESE
 * @brief 完成多边形区域选择
 * @return 如果完成成功则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::completePolygonRegion()
{
    if (d_ptr->mPolygon.size() <= 2) {
        d_ptr->mPolygon.clear();
        d_ptr->releaseTmpItem();
        d_ptr->mIsStartDrawRegion = false;
        return false;
    }

    QPainterPath painterPath = strategy()->createPath(QVariant::fromValue(d_ptr->mPolygon));

    mergeSelectionPath(painterPath);

    d_ptr->mIsStartDrawRegion = false;
    Q_EMIT finishSelection(d_ptr->mLastPainterPath);
    Q_EMIT finishedEdit(false);
    return true;
}

/**
 * \if ENGLISH
 * @brief Remove the last point from the polygon
 * @return true if backspace was successful
 * \endif
 *
 * \if CHINESE
 * @brief 从多边形中移除最后一个点
 * @return 如果回退成功则返回 true
 * \endif
 */
bool DAChartRegionSelectEditor::backspacePolygonRegion()
{
    if (!d_ptr->mIsStartDrawRegion) {
        return false;
    }
    if (d_ptr->mPolygon.size() <= 1) {
        return false;
    }
    d_ptr->mPolygon.pop_back();
    if (d_ptr->mTmpItem) {
        d_ptr->mTmpItem->setPolygon(d_ptr->mPolygon);
    }
    return true;
}

/**
 * \if ENGLISH
 * @brief Merge the new path with the existing selection based on selection mode
 * @param newPath The new path to merge
 * \endif
 *
 * \if CHINESE
 * @brief 根据选择模式将新路径与现有选区合并
 * @param newPath 要合并的新路径
 * \endif
 */
void DAChartRegionSelectEditor::mergeSelectionPath(const QPainterPath& newPath)
{
    switch (getSelectionMode()) {
    case SingleSelection:
        d_ptr->mLastPainterPath = newPath;
        break;
    case AdditionalSelection:
        d_ptr->mLastPainterPath = d_ptr->mLastPainterPath.united(newPath);
        break;
    case SubtractionSelection:
        d_ptr->mLastPainterPath = d_ptr->mLastPainterPath.subtracted(newPath);
        break;
    case IntersectionSelection:
        d_ptr->mLastPainterPath = d_ptr->mLastPainterPath.intersected(newPath);
        break;
    default:
        break;
    }
}

/**
 * \if ENGLISH
 * @brief Update the cached strategy when shape type changes
 * \endif
 *
 * \if CHINESE
 * @brief 当形状类型改变时更新缓存的策略
 * \endif
 */
void DAChartRegionSelectEditor::updateStrategy()
{
    d_ptr->mStrategy = DA::createShapeStrategy(static_cast< int >(d_ptr->mShapeType));
}

/**
 * \if ENGLISH
 * @brief Get the current cached strategy
 * @return Pointer to the current strategy
 * \endif
 *
 * \if CHINESE
 * @brief 获取当前缓存的策略
 * @return 指向当前策略的指针
 * \endif
 */
DARegionShapeStrategy* DAChartRegionSelectEditor::strategy() const
{
    return d_ptr->mStrategy.get();
}

}  // namespace DA
