#include "DAColormapMatch.h"
#include <memory>
#include <QVector>
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"

namespace DA
{
namespace
{
// 比较色表时使用的颜色数：32 色足以区分 22 个预设，同时避免逐像素比较的开销
constexpr int cColorTableSize = 32;
}  // namespace

/**
 * @brief 从色图反查预设名
 *
 * 空指针直接返回空串；QwtLinearColorMap 的 colorTable 由色标插值生成，
 * 与预设按同一算法生成，因此逐项比较即可判定「是否就是该预设」。
 * @param map 待匹配色图
 * @return 预设名；不匹配返回空字符串
 */
QString daMatchColormapPreset(const QwtColorMap* map)
{
    if (nullptr == map) {
        return QString();
    }
    const QVector< QRgb > targetTable = map->colorTable(cColorTableSize);
    if (targetTable.isEmpty()) {
        return QString();
    }
    const QStringList presets = QwtColorMapPreset::availablePresets();
    for (const QString& preset : presets) {
        const std::unique_ptr< QwtLinearColorMap > candidate = QwtColorMapPreset::create(preset);
        if (!candidate) {
            continue;
        }
        if (candidate->colorTable(cColorTableSize) == targetTable) {
            return preset;
        }
    }
    return QString();
}
}  // namespace DA