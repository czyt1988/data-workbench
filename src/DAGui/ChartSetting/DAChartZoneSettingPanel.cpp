#include "DAChartZoneSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_interval.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartZoneSettingPanel::DAChartZoneSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartZoneSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartZoneSettingPanel::~DAChartZoneSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - XAxis/YAxis: 坐标轴属性
 * - Orientation: 方向属性
 * - IntervalMin/Max: 双精度属性(宽范围,任意坐标值)
 * - Pen: 笔属性
 * - Brush: 画刷属性
 */
void DAChartZoneSettingPanel::buildPropertyPanel()
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

    // 区间属性组
    panel->addCollapsibleGroup(tr("Zone")  // cn:区间
    );
    addOrientationProperty(PropOrientation, tr("Orientation")  // cn:方向
    );
    panel->addDoubleProperty(PropIntervalMin, tr("Interval Min")  // cn:区间最小值
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addDoubleProperty(PropIntervalMax, tr("Interval Max")  // cn:区间最大值
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->endGroup();

    // 外观属性组
    panel->addCollapsibleGroup(tr("Appearance")  // cn:外观
    );
    panel->addPenProperty(PropPen, tr("Pen")  // cn:画笔
    );
    panel->addBrushProperty(PropBrush, tr("Brush")  // cn:画刷
    );
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartZoneSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotZone) {
        return;
    }

    QwtPlotZoneItem* zone = static_cast< QwtPlotZoneItem* >(item);
    auto panel            = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, zone->title().text());
    panel->setDoubleValue(PropZValue, zone->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(zone->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(zone->yAxis()));

    // 区间属性
    setOrientationValue(PropOrientation, zone->orientation());
    const QwtInterval& itv = zone->interval();
    panel->setDoubleValue(PropIntervalMin, itv.minValue());
    panel->setDoubleValue(PropIntervalMax, itv.maxValue());

    // 外观属性
    panel->setPenValue(PropPen, zone->pen());
    panel->setBrushValue(PropBrush, zone->brush());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartZoneSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotZoneItem* zone = s_cast< QwtPlotZoneItem* >();
    if (nullptr == zone) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        zone->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        zone->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        zone->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        zone->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropOrientation:
        zone->setOrientation(getOrientationValue(PropOrientation));
        break;
    case PropIntervalMin: {
        // 区间min/max联动:获取当前区间,仅替换min,整体写回
        QwtInterval itv = zone->interval();
        itv.setMinValue(panel->getDoubleValue(PropIntervalMin));
        zone->setInterval(itv);
        break;
    }
    case PropIntervalMax: {
        QwtInterval itv = zone->interval();
        itv.setMaxValue(panel->getDoubleValue(PropIntervalMax));
        zone->setInterval(itv);
        break;
    }
    case PropPen:
        zone->setPen(panel->getPenValue(PropPen));
        break;
    case PropBrush:
        zone->setBrush(panel->getBrushValue(PropBrush));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
