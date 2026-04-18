#include "DAChartCurveSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include <QPen>
#include "qwt_plot_curve.h"
#include "qwt_text.h"

namespace DA
{

/**
 * @brief 构造函数
 *
 * 在基类初始化后调用buildPropertyPanel()构建面板。
 *
 * @param parent 父控件
 */
DAChartCurveSettingPanel::DAChartCurveSettingPanel(QWidget* parent)
    : DAChartItemSettingPanel(parent)
{
    // 连接属性值变化信号到曲线属性处理槽
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &DAChartCurveSettingPanel::onCurvePropertyValueChanged);

    // 构建属性面板
    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartCurveSettingPanel::~DAChartCurveSettingPanel()
{
}

/**
 * @brief 构建属性面板布局
 *
 * 按照以下分区组织属性：
 * General (通用属性) -> Curve Style -> Pen -> Marker -> Attributes -> Legend -> Fill -> Baseline -> Orientation
 */
void DAChartCurveSettingPanel::buildPropertyPanel()
{
    DAPropertyPanelContainerWidget* pp = propertyPanel();

    // ── 通用属性 ──
    pp->addGroupLabel(tr("General"));
    pp->addStringProperty(PID_Title, tr("Title"));
    pp->addDoubleProperty(PID_ZValue, tr("Z Value"), 0.0, -9999.0, 9999.0, 1);
    addAxisProperty(PID_XAxis, tr("X Axis"), false);
    addAxisProperty(PID_YAxis, tr("Y Axis"), true);

    // ── 曲线样式 ──
    pp->addGroupLabel(tr("Curve Style"));
    addCurveStyleProperty(PID_CurveStyle, tr("Style"));

    // ── 画笔 ──
    pp->addGroupLabel(tr("Pen"));
    pp->addPenProperty(PID_Pen, tr("Pen"));

    // ── 标记 ──
    pp->addGroupLabel(tr("Marker"));
    pp->addBoolProperty(PID_EnableMarker, tr("Enable Marker"));
    addSymbolProperty(PID_Symbol, tr("Symbol"));

    // ── 属性 ──
    pp->addGroupLabel(tr("Attributes"));
    pp->addBoolProperty(PID_Fitted, tr("Fitted"));
    pp->addBoolProperty(PID_Inverted, tr("Inverted"));

    // ── 图例 ──
    pp->addGroupLabel(tr("Legend"));
    pp->addBoolProperty(PID_LegendShowLine, tr("Show Line"));
    pp->addBoolProperty(PID_LegendShowSymbol, tr("Show Symbol"));
    pp->addBoolProperty(PID_LegendShowBrush, tr("Show Brush"));

    // ── 填充 ──
    pp->addGroupLabel(tr("Fill"));
    pp->addBoolProperty(PID_EnableFill, tr("Enable Fill"));
    pp->addBrushProperty(PID_Fill, tr("Fill Brush"));

    // ── 基线 ──
    pp->addGroupLabel(tr("Baseline"));
    pp->addStringProperty(PID_BaseLine, tr("Baseline"));

    // ── 方向 ──
    pp->addGroupLabel(tr("Orientation"));
    addOrientationProperty(PID_Orientation, tr("Orientation"));
}

/**
 * @brief 根据QwtPlotCurve更新UI
 *
 * 使用QSignalBlocker全局block面板，防止逐项设置时触发信号，
 * 然后逐项从QwtPlotCurve读取当前状态写入面板。
 *
 * @param item 图表项指针
 */
void DAChartCurveSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        DAChartItemSettingPanel::updateUI(item);
        return;
    }

    // RTTI check before cast
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotCurve)) {
        DAChartItemSettingPanel::updateUI(item);
        return;
    }

    QwtPlotCurve* curve = d_cast< QwtPlotCurve* >();
    if (nullptr == curve) {
        return;
    }

    // 调用基类更新
    DAChartItemSettingPanel::updateUI(item);

    DAPropertyPanelContainerWidget* pp = propertyPanel();
    QSignalBlocker blocker(pp);

    // 通用属性
    pp->setStringValue(PID_Title, curve->title().text());
    pp->setDoubleValue(PID_ZValue, curve->z());
    setAxisValue(PID_XAxis, static_cast< QwtAxis::Position >(curve->xAxis()));
    setAxisValue(PID_YAxis, static_cast< QwtAxis::Position >(curve->yAxis()));

    // 曲线样式
    setCurveStyleValue(PID_CurveStyle, curve->style());

    // 画笔
    pp->setPenValue(PID_Pen, curve->pen());

    // 标记
    bool hasSymbol = (curve->symbol() != nullptr);
    pp->setBoolValue(PID_EnableMarker, hasSymbol);
    DAChartSymbolEditWidget* symbolWidget = getSymbolWidget(PID_Symbol);
    if (symbolWidget && hasSymbol) {
        const QwtSymbol* sym = curve->symbol();
        symbolWidget->setSymbolStyle(sym->style());
        symbolWidget->setSymbolSize(sym->size().width());
        symbolWidget->setSymbolColor(sym->brush().color());
        symbolWidget->setSymbolOutlinePen(sym->pen());
    }

    // 属性
    pp->setBoolValue(PID_Fitted, curve->testCurveAttribute(QwtPlotCurve::Fitted));
    pp->setBoolValue(PID_Inverted, curve->testCurveAttribute(QwtPlotCurve::Inverted));

    // 图例
    pp->setBoolValue(PID_LegendShowLine, curve->testLegendAttribute(QwtPlotCurve::LegendShowLine));
    pp->setBoolValue(PID_LegendShowSymbol, curve->testLegendAttribute(QwtPlotCurve::LegendShowSymbol));
    pp->setBoolValue(PID_LegendShowBrush, curve->testLegendAttribute(QwtPlotCurve::LegendShowBrush));

    // 填充
    bool hasFill = (curve->brush() != Qt::NoBrush);
    pp->setBoolValue(PID_EnableFill, hasFill);
    pp->setBrushValue(PID_Fill, curve->brush());

    // 基线
    pp->setStringValue(PID_BaseLine, QString::number(curve->baseline()));

    // 方向
    setOrientationValue(PID_Orientation, curve->orientation());
}

/**
 * @brief 处理属性值变化
 *
 * 根据propertyId区分不同属性，直接修改QwtPlotCurve对象并调用replot()。
 * 处理属性联动：CurveStyle=Steps时才允许Inverted；EnableMarker控制Symbol可见性。
 *
 * @param propertyId 属性ID
 */
void DAChartCurveSettingPanel::onCurvePropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    // RTTI check before cast
    if (!checkItemRTTI(QwtPlotItem::Rtti_PlotCurve)) {
        return;
    }
    QwtPlotCurve* curve = d_cast< QwtPlotCurve* >();
    if (nullptr == curve) {
        return;
    }

    DAPropertyPanelContainerWidget* pp = propertyPanel();

    switch (propertyId) {
    case PID_Title: {
        QString title = pp->getStringValue(PID_Title);
        curve->setTitle(title);
        break;
    }
    case PID_ZValue: {
        double z = pp->getDoubleValue(PID_ZValue);
        curve->setZ(z);
        break;
    }
    case PID_XAxis: {
        QwtAxis::Position xAxis = getAxisValue(PID_XAxis);
        curve->setXAxis(static_cast< QwtAxisId >(xAxis));
        break;
    }
    case PID_YAxis: {
        QwtAxis::Position yAxis = getAxisValue(PID_YAxis);
        curve->setYAxis(static_cast< QwtAxisId >(yAxis));
        break;
    }
    case PID_CurveStyle: {
        QwtPlotCurve::CurveStyle style = getCurveStyleValue(PID_CurveStyle);
        curve->setStyle(style);
        // 联动：只有Steps样式才允许Inverted
        pp->setPropertyEnabled(PID_Inverted, (style == QwtPlotCurve::Steps));
        // 联动：Sticks样式不允许Fill
        if (style == QwtPlotCurve::Sticks) {
            pp->setPropertyEnabled(PID_EnableFill, false);
            pp->setPropertyEnabled(PID_Fill, false);
        } else {
            pp->setPropertyEnabled(PID_EnableFill, true);
            if (pp->getBoolValue(PID_EnableFill)) {
                pp->setPropertyEnabled(PID_Fill, true);
            }
        }
        break;
    }
    case PID_Pen: {
        QPen pen = pp->getPenValue(PID_Pen);
        curve->setPen(pen);
        break;
    }
    case PID_EnableMarker: {
        bool enable = pp->getBoolValue(PID_EnableMarker);
        if (enable) {
            DAChartSymbolEditWidget* symbolWidget = getSymbolWidget(PID_Symbol);
            if (symbolWidget) {
                QwtSymbol* symbol = symbolWidget->createSymbol();
                curve->setSymbol(symbol);  // QwtPlotCurve接管所有权
            }
            pp->setPropertyEnabled(PID_Symbol, true);
        } else {
            curve->setSymbol(nullptr);
            pp->setPropertyEnabled(PID_Symbol, false);
        }
        break;
    }
    case PID_Symbol: {
        if (pp->getBoolValue(PID_EnableMarker)) {
            DAChartSymbolEditWidget* symbolWidget = getSymbolWidget(PID_Symbol);
            if (symbolWidget) {
                QwtSymbol* symbol = symbolWidget->createSymbol();
                curve->setSymbol(symbol);  // QwtPlotCurve接管所有权
            }
        }
        break;
    }
    case PID_Fitted: {
        bool fitted = pp->getBoolValue(PID_Fitted);
        curve->setCurveAttribute(QwtPlotCurve::Fitted, fitted);
        break;
    }
    case PID_Inverted: {
        bool inverted = pp->getBoolValue(PID_Inverted);
        curve->setCurveAttribute(QwtPlotCurve::Inverted, inverted);
        break;
    }
    case PID_LegendShowLine: {
        bool show = pp->getBoolValue(PID_LegendShowLine);
        curve->setLegendAttribute(QwtPlotCurve::LegendShowLine, show);
        break;
    }
    case PID_LegendShowSymbol: {
        bool show = pp->getBoolValue(PID_LegendShowSymbol);
        curve->setLegendAttribute(QwtPlotCurve::LegendShowSymbol, show);
        break;
    }
    case PID_LegendShowBrush: {
        bool show = pp->getBoolValue(PID_LegendShowBrush);
        curve->setLegendAttribute(QwtPlotCurve::LegendShowBrush, show);
        break;
    }
    case PID_EnableFill: {
        bool enable = pp->getBoolValue(PID_EnableFill);
        if (enable) {
            curve->setBrush(pp->getBrushValue(PID_Fill));
            pp->setPropertyEnabled(PID_Fill, true);
        } else {
            curve->setBrush(Qt::NoBrush);
            pp->setPropertyEnabled(PID_Fill, false);
        }
        break;
    }
    case PID_Fill: {
        if (pp->getBoolValue(PID_EnableFill)) {
            curve->setBrush(pp->getBrushValue(PID_Fill));
        }
        break;
    }
    case PID_BaseLine: {
        bool ok = false;
        double baseline = pp->getStringValue(PID_BaseLine).toDouble(&ok);
        if (ok) {
            curve->setBaseline(baseline);
        }
        break;
    }
    case PID_Orientation: {
        Qt::Orientation orientation = getOrientationValue(PID_Orientation);
        curve->setOrientation(orientation);
        break;
    }
    default:
        break;
    }

    replot();
}

}  // end namespace DA
