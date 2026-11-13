#ifndef DACHART3DGENERICITEMSETTINGPANEL_H
#define DACHART3DGENERICITEMSETTINGPANEL_H
#include "DAGuiAPI.h"
#include "Chart/Setting3D/DAChart3DItemSettingPanel.h"

namespace DA
{
// 3D 通用图元属性设置面板（未注册 rtti 的回退面板，只提供 Qwt3DPlotItem 基类安全属性）
class DAGUI_API DAChart3DGenericItemSettingPanel : public DAChart3DItemSettingPanel
{
    Q_OBJECT
public:
    // 属性ID枚举
    enum PropertyId {
        PropTitle = 0,
        PropVisible,
        PropZ
    };

    explicit DAChart3DGenericItemSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DGenericItemSettingPanel() override;

    void updateUI(Qwt3DPlotItem* item) override;

private:
    void buildPropertyPanel() override;

private Q_SLOTS:
    // 属性值变化处理
    void onGenericPropertyValueChanged(int propertyId);
};
}  // namespace DA
#endif  // DACHART3DGENERICITEMSETTINGPANEL_H
