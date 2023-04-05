#include "DAChartEllipseRegionSelectEditor.h"
#include <QMouseEvent>
#include <QKeyEvent>
namespace DA
{
class DAChartEllipseRegionSelectEditorPrivate
{
    DA_IMPL_PUBLIC(DAChartEllipseRegionSelectEditor)
public:
    bool m_isStartDrawRegion;
    DAChartSelectRegionShapeItem* m_tmpItem;
    QPointF m_pressedPoint;
    QRectF m_selectedRect;
    QPainterPath m_lastPainterPath;
    DAChartEllipseRegionSelectEditorPrivate(DAChartEllipseRegionSelectEditor* p)
        : q_ptr(p), m_isStartDrawRegion(false), m_tmpItem(nullptr), m_selectedRect(QRectF())
    {
    }
    ~DAChartEllipseRegionSelectEditorPrivate()
    {
        releaseTmpItem();
    }
    void releaseTmpItem()
    {
        if (m_tmpItem) {
            m_tmpItem->detach();
            delete m_tmpItem;
            m_tmpItem = nullptr;
        }
    }
    void createTmpItem()
    {
        if (nullptr == m_tmpItem) {
            m_tmpItem = new DAChartSelectRegionShapeItem("temp region");
            m_tmpItem->attach(q_ptr->plot());
        }
    }
};

DAChartEllipseRegionSelectEditor::DAChartEllipseRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), d_ptr(new DAChartEllipseRegionSelectEditorPrivate(this))
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartEllipseRegionSelectEditor::onItemAttached);
}

DAChartEllipseRegionSelectEditor::~DAChartEllipseRegionSelectEditor()
{
}

QPainterPath DAChartEllipseRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->m_lastPainterPath;
}

void DAChartEllipseRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->m_lastPainterPath = shape;
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
    d_ptr->m_selectedRect    = QRectF();
    d_ptr->m_lastPainterPath = QPainterPath();
}

void DAChartEllipseRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on) {
        if (item == d_ptr->m_tmpItem) {
            d_ptr->m_tmpItem = nullptr;
        }
    }
}

bool DAChartEllipseRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p                   = e->pos();
    d_ptr->m_isStartDrawRegion = true;
    d_ptr->m_pressedPoint      = invTransform(p);
    d_ptr->createTmpItem();
    switch (getSelectionMode()) {
    case SingleSelection:  //单一选择
    {
        d_ptr->m_lastPainterPath = QPainterPath();
        break;
    }
    default:
        return false;
    }
    return true;
}

bool DAChartEllipseRegionSelectEditor::mouseMovedEvent(const QMouseEvent* e)
{
    if (!d_ptr->m_isStartDrawRegion) {
        return false;
    }
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = e->pos();
    QPointF pf = invTransform(p);
    d_ptr->m_selectedRect.setX(d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setY(d_ptr->m_pressedPoint.y());
    d_ptr->m_selectedRect.setWidth(pf.x() - d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setHeight(pf.y() - d_ptr->m_pressedPoint.y());
    if (d_ptr->m_tmpItem) {
        d_ptr->m_tmpItem->setEllipse(d_ptr->m_selectedRect);
    }
    return true;
}

bool DAChartEllipseRegionSelectEditor::mouseReleasedEvent(const QMouseEvent* e)
{
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = e->pos();
    QPointF pf = invTransform(p);
    if (pf == d_ptr->m_pressedPoint) {
        //如果点击和松开是一个点，就取消当前的选区
        d_ptr->releaseTmpItem();
        d_ptr->m_isStartDrawRegion = false;
        return true;
    }
    d_ptr->m_selectedRect.setX(d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setY(d_ptr->m_pressedPoint.y());
    d_ptr->m_selectedRect.setWidth(pf.x() - d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setHeight(pf.y() - d_ptr->m_pressedPoint.y());
    QPainterPath painterPath;
    painterPath.addEllipse(d_ptr->m_selectedRect);
    switch (getSelectionMode()) {
    case SingleSelection: {
        d_ptr->m_lastPainterPath = painterPath;
        break;
    }
    case AdditionalSelection: {
        d_ptr->m_lastPainterPath = d_ptr->m_lastPainterPath.united(painterPath);
        break;
    }
    case SubtractionSelection: {
        d_ptr->m_lastPainterPath = d_ptr->m_lastPainterPath.subtracted(painterPath);
        break;
    }
    case IntersectionSelection: {
        d_ptr->m_lastPainterPath = d_ptr->m_lastPainterPath.intersected(painterPath);
        break;
    }
    default:
        break;
    }
    d_ptr->releaseTmpItem();
    d_ptr->m_isStartDrawRegion = false;
    emit finishSelection(d_ptr->m_lastPainterPath);
    return true;
}
}  // End Of Namespace DA
