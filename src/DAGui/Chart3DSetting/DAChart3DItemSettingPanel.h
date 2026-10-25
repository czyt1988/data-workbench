#ifndef DACHART3DITEMSETTINGPANEL_H
#define DACHART3DITEMSETTINGPANEL_H
#include "DAGuiAPI.h"
#include "DAAbstractChart3DItemSettingWidget.h"
#include "DAPropertyPanelContainerWidget.h"
// qwt3d（子类通过本头文件使用这些类型，保留传递包含）
#include "qwt3d_types.h"
#include "qwt3d_surface.h"
#include "qwt3d_bar.h"
#include "qwt3d_line3d.h"
#include "qwt3d_theme.h"

namespace DA
{
class DAChart3DWidget;

/**
 * @brief Qwt3D专有3D图表项属性设置面板基类
 *
 * 继承DAAbstractChart3DItemSettingWidget，内部持有DAPropertyPanelContainerWidget，
 * 统一转发属性面板的 propertyValueChanged 信号。
 *
 * 使用方式：
 * 1. 继承此类，实现buildPropertyPanel()
 * 2. 通过propertyPanel()调用DAPropertyPanelContainerWidget通用便捷方法
 *
 * 使用方式：
 * 1. 继承此类，实现buildPropertyPanel()
 * 2. 在buildPropertyPanel()中调用addXxxProperty方法构建面板
 * 3. 通过propertyPanel()调用DAPropertyPanelContainerWidget通用便捷方法
 * 4. 使用get/setXxxValue进行Qwt3D专有类型的值读写
 */
class DAGUI_API DAChart3DItemSettingPanel : public DAAbstractChart3DItemSettingWidget
{
    Q_OBJECT
public:
    explicit DAChart3DItemSettingPanel(QWidget* parent = nullptr);
    ~DAChart3DItemSettingPanel() override;

    // 获取通用的属性面板
    DAPropertyPanelContainerWidget* propertyPanel() const;

    // 获取关联的DAChart3DWidget（如果plot是DAChart3DWidget）
    DAChart3DWidget* getChart3DWidget() const;


Q_SIGNALS:
    /**
     * @brief Qwt3D专有属性值变化信号
     * @param propertyId 属性ID
     * @note 此信号转发自DAPropertyPanelContainerWidget::propertyValueChanged
     */
    void propertyValueChanged(int propertyId);

protected:
    // 纯虚函数，子类在此构建面板布局
    virtual void buildPropertyPanel() = 0;
    // 返回常用 colormap 预设名称列表，供 item 子类的 colormap 属性共用
    static QStringList colormapPresetNames();

protected Q_SLOTS:
    // 转发DAPropertyPanelContainerWidget::propertyValueChanged
    void onPanelPropertyValueChanged(int propertyId);

private:
    DAPropertyPanelContainerWidget* mPanel;
};

}  // namespace DA

#endif  // DACHART3DITEMSETTINGPANEL_H
