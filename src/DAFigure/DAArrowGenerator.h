#ifndef DAARROWGENERATOR_H
#define DAARROWGENERATOR_H
#include "DAFigureAPI.h"
#include <QPainterPath>
#include <QColor>

namespace DA
{
/**
 * \if ENGLISH
 * @brief Arrow generation class
 * \endif
 *
 * \if CHINESE
 * @brief 箭头生成类
 * \endif
 */
class DAFIGURE_API DAArrowGenerator
{
public:
    /**
     * \if ENGLISH
     * @brief Arrow end type enumeration
     * \endif
     *
     * \if CHINESE
     * @brief 箭头端点类型枚举
     * \endif
     */
    enum ArrowEndType
    {
        NoEnd,       ///< 无端点
        SimpleEnd,   ///< 简单V形端点
        FilledEnd,   ///< 填充三角形端点
        CircleEnd,   ///< 圆形端点
        DiamondEnd,  ///< 菱形端点
        SquareEnd    ///< 方形端点
    };

    /**
     * \if ENGLISH
     * @brief Arrow origin position enumeration
     * \endif
     *
     * \if CHINESE
     * @brief 箭头原点位置枚举
     * \endif
     */
    enum OriginPosition
    {
        OriginAtStart,  ///< 原点在开始端
        OriginAtEnd     ///< 原点在结束端
    };

public:
    // Default constructor
    DAArrowGenerator();

    // Constructor with end types
    DAArrowGenerator(
        ArrowEndType startEnd, ArrowEndType endEnd, qreal size = 10.0, qreal length = 50.0, qreal angle = 0.0, qreal lineWidth = 1.0
    );

    // Destructor
    virtual ~DAArrowGenerator();

    // Set start end type
    void setStartEndType(ArrowEndType type);

    // Get start end type
    ArrowEndType getStartEndType() const;

    // Set end end type
    void setEndEndType(ArrowEndType type);

    // Get end end type
    ArrowEndType getEndEndType() const;

    // Set arrow size
    void setArrowSize(qreal size);

    // Get arrow size
    qreal getArrowSize() const;

    // Set arrow length
    void setArrowLength(qreal length);

    // Get arrow length
    qreal getArrowLength() const;

    // Set arrow angle
    void setArrowAngle(qreal angle);

    // Get arrow angle
    qreal getArrowAngle() const;

    // Set arrow line width
    void setArrowLineWidth(qreal width);

    // Get arrow line width
    qreal getArrowLineWidth() const;

    // Set origin position
    void setOriginPosition(OriginPosition pos);

    // Get origin position
    OriginPosition getOriginPosition() const;

    // Generate arrow path
    QPainterPath generatePath() const;

    // Static method to generate arrow path with full parameters
    static QPainterPath createArrowPath(
        qreal arrowSize, ArrowEndType startEnd, ArrowEndType endEnd, qreal length, qreal angle = 0.0, OriginPosition originPos = OriginAtEnd
    );

private:
    // Create end path for arrow
    static QPainterPath createEndPath(ArrowEndType endType, qreal size, bool isStart);

private:
    ArrowEndType m_startEndType;      ///< Start end type
    ArrowEndType m_endEndType;        ///< End end type
    qreal m_arrowSize;                ///< Arrow size
    qreal m_arrowLength;              ///< Arrow length
    qreal m_arrowAngle;               ///< Arrow angle in degrees
    qreal m_arrowLineWidth;           ///< Arrow line width
    OriginPosition m_originPosition;  ///< Origin position
};

}  // End Of Namespace DA

#endif  // DAARROWGENERATOR_H