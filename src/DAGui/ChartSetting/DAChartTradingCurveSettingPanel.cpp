#include "DAChartTradingCurveSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartTradingCurveSettingPanel::DAChartTradingCurveSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartTradingCurveSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartTradingCurveSettingPanel::~DAChartTradingCurveSettingPanel()
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
 * - SymbolAttribute: 枚举属性(Bar/CandleStick)
 * - IncreasingBrush: 画刷属性（上涨填充色）
 * - DecreasingBrush: 画刷属性（下跌填充色）
 * - Orientation: 方向属性(Horizontal/Vertical)
 * - SymbolExtent: 双精度属性（符号宽度比例）
 * - MinPrice: 双精度属性（最小符号宽度）
 * - MaxPrice: 双精度属性（最大符号宽度）
 */
void DAChartTradingCurveSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    // 基础属性组
    panel->addCollapsibleGroup(tr("Basic")  // cn:基础
    );
    panel->addStringProperty(PropTitle, tr("Title")  // cn:标题
    );
    panel->addDoubleProperty(PropZValue, tr("Z Value")  // cn:Z值
    );
    panel->endGroup();

    // 坐标轴属性组
    panel->addCollapsibleGroup(tr("Axis")  // cn:坐标轴
    );
    addAxisProperty(PropXAxis, tr("X Axis")  // cn:X轴
                    ,
                    false);
    addAxisProperty(PropYAxis, tr("Y Axis")  // cn:Y轴
                    ,
                    true);
    panel->endGroup();

    // 符号属性组
    panel->addCollapsibleGroup(tr("Symbol")  // cn:符号
    );
    // QwtPlotTradingCurve::SymbolStyle: Bar=1, CandleStick=2
    panel->addEnumProperty(
        PropSymbolAttribute,
        tr("Symbol Attribute")  // cn:符号属性
        ,
        QStringList() << tr("Bar")  // cn:柱状
                      << tr("Candlestick")  // cn:K线
        ,
        QList< int >() << static_cast< int >(QwtPlotTradingCurve::Bar) << static_cast< int >(QwtPlotTradingCurve::CandleStick));
    panel->endGroup();

    // 颜色属性组
    panel->addCollapsibleGroup(tr("Color")  // cn:颜色
    );
    panel->addBrushProperty(PropIncreasingBrush, tr("Increasing Brush")  // cn:上涨画刷
    );
    panel->addBrushProperty(PropDecreasingBrush, tr("Decreasing Brush")  // cn:下跌画刷
    );
    panel->endGroup();
    // 方向属性组
    panel->addCollapsibleGroup(tr("Direction")  // cn:方向
    );
    addOrientationProperty(PropOrientation, tr("Orientation")  // cn:方向
    );
    panel->endGroup();
    // 尺寸属性组
    panel->addCollapsibleGroup(tr("Size")  // cn:尺寸
    );
    panel->addDoubleProperty(PropSymbolExtent, tr("Symbol Extent")  // cn:符号范围
                             ,
                             0.6, 0.0, 1.0);
    panel->addDoubleProperty(PropMinPrice, tr("Min Symbol Width")  // cn:最小符号宽度
                             ,
                             2.0, 0.0, 1000.0);
    panel->addDoubleProperty(PropMaxPrice, tr("Max Symbol Width")  // cn:最大符号宽度
                             ,
                             0.0, 0.0, 1000.0);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartTradingCurveSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotTradingCurve) {
        return;
    }

    QwtPlotTradingCurve* tradingCurve = static_cast< QwtPlotTradingCurve* >(item);
    auto panel                        = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, tradingCurve->title().text());
    panel->setDoubleValue(PropZValue, tradingCurve->z());

    // 坐标轴属性
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(tradingCurve->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(tradingCurve->yAxis()));

    // 符号属性
    panel->setEnumValue(PropSymbolAttribute, static_cast< int >(tradingCurve->symbolStyle()));

    // 颜色属性
    panel->setBrushValue(PropIncreasingBrush, tradingCurve->symbolBrush(QwtPlotTradingCurve::Increasing));
    panel->setBrushValue(PropDecreasingBrush, tradingCurve->symbolBrush(QwtPlotTradingCurve::Decreasing));

    // 方向属性
    setOrientationValue(PropOrientation, tradingCurve->orientation());

    // 尺寸属性
    panel->setDoubleValue(PropSymbolExtent, tradingCurve->symbolExtent());
    panel->setDoubleValue(PropMinPrice, tradingCurve->minSymbolWidth());
    panel->setDoubleValue(PropMaxPrice, tradingCurve->maxSymbolWidth());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartTradingCurveSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotTradingCurve* tradingCurve = s_cast< QwtPlotTradingCurve* >();
    if (nullptr == tradingCurve) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        tradingCurve->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        tradingCurve->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        tradingCurve->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        tradingCurve->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropSymbolAttribute: {
        int style = panel->getEnumValue(PropSymbolAttribute);
        tradingCurve->setSymbolStyle(static_cast< QwtPlotTradingCurve::SymbolStyle >(style));
        break;
    }
    case PropIncreasingBrush:
        tradingCurve->setSymbolBrush(QwtPlotTradingCurve::Increasing, panel->getBrushValue(PropIncreasingBrush));
        break;
    case PropDecreasingBrush:
        tradingCurve->setSymbolBrush(QwtPlotTradingCurve::Decreasing, panel->getBrushValue(PropDecreasingBrush));
        break;
    case PropOrientation:
        tradingCurve->setOrientation(getOrientationValue(PropOrientation));
        break;
    case PropSymbolExtent:
        tradingCurve->setSymbolExtent(panel->getDoubleValue(PropSymbolExtent));
        break;
    case PropMinPrice:
        tradingCurve->setMinSymbolWidth(panel->getDoubleValue(PropMinPrice));
        break;
    case PropMaxPrice:
        tradingCurve->setMaxSymbolWidth(panel->getDoubleValue(PropMaxPrice));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
