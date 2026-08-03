#include "DAChart3DBarSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include "qwt3d_colormap_color.h"
#include "qwt3d_color.h"

namespace DA
{

/**
 * @brief 构造函数
 */
DAChart3DBarSettingPanel::DAChart3DBarSettingPanel(QWidget* parent)
    : DAChart3DItemSettingPanel(parent)
{
    connect(this, &DAChart3DItemSettingPanel::propertyValueChanged,
            this, &DAChart3DBarSettingPanel::onBarPropertyValueChanged);
    buildPropertyPanel();
}

DAChart3DBarSettingPanel::~DAChart3DBarSettingPanel()
{
}

/**
 * @brief 构建属性面板布局
 *
 * 分组：General / Style / Size / Color
 */
void DAChart3DBarSettingPanel::buildPropertyPanel()
{
    DAPropertyPanelContainerWidget* pp = propertyPanel();

    // ── General ──
    pp->addCollapsibleGroup(tr("General")  // cn:通用
    );
    pp->addStringProperty(PID_Title, tr("Title")  // cn:标题
    );
    pp->endGroup();

    // ── Style ──
    pp->addCollapsibleGroup(tr("Style")  // cn:样式
    );
    pp->addEnumProperty(PID_BarStyle, tr("Bar Style")  // cn:柱样式
                        ,
                        QStringList() << tr("Filled")  // cn:填充
                                      << tr("Filled Mesh")  // cn:填充网格
                                      << tr("Wireframe")  // cn:线框
                        ,
                        QList<int>() << static_cast<int>(Qwt3DBar::Filled)
                                    << static_cast<int>(Qwt3DBar::FilledMesh)
                                    << static_cast<int>(Qwt3DBar::Wireframe)
    );
    pp->endGroup();

    // ── Size ──
    pp->addCollapsibleGroup(tr("Size")  // cn:尺寸
    );
    pp->addDoubleProperty(PID_BarWidth, tr("Bar Width")  // cn:柱宽
                           ,
                           -1.0, -1.0, 10.0, 2);
    pp->addDoubleProperty(PID_BarDepth, tr("Bar Depth")  // cn:柱深
                           ,
                           -1.0, -1.0, 10.0, 2);
    pp->addDoubleProperty(PID_Baseline, tr("Baseline")  // cn:基线
                           ,
                           0.0, -1e9, 1e9, 4);
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
}

/**
 * @brief 从 Qwt3DBar 读取当前状态写入面板
 */
void DAChart3DBarSettingPanel::updateUI(Qwt3DPlotItem* item)
{
    if (nullptr == item) {
        DAChart3DItemSettingPanel::updateUI(item);
        return;
    }
    if (!checkItemRTTI(Rtti_Plot3DBar)) {
        DAChart3DItemSettingPanel::updateUI(item);
        return;
    }
    Qwt3DBar* bar = s_cast< Qwt3DBar* >();
    if (nullptr == bar) {
        return;
    }

    DAChart3DItemSettingPanel::updateUI(item);

    DAPropertyPanelContainerWidget* pp = propertyPanel();
    QSignalBlocker blocker(pp);

    pp->setStringValue(PID_Title, bar->title());
    pp->setEnumValue(PID_BarStyle, static_cast<int>(bar->barStyle()));
    pp->setDoubleValue(PID_BarWidth, bar->barWidth());
    pp->setDoubleValue(PID_BarDepth, bar->barDepth());
    pp->setDoubleValue(PID_Baseline, bar->baseline());

    // Colormap preset
    const Qwt3DColor* dc = bar->dataColor();
    const Qwt3DColorMapColor* cmc = dynamic_cast< const Qwt3DColorMapColor* >(dc);
    if (cmc) {
        QString preset = cmc->presetName();
        QStringList names = colormapPresetNames();
        int idx = names.indexOf(preset);
        if (idx >= 0) {
            pp->setEnumValue(PID_ColormapPreset, idx);
        }
    }

    RGBA meshC = bar->meshColor();
    pp->setColorValue(PID_MeshColor, GL2Qt(meshC.r, meshC.g, meshC.b));
    pp->setDoubleValue(PID_MeshLineWidth, bar->meshLineWidth());
}

/**
 * @brief 属性值变化处理
 */
void DAChart3DBarSettingPanel::onBarPropertyValueChanged(int propertyId)
{
    DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull;
    if (!checkItemRTTI(Rtti_Plot3DBar)) {
        return;
    }
    Qwt3DBar* bar = s_cast< Qwt3DBar* >();
    if (nullptr == bar) {
        return;
    }

    DAPropertyPanelContainerWidget* pp = propertyPanel();

    switch (propertyId) {
    case PID_Title:
        bar->setTitle(pp->getStringValue(PID_Title));
        break;
    case PID_BarStyle: {
        int val = pp->getEnumValue(PID_BarStyle);
        bar->setBarStyle(static_cast< Qwt3DBar::BarStyle >(val));
        break;
    }
    case PID_BarWidth:
        bar->setBarWidth(pp->getDoubleValue(PID_BarWidth));
        break;
    case PID_BarDepth:
        bar->setBarDepth(pp->getDoubleValue(PID_BarDepth));
        break;
    case PID_Baseline:
        bar->setBaseline(pp->getDoubleValue(PID_Baseline));
        break;
    case PID_ColormapPreset: {
        int idx = pp->getEnumValue(PID_ColormapPreset);
        QStringList names = colormapPresetNames();
        if (idx >= 0 && idx < names.size()) {
            bar->setDataColor(new Qwt3DColorMapColor(names.at(idx)));
        }
        break;
    }
    case PID_MeshColor: {
        QColor c = pp->getColorValue(PID_MeshColor);
        bar->setMeshColor(Qt2GL(c));
        break;
    }
    case PID_MeshLineWidth:
        bar->setMeshLineWidth(pp->getDoubleValue(PID_MeshLineWidth));
        break;
    default:
        break;
    }

    replot();
}

}  // namespace DA
