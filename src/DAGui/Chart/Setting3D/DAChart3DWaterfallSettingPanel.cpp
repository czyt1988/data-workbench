#include "Chart/Setting3D/DAChart3DWaterfallSettingPanel.h"
#include "Common/Settings/DAPropertyPanelContainerWidget.h"
#include <QLabel>
#include <QSignalBlocker>
#include "qwt3d_types.h"
#include "qwt3d_waterfall.h"

namespace DA
{

/**
 * @brief 构造函数：连接属性值变化信号并构建面板
 */
DAChart3DWaterfallSettingPanel::DAChart3DWaterfallSettingPanel(QWidget* parent) : DAChart3DItemSettingPanel(parent)
{
    connect(this,
            &DAChart3DItemSettingPanel::propertyValueChanged,
            this,
            &DAChart3DWaterfallSettingPanel::onWaterfallPropertyValueChanged);
    buildPropertyPanel();
}

DAChart3DWaterfallSettingPanel::~DAChart3DWaterfallSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 分组：基础（标题/可见/Z 值）/ 样式（几何样式）/ 着色（着色方式/单色）/
 * 外观（不透明度/深度方向/描边颜色/描边线宽）+ 只读提示（色带与色域在丝带面板）。
 */
void DAChart3DWaterfallSettingPanel::buildPropertyPanel()
{
    DAPropertyPanelContainerWidget* pp = propertyPanel();

    // ── 基础 ──
    pp->addCollapsibleGroup(tr("Basic")  // cn:基础
    );
    pp->addStringProperty(PID_Title, tr("Title")  // cn:标题
    );
    pp->addBoolProperty(PID_Visible, tr("Visible")  // cn:可见
                        ,
                        true);
    pp->addDoubleProperty(PID_ZValue, tr("Z Value")  // cn:Z值
                           ,
                           0.0,
                           -1.0e6,
                           1.0e6);
    pp->endGroup();

    // ── 样式 ──
    pp->addCollapsibleGroup(tr("Style")  // cn:样式
    );
    pp->addEnumProperty(PID_PlotStyle, tr("Plot Style")  // cn:几何样式
                        ,
                        QStringList() << tr("Ribbon")  // cn:帘幕
                                      << tr("Ribbon + Mesh")  // cn:帘幕+描边
                                      << tr("Wireframe")  // cn:线框
                                      << tr("Tube")  // cn:管状
                        ,
                        QList< int >() << static_cast< int >(Qwt3DWaterfall::Ribbon)
                                       << static_cast< int >(Qwt3DWaterfall::RibbonMesh)
                                       << static_cast< int >(Qwt3DWaterfall::Wireframe)
                                       << static_cast< int >(Qwt3DWaterfall::Tube));
    pp->endGroup();

    // ── 着色 ──
    pp->addCollapsibleGroup(tr("Color")  // cn:着色
    );
    pp->addEnumProperty(PID_ColorMode, tr("Color Mode")  // cn:着色方式
                        ,
                        QStringList() << tr("By Amplitude")  // cn:按幅值
                                      << tr("By Depth")  // cn:按深度
                                      << tr("Solid")  // cn:单色
                        ,
                        QList< int >() << static_cast< int >(Qwt3DWaterfall::ByAmplitude)
                                       << static_cast< int >(Qwt3DWaterfall::ByDepth)
                                       << static_cast< int >(Qwt3DWaterfall::Solid));
    pp->addColorProperty(PID_SolidColor, tr("Solid Color")  // cn:单色
    );
    // 色带与 dB 色域在插件丝带面板（单一入口）
    auto* hint = new QLabel(tr("Color map and dB range are set in Signal Display > Waterfall"),
                            pp);  // cn:色带与 dB 范围在「信号显示 → 瀑布图设置」里设置
    hint->setWordWrap(true);
    pp->addProperty(PID_Hint, tr("Color Map"), hint);
    pp->endGroup();

    // ── 外观 ──
    pp->addCollapsibleGroup(tr("Appearance")  // cn:外观
    );
    pp->addDoubleProperty(PID_Opacity, tr("Opacity")  // cn:不透明度
                           ,
                           1.0,
                           0.0,
                           1.0,
                           2);
    pp->addEnumProperty(PID_DepthOrder, tr("Depth Order")  // cn:深度方向
                        ,
                        QStringList() << tr("Newest in Front")  // cn:最新在前
                                      << tr("Newest Behind")  // cn:最新在后
                        ,
                        QList< int >() << static_cast< int >(Qwt3DWaterfall::NewestInFront)
                                       << static_cast< int >(Qwt3DWaterfall::NewestBehind));
    pp->addColorProperty(PID_OutlineColor, tr("Outline Color")  // cn:描边颜色
    );
    pp->addDoubleProperty(PID_OutlineWidth, tr("Outline Width")  // cn:描边线宽
                           ,
                           1.0,
                           0.0,
                           10.0,
                           2);
    pp->endGroup();
}

/**
 * @brief 从三维瀑布图元读取当前状态写入面板
 * @param item 3D 图元
 */
void DAChart3DWaterfallSettingPanel::updateUI(Qwt3DPlotItem* item)
{
    if (nullptr == item) {
        DAChart3DItemSettingPanel::updateUI(item);
        return;
    }
    DAChart3DItemSettingPanel::updateUI(item);
    if (!checkItemRTTI(Rtti_Plot3DWaterfall)) {
        return;
    }
    Qwt3DWaterfall* waterfall = s_cast< Qwt3DWaterfall* >();
    if (nullptr == waterfall) {
        return;
    }
    DAPropertyPanelContainerWidget* pp = propertyPanel();
    QSignalBlocker blocker(pp);

    pp->setStringValue(PID_Title, waterfall->title());
    pp->setBoolValue(PID_Visible, waterfall->isVisible());
    pp->setDoubleValue(PID_ZValue, waterfall->z());
    pp->setEnumValue(PID_PlotStyle, static_cast< int >(waterfall->plotStyle()));
    pp->setEnumValue(PID_ColorMode, static_cast< int >(waterfall->colorMode()));
    const RGBA solid = waterfall->color();
    pp->setColorValue(PID_SolidColor, GL2Qt(solid.r, solid.g, solid.b));
    pp->setDoubleValue(PID_Opacity, waterfall->opacity());
    pp->setEnumValue(PID_DepthOrder, static_cast< int >(waterfall->depthOrder()));
    const RGBA outline = waterfall->outlineColor();
    pp->setColorValue(PID_OutlineColor, GL2Qt(outline.r, outline.g, outline.b));
    pp->setDoubleValue(PID_OutlineWidth, waterfall->outlineWidth());
}

/**
 * @brief 属性值变化：写回三维瀑布图元并重绘
 * @param propertyId 属性 ID
 */
void DAChart3DWaterfallSettingPanel::onWaterfallPropertyValueChanged(int propertyId)
{
    DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull;
    if (!checkItemRTTI(Rtti_Plot3DWaterfall)) {
        return;
    }
    Qwt3DWaterfall* waterfall = s_cast< Qwt3DWaterfall* >();
    if (nullptr == waterfall) {
        return;
    }
    DAPropertyPanelContainerWidget* pp = propertyPanel();

    switch (propertyId) {
    case PID_Title:
        waterfall->setTitle(pp->getStringValue(PID_Title));
        break;
    case PID_Visible:
        waterfall->setVisible(pp->getBoolValue(PID_Visible));
        break;
    case PID_ZValue:
        waterfall->setZ(pp->getDoubleValue(PID_ZValue));
        break;
    case PID_PlotStyle:
        waterfall->setPlotStyle(static_cast< Qwt3DWaterfall::PlotStyle >(pp->getEnumValue(PID_PlotStyle)));
        break;
    case PID_ColorMode:
        waterfall->setColorMode(static_cast< Qwt3DWaterfall::ColorMode >(pp->getEnumValue(PID_ColorMode)));
        break;
    case PID_SolidColor:
        waterfall->setColor(Qt2GL(pp->getColorValue(PID_SolidColor)));
        break;
    case PID_Opacity:
        waterfall->setOpacity(pp->getDoubleValue(PID_Opacity));
        break;
    case PID_DepthOrder:
        waterfall->setDepthOrder(static_cast< Qwt3DWaterfall::DepthOrder >(pp->getEnumValue(PID_DepthOrder)));
        break;
    case PID_OutlineColor:
        waterfall->setOutlineColor(Qt2GL(pp->getColorValue(PID_OutlineColor)));
        break;
    case PID_OutlineWidth:
        waterfall->setOutlineWidth(pp->getDoubleValue(PID_OutlineWidth));
        break;
    default:
        break;
    }

    replot();
}

}  // namespace DA