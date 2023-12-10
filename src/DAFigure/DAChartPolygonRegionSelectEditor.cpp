#include "DAChartPolygonRegionSelectEditor.h"
#include <QMouseEvent>
#include <QKeyEvent>
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
    QPoint p                  = e->pos();
    d_ptr->mIsStartDrawRegion = true;
    d_ptr->createTmpItem();
    d_ptr->mPolygon.append(invTransform(p));
    if (d_ptr->mTmpItem) {
        d_ptr->mTmpItem->setPolygon(d_ptr->mPolygon);
    }
    return true;
}

bool DAChartPolygonRegionSelectEditor::mouseMovedEvent(const QMouseEvent* e)
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
    }
    return false;  //把移动的事件继续传递下去
}

bool DAChartPolygonRegionSelectEditor::keyPressEvent(const QKeyEvent* e)
{
    if (Qt::Key_Enter == e->key() || Qt::Key_Return == e->key()) {
        return completeRegion();
    } else if (Qt::Key_Escape == e->key() || Qt::Key_Backspace == e->key()) {
        return backspaceRegion();
    }
    return false;
}

bool DAChartPolygonRegionSelectEditor::completeRegion()
{
    if (d_ptr->mPolygon.size() <= 2) {
        d_ptr->mPolygon.clear();
        d_ptr->releaseTmpItem();
        d_ptr->mIsStartDrawRegion = false;
        return false;  //点数不足，完成失败
    } else {
        //点数足够，封闭多边形
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
    d_ptr->releaseTmpItem();
    d_ptr->mPolygon.clear();
    d_ptr->mIsStartDrawRegion = false;
    emit finishSelection(d_ptr->mLastPainterPath);
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
