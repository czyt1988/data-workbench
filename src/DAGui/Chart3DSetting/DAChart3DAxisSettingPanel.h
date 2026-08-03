#ifndef DACHART3DAXISSETTINGPANEL_H
#define DACHART3DAXISSETTINGPANEL_H
#include "DAGuiAPI.h"
#include <QWidget>
#include <QPointer>
#include <QList>

class Qwt3DAxis;

namespace DA
{
class DAPropertyPanelContainerWidget;
class DAChart3DWidget;

// 3D 轴级属性设置面板
// 构造接收 axis direction(X/Y/Z)，面板内可选择该方向的具体轴(X1-X4/Y1-Y4/Z1-Z4)
class DAGUI_API DAChart3DAxisSettingPanel : public QWidget
{
    Q_OBJECT
public:
    // 轴方向枚举
    enum AxisDirection {
        DirX = 0,
        DirY,
        DirZ
    };

    enum PropertyID {
        PID_AxisSelector = 1,
        PID_LabelText,
        PID_LabelFont,
        PID_LabelColor,
        PID_NumberFont,
        PID_NumberColor,
        PID_MinRange,
        PID_MaxRange,
        PID_MajorCount,
        PID_MinorCount,
        PID_LineWidth,
        PID_TickPosition,
        PID_SmoothLine,
        PID_GridMajor,
        PID_GridMinor,
        PID_GridColor
    };

    explicit DAChart3DAxisSettingPanel(AxisDirection dir, QWidget* parent = nullptr);
    ~DAChart3DAxisSettingPanel() override;

    DAPropertyPanelContainerWidget* propertyPanel() const;
    void setTarget(DAChart3DWidget* chart);
    DAChart3DWidget* target() const;
    AxisDirection axisDirection() const;
    void updateUI();
    void replot();

Q_SIGNALS:
    void propertyValueChanged(int propertyId);

protected Q_SLOTS:
    void buildPropertyPanel();
    void onPanelPropertyValueChanged(int propertyId);
    void onPropertyValueChanged(int propertyId);

private:
    // 获取当前选中的 AXIS 枚举值
    int currentAxisEnum() const;
    // 获取当前选中的 Qwt3DAxis 引用
    Qwt3DAxis* currentAxis();
    // 该方向的所有 AXIS 值
    QList< int > axisEnumList() const;

private:
    DAPropertyPanelContainerWidget* mPanel;
    QPointer< DAChart3DWidget > mChart3D;
    AxisDirection mDirection;
};
}  // namespace DA
#endif  // DACHART3DAXISSETTINGPANEL_H
