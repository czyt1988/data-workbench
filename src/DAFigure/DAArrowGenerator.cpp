#include "DAArrowGenerator.h"
#include <QTransform>
#include <cmath>

namespace DA
{

/**
 * \if ENGLISH
 * @brief Default constructor
 * @details Creates an arrow generator with default parameters: NoEnd at start, SimpleEnd at end, size 10.0, length 50.0, angle 0.0, line width 1.0
 * \endif
 *
 * \if CHINESE
 * @brief 默认构造函数
 * @details 创建一个带有默认参数的箭头生成器：开始端无端点，结束端简单V形，大小10.0，长度50.0，角度0.0，线宽1.0
 * \endif
 */
DAArrowGenerator::DAArrowGenerator()
    : m_startEndType(NoEnd)
    , m_endEndType(SimpleEnd)
    , m_arrowSize(10.0)
    , m_arrowLength(50.0)
    , m_arrowAngle(0.0)
    , m_arrowLineWidth(1.0)
    , m_originPosition(OriginAtEnd)
{
}

/**
 * \if ENGLISH
 * @brief Constructor with end types
 * @param startEnd Start end type
 * @param endEnd End end type
 * @param size Arrow size
 * @param length Arrow length
 * @param angle Arrow angle in degrees
 * @param lineWidth Arrow line width
 * @details Creates an arrow generator with the specified end types and parameters
 * \endif
 *
 * \if CHINESE
 * @brief 带端点类型的构造函数
 * @param startEnd 开始端类型
 * @param endEnd 结束端类型
 * @param size 箭头大小
 * @param length 箭头长度
 * @param angle 箭头角度（度）
 * @param lineWidth 箭头线宽
 * @details 使用指定的端点类型和参数创建箭头生成器
 * \endif
 */
DAArrowGenerator::DAArrowGenerator(ArrowEndType startEnd, ArrowEndType endEnd, qreal size, qreal length, qreal angle, qreal lineWidth)
    : m_startEndType(startEnd)
    , m_endEndType(endEnd)
    , m_arrowSize(size)
    , m_arrowLength(length)
    , m_arrowAngle(angle)
    , m_arrowLineWidth(lineWidth)
    , m_originPosition(OriginAtEnd)
{
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
DAArrowGenerator::~DAArrowGenerator()
{
}


/**
 * \if ENGLISH
 * @brief Set start end type
 * @param type Start end type
 * @details Sets the type of the start end
 * \endif
 *
 * \if CHINESE
 * @brief 设置开始端类型
 * @param type 开始端类型
 * @details 设置开始端的类型
 * \endif
 */
void DAArrowGenerator::setStartEndType(ArrowEndType type)
{
    m_startEndType = type;
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
DAArrowGenerator::ArrowEndType DAArrowGenerator::getStartEndType() const
{
    return m_startEndType;
}

/**
 * \if ENGLISH
 * @brief Set end end type
 * @param type End end type
 * @details Sets the type of the end end
 * \endif
 *
 * \if CHINESE
 * @brief 设置结束端类型
 * @param type 结束端类型
 * @details 设置结束端的类型
 * \endif
 */
void DAArrowGenerator::setEndEndType(ArrowEndType type)
{
    m_endEndType = type;
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
DAArrowGenerator::ArrowEndType DAArrowGenerator::getEndEndType() const
{
    return m_endEndType;
}

/**
 * \if ENGLISH
 * @brief Set arrow size
 * @param size Arrow size
 * @details Sets the size of the arrow
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头大小
 * @param size 箭头大小
 * @details 设置箭头的大小
 * \endif
 */
void DAArrowGenerator::setArrowSize(qreal size)
{
    m_arrowSize = size;
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
qreal DAArrowGenerator::getArrowSize() const
{
    return m_arrowSize;
}

/**
 * \if ENGLISH
 * @brief Set arrow length
 * @param length Arrow length
 * @details Sets the length of the arrow line
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头长度
 * @param length 箭头长度
 * @details 设置箭头线条的长度
 * \endif
 */
void DAArrowGenerator::setArrowLength(qreal length)
{
    m_arrowLength = length;
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
qreal DAArrowGenerator::getArrowLength() const
{
    return m_arrowLength;
}

/**
 * \if ENGLISH
 * @brief Set arrow angle
 * @param angle Arrow angle in degrees
 * @details Sets the angle of the arrow (0 degrees points to the right)
 * \endif
 *
 * \if CHINESE
 * @brief 设置箭头角度
 * @param angle 箭头角度（度）
 * @details 设置箭头的角度（0度指向右）
 * \endif
 */
void DAArrowGenerator::setArrowAngle(qreal angle)
{
    m_arrowAngle = angle;
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
qreal DAArrowGenerator::getArrowAngle() const
{
    return m_arrowAngle;
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
void DAArrowGenerator::setArrowLineWidth(qreal width)
{
    m_arrowLineWidth = width;
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
qreal DAArrowGenerator::getArrowLineWidth() const
{
    return m_arrowLineWidth;
}

/**
 * \if ENGLISH
 * @brief Set origin position
 * @param pos Origin position
 * @details Sets the position of the origin (start or end of the arrow)
 * \endif
 *
 * \if CHINESE
 * @brief 设置原点位置
 * @param pos 原点位置
 * @details 设置原点的位置（箭头的开始端或结束端）
 * \endif
 */
void DAArrowGenerator::setOriginPosition(OriginPosition pos)
{
    m_originPosition = pos;
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
DAArrowGenerator::OriginPosition DAArrowGenerator::getOriginPosition() const
{
    return m_originPosition;
}

/**
 * \if ENGLISH
 * @brief Generate arrow path
 * @return Generated arrow path
 * @details Generates and returns the arrow path based on the current parameters
 * \endif
 *
 * \if CHINESE
 * @brief 生成箭头路径
 * @return 生成的箭头路径
 * @details 根据当前参数生成并返回箭头路径
 * \endif
 */
QPainterPath DAArrowGenerator::generatePath() const
{
    return createArrowPath(m_arrowSize, m_startEndType, m_endEndType, m_arrowLength, m_arrowAngle, m_originPosition);
}

/**
 * \if ENGLISH
 * @brief Static method to generate arrow path with full parameters
 * @param arrowSize Arrow size
 * @param startEnd Start end type
 * @param endEnd End end type
 * @param length Arrow length
 * @param angle Arrow angle in degrees
 * @param originPos Origin position
 * @return Generated arrow path
 * @details Generates and returns an arrow path with the specified parameters
 * \endif
 *
 * \if CHINESE
 * @brief 静态方法生成箭头路径（完整参数）
 * @param arrowSize 箭头大小
 * @param startEnd 开始端类型
 * @param endEnd 结束端类型
 * @param length 箭头长度
 * @param angle 箭头角度（度）
 * @param originPos 原点位置
 * @return 生成的箭头路径
 * @details 使用指定的参数生成并返回箭头路径
 * \endif
 */
QPainterPath DAArrowGenerator::createArrowPath(
    qreal arrowSize, ArrowEndType startEnd, ArrowEndType endEnd, qreal length, qreal angle, OriginPosition originPos
)
{
    QPainterPath path;

    qreal startX, endX;

    if (originPos == OriginAtEnd) {
        startX = -length;
        endX   = 0;
    } else {
        startX = 0;
        endX   = length;
    }

    path.moveTo(startX, 0);
    path.lineTo(endX, 0);

    if (startEnd != NoEnd) {
        QPainterPath startEndPath = createEndPath(startEnd, arrowSize, true);
        path.addPath(startEndPath.translated(startX, 0));
    }

    if (endEnd != NoEnd) {
        QPainterPath endEndPath = createEndPath(endEnd, arrowSize, false);
        path.addPath(endEndPath.translated(endX, 0));
    }

    if (!qFuzzyIsNull(angle)) {
        QTransform transform;
        transform.rotate(angle);
        path = transform.map(path);
    }

    return path;
}

/**
 * \if ENGLISH
 * @brief Create end path for arrow
 * @param endType End type
 * @param size End size
 * @param isStart Whether this is the start end
 * @return Generated end path
 * @details Creates and returns an end path for the arrow
 * \endif
 *
 * \if CHINESE
 * @brief 创建箭头端点路径
 * @param endType 端点类型
 * @param size 端点大小
 * @param isStart 是否为开始端
 * @return 生成的端点路径
 * @details 创建并返回箭头的端点路径
 * \endif
 */
QPainterPath DAArrowGenerator::createEndPath(ArrowEndType endType, qreal size, bool isStart)
{
    QPainterPath path;

    qreal direction = isStart ? 1.0 : -1.0;

    switch (endType) {
    case SimpleEnd: {
        path.moveTo(0, 0);
        path.lineTo(direction * size, size * 0.5);
        path.moveTo(0, 0);
        path.lineTo(direction * size, -size * 0.5);
        break;
    }
    case FilledEnd: {
        path.moveTo(0, 0);
        path.lineTo(direction * size, size * 0.5);
        path.lineTo(direction * size * 0.7, 0);
        path.lineTo(direction * size, -size * 0.5);
        path.closeSubpath();
        break;
    }
    case CircleEnd: {
        path.addEllipse(QPointF(direction * size * 0.5, 0), size * 0.5, size * 0.5);
        break;
    }
    case DiamondEnd: {
        path.moveTo(0, 0);
        path.lineTo(direction * size * 0.5, size * 0.5);
        path.lineTo(direction * size, 0);
        path.lineTo(direction * size * 0.5, -size * 0.5);
        path.closeSubpath();
        break;
    }
    case SquareEnd: {
        QRectF rect(direction * size * 0.5 - size * 0.5, -size * 0.5, size, size);
        path.addRect(rect);
        break;
    }
    case NoEnd:
    default:
        break;
    }

    return path;
}

}  // End Of Namespace DA