#ifndef DACHART3DCOORDSYSSETTINGPANEL_H
#define DACHART3DCOORDSYSSETTINGPANEL_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>

namespace DA
{
class DAPropertyPanelContainerWidget;
class DAChart3DWidget;

// 3D 坐标系级属性设置面板
class DAGUI_API DAChart3DCoordSysSettingPanel : public QWidget
{
    Q_OBJECT
public:
    enum PropertyID {
        PID_Style = 1,
        PID_AxesColor,
        PID_NumberColor,
        PID_LabelColor,
        PID_NumberFont,
        PID_LabelFont,
        PID_GridLinesColor,
        PID_InteriorGridLinesColor,
        PID_InteriorGridMajorWidth,
        PID_InteriorGridMinorWidth,
        PID_TicLength,
        PID_TicLengthScale,
        PID_AutoScale,
        PID_AutoDecoration,
        PID_TickPosition,
        PID_LineSmooth
    };

    explicit DAChart3DCoordSysSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DCoordSysSettingPanel() override;

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
#endif  // DACHART3DCOORDSYSSETTINGPANEL_H
