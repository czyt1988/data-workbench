#include "DAChartPolygonRegionSelectEditor.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include "da_qt5qt6_compat.hpp"

namespace DA
{
class DAChartPolygonRegionSelectEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartPolygonRegionSelectEditor)
public:
    bool mIsStartDrawRegion { false };  ///< 是否生效
    bool mIsFinishOneRegion { false };  ///< 标定是否已经完成了一次区域，mTmpItem还是mShapeItem显示
    DAChartSelectRegionShapeItem* mTmpItem { nullptr };
    QPolygonF mPolygon;  ///< 多边形
    QPainterPath mLastPainterPath;
    static bool isPointClose(const QPoint& p1, const QPoint& p2, int threshold = 3);

public:
    /**
     * @brief 构造函数
     * @param p 父对象指针
     */
    PrivateData(DAChartPolygonRegionSelectEditor* p) : q_ptr(p)
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
     * @brief 释放临时图元
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
     * @brief 创建临时图元
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
 * @brief 判断两个点是否足够接近
 * @param p1 第一个点
 * @param p2 第二个点
 * @param threshold 距离阈值，默认为3
 * @return 如果两点距离小于阈值返回true，否则返回false
 */
bool DAChartPolygonRegionSelectEditor::PrivateData::isPointClose(const QPoint& p1, const QPoint& p2, int threshold)
{
    int dx = p1.x() - p2.x();
    int dy = p1.y() - p2.y();
    return (dx * dx + dy * dy) < threshold * threshold;
}
//===================================================
// DAChartPolygonRegionSelectEditor
//===================================================

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot指针
 */
DAChartPolygonRegionSelectEditor::DAChartPolygonRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartPolygonRegionSelectEditor::onItemAttached);
}

/**
 * @brief 析构函数
 */
DAChartPolygonRegionSelectEditor::~DAChartPolygonRegionSelectEditor()
{
}

/**
 * @brief 获取当前选中的区域
 * @return 返回选中区域的QPainterPath
 */
QPainterPath DAChartPolygonRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->mLastPainterPath;
}

/**
 * @brief 设置选中区域
 * @param shape 要设置的QPainterPath
 */
void DAChartPolygonRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->mLastPainterPath = shape;
}

/**
 * @brief 设置选择模式
 * @param selectionMode 选择模式
 */
void DAChartPolygonRegionSelectEditor::setSelectionMode(const DAAbstractRegionSelectEditor::SelectionMode& selectionMode)
{
    DAAbstractRegionSelectEditor::setSelectionMode(selectionMode);
}

/**
 * @brief 获取运行时类型标识
 * @return 返回RTTIPolygonRegionSelectEditor
 */
int DAChartPolygonRegionSelectEditor::rtti() const
{
    return RTTIPolygonRegionSelectEditor;
}

/**
 * @brief 清除所有状态和临时图元
 */
void DAChartPolygonRegionSelectEditor::clear()
{
    d_ptr->releaseTmpItem();
    d_ptr->mPolygon         = QPolygonF();
    d_ptr->mLastPainterPath = QPainterPath();
}

/**
 * @brief 取消当前操作
 * @return 始终返回true
 */
bool DAChartPolygonRegionSelectEditor::cancel()
{
    clear();
    return true;
}

/**
 * @brief 取出临时图元并转移所有权
 * @return 返回临时图元指针，调用者获得所有权
 */
QwtPlotItem* DAChartPolygonRegionSelectEditor::takeItem()
{
    QwtPlotItem* item = d_ptr->mTmpItem;
    d_ptr->mTmpItem   = nullptr;
    return item;
}

/**
 * @brief 图元附加到plot时的回调
 * @param item 关联的图元
 * @param on 是否附加
 */
void DAChartPolygonRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
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
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAChartPolygonRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button()) {
        return false;
    }
    QPoint p = compat::eventPos(e);

    DA_D(d);
    bool firstBegin = (!d->mIsStartDrawRegion);
    if (Qt::LeftButton == e->button()) {
        if (!d->mIsStartDrawRegion) {
            d->mIsStartDrawRegion = true;
            d->createTmpItem();
        }
        QPointF pf = invTransform(p);
        if (d->mPolygon.size() > 1) {
            // 这里要检测pf是否和第一个点接近，如果接近，就相当于闭合多边形
            QPoint firstScreenPos = transform(d->mPolygon.first()).toPoint();
            if (PrivateData::isPointClose(p, firstScreenPos, 5)) {
                completeRegion();
                // 结束
                return true;
            }
        }

        d->mPolygon.append(pf);
        if (d->mTmpItem) {
            d->mTmpItem->setPolygon(d->mPolygon);
        }
        if (firstBegin) {
            Q_EMIT beginEdit();
        }
    } else if (Qt::RightButton == e->button()) {
        // 右键取消上个点击点
        if (d->mPolygon.size() > 0) {
            d->mPolygon.pop_back();
            if (d->mTmpItem) {
                d->mTmpItem->setPolygon(d->mPolygon);
                if(QwtPlot* p = d_ptr->mTmpItem->plot()){
                    p->replot();
                }
            }
        }
    }
    return true;
}

/**
 * @brief 鼠标移动事件处理
 * @param e 鼠标事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAChartPolygonRegionSelectEditor::mouseMoveEvent(const QMouseEvent* e)
{
    if (!d_ptr->mIsStartDrawRegion) {
        return false;
    }
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p      = e->pos();
    QPointF pf    = invTransform(p);
    QPolygonF tmp = d_ptr->mPolygon;
    tmp.append(pf);
    if (d_ptr->mTmpItem) {
        d_ptr->mTmpItem->setPolygon(tmp);
        if(QwtPlot* p = d_ptr->mTmpItem->plot()){
            p->replot();
        }
    }
    return DAAbstractRegionSelectEditor::mouseMoveEvent(e);  // 把移动的事件继续传递下去
}

/**
 * @brief 键盘按键事件处理
 * @param e 键盘事件
 * @return 如果事件被处理返回true，否则返回false
 */
bool DAChartPolygonRegionSelectEditor::keyPressEvent(const QKeyEvent* e)
{
    if (Qt::Key_Enter == e->key() || Qt::Key_Return == e->key()) {
        return completeRegion();
    } else if (Qt::Key_Backspace == e->key()) {
        return backspaceRegion();
    }
    return DAAbstractRegionSelectEditor::keyPressEvent(e);
}

/**
 * @brief 完成多边形区域选择
 * @return 如果成功完成返回true，点数不足时返回false
 */
bool DAChartPolygonRegionSelectEditor::completeRegion()
{
    if (d_ptr->mPolygon.size() <= 2) {
        d_ptr->mPolygon.clear();
        d_ptr->releaseTmpItem();
        d_ptr->mIsStartDrawRegion = false;
        return false;  // 点数不足，完成失败
    } else {
        // 点数足够，封闭多边形
        if (d_ptr->mPolygon.last() != d_ptr->mPolygon.first()) {
            d_ptr->mPolygon.append(d_ptr->mPolygon.first());
        }
    }
    QPainterPath painterPath;
    painterPath.addPolygon(d_ptr->mPolygon);
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
    Q_EMIT finishSelection(d_ptr->mLastPainterPath);
    Q_EMIT finishedEdit(false);
    return true;
}
///
/// \brief 回退
/// \return
///
bool DAChartPolygonRegionSelectEditor::backspaceRegion()
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
}  // End Of Namespace DA
