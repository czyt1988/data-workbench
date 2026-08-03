#ifndef DACHART3DSURFACESETTINGPANEL_H
#define DACHART3DSURFACESETTINGPANEL_H
#include "DAGuiAPI.h"
#include "DAChart3DItemSettingPanel.h"

namespace DA
{
// Surface 曲面属性设置面板
class DAGUI_API DAChart3DSurfaceSettingPanel : public DAChart3DItemSettingPanel
{
    Q_OBJECT
public:
    enum PropertyID {
        PID_Title = 1,
        PID_PlotStyle,
        PID_ColormapPreset,
        PID_MeshColor,
        PID_MeshLineWidth,
        PID_Isolines,
        PID_SmoothMesh,
        PID_FloorStyle,
        PID_ShowNormals,
        PID_NormalLength,
        PID_NormalQuality,
        PID_Shading,
        PID_PolygonOffset,
        PID_Resolution
    };

    explicit DAChart3DSurfaceSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DSurfaceSettingPanel() override;

    void updateUI(Qwt3DPlotItem* item) override;

private:
    void buildPropertyPanel() override;

private Q_SLOTS:
    void onSurfacePropertyValueChanged(int propertyId);
};
}  // namespace DA
#endif  // DACHART3DSURFACESETTINGPANEL_H
