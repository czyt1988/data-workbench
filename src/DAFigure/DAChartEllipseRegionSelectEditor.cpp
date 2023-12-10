#include "DAChartEllipseRegionSelectEditor.h"
#include <QMouseEvent>
#include <QKeyEvent>
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

public:
    PrivateData(DAChartEllipseRegionSelectEditor* p) : q_ptr(p)
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
// DAChartEllipseRegionSelectEditor
//===================================================

DAChartEllipseRegionSelectEditor::DAChartEllipseRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartEllipseRegionSelectEditor::onItemAttached);
}

DAChartEllipseRegionSelectEditor::~DAChartEllipseRegionSelectEditor()
{
}

QPainterPath DAChartEllipseRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->mLastPainterPath;
}

void DAChartEllipseRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->mLastPainterPath = shape;
}

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

void DAChartEllipseRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on) {
        if (item == d_ptr->mTmpItem) {
            d_ptr->mTmpItem = nullptr;
        }
    }
}

bool DAChartEllipseRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p                  = e->pos();
    d_ptr->mIsStartDrawRegion = true;
    d_ptr->mPressedPoint      = invTransform(p);
    d_ptr->createTmpItem();
    switch (getSelectionMode()) {
    case SingleSelection:  //单一选择
    {
        d_ptr->mLastPainterPath = QPainterPath();
        break;
    }
    default:
        return false;
    }
    return true;
}

bool DAChartEllipseRegionSelectEditor::mouseMovedEvent(const QMouseEvent* e)
{
    if (!d_ptr->mIsStartDrawRegion) {
        return false;
    }
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = e->pos();
    QPointF pf = invTransform(p);
    d_ptr->mSelectedRect.setX(d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setY(d_ptr->mPressedPoint.y());
    d_ptr->mSelectedRect.setWidth(pf.x() - d_ptr->mPressedPoint.x());
    d_ptr->mSelectedRect.setHeight(pf.y() - d_ptr->mPressedPoint.y());
    if (d_ptr->mTmpItem) {
        d_ptr->mTmpItem->setEllipse(d_ptr->mSelectedRect);
    }
    return true;
}

bool DAChartEllipseRegionSelectEditor::mouseReleasedEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = e->pos();
    QPointF pf = invTransform(p);
    if (pf == d_ptr->mPressedPoint) {
        //如果点击和松开是一个点，就取消当前的选区
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
    d_ptr->releaseTmpItem();
    d_ptr->mIsStartDrawRegion = false;
    emit finishSelection(d_ptr->mLastPainterPath);
    return true;
}
}  // End Of Namespace DA
