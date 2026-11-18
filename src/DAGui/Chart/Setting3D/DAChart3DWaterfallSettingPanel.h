#ifndef DACHART3DWATERFALLSETTINGPANEL_H
#define DACHART3DWATERFALLSETTINGPANEL_H
#include "DAGuiAPI.h"
#include "Chart/Setting3D/DAChart3DItemSettingPanel.h"

namespace DA
{
/**
 * @brief 三维瀑布图元设置面板
 *
 * 只放图元自身的样式（几何样式 / 着色方式 / 单色 / 不透明度 / 深度方向 / 描边），
 * 色带与 dB 色域归插件「信号显示 → 瀑布图设置」丝带面板（单一入口，不在这里重复）。
 * 图级外观（标题/背景/投影/光照/主题/坐标轴/图例）在 3D 图表区面板，也不在这里重复。
 */
class DAGUI_API DAChart3DWaterfallSettingPanel : public DAChart3DItemSettingPanel
{
    Q_OBJECT
public:
    enum PropertyID {
        PID_Title = 1,
        PID_Visible,
        PID_ZValue,
        PID_PlotStyle,
        PID_ColorMode,
        PID_SolidColor,
        PID_Opacity,
        PID_DepthOrder,
        PID_OutlineColor,
        PID_OutlineWidth,
        PID_Hint
    };

    explicit DAChart3DWaterfallSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DWaterfallSettingPanel() override;

    void updateUI(Qwt3DPlotItem* item) override;

private:
    void buildPropertyPanel() override;

private Q_SLOTS:
    void onWaterfallPropertyValueChanged(int propertyId);
};
}  // namespace DA
#endif  // DACHART3DWATERFALLSETTINGPANEL_H