#include "DAChartShapeSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartShapeSettingPanel::DAChartShapeSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartShapeSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartShapeSettingPanel::~DAChartShapeSettingPanel()
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
 * - Pen: 笔属性
 * - Brush: 画刷属性
 * - RenderTolerance: 双精度属性(0.0-10.0)
 * - ClipPolygons: 布尔属性
 * - LegendMode: 枚举属性(Shape/Color)
 */
void DAChartShapeSettingPanel::buildPropertyPanel()
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

    // 外观属性组
    panel->addCollapsibleGroup(tr("Appearance")  // cn:外观
    );
    panel->addPenProperty(PropPen, tr("Pen")  // cn:画笔
    );
    panel->addBrushProperty(PropBrush, tr("Brush")  // cn:画刷
    );
    // QwtPlotShapeItem::LegendMode: LegendShape=0, LegendColor=1
    panel->addEnumProperty(PropLegendMode,
                           tr("Legend Mode")  // cn:图例模式
                           ,
                           QStringList() << tr("Shape")  // cn:形状
                                         << tr("Color")  // cn:颜色
                           ,
                           QList< int >() << static_cast< int >(QwtPlotShapeItem::LegendShape)
                                          << static_cast< int >(QwtPlotShapeItem::LegendColor));
    panel->endGroup();

    // 渲染属性组
    panel->addCollapsibleGroup(tr("Render")  // cn:渲染
    );
    panel->addDoubleProperty(PropRenderTolerance, tr("Render Tolerance")  // cn:渲染容差
                             ,
                             0.0, 0.0, 10.0, 3);
    panel->addBoolProperty(PropClipPolygons, tr("Clip Polygons")  // cn:裁剪多边形
    );
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartShapeSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotShape) {
        return;
    }

    QwtPlotShapeItem* shape = static_cast< QwtPlotShapeItem* >(item);
    auto panel              = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, shape->title().text());
    panel->setDoubleValue(PropZValue, shape->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(shape->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(shape->yAxis()));

    // 外观属性
    panel->setPenValue(PropPen, shape->pen());
    panel->setBrushValue(PropBrush, shape->brush());
    panel->setEnumValue(PropLegendMode, static_cast< int >(shape->legendMode()));

    // 渲染属性
    panel->setDoubleValue(PropRenderTolerance, shape->renderTolerance());
    panel->setBoolValue(PropClipPolygons, shape->testPaintAttribute(QwtPlotShapeItem::ClipPolygons));
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartShapeSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotShapeItem* shape = s_cast< QwtPlotShapeItem* >();
    if (nullptr == shape) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        shape->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        shape->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        shape->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        shape->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropPen:
        shape->setPen(panel->getPenValue(PropPen));
        break;
    case PropBrush:
        shape->setBrush(panel->getBrushValue(PropBrush));
        break;
    case PropRenderTolerance:
        shape->setRenderTolerance(panel->getDoubleValue(PropRenderTolerance));
        break;
    case PropClipPolygons:
        shape->setPaintAttribute(QwtPlotShapeItem::ClipPolygons, panel->getBoolValue(PropClipPolygons));
        break;
    case PropLegendMode: {
        int modeVal = panel->getEnumValue(PropLegendMode);
        shape->setLegendMode(static_cast< QwtPlotShapeItem::LegendMode >(modeVal));
        break;
    }
    default:
        break;
    }

    replot();
}

}  // end namespace DA
