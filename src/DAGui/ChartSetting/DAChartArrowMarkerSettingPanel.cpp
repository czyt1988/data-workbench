#include "DAChartArrowMarkerSettingPanel.h"
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
DAChartArrowMarkerSettingPanel::DAChartArrowMarkerSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartArrowMarkerSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartArrowMarkerSettingPanel::~DAChartArrowMarkerSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - XAxis/YAxis: 坐标轴属性
 * - PositionMode: 枚举属性(ExplicitPoints/StartLengthAngle)
 * - StartX/StartY: 双精度属性(任意坐标值)
 * - EndX/EndY: 双精度属性(ExplicitPoints模式)
 * - Length: 双精度属性(StartLengthAngle模式)
 * - Angle: 双精度属性(StartLengthAngle模式)
 * - LinePen: 笔属性
 * - HeadStyle: 枚举属性(7种端点样式)
 * - HeadSize: 双精度属性(像素)
 * - HeadPen/HeadBrush: 头部画笔/画刷
 * - TailStyle/TailSize/TailPen/TailBrush: 尾部对应属性
 */
void DAChartArrowMarkerSettingPanel::buildPropertyPanel()
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
    // QwtPlotArrowMarker::PositionMode: ExplicitPoints=0, StartLengthAngle=1
    panel->addEnumProperty(PropPositionMode,
                           tr("Position Mode")  // cn:定位模式
                           ,
                           QStringList() << tr("Explicit Points")  // cn:显式起止点
                                         << tr("Start Length Angle")  // cn:起点长度角度
                           ,
                           QList< int >() << static_cast< int >(QwtPlotArrowMarker::ExplicitPoints)
                                          << static_cast< int >(QwtPlotArrowMarker::StartLengthAngle));
    panel->addDoubleProperty(PropStartX, tr("Start X")  // cn:起点X
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addDoubleProperty(PropStartY, tr("Start Y")  // cn:起点Y
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addDoubleProperty(PropEndX, tr("End X")  // cn:终点X
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addDoubleProperty(PropEndY, tr("End Y")  // cn:终点Y
                             ,
                             0.0, -1e15, 1e15, 6);
    panel->addDoubleProperty(PropLength, tr("Length")  // cn:长度
                             ,
                             0.0, 0.0, 10000.0, 3);
    panel->addDoubleProperty(PropAngle, tr("Angle")  // cn:角度
                             ,
                             0.0, -360.0, 360.0, 3);
    panel->endGroup();

    // 线条属性组
    panel->addCollapsibleGroup(tr("Line")  // cn:线条
    );
    panel->addPenProperty(PropLinePen, tr("Line Pen")  // cn:线条画笔
    );
    panel->endGroup();

    // 头部端点属性组
    panel->addCollapsibleGroup(tr("Head")  // cn:头部
    );
    // QwtPlotArrowMarker::EndpointStyle: NoEndpoint=0..CustomPath=6
    panel->addEnumProperty(PropHeadStyle,
                           tr("Head Style")  // cn:头部样式
                           ,
                           QStringList() << tr("None")  // cn:无
                                         << tr("Arrow Head")  // cn:箭头
                                         << tr("Circle")  // cn:圆形
                                         << tr("Square")  // cn:方形
                                         << tr("Diamond")  // cn:菱形
                                         << tr("Triangle")  // cn:三角形
                                         << tr("Custom")  // cn:自定义
                           ,
                           QList< int >() << static_cast< int >(QwtPlotArrowMarker::NoEndpoint)
                                          << static_cast< int >(QwtPlotArrowMarker::ArrowHead)
                                          << static_cast< int >(QwtPlotArrowMarker::Circle)
                                          << static_cast< int >(QwtPlotArrowMarker::Square)
                                          << static_cast< int >(QwtPlotArrowMarker::Diamond)
                                          << static_cast< int >(QwtPlotArrowMarker::Triangle)
                                          << static_cast< int >(QwtPlotArrowMarker::CustomPath));
    panel->addDoubleProperty(PropHeadSize, tr("Head Size")  // cn:头部尺寸
                             ,
                             0.0, 0.0, 1000.0, 3);
    panel->addPenProperty(PropHeadPen, tr("Head Pen")  // cn:头部画笔
    );
    panel->addBrushProperty(PropHeadBrush, tr("Head Brush")  // cn:头部画刷
    );
    panel->endGroup();

    // 尾部端点属性组
    panel->addCollapsibleGroup(tr("Tail")  // cn:尾部
    );
    panel->addEnumProperty(PropTailStyle,
                           tr("Tail Style")  // cn:尾部样式
                           ,
                           QStringList() << tr("None")  // cn:无
                                         << tr("Arrow Head")  // cn:箭头
                                         << tr("Circle")  // cn:圆形
                                         << tr("Square")  // cn:方形
                                         << tr("Diamond")  // cn:菱形
                                         << tr("Triangle")  // cn:三角形
                                         << tr("Custom")  // cn:自定义
                           ,
                           QList< int >() << static_cast< int >(QwtPlotArrowMarker::NoEndpoint)
                                          << static_cast< int >(QwtPlotArrowMarker::ArrowHead)
                                          << static_cast< int >(QwtPlotArrowMarker::Circle)
                                          << static_cast< int >(QwtPlotArrowMarker::Square)
                                          << static_cast< int >(QwtPlotArrowMarker::Diamond)
                                          << static_cast< int >(QwtPlotArrowMarker::Triangle)
                                          << static_cast< int >(QwtPlotArrowMarker::CustomPath));
    panel->addDoubleProperty(PropTailSize, tr("Tail Size")  // cn:尾部尺寸
                             ,
                             0.0, 0.0, 1000.0, 3);
    panel->addPenProperty(PropTailPen, tr("Tail Pen")  // cn:尾部画笔
    );
    panel->addBrushProperty(PropTailBrush, tr("Tail Brush")  // cn:尾部画刷
    );
    panel->endGroup();
}

/**
 * @brief 根据定位模式启用/禁用相关属性
 * @param mode 定位模式
 */
void DAChartArrowMarkerSettingPanel::updatePositionModeVisibility(QwtPlotArrowMarker::PositionMode mode)
{
    auto panel = propertyPanel();
    bool isExplicit = (mode == QwtPlotArrowMarker::ExplicitPoints);
    // ExplicitPoints 模式启用 EndX/EndY,禁用 Length/Angle
    panel->setPropertyEnabled(PropEndX, isExplicit);
    panel->setPropertyEnabled(PropEndY, isExplicit);
    panel->setPropertyEnabled(PropLength, !isExplicit);
    panel->setPropertyEnabled(PropAngle, !isExplicit);
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartArrowMarkerSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotArrowMarker) {
        return;
    }

    QwtPlotArrowMarker* arrow = static_cast< QwtPlotArrowMarker* >(item);
    auto panel                = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, arrow->title().text());
    panel->setDoubleValue(PropZValue, arrow->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(arrow->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(arrow->yAxis()));

    // 位置属性
    panel->setEnumValue(PropPositionMode, static_cast< int >(arrow->positionMode()));
    QPointF start = arrow->startPoint();
    QPointF end   = arrow->endPoint();
    panel->setDoubleValue(PropStartX, start.x());
    panel->setDoubleValue(PropStartY, start.y());
    panel->setDoubleValue(PropEndX, end.x());
    panel->setDoubleValue(PropEndY, end.y());
    panel->setDoubleValue(PropLength, arrow->length());
    panel->setDoubleValue(PropAngle, arrow->angle());

    // 线条属性
    panel->setPenValue(PropLinePen, arrow->linePen());

    // 头部属性
    panel->setEnumValue(PropHeadStyle, static_cast< int >(arrow->headStyle()));
    panel->setDoubleValue(PropHeadSize, arrow->headSize().width());
    panel->setPenValue(PropHeadPen, arrow->headPen());
    panel->setBrushValue(PropHeadBrush, arrow->headBrush());

    // 尾部属性
    panel->setEnumValue(PropTailStyle, static_cast< int >(arrow->tailStyle()));
    panel->setDoubleValue(PropTailSize, arrow->tailSize().width());
    panel->setPenValue(PropTailPen, arrow->tailPen());
    panel->setBrushValue(PropTailBrush, arrow->tailBrush());

    // 根据定位模式更新属性启用状态(blocker 期间不触发信号)
    updatePositionModeVisibility(arrow->positionMode());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartArrowMarkerSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotArrowMarker* arrow = s_cast< QwtPlotArrowMarker* >();
    if (nullptr == arrow) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        arrow->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        arrow->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        arrow->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        arrow->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropPositionMode: {
        int modeVal = panel->getEnumValue(PropPositionMode);
        arrow->setPositionMode(static_cast< QwtPlotArrowMarker::PositionMode >(modeVal));
        // 模式切换后更新相关属性的启用状态
        updatePositionModeVisibility(static_cast< QwtPlotArrowMarker::PositionMode >(modeVal));
        break;
    }
    case PropStartX: {
        QPointF start = arrow->startPoint();
        start.setX(panel->getDoubleValue(PropStartX));
        arrow->setStartPoint(start);
        break;
    }
    case PropStartY: {
        QPointF start = arrow->startPoint();
        start.setY(panel->getDoubleValue(PropStartY));
        arrow->setStartPoint(start);
        break;
    }
    case PropEndX: {
        QPointF end = arrow->endPoint();
        end.setX(panel->getDoubleValue(PropEndX));
        arrow->setEndPoint(end);
        break;
    }
    case PropEndY: {
        QPointF end = arrow->endPoint();
        end.setY(panel->getDoubleValue(PropEndY));
        arrow->setEndPoint(end);
        break;
    }
    case PropLength:
        arrow->setLength(panel->getDoubleValue(PropLength));
        break;
    case PropAngle:
        arrow->setAngle(panel->getDoubleValue(PropAngle));
        break;
    case PropLinePen:
        arrow->setLinePen(panel->getPenValue(PropLinePen));
        break;
    case PropHeadStyle: {
        int styleVal = panel->getEnumValue(PropHeadStyle);
        arrow->setHeadStyle(static_cast< QwtPlotArrowMarker::EndpointStyle >(styleVal));
        break;
    }
    case PropHeadSize: {
        double size = panel->getDoubleValue(PropHeadSize);
        arrow->setHeadSize(QSizeF(size, size));
        break;
    }
    case PropHeadPen:
        arrow->setHeadPen(panel->getPenValue(PropHeadPen));
        break;
    case PropHeadBrush:
        arrow->setHeadBrush(panel->getBrushValue(PropHeadBrush));
        break;
    case PropTailStyle: {
        int styleVal = panel->getEnumValue(PropTailStyle);
        arrow->setTailStyle(static_cast< QwtPlotArrowMarker::EndpointStyle >(styleVal));
        break;
    }
    case PropTailSize: {
        double size = panel->getDoubleValue(PropTailSize);
        arrow->setTailSize(QSizeF(size, size));
        break;
    }
    case PropTailPen:
        arrow->setTailPen(panel->getPenValue(PropTailPen));
        break;
    case PropTailBrush:
        arrow->setTailBrush(panel->getBrushValue(PropTailBrush));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
