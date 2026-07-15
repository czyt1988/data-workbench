#include "DAChartGraphicSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartGraphicSettingPanel::DAChartGraphicSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartGraphicSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartGraphicSettingPanel::~DAChartGraphicSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - XAxis: 坐标轴属性(XBottom/XTop)
 * - YAxis: 坐标轴属性(YLeft/YRight)
 * - RenderAntialiased: 布尔属性
 */
void DAChartGraphicSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    // 基础属性组
    panel->addCollapsibleGroup(tr("Basic")  // cn:基础
    );
    panel->addStringProperty(PropTitle, tr("Title")  // cn:标题
    );
    panel->addDoubleProperty(PropZValue, tr("Z Value")  // cn:Z值
    );
    addAxisProperty(PropXAxis, tr("X Axis")  // cn:X轴
                    ,
                    false);
    addAxisProperty(PropYAxis, tr("Y Axis")  // cn:Y轴
                    ,
                    true);
    panel->endGroup();

    // 渲染属性组
    panel->addCollapsibleGroup(tr("Render")  // cn:渲染
    );
    panel->addBoolProperty(PropRenderAntialiased, tr("Antialiased")  // cn:抗锯齿
    );
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartGraphicSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotGraphic) {
        return;
    }

    QwtPlotGraphicItem* graphic = static_cast< QwtPlotGraphicItem* >(item);
    auto panel                  = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, graphic->title().text());
    panel->setDoubleValue(PropZValue, graphic->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(graphic->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(graphic->yAxis()));

    // 渲染属性
    panel->setBoolValue(PropRenderAntialiased, graphic->testRenderHint(QwtPlotItem::RenderAntialiased));
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartGraphicSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotGraphicItem* graphic = s_cast< QwtPlotGraphicItem* >();
    if (nullptr == graphic) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        graphic->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        graphic->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        graphic->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        graphic->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropRenderAntialiased:
        graphic->setRenderHint(QwtPlotItem::RenderAntialiased, panel->getBoolValue(PropRenderAntialiased));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
