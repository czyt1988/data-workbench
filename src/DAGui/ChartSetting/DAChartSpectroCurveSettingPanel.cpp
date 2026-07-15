#include "DAChartSpectroCurveSettingPanel.h"
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
DAChartSpectroCurveSettingPanel::DAChartSpectroCurveSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartSpectroCurveSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartSpectroCurveSettingPanel::~DAChartSpectroCurveSettingPanel()
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
 * - Orientation: 方向属性(Horizontal/Vertical)
 * - PenWidth: 双精度属性(0.0-10.0)
 * - ColorRangeMin/Max: 双精度属性(颜色映射范围)
 * - ClipPoints: 布尔属性
 */
void DAChartSpectroCurveSettingPanel::buildPropertyPanel()
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

    // 绘制属性组
    panel->addCollapsibleGroup(tr("Drawing")  // cn:绘制
    );
    addOrientationProperty(PropOrientation, tr("Orientation")  // cn:方向
    );
    panel->addDoubleProperty(PropPenWidth, tr("Pen Width")  // cn:笔宽
                             ,
                             0.0, 0.0, 10.0, 3);
    panel->addBoolProperty(PropClipPoints, tr("Clip Points")  // cn:裁剪点
    );
    panel->endGroup();

    // 颜色映射属性组
    panel->addCollapsibleGroup(tr("Color Map")  // cn:颜色映射
    );
    panel->addDoubleProperty(PropColorRangeMin, tr("Color Range Min")  // cn:颜色范围最小值
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addDoubleProperty(PropColorRangeMax, tr("Color Range Max")  // cn:颜色范围最大值
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartSpectroCurveSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotSpectroCurve) {
        return;
    }

    QwtPlotSpectroCurve* curve = static_cast< QwtPlotSpectroCurve* >(item);
    auto panel               = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, curve->title().text());
    panel->setDoubleValue(PropZValue, curve->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(curve->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(curve->yAxis()));

    // 绘制属性
    setOrientationValue(PropOrientation, curve->orientation());
    panel->setDoubleValue(PropPenWidth, curve->penWidth());
    panel->setBoolValue(PropClipPoints, curve->testPaintAttribute(QwtPlotSpectroCurve::ClipPoints));

    // 颜色映射范围
    const QwtInterval& range = curve->colorRange();
    panel->setDoubleValue(PropColorRangeMin, range.minValue());
    panel->setDoubleValue(PropColorRangeMax, range.maxValue());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartSpectroCurveSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotSpectroCurve* curve = s_cast< QwtPlotSpectroCurve* >();
    if (nullptr == curve) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        curve->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        curve->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        curve->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        curve->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropOrientation:
        curve->setOrientation(getOrientationValue(PropOrientation));
        break;
    case PropPenWidth:
        curve->setPenWidth(panel->getDoubleValue(PropPenWidth));
        break;
    case PropColorRangeMin:
    case PropColorRangeMax: {
        // 改变 min 或 max 时都需重新构造完整区间
        double minVal = panel->getDoubleValue(PropColorRangeMin);
        double maxVal = panel->getDoubleValue(PropColorRangeMax);
        curve->setColorRange(QwtInterval(minVal, maxVal));
        break;
    }
    case PropClipPoints:
        curve->setPaintAttribute(QwtPlotSpectroCurve::ClipPoints, panel->getBoolValue(PropClipPoints));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
