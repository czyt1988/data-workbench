#include "Chart/Setting3D/DAChart3DGenericItemSettingPanel.h"
#include "Common/Settings/DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include "qwt3d_plotitem.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChart3DGenericItemSettingPanel::DAChart3DGenericItemSettingPanel(QWidget* parent)
    : DAChart3DItemSettingPanel(parent)
{
    // 连接属性值变化信号
    connect(this,
            &DAChart3DItemSettingPanel::propertyValueChanged,
            this,
            &DAChart3DGenericItemSettingPanel::onGenericPropertyValueChanged);
    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChart3DGenericItemSettingPanel::~DAChart3DGenericItemSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 只包含所有 Qwt3DPlotItem 基类安全可读写的通用属性（对任意 3D 图元有效，
 * 包括未注册 rtti 的类型如 Qwt3DWaterfall 与插件自定义 3D 图元）：
 * - Title: 标题字符串
 * - Visible: 图元可见性
 * - Z Value: 绘制顺序（qwt3d 按 z 从小到大绘制）
 */
void DAChart3DGenericItemSettingPanel::buildPropertyPanel()
{
    DAPropertyPanelContainerWidget* pp = propertyPanel();

    pp->addCollapsibleGroup(tr("Basic")  // cn:基础
    );
    pp->addStringProperty(PropTitle, tr("Title")  // cn:标题
    );
    pp->addBoolProperty(PropVisible, tr("Visible")  // cn:可见
                         ,
                         true);
    // Z 值允许为负，范围给足避免被 spin box 夹断（与 2D 通用面板口径一致）
    pp->addDoubleProperty(PropZ, tr("Z Value")  // cn:Z值
                           ,
                           0.0,
                           -1.0e6,
                           1.0e6);
    pp->endGroup();
}

/**
 * @brief 从任意 3D 图元读取通用属性写入面板
 *
 * 不校验 rtti（通用面板对任意 3D 图元都安全），只做空指针防御。
 * @param item 3D图元
 */
void DAChart3DGenericItemSettingPanel::updateUI(Qwt3DPlotItem* item)
{
    DAChart3DItemSettingPanel::updateUI(item);
    if (nullptr == item) {
        return;
    }

    DAPropertyPanelContainerWidget* pp = propertyPanel();
    QSignalBlocker blocker(pp);

    pp->setStringValue(PropTitle, item->title());
    pp->setBoolValue(PropVisible, item->isVisible());
    pp->setDoubleValue(PropZ, item->z());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChart3DGenericItemSettingPanel::onGenericPropertyValueChanged(int propertyId)
{
    DAAbstractChart3DItemSettingWidget_ReturnWhenItemNull;
    Qwt3DPlotItem* item = getPlot3DItem();
    if (nullptr == item) {
        return;
    }

    DAPropertyPanelContainerWidget* pp = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        item->setTitle(pp->getStringValue(PropTitle));
        break;
    case PropVisible:
        item->setVisible(pp->getBoolValue(PropVisible));
        break;
    case PropZ:
        item->setZ(pp->getDoubleValue(PropZ));
        break;
    default:
        break;
    }

    replot();
}

}  // namespace DA
