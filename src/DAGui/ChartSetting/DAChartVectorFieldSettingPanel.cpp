#include "DAChartVectorFieldSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>
#include <QSizeF>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartVectorFieldSettingPanel::DAChartVectorFieldSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartVectorFieldSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartVectorFieldSettingPanel::~DAChartVectorFieldSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - XAxis/YAxis: 坐标轴属性
 * - Pen: 笔属性
 * - Brush: 画刷属性
 * - IndicatorOrigin: 枚举属性(Head/Tail/Center)
 * - MagnitudeAsColor/MagnitudeAsLength: 布尔属性(可同时启用)
 * - MinArrowLength/MaxArrowLength: 双精度属性(像素)
 * - MagnitudeScaleFactor: 双精度属性
 * - FilterVectors: 布尔属性
 * - RasterSizeW/RasterSizeH: 双精度属性(宽/高)
 */
void DAChartVectorFieldSettingPanel::buildPropertyPanel()
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
    // QwtPlotVectorField::IndicatorOrigin: OriginHead=0, OriginTail=1, OriginCenter=2
    panel->addEnumProperty(PropIndicatorOrigin,
                           tr("Indicator Origin")  // cn:箭头原点
                           ,
                           QStringList() << tr("Head")  // cn:头部
                                         << tr("Tail")  // cn:尾部
                                         << tr("Center")  // cn:中心
                           ,
                           QList< int >() << static_cast< int >(QwtPlotVectorField::OriginHead)
                                          << static_cast< int >(QwtPlotVectorField::OriginTail)
                                          << static_cast< int >(QwtPlotVectorField::OriginCenter));
    panel->endGroup();

    // 幅值属性组
    panel->addCollapsibleGroup(tr("Magnitude")  // cn:幅值
    );
    panel->addBoolProperty(PropMagnitudeAsColor, tr("Magnitude As Color")  // cn:幅值映射颜色
    );
    panel->addBoolProperty(PropMagnitudeAsLength, tr("Magnitude As Length")  // cn:幅值映射长度
    );
    panel->addDoubleProperty(PropMinArrowLength, tr("Min Arrow Length")  // cn:最小箭头长度
                             ,
                             0.0, 0.0, 10000.0, 3);
    panel->addDoubleProperty(PropMaxArrowLength, tr("Max Arrow Length")  // cn:最大箭头长度
                             ,
                             0.0, 0.0, 10000.0, 3);
    panel->addDoubleProperty(PropMagnitudeScaleFactor, tr("Magnitude Scale Factor")  // cn:幅值缩放因子
                             ,
                             1.0, -1e15, 1e15, 6);
    panel->endGroup();

    // 过滤属性组
    panel->addCollapsibleGroup(tr("Filter")  // cn:过滤
    );
    panel->addBoolProperty(PropFilterVectors, tr("Filter Vectors")  // cn:过滤矢量
    );
    panel->addDoubleProperty(PropRasterSizeW, tr("Raster Width")  // cn:栅格宽度
                             ,
                             0.0, 0.0, 10000.0, 3);
    panel->addDoubleProperty(PropRasterSizeH, tr("Raster Height")  // cn:栅格高度
                             ,
                             0.0, 0.0, 10000.0, 3);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartVectorFieldSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotVectorField) {
        return;
    }

    QwtPlotVectorField* field = static_cast< QwtPlotVectorField* >(item);
    auto panel                = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, field->title().text());
    panel->setDoubleValue(PropZValue, field->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(field->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(field->yAxis()));

    // 外观属性
    panel->setPenValue(PropPen, field->pen());
    panel->setBrushValue(PropBrush, field->brush());
    panel->setEnumValue(PropIndicatorOrigin, static_cast< int >(field->indicatorOrigin()));

    // 幅值属性
    panel->setBoolValue(PropMagnitudeAsColor, field->testMagnitudeMode(QwtPlotVectorField::MagnitudeAsColor));
    panel->setBoolValue(PropMagnitudeAsLength, field->testMagnitudeMode(QwtPlotVectorField::MagnitudeAsLength));
    panel->setDoubleValue(PropMinArrowLength, field->minArrowLength());
    panel->setDoubleValue(PropMaxArrowLength, field->maxArrowLength());
    panel->setDoubleValue(PropMagnitudeScaleFactor, field->magnitudeScaleFactor());

    // 过滤属性
    panel->setBoolValue(PropFilterVectors, field->testPaintAttribute(QwtPlotVectorField::FilterVectors));
    QSizeF raster = field->rasterSize();
    panel->setDoubleValue(PropRasterSizeW, raster.width());
    panel->setDoubleValue(PropRasterSizeH, raster.height());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartVectorFieldSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotVectorField* field = s_cast< QwtPlotVectorField* >();
    if (nullptr == field) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        field->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        field->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        field->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        field->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropPen:
        field->setPen(panel->getPenValue(PropPen));
        break;
    case PropBrush:
        field->setBrush(panel->getBrushValue(PropBrush));
        break;
    case PropIndicatorOrigin: {
        int originVal = panel->getEnumValue(PropIndicatorOrigin);
        field->setIndicatorOrigin(static_cast< QwtPlotVectorField::IndicatorOrigin >(originVal));
        break;
    }
    case PropMagnitudeAsColor:
        field->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsColor, panel->getBoolValue(PropMagnitudeAsColor));
        break;
    case PropMagnitudeAsLength:
        field->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsLength, panel->getBoolValue(PropMagnitudeAsLength));
        break;
    case PropMinArrowLength:
        field->setMinArrowLength(panel->getDoubleValue(PropMinArrowLength));
        break;
    case PropMaxArrowLength:
        field->setMaxArrowLength(panel->getDoubleValue(PropMaxArrowLength));
        break;
    case PropMagnitudeScaleFactor:
        field->setMagnitudeScaleFactor(panel->getDoubleValue(PropMagnitudeScaleFactor));
        break;
    case PropFilterVectors:
        field->setPaintAttribute(QwtPlotVectorField::FilterVectors, panel->getBoolValue(PropFilterVectors));
        break;
    case PropRasterSizeW:
    case PropRasterSizeH: {
        // 栅格宽高联动:构造完整QSizeF后整体写回
        double w = panel->getDoubleValue(PropRasterSizeW);
        double h = panel->getDoubleValue(PropRasterSizeH);
        field->setRasterSize(QSizeF(w, h));
        break;
    }
    default:
        break;
    }

    replot();
}

}  // end namespace DA
