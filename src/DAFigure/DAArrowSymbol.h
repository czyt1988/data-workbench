#ifndef DAARROWSYMBOL_H
#define DAARROWSYMBOL_H
#include "DAFigureAPI.h"
#include <qwt_symbol.h>
namespace DA
{
/**
 * @brief 箭头符号
 * 
 * 用于在图表上绘制箭头符号，支持自定义箭头大小、颜色和方向
 */
class DAFIGURE_API DAArrowSymbol : public QwtSymbol
{
public:
    /**
     * @brief 箭头样式
     */
    enum ArrowStyle
    {
        SimpleArrow,    ///< 简单箭头
        FilledArrow,    ///< 填充箭头
        DoubleArrow     ///< 双箭头
    };
    
public:
    DAArrowSymbol();
    DAArrowSymbol(const QColor& color, qreal arrowSize = 10.0, ArrowStyle style = SimpleArrow);
    virtual ~DAArrowSymbol();
    
    // 设置箭头大小
    void setArrowSize(qreal size);
    qreal getArrowSize() const;
    
    // 设置箭头样式
    void setArrowStyle(ArrowStyle style);
    ArrowStyle getArrowStyle() const;
    
    // 设置箭头颜色
    void setArrowColor(const QColor& color);
    QColor getArrowColor() const;
    
    // 设置箭头方向（角度，以度为单位，0度表示向右）
    void setArrowAngle(qreal angle);
    qreal getArrowAngle() const;
    
    // 设置箭头线宽
    void setArrowLineWidth(qreal width);
    qreal getArrowLineWidth() const;
    
    // 创建箭头路径
    static QPainterPath createArrowPath(qreal arrowSize, ArrowStyle style = SimpleArrow, qreal angle = 0.0);
    
private:
    void updatePath();
    
private:
    qreal m_arrowSize;
    ArrowStyle m_arrowStyle;
    QColor m_arrowColor;
    qreal m_arrowAngle;
    qreal m_arrowLineWidth;
};
}  // End Of Namespace DA
#endif  // DAARROWSYMBOL_H