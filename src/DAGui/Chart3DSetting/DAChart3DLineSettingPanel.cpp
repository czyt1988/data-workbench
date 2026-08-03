#include "DAChart3DLineSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include "qwt3d_colormap_color.h"
#include "qwt3d_color.h"

namespace DA
{

/**
 * @brief 构造函数
 */
DAChart3DLineSettingPanel::DAChart3DLineSettingPanel(QWidget* parent)
    : DAChart3DItemSettingPanel(parent)
{
    connect(this, &DAChart3DItemSettingPanel::propertyValueChanged,
            this, &DAChart3DLineSettingPanel::onLinePropertyValueChanged);
    buildPropertyPanel();
}

DAChart3DLineSettingPanel::~DAChart3DLineSettingPanel()
{
}

/**
 * @brief 构建属性面板布局
 *
 * 分组：General / Line / Points / Color
 */
void DAChart3DLineSettingPanel::buildPropertyPanel()
{
    DAPropertyPanelContainerWidget* pp = propertyPanel();

    // ── General ──
    pp->addCollapsibleGroup(tr("General")  // cn:通用
    );
    pp->addStringProperty(PID_Title, tr("Title")  // cn:标题
    );
    pp->endGroup();

    // ── Line ──
    pp->addCollapsibleGroup(tr("Line")  // cn:线条
    );
    pp->addEnumProperty(PID_LineStyle, tr("Line Style")  // cn:线样式
                        ,
                        QStringList() << tr("Lines")  // cn:线条
                                      << tr("Tube")  // cn:管状
                                      << tr("Dots")  // cn:点
                        ,
                        QList<int>() << static_cast<int>(Qwt3DLine::Lines)
                                    << static_cast<int>(Qwt3DLine::Tube)
                                    << static_cast<int>(Qwt3DLine::Dots)
    );
    pp->addDoubleProperty(PID_LineWidth, tr("Line Width")  // cn:线宽
                           ,
                           1.0, 0.5, 20.0, 1);
    pp->addDoubleProperty(PID_TubeRadius, tr("Tube Radius")  // cn:管半径
                           ,
                           -1.0, -1.0, 5.0, 3);
    pp->addIntProperty(PID_TubeSegments, tr("Tube Segments")  // cn:管段数
                       ,
                       3, 3, 64);
    pp->endGroup();

    // ── Points ──
    pp->addCollapsibleGroup(tr("Points")  // cn:点
    );
    pp->addBoolProperty(PID_PointVisible, tr("Show Points")  // cn:显示点
    );
    pp->addDoubleProperty(PID_PointSize, tr("Point Size")  // cn:点大小
                           ,
                           3.0, 1.0, 50.0, 1);
    pp->addEnumProperty(PID_PointShape, tr("Point Shape")  // cn:点形状
                        ,
                        QStringList() << tr("Dot")  // cn:圆点
                                      << tr("Cube")  // cn:立方体
                                      << tr("Tetrahedron")  // cn:四面体
                                      << tr("Octahedron")  // cn:八面体
                                      << tr("Sphere")  // cn:球体
                        ,
                        QList<int>() << static_cast<int>(Qwt3DLine::Dot)
                                    << static_cast<int>(Qwt3DLine::Cube)
                                    << static_cast<int>(Qwt3DLine::Tetrahedron)
                                    << static_cast<int>(Qwt3DLine::Octahedron)
                                    << static_cast<int>(Qwt3DLine::Sphere)
    );
    pp->endGroup();

    // ── Color ──
    pp->addCollapsibleGroup(tr("Color")  // cn:颜色
    );
    pp->addBoolProperty(PID_UseDataColor, tr("Use Data Color")  // cn:使用数据着色
    );
    pp->addColorProperty(PID_SolidColor, tr("Solid Color")  // cn:纯色
    );
    QStringList cmapNames = colormapPresetNames();
    QList<int> cmapData;
    for (int i = 0; i < cmapNames.size(); ++i) {
        cmapData << i;
    }
    pp->addEnumProperty(PID_ColormapPreset, tr("Colormap")  // cn:色图
                        ,
                        cmapNames, cmapData);
    pp->endGroup();
}

/**
 * @brief 从 Qwt3DLine 读取当前状态写入面板
 */
void DAChart3DLineSettingPanel::updateUI(Qwt3DPlotItem* item)
{
    if (nullptr == item) {
        DAChart3DItemSettingPanel::updateUI(item);
        return;
    }
    if (!checkItemRTTI(Rtti_Plot3DLine)) {
        DAChart3DItemSettingPanel::updateUI(item);
        return;
    }
    Qwt3DLine* line = s_cast< Qwt3DLine* >();
    if (nullptr == line) {
        return;
    }

    DAChart3DItemSettingPanel::updateUI(item);

    DAPropertyPanelContainerWidget* pp = propertyPanel();
    QSignalBlocker blocker(pp);

    pp->setStringValue(PID_Title, line->title());
    pp->setEnumValue(PID_LineStyle, static_cast<int>(line->lineStyle()));
    pp->setDoubleValue(PID_LineWidth, line->lineWidth());
    pp->setDoubleValue(PID_TubeRadius, line->tubeRadius());
    pp->setIntValue(PID_TubeSegments, line->tubeSegments());
    pp->setBoolValue(PID_PointVisible, line->pointVisible());
    pp->setDoubleValue(PID_PointSize, line->pointSize());
    pp->setEnumValue(PID_PointShape, static_cast<int>(line->pointShape()));

    bool useData = (line->dataColor() != nullptr);
    pp->setBoolValue(PID_UseDataColor, useData);

    if (!useData) {
        // solid color 模式
        RGBA c = line->color();
        pp->setColorValue(PID_SolidColor, GL2Qt(c.r, c.g, c.b));
        pp->setPropertyEnabled(PID_SolidColor, true);
        pp->setPropertyEnabled(PID_ColormapPreset, false);
    } else {
        // data color 模式 — 尝试读回 colormap preset
        const Qwt3DColor* dc = line->dataColor();
        const Qwt3DColorMapColor* cmc = dynamic_cast< const Qwt3DColorMapColor* >(dc);
        if (cmc) {
            QString preset = cmc->presetName();
            QStringList names = colormapPresetNames();
            int idx = names.indexOf(preset);
            if (idx >= 0) {
                pp->setEnumValue(PID_ColormapPreset, idx);
            }
        }
        pp->setPropertyEnabled(PID_SolidColor, false);
        pp->setPropertyEnabled(PID_ColormapPreset, true);
    }
}

/**
 * @brief 属性值变化处理
 */
void DAChart3DLineSettingPanel::onLinePropertyValueChanged(int propertyId)
{
    DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull;
    if (!checkItemRTTI(Rtti_Plot3DLine)) {
        return;
    }
    Qwt3DLine* line = s_cast< Qwt3DLine* >();
    if (nullptr == line) {
        return;
    }

    DAPropertyPanelContainerWidget* pp = propertyPanel();

    switch (propertyId) {
    case PID_Title:
        line->setTitle(pp->getStringValue(PID_Title));
        break;
    case PID_LineStyle: {
        int val = pp->getEnumValue(PID_LineStyle);
        line->setLineStyle(static_cast< Qwt3DLine::LineStyle >(val));
        break;
    }
    case PID_LineWidth:
        line->setLineWidth(pp->getDoubleValue(PID_LineWidth));
        break;
    case PID_TubeRadius:
        line->setTubeRadius(pp->getDoubleValue(PID_TubeRadius));
        break;
    case PID_TubeSegments:
        line->setTubeSegments(pp->getIntValue(PID_TubeSegments));
        break;
    case PID_PointVisible:
        line->setPointVisible(pp->getBoolValue(PID_PointVisible));
        break;
    case PID_PointSize:
        line->setPointSize(pp->getDoubleValue(PID_PointSize));
        break;
    case PID_PointShape: {
        int val = pp->getEnumValue(PID_PointShape);
        line->setPointShape(static_cast< Qwt3DLine::PointShape >(val));
        break;
    }
    case PID_UseDataColor: {
        bool useData = pp->getBoolValue(PID_UseDataColor);
        if (useData) {
            int idx = pp->getEnumValue(PID_ColormapPreset);
            QStringList names = colormapPresetNames();
            if (idx >= 0 && idx < names.size()) {
                line->setDataColor(new Qwt3DColorMapColor(names.at(idx)));
            }
            pp->setPropertyEnabled(PID_SolidColor, false);
            pp->setPropertyEnabled(PID_ColormapPreset, true);
        } else {
            line->setDataColor(nullptr);
            QColor c = pp->getColorValue(PID_SolidColor);
            line->setColor(Qt2GL(c));
            pp->setPropertyEnabled(PID_SolidColor, true);
            pp->setPropertyEnabled(PID_ColormapPreset, false);
        }
        break;
    }
    case PID_SolidColor: {
        if (!pp->getBoolValue(PID_UseDataColor)) {
            QColor c = pp->getColorValue(PID_SolidColor);
            line->setColor(Qt2GL(c));
        }
        break;
    }
    case PID_ColormapPreset: {
        if (pp->getBoolValue(PID_UseDataColor)) {
            int idx = pp->getEnumValue(PID_ColormapPreset);
            QStringList names = colormapPresetNames();
            if (idx >= 0 && idx < names.size()) {
                line->setDataColor(new Qwt3DColorMapColor(names.at(idx)));
            }
        }
        break;
    }
    default:
        break;
    }

    replot();
}

}  // namespace DA
