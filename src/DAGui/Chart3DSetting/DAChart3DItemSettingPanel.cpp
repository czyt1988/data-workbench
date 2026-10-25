#include "DAChart3DItemSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAChart3DWidget.h"
#include <QVBoxLayout>

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
