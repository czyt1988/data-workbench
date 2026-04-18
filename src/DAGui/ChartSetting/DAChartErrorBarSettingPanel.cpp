#include "DAChartErrorBarSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_interval_symbol.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartErrorBarSettingPanel::DAChartErrorBarSettingPanel(QWidget* parent)
    : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &DAChartErrorBarSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartErrorBarSettingPanel::~DAChartErrorBarSettingPanel()
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
 * - EnableErrorBar: 布尔属性
 * - ErrorBarStyle: 枚举属性(Bar/Stick)
 * - ErrorBarPen: 笔属性（依赖EnableErrorBar）
 * - EnableFill: 布尔属性
 * - FillBrush: 画刷属性（依赖EnableFill）
 * - Orientation: 方向属性(Horizontal/Vertical)
 * - CurvePen: 笔属性
 */
void DAChartErrorBarSettingPanel::buildPropertyPanel()
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

    // 误差棒属性组
    panel->addGroupLabel(tr("Error Bar"));
    panel->addBoolProperty(PropEnableErrorBar, tr("Enable Error Bar"));
    // QwtIntervalSymbol样式：Bar, Box
    panel->addEnumProperty(PropErrorBarStyle, tr("Error Bar Style"),
                           QStringList() << tr("Bar") << tr("Box"),
                           QList< int >() << static_cast< int >(QwtIntervalSymbol::Bar)
                                               << static_cast< int >(QwtIntervalSymbol::Box));
    panel->addPenProperty(PropErrorBarPen, tr("Error Bar Pen"));
    panel->setPropertyEnabled(PropErrorBarStyle, false);
    panel->setPropertyEnabled(PropErrorBarPen, false);

    // 填充属性组
    panel->addGroupLabel(tr("Fill"));
    panel->addBoolProperty(PropEnableFill, tr("Enable Fill"));
    panel->addBrushProperty(PropFillBrush, tr("Fill Brush"));
    panel->setPropertyEnabled(PropFillBrush, false);

    // 曲线属性组
    panel->addGroupLabel(tr("Curve"));
    addOrientationProperty(PropOrientation, tr("Orientation"));
    panel->addPenProperty(PropCurvePen, tr("Curve Pen"));
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartErrorBarSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotIntervalCurve) {
        return;
    }

    QwtPlotIntervalCurve* curve = static_cast< QwtPlotIntervalCurve* >(item);
    auto panel = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, curve->title().text());
    panel->setDoubleValue(PropZValue, curve->z());

    // 坐标轴属性
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(curve->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(curve->yAxis()));

    // 误差棒属性
    bool hasErrorBar = (curve->symbol() != nullptr);
    panel->setBoolValue(PropEnableErrorBar, hasErrorBar);
    panel->setPropertyEnabled(PropErrorBarStyle, hasErrorBar);
    panel->setPropertyEnabled(PropErrorBarPen, hasErrorBar);
    if (hasErrorBar && curve->symbol()) {
        panel->setEnumValue(PropErrorBarStyle, static_cast< int >(curve->symbol()->style()));
        panel->setPenValue(PropErrorBarPen, curve->symbol()->pen());
    }

    // 填充属性
    QBrush brush = curve->brush();
    bool hasFill = (brush != Qt::NoBrush);
    panel->setBoolValue(PropEnableFill, hasFill);
    panel->setPropertyEnabled(PropFillBrush, hasFill);
    if (hasFill) {
        panel->setBrushValue(PropFillBrush, brush);
    }

    // 曲线属性
    setOrientationValue(PropOrientation, curve->orientation());
    panel->setPenValue(PropCurvePen, curve->pen());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartErrorBarSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotIntervalCurve* curve = s_cast< QwtPlotIntervalCurve* >();
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
    case PropEnableErrorBar: {
        bool enabled = panel->getBoolValue(PropEnableErrorBar);
        panel->setPropertyEnabled(PropErrorBarStyle, enabled);
        panel->setPropertyEnabled(PropErrorBarPen, enabled);
        if (enabled) {
            QwtIntervalSymbol::Style style = static_cast< QwtIntervalSymbol::Style >(
                panel->getEnumValue(PropErrorBarStyle));
            QwtIntervalSymbol* symbol = new QwtIntervalSymbol(style);
            symbol->setPen(panel->getPenValue(PropErrorBarPen));
            curve->setSymbol(symbol);
        } else {
            curve->setSymbol(nullptr);
        }
        break;
    }
    case PropErrorBarStyle:
    case PropErrorBarPen: {
        if (curve->symbol()) {
            // QwtIntervalSymbol* symbol = curve->symbol(); returns const, so recreate
            QwtIntervalSymbol::Style style = static_cast< QwtIntervalSymbol::Style >(
                panel->getEnumValue(PropErrorBarStyle));
            QwtIntervalSymbol* newSymbol = new QwtIntervalSymbol(style);
            newSymbol->setPen(panel->getPenValue(PropErrorBarPen));
            newSymbol->setWidth(curve->symbol()->width());
            newSymbol->setBrush(curve->symbol()->brush());
            curve->setSymbol(newSymbol);
        }
        break;
    }
    case PropEnableFill: {
        bool enabled = panel->getBoolValue(PropEnableFill);
        panel->setPropertyEnabled(PropFillBrush, enabled);
        if (enabled) {
            curve->setBrush(panel->getBrushValue(PropFillBrush));
        } else {
            curve->setBrush(Qt::NoBrush);
        }
        break;
    }
    case PropFillBrush:
        curve->setBrush(panel->getBrushValue(PropFillBrush));
        break;
    case PropOrientation:
        curve->setOrientation(getOrientationValue(PropOrientation));
        break;
    case PropCurvePen:
        curve->setPen(panel->getPenValue(PropCurvePen));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
