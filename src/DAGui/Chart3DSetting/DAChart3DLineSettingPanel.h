#ifndef DACHART3DLINESETTINGPANEL_H
#define DACHART3DLINESETTINGPANEL_H
#include "DAGuiAPI.h"
#include "DAChart3DItemSettingPanel.h"

namespace DA
{
// 3D 线图属性设置面板
class DAGUI_API DAChart3DLineSettingPanel : public DAChart3DItemSettingPanel
{
    Q_OBJECT
public:
    enum PropertyID {
        PID_Title = 1,
        PID_LineStyle,
        PID_LineWidth,
        PID_TubeRadius,
        PID_TubeSegments,
        PID_PointSize,
        PID_PointShape,
        PID_PointVisible,
        PID_UseDataColor,
        PID_SolidColor,
        PID_ColormapPreset
    };

    explicit DAChart3DLineSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DLineSettingPanel() override;

    void updateUI(Qwt3DPlotItem* item) override;

private:
    void buildPropertyPanel() override;

private Q_SLOTS:
    void onLinePropertyValueChanged(int propertyId);
};
}  // namespace DA
#endif  // DACHART3DLINESETTINGPANEL_H
