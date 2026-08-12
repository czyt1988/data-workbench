#include "DAChartRectRegionSelectEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QKeyEvent>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
namespace DA
{
class DAChartRectRegionSelectEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartRectRegionSelectEditor)
public:
    bool mIsStartDrawRegion { false };
    DAChartSelectRegionShapeItem* mTmpItem { nullptr };
    QPointF mPressedPoint { 0, 0 };
    QRectF mSelectedRect { 0, 0, 0, 0 };
    QPainterPath mLastPainterPath;
    bool mIsPlotEnableZoom { false };  ///< 记录绘图是否允许缩放，在结束的时候还原状态
public:
    /**
     * @brief 构造函数
     * @param p 父指针
     */
    PrivateData(DAChartRectRegionSelectEditor* p) : q_ptr(p)
    {
    }
    /**
     * @brief 析构函数
     */
    ~PrivateData()
    {
        releaseTmpItem();
    }
    /**
     * @brief 清除临时项
     *
     * 不释放临时项的内存，只是清除指针
     */
    void clearTmpItem()
    {
        mTmpItem = nullptr;
    }
    /**
     * @brief 释放临时项
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
     * @brief 创建临时项
     *
     * 如果临时项不存在则创建并附加到绘图上
     */
    void createTmpItem()
    {
        if (nullptr == mTmpItem) {
            mTmpItem = new DAChartSelectRegionShapeItem("temp region");
            mTmpItem->attach(q_ptr->plot());
        }
    }
};

/**
 * @brief 构造函数
 * @param parent 父QwtPlot对象
 */
DAChartRectRegionSelectEditor::DAChartRectRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartRectRegionSelectEditor::onItemAttached);
}

/**
 * @brief 析构函数
 */
DAChartRectRegionSelectEditor::~DAChartRectRegionSelectEditor()
{
}

/**
 * @brief 鼠标按下事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAChartRectRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p = compat::eventPos(e);
    if (!d_ptr->mIsStartDrawRegion) {
        d_ptr->createTmpItem();
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
        if (!chart) {
            return false;
        }
        d_ptr->mIsPlotEnableZoom = chart->isZoomEnabled();
        if (d_ptr->mIsPlotEnableZoom) {
            chart->enableZoom(false);
        }
    }
    d_ptr->mIsStartDrawRegion = true;
    d_ptr->mPressedPoint      = invTransform(p);
    switch (getSelectionMode()) {
    case SingleSelection: {
        d_ptr->mLastPainterPath = QPainterPath();
        break;
    }
    default:
        break;
    }
    Q_EMIT beginEdit();
    return true;
}

/**
 * @brief 鼠标移动事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAChartRectRegionSelectEditor::mouseMoveEvent(const QMouseEvent* e)
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
    if (d_ptr->mTmpItem) {
        d_ptr->mTmpItem->setRect(d_ptr->mSelectedRect);
        if(QwtPlot* p = d_ptr->mTmpItem->plot()){
            p->replot();
        }
    }
    return true;
}

/**
 * @brief 鼠标释放事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAChartRectRegionSelectEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);
    if (pf == d_ptr->mPressedPoint) {
        // 如果点击和松开是一个点，就取消当前的选区
        d_ptr->releaseTmpItem();
        d_ptr->mIsStartDrawRegion = false;
        return true;
    }
    d_ptr->mSelectedRect.setX(d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setY(d_ptr->mPressedPoint.y());
    d_ptr->mSelectedRect.setWidth(pf.x() - d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setHeight(pf.y() - d_ptr->mPressedPoint.y());
    QPainterPath painterPath;
    painterPath.addRect(d_ptr->mSelectedRect);
    switch (getSelectionMode()) {
    case SingleSelection: {
        d_ptr->mLastPainterPath = painterPath;
        break;
    }
    case AdditionalSelection: {
        d_ptr->mLastPainterPath = d_ptr->mLastPainterPath.united(painterPath);
        break;
    }
    case SubtractionSelection: {
        d_ptr->mLastPainterPath = d_ptr->mLastPainterPath.subtracted(painterPath);
        break;
    }
    case IntersectionSelection: {
        d_ptr->mLastPainterPath = d_ptr->mLastPainterPath.intersected(painterPath);
        break;
    }
    default:
        break;
    }
    d_ptr->mIsStartDrawRegion = false;
    if (d_ptr->mIsPlotEnableZoom) {
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
        // 还原zoomer
        if (chart) {
            chart->enableZoom(true);
        }
    }
    Q_EMIT finishSelection(d_ptr->mLastPainterPath);
    Q_EMIT finishedEdit(false);
    return true;
}

/**
 * @brief 取消当前选区
 * @return 始终返回true
 */
bool DAChartRectRegionSelectEditor::cancel()
{
    clear();
    return true;
}

/**
 * @brief 获取选框对应的item
 * @return
 */
QwtPlotItem* DAChartRectRegionSelectEditor::takeItem()
{
    DAChartSelectRegionShapeItem* item = d_ptr->mTmpItem;
    d_ptr->mTmpItem                   = nullptr;
    return item;
}


///
/// \brief 处理按钮事件
/// \param e
///
bool DAChartRectRegionSelectEditor::keyPressEvent(const QKeyEvent* e)
{
    return DAAbstractRegionSelectEditor::keyPressEvent(e);
}
/**
 * @brief 按键释放事件处理
 * @param e 按键事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAChartRectRegionSelectEditor::keyReleaseEvent(const QKeyEvent* e)
{
    return DAAbstractRegionSelectEditor::keyReleaseEvent(e);
}

///
/// \brief 获取选择的数据区域
/// \return
///
QPainterPath DAChartRectRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->mLastPainterPath;
}
///
/// \brief SARectRegionSelectEditor::setSelectRegion
/// \param shape
///
void DAChartRectRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->mLastPainterPath = shape;
}

///
/// \brief 设置选择模式
/// \param selectionMode
///
void DAChartRectRegionSelectEditor::setSelectionMode(const DAChartRectRegionSelectEditor::SelectionMode& selectionMode)
{
    DAAbstractRegionSelectEditor::setSelectionMode(selectionMode);
}
///
/// \brief rtti
/// \return
///
int DAChartRectRegionSelectEditor::rtti() const
{
    return RTTIRectRegionSelectEditor;
}

/**
 * @brief 清理数据
 */
void DAChartRectRegionSelectEditor::clear()
{
    d_ptr->releaseTmpItem();
    d_ptr->mSelectedRect    = QRectF();
    d_ptr->mLastPainterPath = QPainterPath();
}

/**
 * @brief 绘图项附加事件处理
 * @param item 附加的绘图项
 * @param on 是否附加，false表示项被移除
 */
void DAChartRectRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on) {
        if (item == d_ptr->mTmpItem) {
            d_ptr->mTmpItem = nullptr;
        }
    }
}
}  // End Of Namespace DA
