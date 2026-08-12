#include "DAChartEllipseRegionSelectEditor.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
namespace DA
{
class DAChartEllipseRegionSelectEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartEllipseRegionSelectEditor)
public:
    bool mIsStartDrawRegion { false };
    DAChartSelectRegionShapeItem* mTmpItem { nullptr };
    QPointF mPressedPoint;
    QRectF mSelectedRect;
    QPainterPath mLastPainterPath;
    bool mIsPlotEnableZoom { false };  ///< 记录绘图是否允许缩放，在结束的时候还原状态
public:
    /**
     * @brief 构造函数
     * @param p 父对象指针
     */
    PrivateData(DAChartEllipseRegionSelectEditor* p) : q_ptr(p)
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
     * @brief 释放临时选区图元
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
     * @brief 创建临时选区图元
     */
    void createTmpItem()
    {
        if (nullptr == mTmpItem) {
            mTmpItem = new DAChartSelectRegionShapeItem("temp region");
            mTmpItem->attach(q_ptr->plot());
        }
    }
};

//===================================================
// DAChartEllipseRegionSelectEditor
//===================================================

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot对象
 */
DAChartEllipseRegionSelectEditor::DAChartEllipseRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartEllipseRegionSelectEditor::onItemAttached);
}

/**
 * @brief 析构函数
 */
DAChartEllipseRegionSelectEditor::~DAChartEllipseRegionSelectEditor()
{
}

/**
 * @brief 获取当前选区路径
 * @return 选区的QPainterPath
 */
QPainterPath DAChartEllipseRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->mLastPainterPath;
}

/**
 * @brief 设置选区路径
 * @param shape 选区路径
 */
void DAChartEllipseRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->mLastPainterPath = shape;
}

/**
 * @brief 设置选择模式
 * @param selectionMode 选择模式
 */
void DAChartEllipseRegionSelectEditor::setSelectionMode(const DAAbstractRegionSelectEditor::SelectionMode& selectionMode)
{
    DAAbstractRegionSelectEditor::setSelectionMode(selectionMode);
}
///
/// \brief rtti
/// \return
///
int DAChartEllipseRegionSelectEditor::rtti() const
{
    return RTTIEllipseRegionSelectEditor;
}

///
/// \brief 清理数据
///
void DAChartEllipseRegionSelectEditor::clear()
{
    d_ptr->releaseTmpItem();
    d_ptr->mSelectedRect    = QRectF();
    d_ptr->mLastPainterPath = QPainterPath();
}

/**
 * @brief 取消当前选区编辑
 * @return 取消成功返回true
 */
bool DAChartEllipseRegionSelectEditor::cancel()
{
    clear();
    return true;
}

/**
 * @brief 提取临时选区图元
 * @return 临时选区图元指针
 */
QwtPlotItem* DAChartEllipseRegionSelectEditor::takeItem()
{
    QwtPlotItem* item = d_ptr->mTmpItem;
    d_ptr->mTmpItem   = nullptr;
    return item;
}

/**
 * @brief 图元附加到绘图时触发的槽函数
 * @param item 附加的图元
 * @param on 是否附加
 */
void DAChartEllipseRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on) {
        if (item == d_ptr->mTmpItem) {
            d_ptr->mTmpItem = nullptr;
        }
    }
}

/**
 * @brief 鼠标按下事件处理
 * @param e 鼠标事件
 * @return 处理成功返回true
 */
bool DAChartEllipseRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p = compat::eventPos(e);

    if (!d_ptr->mIsStartDrawRegion) {
        d_ptr->createTmpItem();
        DAChartWidget* chart      = qobject_cast< DAChartWidget* >(parent());
        d_ptr->mIsPlotEnableZoom = chart->isZoomEnabled();
        if (d_ptr->mIsPlotEnableZoom) {
            chart->enableZoom(false);
        }
    }
    d_ptr->mIsStartDrawRegion = true;
    d_ptr->mPressedPoint      = invTransform(p);
    d_ptr->createTmpItem();
    switch (getSelectionMode()) {
    case SingleSelection:  // 单一选择
    {
        d_ptr->mLastPainterPath = QPainterPath();
        break;
    }
    default:
        return false;
    }
    Q_EMIT beginEdit();
    return true;
}

/**
 * @brief 鼠标移动事件处理
 * @param e 鼠标事件
 * @return 处理成功返回true
 */
bool DAChartEllipseRegionSelectEditor::mouseMoveEvent(const QMouseEvent* e)
{
    if (!d_ptr->mIsStartDrawRegion) {
        return false;
    }
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    DA_D(d);
    QPoint p = compat::eventPos(e);

    QPointF pf = invTransform(p);
    d->mSelectedRect.setX(d->mPressedPoint.x());
    d->mSelectedRect.setY(d->mPressedPoint.y());
    d->mSelectedRect.setWidth(pf.x() - d->mPressedPoint.x());
    d->mSelectedRect.setHeight(pf.y() - d->mPressedPoint.y());
    if (d->mTmpItem) {
        d->mTmpItem->setEllipse(d->mSelectedRect);
        if(QwtPlot* p = d->mTmpItem->plot()){
            p->replot();
        }
    }
    return true;
}

/**
 * @brief 鼠标释放事件处理
 * @param e 鼠标事件
 * @return 处理成功返回true
 */
bool DAChartEllipseRegionSelectEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p = compat::eventPos(e);

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
    painterPath.addEllipse(d_ptr->mSelectedRect);
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
        chart->enableZoom(true);
    }
    Q_EMIT finishSelection(d_ptr->mLastPainterPath);
    Q_EMIT finishedEdit(false);
    return true;
}
}  // End Of Namespace DA
