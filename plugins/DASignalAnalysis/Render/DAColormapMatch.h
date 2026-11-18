#ifndef DACOLORMAPMATCH_H
#define DACOLORMAPMATCH_H
#include <QString>

class QwtColorMap;

namespace DA
{
/**
 * @brief 从色图反查预设名（纯函数，便于单测）
 *
 * 逐一把 QwtColorMapPreset::availablePresets() 建成预设色带，与传入色图比较
 * colorTable(32)：命中返回预设名；不匹配或空指针返回空字符串。
 *
 * 用途：「信号显示 → 云图设置」的色带菜单勾选必须回读图元当前 colorMap（单一真相），
 * 宿主热图面板改过起止色时不再是任何预设，菜单不勾选任何项（界面层显示为 custom）。
 * @param map 待匹配色图，可为 nullptr
 * @return 预设名；不匹配返回空字符串
 */
QString daMatchColormapPreset(const QwtColorMap* map);
}  // namespace DA

#endif  // DACOLORMAPMATCH_H