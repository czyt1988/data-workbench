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
    bool m_isStartDrawRegion { false };
    DAChartSelectRegionShapeItem* m_tmpItem { nullptr };
    QPointF m_pressedPoint { 0, 0 };
    QRectF m_selectedRect { 0, 0, 0, 0 };
    QPainterPath m_lastPainterPath;
    bool m_isPlotEnableZoom { false };  ///< 记录绘图是否允许缩放，在结束的时候还原状态
public:
    PrivateData(DAChartRectRegionSelectEditor* p) : q_ptr(p)
    {
    }
    ~PrivateData()
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
    : DAAbstractRegionSelectEditor(parent), DA_PIMPL_CONSTRUCT
{
    setEnabled(true);
    connect(parent, &QwtPlot::itemAttached, this, &DAChartRectRegionSelectEditor::onItemAttached);
}

DAChartRectRegionSelectEditor::~DAChartRectRegionSelectEditor()
{
}

bool DAChartRectRegionSelectEditor::mousePressEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p = compat::eventPos(e);
    if (!d_ptr->m_isStartDrawRegion) {
        d_ptr->createTmpItem();
        DAChartWidget* chart      = qobject_cast< DAChartWidget* >(parent());
        d_ptr->m_isPlotEnableZoom = chart->isZoomEnabled();
        if (d_ptr->m_isPlotEnableZoom) {
            chart->enableZoom(false);
        }
    }
    d_ptr->m_isStartDrawRegion = true;
    d_ptr->m_pressedPoint      = invTransform(p);
    switch (getSelectionMode()) {
    case SingleSelection: {
        d_ptr->m_lastPainterPath = QPainterPath();
    }
    default:
        break;
    }
    Q_EMIT beginEdit();
    return true;
}

bool DAChartRectRegionSelectEditor::mouseMoveEvent(const QMouseEvent* e)
{
    if (!d_ptr->m_isStartDrawRegion) {
        return false;
    }
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);

    d_ptr->m_selectedRect.setX(d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setY(d_ptr->m_pressedPoint.y());
    d_ptr->m_selectedRect.setWidth(pf.x() - d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setHeight(pf.y() - d_ptr->m_pressedPoint.y());
    if (d_ptr->m_tmpItem) {
        d_ptr->m_tmpItem->setRect(d_ptr->m_selectedRect);
    }
    return true;
}

bool DAChartRectRegionSelectEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    if (Qt::MiddleButton == e->button() || Qt::RightButton == e->button()) {
        return false;
    }
    QPoint p   = compat::eventPos(e);
    QPointF pf = invTransform(p);
    if (pf == d_ptr->m_pressedPoint) {
        // 如果点击和松开是一个点，就取消当前的选区
        d_ptr->releaseTmpItem();
        d_ptr->m_isStartDrawRegion = false;
        return true;
    }
    d_ptr->m_selectedRect.setX(d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setY(d_ptr->m_pressedPoint.y());
    d_ptr->m_selectedRect.setWidth(pf.x() - d_ptr->m_pressedPoint.x());
    d_ptr->m_selectedRect.setHeight(pf.y() - d_ptr->m_pressedPoint.y());
    QPainterPath painterPath;
    painterPath.addRect(d_ptr->m_selectedRect);
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
    d_ptr->m_isStartDrawRegion = false;
    if (d_ptr->m_isPlotEnableZoom) {
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(parent());
        // 还原zoomer
        chart->enableZoom(true);
    }
    Q_EMIT finishSelection(d_ptr->m_lastPainterPath);
    Q_EMIT finishedEdit(false);
    return true;
}

bool DAChartRectRegionSelectEditor::cancel()
{
    clear();
    return true;
}

/**
 * @brief 获取选框对应的item
 * @return
 */
DAChartSelectRegionShapeItem* DAChartRectRegionSelectEditor::takeItem()
{
    DAChartSelectRegionShapeItem* item = d_ptr->m_tmpItem;
    d_ptr->m_tmpItem                   = nullptr;
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
    return d_ptr->m_lastPainterPath;
}
///
/// \brief SARectRegionSelectEditor::setSelectRegion
/// \param shape
///
void DAChartRectRegionSelectEditor::setSelectRegion(const QPainterPath& shape)
{
    d_ptr->m_lastPainterPath = shape;
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
    d_ptr->m_selectedRect    = QRectF();
    d_ptr->m_lastPainterPath = QPainterPath();
}

void DAChartRectRegionSelectEditor::onItemAttached(QwtPlotItem* item, bool on)
{
    if (!on) {
        if (item == d_ptr->m_tmpItem) {
            d_ptr->m_tmpItem = nullptr;
        }
    }
}
}  // End Of Namespace DA
