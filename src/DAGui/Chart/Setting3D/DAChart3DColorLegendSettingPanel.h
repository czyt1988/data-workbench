#ifndef DACHART3DCOLORLEGENDSETTINGPANEL_H
#define DACHART3DCOLORLEGENDSETTINGPANEL_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>

namespace DA
{
class DAPropertyPanelContainerWidget;
class DAChart3DWidget;

// 3D 颜色图例属性设置面板
class DAGUI_API DAChart3DColorLegendSettingPanel : public QWidget
{
    Q_OBJECT
public:
    enum PropertyID {
        PID_Visible = 1,
        PID_Position,
        PID_AbsPosX,
        PID_AbsPosY,
        PID_AbsWidth,
        PID_AbsHeight
    };

    explicit DAChart3DColorLegendSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DColorLegendSettingPanel() override;

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
#endif  // DACHART3DCOLORLEGENDSETTINGPANEL_H
