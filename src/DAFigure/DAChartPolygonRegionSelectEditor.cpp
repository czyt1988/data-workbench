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
    bool mIsFinishOneRegion { false };  ///< 标定是否已经完成了一次区域，m_tmpItem还是m_shapeItem显示
    DAChartSelectRegionShapeItem* mTmpItem { nullptr };
    QPolygonF mPolygon;  ///< 多边形
    QPainterPath mLastPainterPath;
    static bool isPointClose(const QPoint& p1, const QPoint& p2, int threshold = 3);

public:
    PrivateData(DAChartPolygonRegionSelectEditor* p) : q_ptr(p)
    {
    }
    ~PrivateData()
    {
        releaseTmpItem();
    }

    void releaseTmpItem()
    {
        if (mTmpItem) {
            mTmpItem->detach();
            delete mTmpItem;
            mTmpItem = nullptr;
        }
    }
    void createTmpItem()
    {
        if (nullptr == mTmpItem) {
            mTmpItem = new DAChartSelectRegionShapeItem("temp region");
            mTmpItem->attach(q_ptr->plot());
        }
    }
};

bool DAChartPolygonRegionSelectEditor::PrivateData::isPointClose(const QPoint& p1, const QPoint& p2, int threshold)
{
    int dx = p1.x() - p2.x();
    int dy = p1.y() - p2.y();
    return (dx * dx + dy * dy) < threshold * threshold;
}
//===================================================
// DAChartPolygonRegionSelectEditor
//===================================================

DAChartPolygonRegionSelectEditor::DAChartPolygonRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartPolygonRegionSelectEditor::onItemAttached);
}

DAChartPolygonRegionSelectEditor::~DAChartPolygonRegionSelectEditor()
{
}

QPainterPath DAChartPolygonRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->mLastPainterPath;
}

void DAChartPolygonRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->mLastPainterPath = shape;
}

void DAChartPolygonRegionSelectEditor::setSelectionMode(const DAAbstractRegionSelectEditor::SelectionMode& selectionMode)
{
    DAAbstractRegionSelectEditor::setSelectionMode(selectionMode);
}

int DAChartPolygonRegionSelectEditor::rtti() const
{
    return RTTIPolygonRegionSelectEditor;
}

void DA::DAChartPolygonRegionSelectEditor::clear()
{
    d_ptr->releaseTmpItem();
    d_ptr->mPolygon         = QPolygonF();
    d_ptr->mLastPainterPath = QPainterPath();
}

bool DA::DAChartPolygonRegionSelectEditor::cancel()
{
    clear();
    return true;
}

DAChartSelectRegionShapeItem* DA::DAChartPolygonRegionSelectEditor::takeItem()
{
    DAChartSelectRegionShapeItem* item = d_ptr->mTmpItem;
    d_ptr->mTmpItem                    = nullptr;
    return item;
}

void DAChartPolygonRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on) {
        if (item == d_ptr->mTmpItem) {
            d_ptr->mTmpItem = nullptr;
        }
    }
}

bool DAChartPolygonRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p = compat::eventPos(e);
    DA_D(d);
    bool firstBegin = (!d->mIsStartDrawRegion);
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
    return true;
}

bool DAChartPolygonRegionSelectEditor::mouseMoveEvent(const QMouseEvent* e)
{
    // if (!d_ptr->mIsStartDrawRegion) {
    //     return false;
    // }
    // if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
    //     return false;
    // }
    // QPoint p      = e->pos();
    // QPointF pf    = invTransform(p);
    // QPolygonF tmp = d_ptr->mPolygon;
    // tmp.append(pf);
    // if (d_ptr->mTmpItem) {
    //     d_ptr->mTmpItem->setPolygon(tmp);
    // }
    return DAAbstractRegionSelectEditor::mouseMoveEvent(e);  // 把移动的事件继续传递下去
}

bool DAChartPolygonRegionSelectEditor::keyPressEvent(const QKeyEvent* e)
{
    if (Qt::Key_Enter == e->key() || Qt::Key_Return == e->key()) {
        return completeRegion();
    } else if (Qt::Key_Backspace == e->key()) {
        return backspaceRegion();
    }
    return DAAbstractRegionSelectEditor::keyPressEvent(e);
}

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
