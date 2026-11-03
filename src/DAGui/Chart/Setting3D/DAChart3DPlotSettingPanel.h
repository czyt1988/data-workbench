#ifndef DACHART3DPLOTSETTINGPANEL_H
#define DACHART3DPLOTSETTINGPANEL_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>

namespace DA
{
class DAChart3DWidget;
class DAPropertyPanelContainerWidget;

// 3D 图表级属性设置面板
class DAGUI_API DAChart3DPlotSettingPanel : public QWidget
{
    Q_OBJECT
public:
    enum PropertyID {
        PID_TitleText = 1,
        PID_TitleFont,
        PID_TitleColor,
        PID_BackgroundColor,
        PID_Projection,
        PID_AspectRatio,
        PID_EnableLighting,
        PID_LightingPreset,
        PID_Shininess,
        PID_SpecularIntensity,
        PID_ThemePreset,
        PID_ResetView
    };

    explicit DAChart3DPlotSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DPlotSettingPanel() override;

    DAPropertyPanelContainerWidget* propertyPanel() const;
    void setTarget(DAChart3DWidget* chart);
    DAChart3DWidget* target() const;
    void updateUI();
    void replot();

Q_SIGNALS:
    void propertyValueChanged(int propertyId);

protected Q_SLOTS:
    void buildPropertyPanel();
    void onPanelPropertyValueChanged(int propertyId);
    void onPropertyValueChanged(int propertyId);

private:
    DAPropertyPanelContainerWidget* mPanel;
    QPointer< DAChart3DWidget > mChart3D;
};
}  // namespace DA
#endif  // DACHART3DPLOTSETTINGPANEL_H
