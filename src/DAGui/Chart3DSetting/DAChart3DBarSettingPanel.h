#ifndef DACHART3DBARSETTINGPANEL_H
#define DACHART3DBARSETTINGPANEL_H
#include "DAGuiAPI.h"
#include "DAChart3DItemSettingPanel.h"

namespace DA
{
// 3D 柱状图属性设置面板
class DAGUI_API DAChart3DBarSettingPanel : public DAChart3DItemSettingPanel
{
    Q_OBJECT
public:
    enum PropertyID {
        PID_Title = 1,
        PID_BarStyle,
        PID_BarWidth,
        PID_BarDepth,
        PID_Baseline,
        PID_ColormapPreset,
        PID_MeshColor,
        PID_MeshLineWidth
    };

    explicit DAChart3DBarSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DBarSettingPanel() override;

    void updateUI(Qwt3DPlotItem* item) override;

private:
    void buildPropertyPanel() override;

private Q_SLOTS:
    void onBarPropertyValueChanged(int propertyId);
};
}  // namespace DA
#endif  // DACHART3DBARSETTINGPANEL_H
