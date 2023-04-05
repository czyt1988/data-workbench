#include "DAChartPolygonRegionSelectEditor.h"
#include <QMouseEvent>
#include <QKeyEvent>
namespace DA
{
class DAChartPolygonRegionSelectEditorPrivate
{
    DA_IMPL_PUBLIC(DAChartPolygonRegionSelectEditor)
public:
    bool m_isStartDrawRegion;  ///< 是否生效
    DAChartSelectRegionShapeItem* m_tmpItem;
    QPolygonF m_polygon;       ///< 多边形
    bool m_isFinishOneRegion;  ///< 标定是否已经完成了一次区域，m_tmpItem还是m_shapeItem显示
    QPainterPath m_lastPainterPath;

    DAChartPolygonRegionSelectEditorPrivate(DAChartPolygonRegionSelectEditor* p)
        : q_ptr(p), m_isStartDrawRegion(false), m_tmpItem(nullptr), m_isFinishOneRegion(false)
    {
    }
    ~DAChartPolygonRegionSelectEditorPrivate()
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

DAChartPolygonRegionSelectEditor::DAChartPolygonRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), d_ptr(new DAChartPolygonRegionSelectEditorPrivate(this))
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartPolygonRegionSelectEditor::onItemAttached);
}

DAChartPolygonRegionSelectEditor::~DAChartPolygonRegionSelectEditor()
{
}

QPainterPath DAChartPolygonRegionSelectEditor::getSelectRegion() const
{
    return d_ptr->m_lastPainterPath;
}

void DAChartPolygonRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->m_lastPainterPath = shape;
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
        if (item == d_ptr->m_tmpItem) {
            d_ptr->m_tmpItem = nullptr;
        }
    }
}

bool DAChartPolygonRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p                   = e->pos();
    d_ptr->m_isStartDrawRegion = true;
    d_ptr->createTmpItem();
    d_ptr->m_polygon.append(invTransform(p));
    if (d_ptr->m_tmpItem) {
        d_ptr->m_tmpItem->setPolygon(d_ptr->m_polygon);
    }
    return true;
}

bool DAChartPolygonRegionSelectEditor::mouseMovedEvent(const QMouseEvent* e)
{
    if (!d_ptr->m_isStartDrawRegion) {
        return false;
    }
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p      = e->pos();
    QPointF pf    = invTransform(p);
    QPolygonF tmp = d_ptr->m_polygon;
    tmp.append(pf);
    if (d_ptr->m_tmpItem) {
        d_ptr->m_tmpItem->setPolygon(tmp);
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
    if (d_ptr->m_polygon.size() <= 2) {
        d_ptr->m_polygon.clear();
        d_ptr->releaseTmpItem();
        d_ptr->m_isStartDrawRegion = false;
        return false;  //点数不足，完成失败
    } else {
        //点数足够，封闭多边形
        if (d_ptr->m_polygon.last() != d_ptr->m_polygon.first()) {
            d_ptr->m_polygon.append(d_ptr->m_polygon.first());
        }
    }
    QPainterPath painterPath;
    painterPath.addPolygon(d_ptr->m_polygon);
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
    d_ptr->m_polygon.clear();
    d_ptr->m_isStartDrawRegion = false;
    emit finishSelection(d_ptr->m_lastPainterPath);
    return true;
}
///
/// \brief 回退
/// \return
///
bool DAChartPolygonRegionSelectEditor::backspaceRegion()
{
    if (!d_ptr->m_isStartDrawRegion) {
        return false;
    }
    if (d_ptr->m_polygon.size() <= 1) {
        return false;
    }
    d_ptr->m_polygon.pop_back();
    if (d_ptr->m_tmpItem) {
        d_ptr->m_tmpItem->setPolygon(d_ptr->m_polygon);
    }
    return true;
}
}  // End Of Namespace DA
