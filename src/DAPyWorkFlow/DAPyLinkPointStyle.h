#ifndef DAPYLINKPOINTSTYLE_H
#define DAPYLINKPOINTSTYLE_H
#include "DAPyWorkFlowAPI.h"
#include <QColor>
#include "DAEnumStringUtils.hpp"
namespace DA
{

// =================================================================================
//                      DAPyLinkPointStyle — 连接点样式配置
// =================================================================================
/**
 * @brief 连接点样式配置结构体
 *
 * 用于自定义端口（连接点）的视觉样式。
 * 当 fillColor 或 borderColor 为无效颜色（default constructed QColor）时，
 * 将使用默认值：输入端口为白色(Qt::white)，输出端口为深灰色(Qt::darkGray)。
 *
 * 使用示例：
 * @code
 * DAPyLinkPointStyle style;
 * style.shape = PortShape::Circle;
 * style.fillColor = QColor(Qt::red);
 * style.borderWidth = 2.0;
 * @endcode
 *
 * @see DAPyNodeStyleDefine.h 中的 PortShape 枚举
 */
class DAPYWORKFLOW_API DAPyLinkPointStyle
{
public:
    /**
     * @brief 端口形状枚举
     */
    enum PortShape
    {
        Rect    = 0,  ///< 矩形端口
        Circle  = 1,  ///< 圆形端口
        Diamond = 2   ///< 菱形端口
    };

public:
    // 默认构造函数，初始化为 Rect 形状、边框宽 1.0
    DAPyLinkPointStyle() : shape(PortShape::Rect), borderWidth(1.0)
    {
    }

    // 公共字段
    PortShape shape;     ///< 端口形状
    QColor fillColor;    ///< 填充颜色（无效时使用默认值：输入=白色，输出=深灰色）
    QColor borderColor;  ///< 边框颜色（无效时使用默认值：黑色）
    qreal borderWidth;   ///< 边框宽度（默认 1.0）

    // 判断填充颜色是否有效
    bool isFillColorValid() const
    {
        return fillColor.isValid();
    }

    // 判断边框颜色是否有效
    bool isBorderColorValid() const
    {
        return borderColor.isValid();
    }
};

DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyLinkPointStyle::PortShape)
}

#endif  // DAPYLINKPOINTSTYLE_H
