#include "DAChartMarkerSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartMarkerSettingPanel::DAChartMarkerSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartMarkerSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartMarkerSettingPanel::~DAChartMarkerSettingPanel()
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
 * - XValue/YValue: 双精度属性
 * - LineStyle: 枚举属性(NoLine/HLine/VLine/Cross)
 * - LinePen: 笔属性
 * - Label: 字符串属性
 * - LabelAlignment: 对齐属性
 * - LabelOrientation: 枚举属性(Horizontal/Vertical)
 * - Spacing: 整数属性
 */
void DAChartMarkerSettingPanel::buildPropertyPanel()
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

    // 位置属性组
    panel->addCollapsibleGroup(tr("Position")  // cn:位置
    );
    panel->addDoubleProperty(PropXValue, tr("X Value")  // cn:X值
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addDoubleProperty(PropYValue, tr("Y Value")  // cn:Y值
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->endGroup();

    // 线条属性组
    panel->addCollapsibleGroup(tr("Line")  // cn:线条
    );
    // QwtPlotMarker::LineStyle: NoLine=0, HLine=1, VLine=2, Cross=3
    panel->addEnumProperty(PropLineStyle,
                           tr("Line Style")  // cn:线条样式
                           ,
                           QStringList() << tr("No Line")  // cn:无线
                                         << tr("Horizontal")  // cn:水平
                                         << tr("Vertical")  // cn:垂直
                                         << tr("Cross")  // cn:十字
                           ,
                           QList< int >() << static_cast< int >(QwtPlotMarker::NoLine)
                                          << static_cast< int >(QwtPlotMarker::HLine)
                                          << static_cast< int >(QwtPlotMarker::VLine)
                                          << static_cast< int >(QwtPlotMarker::Cross));
    panel->addPenProperty(PropLinePen, tr("Line Pen")  // cn:线条画笔
    );
    panel->endGroup();

    // 标签属性组
    panel->addCollapsibleGroup(tr("Label")  // cn:标签
    );
    panel->addStringProperty(PropLabel, tr("Label")  // cn:标签文本
    );
    panel->addAlignmentProperty(PropLabelAlignment, tr("Label Alignment")  // cn:标签对齐
    );
    // Qt::Orientation: Horizontal=0, Vertical=1
    panel->addEnumProperty(PropLabelOrientation,
                           tr("Label Orientation")  // cn:标签方向
                           ,
                           QStringList() << tr("Horizontal")  // cn:水平
                                         << tr("Vertical")  // cn:垂直
                           ,
                           QList< int >() << static_cast< int >(Qt::Horizontal)
                                          << static_cast< int >(Qt::Vertical));
    panel->addIntProperty(PropSpacing, tr("Spacing")  // cn:间距
                          ,
                          0, 0, 1000);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartMarkerSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotMarker) {
        return;
    }

    QwtPlotMarker* marker = static_cast< QwtPlotMarker* >(item);
    auto panel            = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, marker->title().text());
    panel->setDoubleValue(PropZValue, marker->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(marker->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(marker->yAxis()));

    // 位置属性
    panel->setDoubleValue(PropXValue, marker->xValue());
    panel->setDoubleValue(PropYValue, marker->yValue());

    // 线条属性
    panel->setEnumValue(PropLineStyle, static_cast< int >(marker->lineStyle()));
    panel->setPenValue(PropLinePen, marker->linePen());

    // 标签属性
    panel->setStringValue(PropLabel, marker->label().text());
    panel->setAlignmentValue(PropLabelAlignment, marker->labelAlignment());
    panel->setEnumValue(PropLabelOrientation, static_cast< int >(marker->labelOrientation()));
    panel->setIntValue(PropSpacing, marker->spacing());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartMarkerSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotMarker* marker = s_cast< QwtPlotMarker* >();
    if (nullptr == marker) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        marker->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        marker->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        marker->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        marker->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropXValue:
        marker->setXValue(panel->getDoubleValue(PropXValue));
        break;
    case PropYValue:
        marker->setYValue(panel->getDoubleValue(PropYValue));
        break;
    case PropLineStyle: {
        int styleVal = panel->getEnumValue(PropLineStyle);
        marker->setLineStyle(static_cast< QwtPlotMarker::LineStyle >(styleVal));
        break;
    }
    case PropLinePen:
        marker->setLinePen(panel->getPenValue(PropLinePen));
        break;
    case PropLabel:
        marker->setLabel(QwtText(panel->getStringValue(PropLabel)));
        break;
    case PropLabelAlignment:
        marker->setLabelAlignment(panel->getAlignmentValue(PropLabelAlignment));
        break;
    case PropLabelOrientation: {
        int orientVal = panel->getEnumValue(PropLabelOrientation);
        marker->setLabelOrientation(static_cast< Qt::Orientation >(orientVal));
        break;
    }
    case PropSpacing:
        marker->setSpacing(panel->getIntValue(PropSpacing));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
