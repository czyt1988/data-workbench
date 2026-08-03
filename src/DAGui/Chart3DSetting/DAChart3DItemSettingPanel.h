#ifndef DACHART3DITEMSETTINGPANEL_H
#define DACHART3DITEMSETTINGPANEL_H
#include "DAGuiAPI.h"
#include "DAAbstractChart3DItemSettingWidget.h"
#include "DAPropertyPanelContainerWidget.h"
// qwt3d
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
 * 在通用属性便捷方法之上叠加Qwt3D专有属性创建方法和值读写方法。
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

    // === Qwt3D专有属性创建方法 ===

    // 添加3D绘图样式属性（QComboBox，填充PLOTSTYLE枚举）
    void addPlotStyle3DProperty(int id, const QString& name);
    // 添加shading属性（QComboBox，填充SHADINGSTYLE枚举）
    void addShadingProperty(int id, const QString& name);
    // 添加floor投影样式属性（QComboBox，填充FLOORSTYLE枚举）
    void addFloorStyleProperty(int id, const QString& name);
    // 添加坐标系样式属性（QComboBox，填充COORDSTYLE枚举）
    void addCoordStyleProperty(int id, const QString& name);
    // 添加缩放类型属性（QComboBox，填充SCALETYPE枚举）
    void addScaleTypeProperty(int id, const QString& name);
    // 添加3D柱状图样式属性（QComboBox，填充Qwt3DBar::BarStyle枚举）
    void addBarStyle3DProperty(int id, const QString& name);
    // 添加3D线图样式属性（QComboBox，填充Qwt3DLine::LineStyle枚举）
    void addLineStyle3DProperty(int id, const QString& name);
    // 添加3D点形状属性（QComboBox，填充Qwt3DLine::PointShape枚举）
    void addPointShape3DProperty(int id, const QString& name);
    // 添加主题预设属性（QComboBox，填充Qwt3DTheme::Preset枚举）
    void addThemePresetProperty(int id, const QString& name);
    // 添加光照预设属性（QComboBox，填充Qwt3DTheme::LightingPreset枚举）
    void addLightingPresetProperty(int id, const QString& name);

    // === Qwt3D专有值读写方法 ===

    PLOTSTYLE getPlotStyle3DValue(int id) const;
    void setPlotStyle3DValue(int id, PLOTSTYLE style);

    SHADINGSTYLE getShadingValue(int id) const;
    void setShadingValue(int id, SHADINGSTYLE style);

    FLOORSTYLE getFloorStyleValue(int id) const;
    void setFloorStyleValue(int id, FLOORSTYLE style);

    COORDSTYLE getCoordStyleValue(int id) const;
    void setCoordStyleValue(int id, COORDSTYLE style);

    SCALETYPE getScaleTypeValue(int id) const;
    void setScaleTypeValue(int id, SCALETYPE type);

    Qwt3DBar::BarStyle getBarStyle3DValue(int id) const;
    void setBarStyle3DValue(int id, Qwt3DBar::BarStyle style);

    Qwt3DLine::LineStyle getLineStyle3DValue(int id) const;
    void setLineStyle3DValue(int id, Qwt3DLine::LineStyle style);

    Qwt3DLine::PointShape getPointShape3DValue(int id) const;
    void setPointShape3DValue(int id, Qwt3DLine::PointShape shape);

    Qwt3DTheme::Preset getThemePresetValue(int id) const;
    void setThemePresetValue(int id, Qwt3DTheme::Preset preset);

    Qwt3DTheme::LightingPreset getLightingPresetValue(int id) const;
    void setLightingPresetValue(int id, Qwt3DTheme::LightingPreset preset);

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

protected Q_SLOTS:
    // 转发DAPropertyPanelContainerWidget::propertyValueChanged
    void onPanelPropertyValueChanged(int propertyId);

private:
    DAPropertyPanelContainerWidget* mPanel;
};

}  // namespace DA

#endif  // DACHART3DITEMSETTINGPANEL_H
