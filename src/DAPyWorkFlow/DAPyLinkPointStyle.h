#ifndef DAPYLINKPOINTSTYLE_H
#define DAPYLINKPOINTSTYLE_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyNodeStyleDefine.h"
#include <QColor>
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
    // 构造函数
    /**
     * @brief 默认构造函数
     *
     * 初始化为默认值：
     * - shape: PortShape::Rect
     * - fillColor: 无效颜色（使用默认值）
     * - borderColor: 无效颜色（使用默认值）
     * - borderWidth: 1.0
     */
    DAPyLinkPointStyle() : shape(PortShape::Rect), borderWidth(1.0)
    {
    }

    // 公共字段
    PortShape shape;     ///< 端口形状
    QColor fillColor;    ///< 填充颜色（无效时使用默认值：输入=白色，输出=深灰色）
    QColor borderColor;  ///< 边框颜色（无效时使用默认值：黑色）
    qreal borderWidth;   ///< 边框宽度（默认 1.0）

    // 辅助方法
    /**
     * @brief 判断填充颜色是否有效
     * @return true 表示 fillColor 有效，false 表示应使用默认值
     */
    bool isFillColorValid() const
    {
        return fillColor.isValid();
    }

    /**
     * @brief 判断边框颜色是否有效
     * @return true 表示 borderColor 有效，false 表示应使用默认值
     */
    bool isBorderColorValid() const
    {
        return borderColor.isValid();
    }
};

DA_ENUM_STRING_DECLARE_EXPORT(DAPYWORKFLOW_API, DAPyLinkPointStyle::PortShape)
}

#endif  // DAPYLINKPOINTSTYLE_H
