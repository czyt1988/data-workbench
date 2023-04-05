#include "DAChartRectRegionSelectEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QKeyEvent>
namespace DA
{
class DAChartRectRegionSelectEditorPrivate
{
    DA_IMPL_PUBLIC(DAChartRectRegionSelectEditor)
public:
    bool m_isStartDrawRegion;
    DAChartSelectRegionShapeItem* m_tmpItem;
    QPointF m_pressedPoint;
    QRectF m_selectedRect;
    QPainterPath m_lastPainterPath;

public:
    DAChartRectRegionSelectEditorPrivate(DAChartRectRegionSelectEditor* p)
        : q_ptr(p), m_isStartDrawRegion(false), m_tmpItem(nullptr), m_selectedRect(QRectF())
    {
    }
    ~DAChartRectRegionSelectEditorPrivate()
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

DAChartRectRegionSelectEditor::DAChartRectRegionSelectEditor(QwtPlot* parent)
    : DAAbstractRegionSelectEditor(parent), d_ptr(new DAChartRectRegionSelectEditorPrivate(this))
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartRectRegionSelectEditor::onItemAttached);
}

DAChartRectRegionSelectEditor::~DAChartRectRegionSelectEditor()
{
}

bool DAChartRectRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    DA_D(DAChartRectRegionSelectEditor, d);
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p = e->pos();
    d->createTmpItem();
    d->m_isStartDrawRegion = true;
    d->m_pressedPoint      = invTransform(p);
    switch (getSelectionMode()) {
    case SingleSelection: {
        d->m_lastPainterPath = QPainterPath();
    }
    default:
        break;
    }
    return true;
}

bool DAChartRectRegionSelectEditor::mouseMovedEvent(const QMouseEvent* e)
{
    DA_D(DAChartRectRegionSelectEditor, d);
    if (!d->m_isStartDrawRegion) {
        return false;
    }
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = e->pos();
    QPointF pf = invTransform(p);
    d->m_selectedRect.setX(d->m_pressedPoint.x());
    d->m_selectedRect.setY(d->m_pressedPoint.y());
    d->m_selectedRect.setWidth(pf.x() - d->m_pressedPoint.x());
    d->m_selectedRect.setHeight(pf.y() - d->m_pressedPoint.y());
    if (d->m_tmpItem) {
        d->m_tmpItem->setRect(d->m_selectedRect);
    }
    return true;
}

bool DAChartRectRegionSelectEditor::mouseReleasedEvent(const QMouseEvent* e)
{
    DA_D(DAChartRectRegionSelectEditor, d);
    if (Qt::MidButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = e->pos();
    QPointF pf = invTransform(p);
    if (pf == d->m_pressedPoint) {
        //如果点击和松开是一个点，就取消当前的选区
        d->releaseTmpItem();
        d->m_isStartDrawRegion = false;
        return true;
    }
    d->m_selectedRect.setX(d->m_pressedPoint.x());
    d->m_selectedRect.setY(d->m_pressedPoint.y());
    d->m_selectedRect.setWidth(pf.x() - d->m_pressedPoint.x());
    d->m_selectedRect.setHeight(pf.y() - d->m_pressedPoint.y());
    QPainterPath painterPath;
    painterPath.addRect(d->m_selectedRect);
    switch (getSelectionMode()) {
    case SingleSelection: {
        d->m_lastPainterPath = painterPath;
        break;
    }
    case AdditionalSelection: {
        d->m_lastPainterPath = d->m_lastPainterPath.united(painterPath);
        break;
    }
    case SubtractionSelection: {
        d->m_lastPainterPath = d->m_lastPainterPath.subtracted(painterPath);
        break;
    }
    case IntersectionSelection: {
        d->m_lastPainterPath = d->m_lastPainterPath.intersected(painterPath);
        break;
    }
    default:
        break;
    }
    d->releaseTmpItem();
    d->m_isStartDrawRegion = false;
    emit finishSelection(d->m_lastPainterPath);
    return true;
}
///
/// \brief 处理按钮事件
/// \param e
///
bool DAChartRectRegionSelectEditor::keyPressEvent(const QKeyEvent* e)
{
    return DAAbstractRegionSelectEditor::keyPressEvent(e);
}
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
    DA_DC(DAChartRectRegionSelectEditor, d);
    return d->m_lastPainterPath;
}
///
/// \brief SARectRegionSelectEditor::setSelectRegion
/// \param shape
///
void DAChartRectRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    DA_D(DAChartRectRegionSelectEditor, d);
    d->m_lastPainterPath = shape;
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

///
/// \brief 清理数据
///
void DAChartRectRegionSelectEditor::clear()
{
    DA_D(DAChartRectRegionSelectEditor, d);
    d->releaseTmpItem();
    d->m_selectedRect    = QRectF();
    d->m_lastPainterPath = QPainterPath();
}

void DAChartRectRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    DA_D(DAChartRectRegionSelectEditor, d);
    if (!on) {
        if (item == d->m_tmpItem) {
            d->m_tmpItem = nullptr;
        }
    }
}
}  // End Of Namespace DA
