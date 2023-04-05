#include "DAAbstractRegionSelectEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include "qwt_scale_map.h"
namespace DA
{
DAAbstractRegionSelectEditor::DAAbstractRegionSelectEditor(QwtPlot* parent)
    : DAAbstractPlotEditor(parent)
    , m_selectionMode(AdditionalSelection)  // SingleSelection
    , m_xAxis(QwtPlot::xBottom)
    , m_yAxis(QwtPlot::yLeft)
{
    m_xAxis             = QwtPlot::xBottom;
    const QwtPlot* plot = parent;
    if (!plot->axisEnabled(QwtPlot::xBottom) && plot->axisEnabled(QwtPlot::xTop)) {
        m_xAxis = QwtPlot::xTop;
    }

    m_yAxis = QwtPlot::yLeft;
    if (!plot->axisEnabled(QwtPlot::yLeft) && plot->axisEnabled(QwtPlot::yRight)) {
        m_yAxis = QwtPlot::yRight;
    }
}

DAAbstractRegionSelectEditor::~DAAbstractRegionSelectEditor()
{
}

DAAbstractRegionSelectEditor::SelectionMode DAAbstractRegionSelectEditor::getSelectionMode() const
{
    return m_selectionMode;
}

void DAAbstractRegionSelectEditor::setSelectionMode(const SelectionMode& selectionMode)
{
    m_selectionMode = selectionMode;
}
///
/// \brief 判断点是否在区域里
/// \param p
/// \return
///
bool DAAbstractRegionSelectEditor::isContains(const QPointF& p) const
{
    return getSelectRegion().contains(p);
}

///
/// \brief 获取绑定的x轴
/// \return
///
int DAAbstractRegionSelectEditor::getXAxis() const
{
    return m_xAxis;
}

///
/// \brief 获取绑定的y轴
/// \return
///
int DAAbstractRegionSelectEditor::getYAxis() const
{
    return m_yAxis;
}
///
/// \brief 设置关联的坐标轴
/// \note 默认是xbottom，yLeft
/// \param xAxis
/// \param yAxis
///
void DAAbstractRegionSelectEditor::setAxis(int xAxis, int yAxis)
{
    m_xAxis = xAxis;
    m_yAxis = yAxis;
}

///
/// \brief 屏幕坐标转换为数据坐标
/// \param pos
/// \return
///
QPointF DAAbstractRegionSelectEditor::invTransform(const QPointF& pos) const
{
    QwtScaleMap xMap = plot()->canvasMap(getXAxis());
    QwtScaleMap yMap = plot()->canvasMap(getYAxis());

    return QPointF(xMap.invTransform(pos.x()), yMap.invTransform(pos.y()));
}
///
/// \brief 数据坐标转换为屏幕坐标
/// \param pos
/// \return
///
QPointF DAAbstractRegionSelectEditor::transform(const QPointF& pos) const
{
    QwtScaleMap xMap = plot()->canvasMap(getXAxis());
    QwtScaleMap yMap = plot()->canvasMap(getYAxis());
    return QwtScaleMap::transform(xMap, yMap, pos);
}

///
/// \brief 把当前区域转换为其它轴系
/// \param axisX
/// \param axisY
/// \return
///
QPainterPath DAAbstractRegionSelectEditor::transformToOtherAxis(int axisX, int axisY) const
{
    QPainterPath shape = getSelectRegion();
    QwtScaleMap xMap   = plot()->canvasMap(axisX);
    QwtScaleMap yMap   = plot()->canvasMap(axisY);

    const int eleCount = shape.elementCount();
    for (int i = 0; i < eleCount; ++i) {
        const QPainterPath::Element& el = shape.elementAt(i);
        QPointF tmp                     = transform(QPointF(el.x, el.y));
        tmp                             = QwtScaleMap::invTransform(xMap, yMap, tmp);
        shape.setElementPositionAt(i, tmp.x(), tmp.y());
    }
    return shape;
}
}  // End Of Namespace DA
