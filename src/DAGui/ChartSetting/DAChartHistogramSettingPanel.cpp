#include "DAChartHistogramSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartHistogramSettingPanel::DAChartHistogramSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartHistogramSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartHistogramSettingPanel::~DAChartHistogramSettingPanel()
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
 * - Style: 枚举属性(Outline/Columns/Lines)
 * - Pen: 笔属性
 * - Brush: 画刷属性
 * - Baseline: 双精度属性
 */
void DAChartHistogramSettingPanel::buildPropertyPanel()
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
    // QwtPlotHistogram::HistogramStyle: Outline=0, Columns=1, Lines=2
    panel->addEnumProperty(PropStyle,
                           tr("Histogram Style")  // cn:直方图样式
                           ,
                           QStringList() << tr("Outline")  // cn:轮廓
                                         << tr("Columns")  // cn:柱状
                                         << tr("Lines")  // cn:线条
                           ,
                           QList< int >() << static_cast< int >(QwtPlotHistogram::Outline)
                                          << static_cast< int >(QwtPlotHistogram::Columns)
                                          << static_cast< int >(QwtPlotHistogram::Lines));
    panel->addPenProperty(PropPen, tr("Pen")  // cn:画笔
    );
    panel->addBrushProperty(PropBrush, tr("Brush")  // cn:画刷
    );
    panel->addDoubleProperty(PropBaseline, tr("Baseline")  // cn:基线
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartHistogramSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotHistogram) {
        return;
    }

    QwtPlotHistogram* hist = static_cast< QwtPlotHistogram* >(item);
    auto panel             = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, hist->title().text());
    panel->setDoubleValue(PropZValue, hist->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(hist->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(hist->yAxis()));

    // 样式属性
    setOrientationValue(PropOrientation, hist->orientation());
    panel->setEnumValue(PropStyle, static_cast< int >(hist->style()));
    panel->setPenValue(PropPen, hist->pen());
    panel->setBrushValue(PropBrush, hist->brush());
    panel->setDoubleValue(PropBaseline, hist->baseline());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartHistogramSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotHistogram* hist = s_cast< QwtPlotHistogram* >();
    if (nullptr == hist) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        hist->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        hist->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        hist->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        hist->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropOrientation:
        hist->setOrientation(getOrientationValue(PropOrientation));
        break;
    case PropStyle: {
        int styleVal = panel->getEnumValue(PropStyle);
        hist->setStyle(static_cast< QwtPlotHistogram::HistogramStyle >(styleVal));
        break;
    }
    case PropPen:
        hist->setPen(panel->getPenValue(PropPen));
        break;
    case PropBrush:
        hist->setBrush(panel->getBrushValue(PropBrush));
        break;
    case PropBaseline:
        hist->setBaseline(panel->getDoubleValue(PropBaseline));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
