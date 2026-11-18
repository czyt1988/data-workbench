#ifndef DAWATERFALLCOLOR_H
#define DAWATERFALLCOLOR_H
#include <QColor>
#include <QString>
#include "qwt3d_colormap_color.h"

namespace DA
{
/**
 * @brief 三维瀑布数据色带：在 Qwt3DColorMapColor 基础上高亮「当前迹线」
 *
 * 瀑布的 y 轴是迹线槽位（depth），这里把当前帧对应槽位的整条迹线画成高亮色
 * （橙红 #CE6043，与云图游标、波形播放头同一语义色），其余点仍走基类的色带映射。
 *
 * 为什么要子类：Qwt3DWaterfall::setColorMode() 用 dynamic_cast<Qwt3DColorMapColor*>
 * 重新指向着色坐标轴，子类仍然命中，因此零改动 qwt 就能做帧高亮（D11）。
 * 高亮只影响绘制：图例取色仍走基类（色带中不出现高亮色）。
 */
class DAWaterfallColor : public Qwt3DColorMapColor
{
public:
    explicit DAWaterfallColor(const QString& presetName = QStringLiteral("viridis"));
    ~DAWaterfallColor() override;

    // 设置高亮槽位与颜色；slot < 0 表示关闭高亮
    void setHighlightSlot(int slot, const QColor& color);
    void clearHighlightSlot();
    int highlightSlot() const;

    // 当前迹线高亮（其余坐标交给基类色带）
    RGBA operator()(double x, double y, double z) const override;
    // 图例取色向量：不加高亮（高亮不是色带的一部分）
    ColorVector& createVector(ColorVector& vec) override;

private:
    int mHighlightSlot { -1 };
    QColor mHighlightColor { 206, 96, 67 };
};
}  // namespace DA

#endif  // DAWATERFALLCOLOR_H