#include "DAWaterfallColor.h"
#include <cmath>
#include "qwt3d_types.h"

namespace DA
{
DAWaterfallColor::DAWaterfallColor(const QString& presetName) : Qwt3DColorMapColor(presetName)
{
}

DAWaterfallColor::~DAWaterfallColor()
{
}

/**
 * @brief 设置高亮槽位（瀑布 y 轴 = 迹线槽位）
 * @param slot 槽位下标，<0 表示关闭
 * @param color 高亮颜色（通常 #CE6043）
 */
void DAWaterfallColor::setHighlightSlot(int slot, const QColor& color)
{
    mHighlightSlot  = slot;
    mHighlightColor = color;
}

/**
 * @brief 关闭高亮
 */
void DAWaterfallColor::clearHighlightSlot()
{
    mHighlightSlot = -1;
}

/**
 * @brief 当前高亮槽位（<0 表示关闭）
 */
int DAWaterfallColor::highlightSlot() const
{
    return mHighlightSlot;
}

/**
 * @brief 逐顶点取色：命中当前槽位的迹线整条用高亮色，其余交给基类色带
 *
 * 槽位比较留 0.5 容差：瀑布的 y 坐标就是槽位序号（整数），迹线顶点落在 ±0.5 内。
 * @param x 频点
 * @param y 迹线槽位
 * @param z 幅值（dB）
 * @return 顶点颜色
 */
RGBA DAWaterfallColor::operator()(double x, double y, double z) const
{
    if (mHighlightSlot >= 0 && std::fabs(y - double(mHighlightSlot)) < 0.5) {
        return Qt2GL(mHighlightColor);
    }
    return Qwt3DColorMapColor::operator()(x, y, z);
}

/**
 * @brief 图例取色向量：直接复用基类色带，高亮色不进入色带
 */
ColorVector& DAWaterfallColor::createVector(ColorVector& vec)
{
    return Qwt3DColorMapColor::createVector(vec);
}
}  // namespace DA