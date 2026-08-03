#include "DAChart3DPlotSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAChart3DWidget.h"
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QPushButton>

namespace DA
{

/**
 * @brief 构造函数
 *
 * 创建属性面板容器，设置布局，连接信号链，构建属性面板。
 */
DAChart3DPlotSettingPanel::DAChart3DPlotSettingPanel(QWidget* parent) : QWidget(parent), mPanel(nullptr)
{
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged,
            this, &DAChart3DPlotSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChart3DPlotSettingPanel::propertyValueChanged,
            this, &DAChart3DPlotSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

DAChart3DPlotSettingPanel::~DAChart3DPlotSettingPanel()
{
}

DAPropertyPanelContainerWidget* DAChart3DPlotSettingPanel::propertyPanel() const
{
    return mPanel;
}

void DAChart3DPlotSettingPanel::setTarget(DAChart3DWidget* chart)
{
    if (mChart3D == chart) {
        return;
    }
    mChart3D = chart;
    updateUI();
}

DAChart3DWidget* DAChart3DPlotSettingPanel::target() const
{
    return mChart3D;
}

void DAChart3DPlotSettingPanel::replot()
{
    if (mChart3D) {
        mChart3D->update();
    }
}

/**
 * @brief 构建属性面板布局
 *
 * 分组：Title / Background / Projection / Lighting / Theme
 */
void DAChart3DPlotSettingPanel::buildPropertyPanel()
{
    // ── Title ──
    mPanel->addCollapsibleGroup(tr("Title")  // cn:标题
    );
    mPanel->addStringProperty(PID_TitleText, tr("Title Text")  // cn:标题文本
    );
    mPanel->addFontProperty(PID_TitleFont, tr("Title Font")  // cn:标题字体
    );
    mPanel->addColorProperty(PID_TitleColor, tr("Title Color")  // cn:标题颜色
    );
    mPanel->endGroup();

    // ── Background ──
    mPanel->addCollapsibleGroup(tr("Background")  // cn:背景
    );
    mPanel->addColorProperty(PID_BackgroundColor, tr("Background Color")  // cn:背景色
    );
    mPanel->endGroup();

    // ── Projection ──
    mPanel->addCollapsibleGroup(tr("Projection")  // cn:投影
    );
    mPanel->addEnumProperty(PID_Projection, tr("Projection")  // cn:投影
                            ,
                            QStringList() << tr("Perspective")  // cn:透视
                                          << tr("Orthographic")  // cn:正交
                            ,
                            QList<int>() << 0 << 1);
    mPanel->addEnumProperty(PID_AspectRatio, tr("Aspect Ratio")  // cn:纵横比
                            ,
                            QStringList() << tr("Auto Fill")  // cn:自动填充
                                          << tr("Data Ratio")  // cn:数据比例
                            ,
                            QList<int>() << static_cast<int>(AUTOFILL)
                                        << static_cast<int>(DATARATIO)
    );
    mPanel->endGroup();

    // ── Lighting ──
    mPanel->addCollapsibleGroup(tr("Lighting")  // cn:光照
    );
    mPanel->addBoolProperty(PID_EnableLighting, tr("Enable Lighting")  // cn:启用光照
    );
    mPanel->addEnumProperty(PID_LightingPreset, tr("Lighting Preset")  // cn:光照预设
                            ,
                            QStringList() << tr("No Lighting")  // cn:无光照
                                          << tr("Flat Light")  // cn:平面光
                                          << tr("Studio")  // cn:工作室
                                          << tr("Outdoor")  // cn:户外
                                          << tr("Soft")  // cn:柔和
                            ,
                            QList<int>() << static_cast<int>(Qwt3DTheme::NoLighting)
                                        << static_cast<int>(Qwt3DTheme::FlatLight)
                                        << static_cast<int>(Qwt3DTheme::Studio)
                                        << static_cast<int>(Qwt3DTheme::Outdoor)
                                        << static_cast<int>(Qwt3DTheme::Soft)
    );
    mPanel->addDoubleProperty(PID_Shininess, tr("Shininess")  // cn:光泽度
                               ,
                               32.0, 0.0, 128.0, 1);
    mPanel->addDoubleProperty(PID_SpecularIntensity, tr("Specular Intensity")  // cn:镜面强度
                               ,
                               1.0, 0.0, 10.0, 2);
    mPanel->endGroup();

    // ── Theme ──
    mPanel->addCollapsibleGroup(tr("Theme")  // cn:主题
    );
    mPanel->addEnumProperty(PID_ThemePreset, tr("Theme")  // cn:主题
                            ,
                            QStringList() << tr("Default")  // cn:默认
                                          << tr("Dark")  // cn:暗色
                                          << tr("Scientific")  // cn:科学
                                          << tr("Warm")  // cn:暖色
                                          << tr("Cool")  // cn:冷色
                                          << tr("Matplotlib")  // cn:Matplotlib
                                          << tr("Earth Tones")  // cn:大地色
                                          << tr("Ocean")  // cn:海洋
                                          << tr("High Contrast")  // cn:高对比
                                          << tr("Presentation")  // cn:演示
                            ,
                            QList<int>() << static_cast<int>(Qwt3DTheme::Default)
                                        << static_cast<int>(Qwt3DTheme::Dark)
                                        << static_cast<int>(Qwt3DTheme::Scientific)
                                        << static_cast<int>(Qwt3DTheme::Warm)
                                        << static_cast<int>(Qwt3DTheme::Cool)
                                        << static_cast<int>(Qwt3DTheme::Matplotlib)
                                        << static_cast<int>(Qwt3DTheme::EarthTones)
                                        << static_cast<int>(Qwt3DTheme::Ocean)
                                        << static_cast<int>(Qwt3DTheme::HighContrast)
                                        << static_cast<int>(Qwt3DTheme::Presentation)
    );
    // Reset View 按钮
    QPushButton* resetBtn = new QPushButton(tr("Reset View")  // cn:重置视图
                                              , this);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        onPanelPropertyValueChanged(PID_ResetView);
    });
    mPanel->addProperty(PID_ResetView, tr("Reset View")  // cn:重置视图
                        , resetBtn);
    mPanel->endGroup();
}

/**
 * @brief 从 Qwt3DPlot 读取当前状态写入面板
 */
void DAChart3DPlotSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    if (!mChart3D) {
        return;
    }

    Qwt3DPlot* plot = static_cast< Qwt3DPlot* >(mChart3D.data());

    mPanel->setStringValue(PID_TitleText, plot->title());

    // 从 theme 读取标题字体
    Qwt3DTheme t = plot->theme();
    QFont titleFont(t.titleFontFamily(), t.titleFontSize());
    titleFont.setBold(t.titleFontBold());
    mPanel->setFontValue(PID_TitleFont, titleFont);

    RGBA tc = t.titleColor();
    mPanel->setColorValue(PID_TitleColor, GL2Qt(tc.r, tc.g, tc.b));

    RGBA bg = plot->backgroundRGBAColor();
    mPanel->setColorValue(PID_BackgroundColor, GL2Qt(bg.r, bg.g, bg.b));

    // Projection: 1=Orthographic, 0=Perspective
    mPanel->setEnumValue(PID_Projection, plot->ortho() ? 1 : 0);
    mPanel->setEnumValue(PID_AspectRatio, static_cast<int>(plot->aspectRatioMode()));

    mPanel->setBoolValue(PID_EnableLighting, plot->lightingEnabled());
    mPanel->setEnumValue(PID_LightingPreset, static_cast<int>(t.lightingPreset()));
    mPanel->setDoubleValue(PID_Shininess, t.shininess());
    mPanel->setDoubleValue(PID_SpecularIntensity, t.specularIntensity());

    // Theme preset: 无 getter 可识别当前预设，重置为 Default
    mPanel->setEnumValue(PID_ThemePreset, static_cast<int>(Qwt3DTheme::Default));
}

/**
 * @brief 信号转发
 */
void DAChart3DPlotSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 */
void DAChart3DPlotSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mChart3D) {
        return;
    }

    Qwt3DPlot* plot = static_cast< Qwt3DPlot* >(mChart3D.data());

    switch (propertyId) {
    case PID_TitleText:
        plot->setTitle(mPanel->getStringValue(PID_TitleText));
        break;
    case PID_TitleFont: {
        QFont f = mPanel->getFontValue(PID_TitleFont);
        plot->setTitleFont(f.family(), f.pointSize(), static_cast<int>(f.weight()), f.italic());
        break;
    }
    case PID_TitleColor:
        plot->setTitleColor(Qt2GL(mPanel->getColorValue(PID_TitleColor)));
        break;
    case PID_BackgroundColor:
        plot->setBackgroundColor(Qt2GL(mPanel->getColorValue(PID_BackgroundColor)));
        break;
    case PID_Projection: {
        int val = mPanel->getEnumValue(PID_Projection);
        plot->setOrtho(val == 1);
        break;
    }
    case PID_AspectRatio: {
        int val = mPanel->getEnumValue(PID_AspectRatio);
        plot->setAspectRatioMode(static_cast< ASPECTRATIOMODE >(val));
        break;
    }
    case PID_EnableLighting:
        plot->enableLighting(mPanel->getBoolValue(PID_EnableLighting));
        break;
    case PID_LightingPreset: {
        int val = mPanel->getEnumValue(PID_LightingPreset);
        Qwt3DTheme t = plot->theme();
        t.setLightingPreset(static_cast< Qwt3DTheme::LightingPreset >(val));
        plot->setTheme(t);
        break;
    }
    case PID_Shininess: {
        Qwt3DTheme t = plot->theme();
        t.setShininess(mPanel->getDoubleValue(PID_Shininess));
        plot->setTheme(t);
        break;
    }
    case PID_SpecularIntensity: {
        Qwt3DTheme t = plot->theme();
        t.setSpecularIntensity(mPanel->getDoubleValue(PID_SpecularIntensity));
        plot->setTheme(t);
        break;
    }
    case PID_ThemePreset: {
        int val = mPanel->getEnumValue(PID_ThemePreset);
        plot->applyTheme(static_cast< Qwt3DTheme::Preset >(val));
        break;
    }
    case PID_ResetView:
        plot->setRotation(0, 0, 0);
        plot->setShift(0, 0, 0);
        plot->setZoom(1.0);
        plot->setViewportShift(0, 0);
        break;
    default:
        break;
    }

    replot();
}

}  // namespace DA
