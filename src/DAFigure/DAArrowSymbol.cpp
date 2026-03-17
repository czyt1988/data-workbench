#include "DAArrowSymbol.h"
#include "DAArrowGenerator.h"
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <cmath>

namespace DA
{

/**
 * \if ENGLISH
 * @brief Default constructor
 * @details Creates an arrow symbol with default parameters
 * \endif
 *
 * \if CHINESE
 * @brief 默认构造函数
 * @details 创建一个带有默认参数的箭头符号
 * \endif
 */
DAArrowSymbol::DAArrowSymbol()
    : m_generator(DAArrowGenerator::SimpleEnd, DAArrowGenerator::SimpleEnd, 10.0, 50.0), m_arrowColor(Qt::black)
{
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Constructor with parameters
 * @param color Arrow color
 * @param arrowSize Arrow size
 * @param style Arrow style
 * @details Creates an arrow symbol with the specified parameters
 * \endif
 *
 * \if CHINESE
 * @brief 带参数的构造函数
 * @param color 箭头颜色
 * @param arrowSize 箭头大小
 * @param style 箭头样式
 * @details 使用指定的参数创建箭头符号
 * \endif
 */
DAArrowSymbol::DAArrowSymbol(const QColor& color, qreal arrowSize, ArrowEndType startEnd, ArrowEndType endEnd)
    : m_generator(startEnd, endEnd, arrowSize), m_arrowColor(color)
{
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Destructor
 * \endif
 *
 * \if CHINESE
 * @brief 析构函数
 * \endif
 */
DAArrowSymbol::~DAArrowSymbol()
{
}

/**
 * \if ENGLISH
 * @brief Set arrow size
 * @param size Arrow size
 * @details Sets the size of the arrow and updates the path
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头大小
 * @param size 箭头大小
 * @details 设置箭头的大小并更新路径
 * \endif
 */
void DAArrowSymbol::setArrowSize(qreal size)
{
    if (qFuzzyCompare(m_generator.getArrowSize(), size)) {
        return;
    }
    m_generator.setArrowSize(size);
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Get arrow size
 * @return Arrow size
 * @details Returns the current arrow size
 * \endif
 *
 * \if CHINESE
 * @brief 获取箭头大小
 * @return 箭头大小
 * @details 返回当前的箭头大小
 * \endif
 */
qreal DAArrowSymbol::getArrowSize() const
{
    return m_generator.getArrowSize();
}


/**
 * \if ENGLISH
 * @brief Set start end type
 * @param type Start end type
 * @details Sets the type of the start end and updates the path
 * \endif
 *
 * \if CHINESE
 * @brief 设置开始端类型
 * @param type 开始端类型
 * @details 设置开始端的类型并更新路径
 * \endif
 */
void DAArrowSymbol::setStartEndType(ArrowEndType type)
{
    m_generator.setStartEndType(static_cast< DAArrowGenerator::ArrowEndType >(type));
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Get start end type
 * @return Start end type
 * @details Returns the current start end type
 * \endif
 *
 * \if CHINESE
 * @brief 获取开始端类型
 * @return 开始端类型
 * @details 返回当前的开始端类型
 * \endif
 */
DAArrowSymbol::ArrowEndType DAArrowSymbol::getStartEndType() const
{
    return static_cast< ArrowEndType >(m_generator.getStartEndType());
}

/**
 * \if ENGLISH
 * @brief Set end end type
 * @param type End end type
 * @details Sets the type of the end end and updates the path
 * \endif
 *
 * \if CHINESE
 * @brief 设置结束端类型
 * @param type 结束端类型
 * @details 设置结束端的类型并更新路径
 * \endif
 */
void DAArrowSymbol::setEndEndType(ArrowEndType type)
{
    m_generator.setEndEndType(static_cast< DAArrowGenerator::ArrowEndType >(type));
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Get end end type
 * @return End end type
 * @details Returns the current end end type
 * \endif
 *
 * \if CHINESE
 * @brief 获取结束端类型
 * @return 结束端类型
 * @details 返回当前的结束端类型
 * \endif
 */
DAArrowSymbol::ArrowEndType DAArrowSymbol::getEndEndType() const
{
    return static_cast< ArrowEndType >(m_generator.getEndEndType());
}

/**
 * \if ENGLISH
 * @brief Set arrow length
 * @param length Arrow length
 * @details Sets the length of the arrow and updates the path
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头长度
 * @param length 箭头长度
 * @details 设置箭头的长度并更新路径
 * \endif
 */
void DAArrowSymbol::setArrowLength(qreal length)
{
    if (qFuzzyCompare(m_generator.getArrowLength(), length)) {
        return;
    }
    m_generator.setArrowLength(length);
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Get arrow length
 * @return Arrow length
 * @details Returns the current arrow length
 * \endif
 *
 * \if CHINESE
 * @brief 获取箭头长度
 * @return 箭头长度
 * @details 返回当前的箭头长度
 * \endif
 */
qreal DAArrowSymbol::getArrowLength() const
{
    return m_generator.getArrowLength();
}

/**
 * \if ENGLISH
 * @brief Set arrow color
 * @param color Arrow color
 * @details Sets the color of the arrow
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头颜色
 * @param color 箭头颜色
 * @details 设置箭头的颜色
 * \endif
 */
void DAArrowSymbol::setArrowColor(const QColor& color)
{
    if (m_arrowColor == color) {
        return;
    }
    m_arrowColor = color;

    QPen pen = this->pen();
    pen.setColor(color);
    setPen(pen);

    setBrush(color);
}

/**
 * \if ENGLISH
 * @brief Get arrow color
 * @return Arrow color
 * @details Returns the current arrow color
 * \endif
 *
 * \if CHINESE
 * @brief 获取箭头颜色
 * @return 箭头颜色
 * @details 返回当前的箭头颜色
 * \endif
 */
QColor DAArrowSymbol::getArrowColor() const
{
    return m_arrowColor;
}

/**
 * \if ENGLISH
 * @brief Set arrow angle
 * @param angle Arrow angle in degrees
 * @details Sets the angle of the arrow and updates the path
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头角度
 * @param angle 箭头角度（度）
 * @details 设置箭头的角度并更新路径
 * \endif
 */
void DAArrowSymbol::setArrowAngle(qreal angle)
{
    if (qFuzzyCompare(m_generator.getArrowAngle(), angle)) {
        return;
    }
    m_generator.setArrowAngle(angle);
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Get arrow angle
 * @return Arrow angle in degrees
 * @details Returns the current arrow angle
 * \endif
 *
 * \if CHINESE
 * @brief 获取箭头角度
 * @return 箭头角度（度）
 * @details 返回当前的箭头角度
 * \endif
 */
qreal DAArrowSymbol::getArrowAngle() const
{
    return m_generator.getArrowAngle();
}

/**
 * \if ENGLISH
 * @brief Set arrow line width
 * @param width Arrow line width
 * @details Sets the line width of the arrow
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头线宽
 * @param width 箭头线宽
 * @details 设置箭头的线宽
 * \endif
 */
void DAArrowSymbol::setArrowLineWidth(qreal width)
{
    if (qFuzzyCompare(m_generator.getArrowLineWidth(), width)) {
        return;
    }
    m_generator.setArrowLineWidth(width);

    QPen pen = this->pen();
    pen.setWidthF(width);
    setPen(pen);
}

/**
 * \if ENGLISH
 * @brief Get arrow line width
 * @return Arrow line width
 * @details Returns the current arrow line width
 * \endif
 *
 * \if CHINESE
 * @brief 获取箭头线宽
 * @return 箭头线宽
 * @details 返回当前的箭头线宽
 * \endif
 */
qreal DAArrowSymbol::getArrowLineWidth() const
{
    return m_generator.getArrowLineWidth();
}

/**
 * \if ENGLISH
 * @brief Set origin position
 * @param pos Origin position
 * @details Sets the position of the origin and updates the path
 * \endif
 *
 * \if CHINESE
 * @brief 设置原点位置
 * @param pos 原点位置
 * @details 设置原点的位置并更新路径
 * \endif
 */
void DAArrowSymbol::setOriginPosition(OriginPosition pos)
{
    m_generator.setOriginPosition(static_cast< DAArrowGenerator::OriginPosition >(pos));
    updatePath();
}

/**
 * \if ENGLISH
 * @brief Get origin position
 * @return Origin position
 * @details Returns the current origin position
 * \endif
 *
 * \if CHINESE
 * @brief 获取原点位置
 * @return 原点位置
 * @details 返回当前的原点位置
 * \endif
 */
DAArrowSymbol::OriginPosition DAArrowSymbol::getOriginPosition() const
{
    return static_cast< OriginPosition >(m_generator.getOriginPosition());
}

/**
 * \if ENGLISH
 * @brief Create arrow path with full parameters
 * @param arrowSize Arrow size
 * @param startEnd Start end type
 * @param endEnd End end type
 * @param length Arrow length
 * @param angle Arrow angle in degrees
 * @param originPos Origin position
 * @return Generated arrow path
 * @details Creates an arrow path with the specified parameters
 * \endif
 *
 * \if CHINESE
 * @brief 创建箭头路径（完整参数）
 * @param arrowSize 箭头大小
 * @param startEnd 开始端类型
 * @param endEnd 结束端类型
 * @param length 箭头长度
 * @param angle 箭头角度（度）
 * @param originPos 原点位置
 * @return 生成的箭头路径
 * @details 使用指定的参数创建箭头路径
 * \endif
 */
QPainterPath DAArrowSymbol::createArrowPath(
    qreal arrowSize, ArrowEndType startEnd, ArrowEndType endEnd, qreal length, qreal angle, OriginPosition originPos
)
{
    return DAArrowGenerator::createArrowPath(
        arrowSize,
        static_cast< DAArrowGenerator::ArrowEndType >(startEnd),
        static_cast< DAArrowGenerator::ArrowEndType >(endEnd),
        length,
        angle,
        static_cast< DAArrowGenerator::OriginPosition >(originPos)
    );
}

/**
 * \if ENGLISH
 * @brief Update arrow path
 * @details Updates the arrow path based on the current parameters
 * \endif
 *
 * \if CHINESE
 * @brief 更新箭头路径
 * @details 根据当前参数更新箭头路径
 * \endif
 */
void DAArrowSymbol::updatePath()
{
    QPainterPath path = m_generator.generatePath();

    setPath(path);

    QPointF pinPoint(0, 0);
    setPinPoint(pinPoint);

    qreal size   = m_generator.getArrowSize();
    qreal length = m_generator.getArrowLength();
    QSize symbolSize(qMax(size * 2, length), size * 2);
    setSize(symbolSize);

    QPen pen(m_arrowColor, m_generator.getArrowLineWidth());
    pen.setJoinStyle(Qt::MiterJoin);
    setPen(pen);

    DAArrowGenerator::ArrowEndType endType = m_generator.getEndEndType();
    if (endType == DAArrowGenerator::FilledEnd || endType == DAArrowGenerator::CircleEnd
        || endType == DAArrowGenerator::DiamondEnd || endType == DAArrowGenerator::SquareEnd) {
        setBrush(m_arrowColor);
    } else {
        setBrush(Qt::NoBrush);
    }
}

}  // End Of Namespace DA