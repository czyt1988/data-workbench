#include "DAChartScaleSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_scale_draw.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartScaleSettingPanel::DAChartScaleSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartScaleSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartScaleSettingPanel::~DAChartScaleSettingPanel()
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
 * - Alignment: 枚举属性(Bottom/Top/Left/Right)
 * - Position: 双精度属性
 * - BorderDistance: 整数属性(-1表示居中)
 * - Font: 字体属性
 * - ScaleDivFromAxis: 布尔属性
 */
void DAChartScaleSettingPanel::buildPropertyPanel()
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

    // 刻度属性组
    panel->addCollapsibleGroup(tr("Scale")  // cn:刻度
    );
    // QwtScaleDraw::Alignment: BottomScale=0, TopScale=1, LeftScale=2, RightScale=3
    panel->addEnumProperty(PropAlignment,
                           tr("Alignment")  // cn:对齐方式
                           ,
                           QStringList() << tr("Bottom")  // cn:底部
                                         << tr("Top")  // cn:顶部
                                         << tr("Left")  // cn:左侧
                                         << tr("Right")  // cn:右侧
                           ,
                           QList< int >() << static_cast< int >(QwtScaleDraw::BottomScale)
                                          << static_cast< int >(QwtScaleDraw::TopScale)
                                          << static_cast< int >(QwtScaleDraw::LeftScale)
                                          << static_cast< int >(QwtScaleDraw::RightScale));
    panel->addDoubleProperty(PropPosition, tr("Position")  // cn:位置
    );
    panel->addIntProperty(PropBorderDistance, tr("Border Distance")  // cn:边框距离
                          ,
                          -1, -1, 9999);
    panel->addBoolProperty(PropScaleDivFromAxis, tr("Sync Scale From Axis")  // cn:同步坐标轴刻度
    );
    panel->endGroup();

    // 字体属性组
    panel->addCollapsibleGroup(tr("Font")  // cn:字体
    );
    panel->addFontProperty(PropFont, tr("Font")  // cn:字体
    );
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartScaleSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotScale) {
        return;
    }

    QwtPlotScaleItem* scaleItem = static_cast< QwtPlotScaleItem* >(item);
    auto panel                  = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, scaleItem->title().text());
    panel->setDoubleValue(PropZValue, scaleItem->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(scaleItem->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(scaleItem->yAxis()));

    // 刻度属性
    // Alignment 需通过 scaleDraw()->alignment() 读取，setEnumValue 优先按 data 值匹配
    panel->setEnumValue(PropAlignment, static_cast< int >(scaleItem->scaleDraw()->alignment()));
    panel->setDoubleValue(PropPosition, scaleItem->position());
    panel->setIntValue(PropBorderDistance, scaleItem->borderDistance());
    panel->setBoolValue(PropScaleDivFromAxis, scaleItem->isScaleDivFromAxis());

    // 字体属性
    panel->setFontValue(PropFont, scaleItem->font());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartScaleSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotScaleItem* scaleItem = s_cast< QwtPlotScaleItem* >();
    if (nullptr == scaleItem) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        scaleItem->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        scaleItem->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        scaleItem->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        scaleItem->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropAlignment: {
        int alignVal = panel->getEnumValue(PropAlignment);
        scaleItem->setAlignment(static_cast< QwtScaleDraw::Alignment >(alignVal));
        break;
    }
    case PropPosition:
        scaleItem->setPosition(panel->getDoubleValue(PropPosition));
        break;
    case PropBorderDistance:
        scaleItem->setBorderDistance(panel->getIntValue(PropBorderDistance));
        break;
    case PropFont:
        scaleItem->setFont(panel->getFontValue(PropFont));
        break;
    case PropScaleDivFromAxis:
        scaleItem->setScaleDivFromAxis(panel->getBoolValue(PropScaleDivFromAxis));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
