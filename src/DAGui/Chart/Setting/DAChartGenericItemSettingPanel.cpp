#include "Chart/Setting/DAChartGenericItemSettingPanel.h"
#include "Common/Settings/DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include "qwt_text.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartGenericItemSettingPanel::DAChartGenericItemSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    // 连接属性值变化信号
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartGenericItemSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartGenericItemSettingPanel::~DAChartGenericItemSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 只包含所有 QwtPlotItem 通用的基础属性：
 * - Title: 标题字符串
 * - LegendVisible: 图例是否可见（QwtPlotItem::Legend 属性）
 * - Visible: 图元可见性
 * - ZValue: 堆叠顺序
 */
void DAChartGenericItemSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addCollapsibleGroup(tr("Basic")  // cn:基础
    );
    panel->addStringProperty(PropTitle, tr("Title")  // cn:标题
    );
    panel->addBoolProperty(PropLegend, tr("Legend Visible")  // cn:图例可见
                           ,
                           false);
    panel->addBoolProperty(PropVisible, tr("Visible")  // cn:可见
                           ,
                           true);
    // Z 值允许为负（qwt 默认按 z 从小到大绘制），范围给足避免被 spin box 夹断
    panel->addDoubleProperty(PropZ, tr("Z Value")  // cn:Z值
                             ,
                             0.0,
                             -1.0e6,
                             1.0e6);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartGenericItemSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(propertyPanel());

    propertyPanel()->setStringValue(PropTitle, item->title().text());
    propertyPanel()->setBoolValue(PropLegend, item->testItemAttribute(QwtPlotItem::Legend));
    propertyPanel()->setBoolValue(PropVisible, item->isVisible());
    propertyPanel()->setDoubleValue(PropZ, item->z());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartGenericItemSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotItem* item = getPlotItem();
    if (nullptr == item) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        item->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropLegend:
        item->setItemAttribute(QwtPlotItem::Legend, panel->getBoolValue(PropLegend));
        break;
    case PropVisible:
        item->setVisible(panel->getBoolValue(PropVisible));
        break;
    case PropZ:
        item->setZ(panel->getDoubleValue(PropZ));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
