#include "DAChart3DItemSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAPropertyItemWidget.h"
#include "DAChart3DWidget.h"
#include <QComboBox>
#include <QVBoxLayout>
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 *
 * 创建DAPropertyPanelContainerWidget并设为自身主布局，
 * 连接propertyValueChanged信号转发。
 * 注意：不在此调用buildPropertyPanel()，由子类构造函数末尾自行调用。
 */
DAChart3DItemSettingPanel::DAChart3DItemSettingPanel(QWidget* parent)
    : DAAbstractChart3DItemSettingWidget(parent), mPanel(nullptr)
{
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    connect(mPanel,
            &DAPropertyPanelContainerWidget::propertyValueChanged,
            this,
            &DAChart3DItemSettingPanel::onPanelPropertyValueChanged);
}

DAChart3DItemSettingPanel::~DAChart3DItemSettingPanel()
{
}

/**
 * @brief 获取通用属性面板
 */
DAPropertyPanelContainerWidget* DAChart3DItemSettingPanel::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 获取关联的DAChart3DWidget
 *
 * 本质是qobject_cast<DAChart3DWidget*>(getPlot3D())的便捷封装。
 * 如果当前plot不是DAChart3DWidget（如直接传入Qwt3DPlot*），返回nullptr。
 * @return 关联的DAChart3DWidget指针，或nullptr
 */
DAChart3DWidget* DAChart3DItemSettingPanel::getChart3DWidget() const
{
    return qobject_cast< DAChart3DWidget* >(getPlot3D());
}

void DAChart3DItemSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 添加3D绘图样式属性
 *
 * 创建QComboBox，填充PLOTSTYLE枚举项：
 * NOPLOT(0), WIREFRAME(1), HIDDENLINE(2), FILLED(3), FILLEDMESH(4), QWT3D_POINTS(5)
 */
void DAChart3DItemSettingPanel::addPlotStyle3DProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("No Plot"), static_cast<int>(NOPLOT));                    // cn:无绘图
    combo->addItem(tr("Wireframe"), static_cast<int>(WIREFRAME));              // cn:线框
    combo->addItem(tr("Hidden Line"), static_cast<int>(HIDDENLINE));           // cn:隐藏线
    combo->addItem(tr("Filled"), static_cast<int>(FILLED));                   // cn:填充
    combo->addItem(tr("Filled Mesh"), static_cast<int>(FILLEDMESH));          // cn:填充网格
    combo->addItem(tr("Points"), static_cast<int>(QWT3D_POINTS));              // cn:点

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加shading属性
 *
 * 创建QComboBox，填充SHADINGSTYLE枚举项：FLAT(0), GOURAUD(1)
 */
void DAChart3DItemSettingPanel::addShadingProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("Flat"), static_cast<int>(FLAT));          // cn:平面着色
    combo->addItem(tr("Gouraud"), static_cast<int>(GOURAUD));   // cn:平滑着色

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加floor投影样式属性
 *
 * 创建QComboBox，填充FLOORSTYLE枚举项：NOFLOOR(0), FLOORISO(1), FLOORDATA(2)
 */
void DAChart3DItemSettingPanel::addFloorStyleProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("No Floor"), static_cast<int>(NOFLOOR));       // cn:无投影
    combo->addItem(tr("Isoline"), static_cast<int>(FLOORISO));        // cn:等高线投影
    combo->addItem(tr("Data"), static_cast<int>(FLOORDATA));         // cn:数据投影

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加坐标系样式属性
 *
 * 创建QComboBox，填充COORDSTYLE枚举项：NOCOORD(0), BOX(1), FRAME(2)
 */
void DAChart3DItemSettingPanel::addCoordStyleProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("No Coord"), static_cast<int>(NOCOORD));       // cn:无坐标系
    combo->addItem(tr("Box"), static_cast<int>(BOX));               // cn:盒形
    combo->addItem(tr("Frame"), static_cast<int>(FRAME));           // cn:框架

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加缩放类型属性
 *
 * 创建QComboBox，填充SCALETYPE枚举项：LINEARSCALE(0), LOG10SCALE(1), USERSCALE(2)
 */
void DAChart3DItemSettingPanel::addScaleTypeProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("Linear"), static_cast<int>(LINEARSCALE));     // cn:线性
    combo->addItem(tr("Log10"), static_cast<int>(LOG10SCALE));       // cn:对数
    combo->addItem(tr("User"), static_cast<int>(USERSCALE));         // cn:自定义

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加3D柱状图样式属性
 *
 * 创建QComboBox，填充Qwt3DBar::BarStyle枚举项：
 * Filled(0), FilledMesh(1), Wireframe(2)
 */
void DAChart3DItemSettingPanel::addBarStyle3DProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("Filled"), static_cast<int>(Qwt3DBar::Filled));           // cn:填充
    combo->addItem(tr("Filled Mesh"), static_cast<int>(Qwt3DBar::FilledMesh)); // cn:填充网格
    combo->addItem(tr("Wireframe"), static_cast<int>(Qwt3DBar::Wireframe));    // cn:线框

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加3D线图样式属性
 *
 * 创建QComboBox，填充Qwt3DLine::LineStyle枚举项：
 * Lines(0), Tube(1), Dots(2)
 */
void DAChart3DItemSettingPanel::addLineStyle3DProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("Lines"), static_cast<int>(Qwt3DLine::Lines));       // cn:线条
    combo->addItem(tr("Tube"), static_cast<int>(Qwt3DLine::Tube));         // cn:管状
    combo->addItem(tr("Dots"), static_cast<int>(Qwt3DLine::Dots));         // cn:点

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加3D点形状属性
 *
 * 创建QComboBox，填充Qwt3DLine::PointShape枚举项：
 * Dot(0), Cube(1), Tetrahedron(2), Octahedron(3), Sphere(4)
 */
void DAChart3DItemSettingPanel::addPointShape3DProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("Dot"), static_cast<int>(Qwt3DLine::Dot));                     // cn:圆点
    combo->addItem(tr("Cube"), static_cast<int>(Qwt3DLine::Cube));                   // cn:立方体
    combo->addItem(tr("Tetrahedron"), static_cast<int>(Qwt3DLine::Tetrahedron));   // cn:四面体
    combo->addItem(tr("Octahedron"), static_cast<int>(Qwt3DLine::Octahedron));       // cn:八面体
    combo->addItem(tr("Sphere"), static_cast<int>(Qwt3DLine::Sphere));               // cn:球体

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加主题预设属性
 *
 * 创建QComboBox，填充Qwt3DTheme::Preset枚举项（10种预设）
 */
void DAChart3DItemSettingPanel::addThemePresetProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("Default"), static_cast<int>(Qwt3DTheme::Default));               // cn:默认
    combo->addItem(tr("Dark"), static_cast<int>(Qwt3DTheme::Dark));                     // cn:暗色
    combo->addItem(tr("Scientific"), static_cast<int>(Qwt3DTheme::Scientific));         // cn:科学
    combo->addItem(tr("Warm"), static_cast<int>(Qwt3DTheme::Warm));                     // cn:暖色
    combo->addItem(tr("Cool"), static_cast<int>(Qwt3DTheme::Cool));                     // cn:冷色
    combo->addItem(tr("Matplotlib"), static_cast<int>(Qwt3DTheme::Matplotlib));         // cn:Matplotlib
    combo->addItem(tr("Earth Tones"), static_cast<int>(Qwt3DTheme::EarthTones));         // cn:大地色
    combo->addItem(tr("Ocean"), static_cast<int>(Qwt3DTheme::Ocean));                   // cn:海洋
    combo->addItem(tr("High Contrast"), static_cast<int>(Qwt3DTheme::HighContrast));    // cn:高对比
    combo->addItem(tr("Presentation"), static_cast<int>(Qwt3DTheme::Presentation));     // cn:演示

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加光照预设属性
 *
 * 创建QComboBox，填充Qwt3DTheme::LightingPreset枚举项（5种预设）
 */
void DAChart3DItemSettingPanel::addLightingPresetProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("No Lighting"), static_cast<int>(Qwt3DTheme::NoLighting));   // cn:无光照
    combo->addItem(tr("Flat Light"), static_cast<int>(Qwt3DTheme::FlatLight));     // cn:平面光
    combo->addItem(tr("Studio"), static_cast<int>(Qwt3DTheme::Studio));           // cn:工作室
    combo->addItem(tr("Outdoor"), static_cast<int>(Qwt3DTheme::Outdoor));         // cn:户外
    combo->addItem(tr("Soft"), static_cast<int>(Qwt3DTheme::Soft));               // cn:柔和

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });

    mPanel->addProperty(id, name, combo);
}

// === 值读写方法 ===

PLOTSTYLE DAChart3DItemSettingPanel::getPlotStyle3DValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return NOPLOT;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return NOPLOT;
    }
    return static_cast< PLOTSTYLE >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setPlotStyle3DValue(int id, PLOTSTYLE style)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(style));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

SHADINGSTYLE DAChart3DItemSettingPanel::getShadingValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return FLAT;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return FLAT;
    }
    return static_cast< SHADINGSTYLE >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setShadingValue(int id, SHADINGSTYLE style)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(style));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

FLOORSTYLE DAChart3DItemSettingPanel::getFloorStyleValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return NOFLOOR;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return NOFLOOR;
    }
    return static_cast< FLOORSTYLE >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setFloorStyleValue(int id, FLOORSTYLE style)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(style));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

COORDSTYLE DAChart3DItemSettingPanel::getCoordStyleValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return BOX;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return BOX;
    }
    return static_cast< COORDSTYLE >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setCoordStyleValue(int id, COORDSTYLE style)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(style));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

SCALETYPE DAChart3DItemSettingPanel::getScaleTypeValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return LINEARSCALE;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return LINEARSCALE;
    }
    return static_cast< SCALETYPE >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setScaleTypeValue(int id, SCALETYPE type)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(type));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

Qwt3DBar::BarStyle DAChart3DItemSettingPanel::getBarStyle3DValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return Qwt3DBar::Filled;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return Qwt3DBar::Filled;
    }
    return static_cast< Qwt3DBar::BarStyle >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setBarStyle3DValue(int id, Qwt3DBar::BarStyle style)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(style));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

Qwt3DLine::LineStyle DAChart3DItemSettingPanel::getLineStyle3DValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return Qwt3DLine::Lines;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return Qwt3DLine::Lines;
    }
    return static_cast< Qwt3DLine::LineStyle >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setLineStyle3DValue(int id, Qwt3DLine::LineStyle style)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(style));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

Qwt3DLine::PointShape DAChart3DItemSettingPanel::getPointShape3DValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return Qwt3DLine::Dot;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return Qwt3DLine::Dot;
    }
    return static_cast< Qwt3DLine::PointShape >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setPointShape3DValue(int id, Qwt3DLine::PointShape shape)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(shape));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

Qwt3DTheme::Preset DAChart3DItemSettingPanel::getThemePresetValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return Qwt3DTheme::Default;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return Qwt3DTheme::Default;
    }
    return static_cast< Qwt3DTheme::Preset >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setThemePresetValue(int id, Qwt3DTheme::Preset preset)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(preset));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

Qwt3DTheme::LightingPreset DAChart3DItemSettingPanel::getLightingPresetValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return Qwt3DTheme::NoLighting;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return Qwt3DTheme::NoLighting;
    }
    return static_cast< Qwt3DTheme::LightingPreset >(combo->currentData().toInt());
}

void DAChart3DItemSettingPanel::setLightingPresetValue(int id, Qwt3DTheme::LightingPreset preset)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(preset));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

/**
 * @brief 返回常用 colormap 预设名称列表
 *
 * 供 Surface/Bar/Line 三个 item 面板的 colormap 属性共用。
 * 名称与 Qwt3DColorMapColor 构造函数接受的 presetName 一致。
 */
QStringList DAChart3DItemSettingPanel::colormapPresetNames()
{
    return QStringList() << "viridis"
                         << "plasma"
                         << "inferno"
                         << "magma"
                         << "cividis"
                         << "jet"
                         << "cool"
                         << "hot"
                         << "hsv"
                         << "spring"
                         << "summer"
                         << "autumn"
                         << "winter"
                         << "gray"
                         << "bone"
                         << "copper"
                         << "pink"
                         << "seismic"
                         << "turbo"
                         << "terrain"
                         << "ocean"
                         << "gist_rainbow";
}

}  // namespace DA
