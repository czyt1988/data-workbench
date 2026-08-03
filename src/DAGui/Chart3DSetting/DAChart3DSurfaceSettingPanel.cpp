#include "DAChart3DSurfaceSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include "qwt3d_colormap_color.h"
#include "qwt3d_color.h"

namespace DA
{

/**
 * @brief 构造函数
 *
 * 连接属性值变化信号到 Surface 专用处理槽，然后构建属性面板。
 */
DAChart3DSurfaceSettingPanel::DAChart3DSurfaceSettingPanel(QWidget* parent)
    : DAChart3DItemSettingPanel(parent)
{
    connect(this, &DAChart3DItemSettingPanel::propertyValueChanged,
            this, &DAChart3DSurfaceSettingPanel::onSurfacePropertyValueChanged);
    buildPropertyPanel();
}

DAChart3DSurfaceSettingPanel::~DAChart3DSurfaceSettingPanel()
{
}

/**
 * @brief 构建属性面板布局
 *
 * 分组：General / Style / Color / Isolines / Floor / Normals
 */
void DAChart3DSurfaceSettingPanel::buildPropertyPanel()
{
    DAPropertyPanelContainerWidget* pp = propertyPanel();

    // ── General ──
    pp->addCollapsibleGroup(tr("General")  // cn:通用
    );
    pp->addStringProperty(PID_Title, tr("Title")  // cn:标题
    );
    pp->addEnumProperty(PID_PlotStyle, tr("Plot Style")  // cn:绘图样式
                        ,
                        QStringList() << tr("Wireframe")  // cn:线框
                                      << tr("Hidden Line")  // cn:隐藏线
                                      << tr("Filled")  // cn:填充
                                      << tr("Filled Mesh")  // cn:填充网格
                                      << tr("Points")  // cn:点
                        ,
                        QList<int>() << static_cast<int>(WIREFRAME)
                                    << static_cast<int>(HIDDENLINE)
                                    << static_cast<int>(FILLED)
                                    << static_cast<int>(FILLEDMESH)
                                    << static_cast<int>(QWT3D_POINTS)
    );
    pp->endGroup();

    // ── Style ──
    pp->addCollapsibleGroup(tr("Style")  // cn:样式
    );
    pp->addEnumProperty(PID_Shading, tr("Shading")  // cn:着色
                        ,
                        QStringList() << tr("Flat")  // cn:平面
                                      << tr("Gouraud")  // cn:平滑
                        ,
                        QList<int>() << static_cast<int>(FLAT)
                                    << static_cast<int>(GOURAUD)
    );
    pp->addDoubleProperty(PID_PolygonOffset, tr("Polygon Offset")  // cn:多边形偏移
                           ,
                           0.0, -10.0, 10.0, 2);
    pp->addIntProperty(PID_Resolution, tr("Resolution")  // cn:分辨率
                       ,
                       1, 1, 100);
    pp->endGroup();

    // ── Color ──
    pp->addCollapsibleGroup(tr("Color")  // cn:颜色
    );
    QStringList cmapNames = colormapPresetNames();
    QList<int> cmapData;
    for (int i = 0; i < cmapNames.size(); ++i) {
        cmapData << i;
    }
    pp->addEnumProperty(PID_ColormapPreset, tr("Colormap")  // cn:色图
                        ,
                        cmapNames, cmapData);
    pp->addColorProperty(PID_MeshColor, tr("Mesh Color")  // cn:网格颜色
    );
    pp->addDoubleProperty(PID_MeshLineWidth, tr("Mesh Line Width")  // cn:网格线宽
                           ,
                           1.0, 0.1, 10.0, 2);
    pp->endGroup();

    // ── Isolines ──
    pp->addCollapsibleGroup(tr("Isolines")  // cn:等值线
    );
    pp->addIntProperty(PID_Isolines, tr("Isolines Count")  // cn:等值线数量
                       ,
                       0, 0, 50);
    pp->addBoolProperty(PID_SmoothMesh, tr("Smooth Mesh")  // cn:平滑网格
    );
    pp->endGroup();

    // ── Floor ──
    pp->addCollapsibleGroup(tr("Floor")  // cn:底面投影
    );
    pp->addEnumProperty(PID_FloorStyle, tr("Floor Style")  // cn:底面样式
                        ,
                        QStringList() << tr("None")  // cn:无
                                      << tr("Iso")  // cn:等值线
                                      << tr("Data")  // cn:数据
                        ,
                        QList<int>() << static_cast<int>(NOFLOOR)
                                    << static_cast<int>(FLOORISO)
                                    << static_cast<int>(FLOORDATA)
    );
    pp->endGroup();

    // ── Normals ──
    pp->addCollapsibleGroup(tr("Normals")  // cn:法线
    );
    pp->addBoolProperty(PID_ShowNormals, tr("Show Normals")  // cn:显示法线
    );
    pp->addDoubleProperty(PID_NormalLength, tr("Normal Length")  // cn:法线长度
                           ,
                           0.0, 0.0, 1.0, 3);
    pp->addIntProperty(PID_NormalQuality, tr("Normal Quality")  // cn:法线质量
                       ,
                       3, 3, 50);
    pp->endGroup();
}

/**
 * @brief 从 Qwt3DSurface 读取当前状态写入面板
 */
void DAChart3DSurfaceSettingPanel::updateUI(Qwt3DPlotItem* item)
{
    if (nullptr == item) {
        DAChart3DItemSettingPanel::updateUI(item);
        return;
    }
    if (!checkItemRTTI(Rtti_Plot3DSurface)) {
        DAChart3DItemSettingPanel::updateUI(item);
        return;
    }
    Qwt3DSurface* surface = s_cast< Qwt3DSurface* >();
    if (nullptr == surface) {
        return;
    }

    DAChart3DItemSettingPanel::updateUI(item);

    DAPropertyPanelContainerWidget* pp = propertyPanel();
    QSignalBlocker blocker(pp);

    pp->setStringValue(PID_Title, surface->title());
    pp->setEnumValue(PID_PlotStyle, static_cast<int>(surface->plotStyle()));

    // Colormap preset
    const Qwt3DColor* dc = surface->dataColor();
    const Qwt3DColorMapColor* cmc = dynamic_cast< const Qwt3DColorMapColor* >(dc);
    if (cmc) {
        QString preset = cmc->presetName();
        QStringList names = colormapPresetNames();
        int idx = names.indexOf(preset);
        if (idx >= 0) {
            pp->setEnumValue(PID_ColormapPreset, idx);
        }
    }

    RGBA meshC = surface->meshColor();
    pp->setColorValue(PID_MeshColor, GL2Qt(meshC.r, meshC.g, meshC.b));
    pp->setDoubleValue(PID_MeshLineWidth, surface->meshLineWidth());
    pp->setIntValue(PID_Isolines, surface->isolines());
    pp->setBoolValue(PID_SmoothMesh, surface->smoothMesh());
    pp->setEnumValue(PID_FloorStyle, static_cast<int>(surface->floorStyle()));
    pp->setBoolValue(PID_ShowNormals, surface->normals());
    pp->setDoubleValue(PID_NormalLength, surface->normalLength());
    pp->setIntValue(PID_NormalQuality, surface->normalQuality());
    pp->setEnumValue(PID_Shading, static_cast<int>(surface->shading()));
    pp->setDoubleValue(PID_PolygonOffset, surface->polygonOffset());
    pp->setIntValue(PID_Resolution, surface->resolution());
}

/**
 * @brief 属性值变化处理
 */
void DAChart3DSurfaceSettingPanel::onSurfacePropertyValueChanged(int propertyId)
{
    DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull;
    if (!checkItemRTTI(Rtti_Plot3DSurface)) {
        return;
    }
    Qwt3DSurface* surface = s_cast< Qwt3DSurface* >();
    if (nullptr == surface) {
        return;
    }

    DAPropertyPanelContainerWidget* pp = propertyPanel();

    switch (propertyId) {
    case PID_Title:
        surface->setTitle(pp->getStringValue(PID_Title));
        break;
    case PID_PlotStyle: {
        int val = pp->getEnumValue(PID_PlotStyle);
        surface->setPlotStyle(static_cast< PLOTSTYLE >(val));
        break;
    }
    case PID_ColormapPreset: {
        int idx = pp->getEnumValue(PID_ColormapPreset);
        QStringList names = colormapPresetNames();
        if (idx >= 0 && idx < names.size()) {
            surface->setDataColor(new Qwt3DColorMapColor(names.at(idx)));
        }
        break;
    }
    case PID_MeshColor: {
        QColor c = pp->getColorValue(PID_MeshColor);
        surface->setMeshColor(Qt2GL(c));
        break;
    }
    case PID_MeshLineWidth:
        surface->setMeshLineWidth(pp->getDoubleValue(PID_MeshLineWidth));
        break;
    case PID_Isolines:
        surface->setIsolines(pp->getIntValue(PID_Isolines));
        break;
    case PID_SmoothMesh:
        surface->setSmoothMesh(pp->getBoolValue(PID_SmoothMesh));
        break;
    case PID_FloorStyle: {
        int val = pp->getEnumValue(PID_FloorStyle);
        surface->setFloorStyle(static_cast< FLOORSTYLE >(val));
        break;
    }
    case PID_ShowNormals:
        surface->showNormals(pp->getBoolValue(PID_ShowNormals));
        break;
    case PID_NormalLength:
        surface->setNormalLength(pp->getDoubleValue(PID_NormalLength));
        break;
    case PID_NormalQuality:
        surface->setNormalQuality(pp->getIntValue(PID_NormalQuality));
        break;
    case PID_Shading: {
        int val = pp->getEnumValue(PID_Shading);
        surface->setShading(static_cast< SHADINGSTYLE >(val));
        break;
    }
    case PID_PolygonOffset:
        surface->setPolygonOffset(pp->getDoubleValue(PID_PolygonOffset));
        break;
    case PID_Resolution:
        surface->setResolution(pp->getIntValue(PID_Resolution));
        break;
    default:
        break;
    }

    replot();
}

}  // namespace DA
