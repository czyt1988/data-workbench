#ifndef DAARROWSYMBOL_H
#define DAARROWSYMBOL_H
#include "DAFigureAPI.h"
#include "DAArrowGenerator.h"
#include <qwt_symbol.h>
namespace DA
{
/**
 * \if ENGLISH
 * @brief Arrow symbol
 * \endif
 *
 * \if CHINESE
 * @brief 箭头符号
 * \endif
 */
class DAFIGURE_API DAArrowSymbol : public QwtSymbol
{
public:
    /**
     * \if ENGLISH
     * @brief Arrow end type
     * \endif
     *
     * \if CHINESE
     * @brief 箭头端点类型
     * \endif
     */
    enum ArrowEndType
    {
        NoEnd      = DAArrowGenerator::NoEnd,       ///< 无端点
        SimpleEnd  = DAArrowGenerator::SimpleEnd,   ///< 简单V形端点
        FilledEnd  = DAArrowGenerator::FilledEnd,   ///< 填充三角形端点
        CircleEnd  = DAArrowGenerator::CircleEnd,   ///< 圆形端点
        DiamondEnd = DAArrowGenerator::DiamondEnd,  ///< 菱形端点
        SquareEnd  = DAArrowGenerator::SquareEnd    ///< 方形端点
    };

    /**
     * \if ENGLISH
     * @brief Arrow origin position
     * \endif
     *
     * \if CHINESE
     * @brief 箭头原点位置
     * \endif
     */
    enum OriginPosition
    {
        OriginAtStart = DAArrowGenerator::OriginAtStart,  ///< 原点在开始端
        OriginAtEnd   = DAArrowGenerator::OriginAtEnd     ///< 原点在结束端
    };

public:
    DAArrowSymbol();
    DAArrowSymbol(const QColor& color, qreal arrowSize = 10.0, ArrowEndType startEnd = NoEnd, ArrowEndType endEnd = SimpleEnd);
    virtual ~DAArrowSymbol();

    // Set arrow size
    void setArrowSize(qreal size);
    qreal getArrowSize() const;

    // Set start end type
    void setStartEndType(ArrowEndType type);
    ArrowEndType getStartEndType() const;

    // Set end end type
    void setEndEndType(ArrowEndType type);
    ArrowEndType getEndEndType() const;

    // Set arrow length
    void setArrowLength(qreal length);
    qreal getArrowLength() const;

    // Set arrow color
    void setArrowColor(const QColor& color);
    QColor getArrowColor() const;

    // Set arrow angle
    void setArrowAngle(qreal angle);
    qreal getArrowAngle() const;

    // Set arrow line width
    void setArrowLineWidth(qreal width);
    qreal getArrowLineWidth() const;

    // Set origin position
    void setOriginPosition(OriginPosition pos);
    OriginPosition getOriginPosition() const;

    // Create arrow path with full parameters
    static QPainterPath createArrowPath(
        qreal arrowSize, ArrowEndType startEnd, ArrowEndType endEnd, qreal length, qreal angle = 0.0, OriginPosition originPos = OriginAtEnd
    );

private:
    void updatePath();

private:
    DAArrowGenerator m_generator;
    QColor m_arrowColor;
};
}  // End Of Namespace DA
#endif  // DAARROWSYMBOL_H