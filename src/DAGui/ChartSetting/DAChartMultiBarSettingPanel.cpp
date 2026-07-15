#include "DAChartMultiBarSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_plot_abstract_barchart.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartMultiBarSettingPanel::DAChartMultiBarSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartMultiBarSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartMultiBarSettingPanel::~DAChartMultiBarSettingPanel()
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
 * - ChartStyle: 枚举属性(Grouped/Stacked)
 * - LayoutPolicy: 枚举属性
 * - LayoutHint: 双精度属性
 * - Spacing/Margin: 整数属性
 * - Baseline: 双精度属性
 */
void DAChartMultiBarSettingPanel::buildPropertyPanel()
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

    // 样式属性组
    panel->addCollapsibleGroup(tr("Style")  // cn:样式
    );
    addOrientationProperty(PropOrientation, tr("Orientation")  // cn:方向
    );
    // QwtPlotMultiBarChart::ChartStyle: Grouped=0, Stacked=1
    panel->addEnumProperty(PropChartStyle,
                           tr("Chart Style")  // cn:图表样式
                           ,
                           QStringList() << tr("Grouped")  // cn:分组
                                         << tr("Stacked")  // cn:堆叠
                           ,
                           QList< int >() << static_cast< int >(QwtPlotMultiBarChart::Grouped)
                                          << static_cast< int >(QwtPlotMultiBarChart::Stacked));
    panel->addDoubleProperty(PropBaseline, tr("Baseline")  // cn:基线
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->endGroup();

    // 布局属性组
    panel->addCollapsibleGroup(tr("Layout")  // cn:布局
    );
    // QwtPlotAbstractBarChart::LayoutPolicy
    panel->addEnumProperty(PropLayoutPolicy,
                           tr("Layout Policy")  // cn:布局策略
                           ,
                           QStringList() << tr("Auto Adjust")  // cn:自动调整
                                         << tr("Scale To Axes")  // cn:缩放到坐标轴
                                         << tr("Scale To Canvas")  // cn:缩放到画布
                                         << tr("Fixed Sample Size")  // cn:固定采样尺寸
                           ,
                           QList< int >() << static_cast< int >(QwtPlotAbstractBarChart::AutoAdjustSamples)
                                          << static_cast< int >(QwtPlotAbstractBarChart::ScaleSamplesToAxes)
                                          << static_cast< int >(QwtPlotAbstractBarChart::ScaleSampleToCanvas)
                                          << static_cast< int >(QwtPlotAbstractBarChart::FixedSampleSize));
    panel->addDoubleProperty(PropLayoutHint, tr("Layout Hint")  // cn:布局提示
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addIntProperty(PropSpacing, tr("Spacing")  // cn:间距
                          ,
                          0, 0, 1000);
    panel->addIntProperty(PropMargin, tr("Margin")  // cn:边距
                          ,
                          0, 0, 1000);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartMultiBarSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotMultiBarChart) {
        return;
    }

    QwtPlotMultiBarChart* barChart = static_cast< QwtPlotMultiBarChart* >(item);
    auto panel                     = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, barChart->title().text());
    panel->setDoubleValue(PropZValue, barChart->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(barChart->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(barChart->yAxis()));

    // 样式属性
    setOrientationValue(PropOrientation, barChart->orientation());
    panel->setEnumValue(PropChartStyle, static_cast< int >(barChart->style()));
    panel->setDoubleValue(PropBaseline, barChart->baseline());

    // 布局属性
    panel->setEnumValue(PropLayoutPolicy, static_cast< int >(barChart->layoutPolicy()));
    panel->setDoubleValue(PropLayoutHint, barChart->layoutHint());
    panel->setIntValue(PropSpacing, barChart->spacing());
    panel->setIntValue(PropMargin, barChart->margin());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartMultiBarSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotMultiBarChart* barChart = s_cast< QwtPlotMultiBarChart* >();
    if (nullptr == barChart) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        barChart->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        barChart->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        barChart->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        barChart->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropOrientation:
        barChart->setOrientation(getOrientationValue(PropOrientation));
        break;
    case PropChartStyle: {
        int styleVal = panel->getEnumValue(PropChartStyle);
        barChart->setStyle(static_cast< QwtPlotMultiBarChart::ChartStyle >(styleVal));
        break;
    }
    case PropLayoutPolicy: {
        int policyVal = panel->getEnumValue(PropLayoutPolicy);
        barChart->setLayoutPolicy(static_cast< QwtPlotAbstractBarChart::LayoutPolicy >(policyVal));
        break;
    }
    case PropLayoutHint:
        barChart->setLayoutHint(panel->getDoubleValue(PropLayoutHint));
        break;
    case PropSpacing:
        barChart->setSpacing(panel->getIntValue(PropSpacing));
        break;
    case PropMargin:
        barChart->setMargin(panel->getIntValue(PropMargin));
        break;
    case PropBaseline:
        barChart->setBaseline(panel->getDoubleValue(PropBaseline));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
