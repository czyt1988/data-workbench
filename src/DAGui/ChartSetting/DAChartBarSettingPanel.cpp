#include "DAChartBarSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include <QSignalBlocker>
#include "qwt_text.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartBarSettingPanel::DAChartBarSettingPanel(QWidget* parent)
    : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &DAChartBarSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartBarSettingPanel::~DAChartBarSettingPanel()
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
 * - LegendMode: 枚举属性(Chart/Bar)
 * - EnableFill: 布尔属性
 * - FillBrush: 画刷属性（依赖EnableFill）
 * - EnableEdge: 布尔属性
 * - EdgePen: 笔属性（依赖EnableEdge）
 * - Baseline: 字符串属性（可为空）
 * - LayoutPolicy: 枚举属性
 * - LayoutHint: 双精度属性
 * - Spacing: 整数属性
 * - Margin: 整数属性
 */
void DAChartBarSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    // 基础属性组
    panel->addGroupLabel(tr("Basic"));
    panel->addStringProperty(PropTitle, tr("Title"));
    panel->addDoubleProperty(PropZValue, tr("Z Value"));

    // 坐标轴属性组
    panel->addGroupLabel(tr("Axis"));
    addAxisProperty(PropXAxis, tr("X Axis"), false);
    addAxisProperty(PropYAxis, tr("Y Axis"), true);

    // 图例属性组
    panel->addGroupLabel(tr("Legend"));
    // QwtPlotBarChart::LegendMode: ChartMode=0, BarMode=1
    panel->addEnumProperty(PropLegendMode, tr("Legend Mode"),
                           QStringList() << tr("Chart Mode") << tr("Bar Mode"),
                           QList< int >() << 0 << 1);

    // 填充属性组
    panel->addGroupLabel(tr("Fill"));
    panel->addBoolProperty(PropEnableFill, tr("Enable Fill"));
    panel->addBrushProperty(PropFillBrush, tr("Fill Brush"));
    propertyPanel()->setPropertyEnabled(PropFillBrush, false);

    // 边框属性组
    panel->addGroupLabel(tr("Edge"));
    panel->addBoolProperty(PropEnableEdge, tr("Enable Edge"));
    panel->addPenProperty(PropEdgePen, tr("Edge Pen"));
    propertyPanel()->setPropertyEnabled(PropEdgePen, false);

    // 基线属性组
    panel->addGroupLabel(tr("Baseline"));
    panel->addStringProperty(PropBaseline, tr("Baseline"));

    // 布局属性组
    panel->addGroupLabel(tr("Layout"));
    // LayoutPolicy枚举：AutoAdjustSamples=0, ScaleSamplesToAxes=1, ScaleSampleToCanvas=2, FixedSampleSize=3
    panel->addEnumProperty(PropLayoutPolicy, tr("Layout Policy"),
                           QStringList() << tr("Auto Adjust Samples") << tr("Scale Samples To Axes")
                                         << tr("Scale Sample To Canvas") << tr("Fixed Sample Size"),
                           QList< int >() << static_cast< int >(QwtPlotAbstractBarChart::AutoAdjustSamples)
                                               << static_cast< int >(QwtPlotAbstractBarChart::ScaleSamplesToAxes)
                                               << static_cast< int >(QwtPlotAbstractBarChart::ScaleSampleToCanvas)
                                               << static_cast< int >(QwtPlotAbstractBarChart::FixedSampleSize));
    panel->addDoubleProperty(PropLayoutHint, tr("Layout Hint"));
    panel->addIntProperty(PropSpacing, tr("Spacing"), 0, 0, 1000);
    panel->addIntProperty(PropMargin, tr("Margin"), 0, 0, 1000);
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartBarSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotBarChart) {
        return;
    }

    QwtPlotBarChart* barChart = static_cast< QwtPlotBarChart* >(item);
    auto panel = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, barChart->title().text());
    panel->setDoubleValue(PropZValue, barChart->z());

    // 坐标轴属性
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(barChart->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(barChart->yAxis()));

    // 图例模式
    QwtPlotBarChart::LegendMode mode = barChart->legendMode();
    int legendModeIndex = (mode & QwtPlotBarChart::LegendBarTitles) ? 1 : 0;
    panel->setEnumValue(PropLegendMode, legendModeIndex);

    // 填充属性
    QBrush brush = barChart->brush();
    bool hasFill = (brush != Qt::NoBrush);
    panel->setBoolValue(PropEnableFill, hasFill);
    if (hasFill) {
        panel->setBrushValue(PropFillBrush, brush);
    }
    panel->setPropertyEnabled(PropFillBrush, hasFill);

    // 边框属性
    QPen pen = barChart->pen();
    bool hasEdge = (pen.style() != Qt::NoPen);
    panel->setBoolValue(PropEnableEdge, hasEdge);
    if (hasEdge) {
        panel->setPenValue(PropEdgePen, pen);
    }
    panel->setPropertyEnabled(PropEdgePen, hasEdge);

    // 基线属性
    panel->setStringValue(PropBaseline, QString::number(barChart->baseline()));

    // 布局属性
    QwtPlotAbstractBarChart::LayoutPolicy policy = barChart->layoutPolicy();
    panel->setEnumValue(PropLayoutPolicy, static_cast< int >(policy));
    panel->setDoubleValue(PropLayoutHint, barChart->layoutHint());
    panel->setIntValue(PropSpacing, barChart->spacing());
    panel->setIntValue(PropMargin, barChart->margin());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartBarSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotBarChart* barChart = s_cast< QwtPlotBarChart* >();
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
    case PropLegendMode: {
        int lm = panel->getEnumValue(PropLegendMode);
        barChart->setLegendMode(static_cast< QwtPlotBarChart::LegendMode >(lm));
        break;
    }
    case PropEnableFill: {
        bool enabled = panel->getBoolValue(PropEnableFill);
        panel->setPropertyEnabled(PropFillBrush, enabled);
        if (enabled) {
            barChart->setBrush(panel->getBrushValue(PropFillBrush));
        } else {
            barChart->setBrush(Qt::NoBrush);
        }
        break;
    }
    case PropFillBrush:
        barChart->setBrush(panel->getBrushValue(PropFillBrush));
        break;
    case PropEnableEdge: {
        bool enabled = panel->getBoolValue(PropEnableEdge);
        panel->setPropertyEnabled(PropEdgePen, enabled);
        if (enabled) {
            barChart->setPen(panel->getPenValue(PropEdgePen));
        } else {
            barChart->setPen(Qt::NoPen);
        }
        break;
    }
    case PropEdgePen:
        barChart->setPen(panel->getPenValue(PropEdgePen));
        break;
    case PropBaseline: {
        QString text = panel->getStringValue(PropBaseline);
        bool ok = false;
        double val = text.toDouble(&ok);
        if (ok) {
            barChart->setBaseline(val);
        }
        break;
    }
    case PropLayoutPolicy: {
        QwtPlotAbstractBarChart::LayoutPolicy policy
            = static_cast< QwtPlotAbstractBarChart::LayoutPolicy >(panel->getEnumValue(PropLayoutPolicy));
        barChart->setLayoutPolicy(policy);
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
    default:
        break;
    }

    replot();
}

}  // end namespace DA
