#include "DAAbstractRegionSelectEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include "qwt_scale_map.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot
 */
DAAbstractRegionSelectEditor::DAAbstractRegionSelectEditor(QwtPlot* parent)
    : DAAbstractChartEditor(parent)
    , mSelectionMode(AdditionalSelection)  // SingleSelection
    , mXAxis(QwtPlot::xBottom)
    , mYAxis(QwtPlot::yLeft)
{
    mXAxis = parent->visibleXAxisId();
    mYAxis = parent->visibleYAxisId();
}

/**
 * @brief 析构函数
 */
DAAbstractRegionSelectEditor::~DAAbstractRegionSelectEditor()
{
}

/**
 * @brief 获取选择模式
 * @return 当前选择模式
 */
DAAbstractRegionSelectEditor::SelectionMode DAAbstractRegionSelectEditor::getSelectionMode() const
{
    return mSelectionMode;
}

/**
 * @brief 设置选择模式
 * @param selectionMode 选择模式
 */
void DAAbstractRegionSelectEditor::setSelectionMode(const SelectionMode& selectionMode)
{
    mSelectionMode = selectionMode;
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
    return mXAxis;
}

///
/// \brief 获取绑定的y轴
/// \return
///
int DAAbstractRegionSelectEditor::getYAxis() const
{
    return mYAxis;
}
///
/// \brief 设置关联的坐标轴
/// \note 默认是xbottom，yLeft
/// \param xAxis
/// \param yAxis
///
void DAAbstractRegionSelectEditor::setAxis(int xAxis, int yAxis)
{
    mXAxis = xAxis;
    mYAxis = yAxis;
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
