#include "DAChartBoxChartSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartBoxChartSettingPanel::DAChartBoxChartSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartBoxChartSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartBoxChartSettingPanel::~DAChartBoxChartSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性:
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - XAxis/YAxis: 坐标轴属性
 * - Orientation: 方向属性
 * - BoxStyle: 枚举属性(NoBox/Rect/Diamond/Notch)
 * - BoxExtent: 双精度属性(0.1-10.0)
 * - MinBoxWidth/MaxBoxWidth: 双精度属性(像素)
 * - Pen: 笔属性
 * - Brush: 画刷属性
 * - WhiskerStyle: 枚举属性(NoWhiskers/StandardWhisker/MinMaxLine)
 * - MedianVisible: 布尔属性
 * - MedianPen: 笔属性
 * - MeanVisible: 布尔属性
 * - OutlierJitter: 双精度属性
 */
void DAChartBoxChartSettingPanel::buildPropertyPanel()
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

    // 箱体属性组
    panel->addCollapsibleGroup(tr("Box")  // cn:箱体
    );
    addOrientationProperty(PropOrientation, tr("Orientation")  // cn:方向
    );
    // QwtPlotBoxChart::BoxStyle: NoBox=0, Rect=1, Diamond=2, Notch=3
    panel->addEnumProperty(PropBoxStyle,
                           tr("Box Style")  // cn:箱体样式
                           ,
                           QStringList() << tr("No Box")  // cn:无箱体
                                         << tr("Rectangle")  // cn:矩形
                                         << tr("Diamond")  // cn:菱形
                                         << tr("Notched")  // cn:带凹槽
                           ,
                           QList< int >() << static_cast< int >(QwtPlotBoxChart::NoBox)
                                          << static_cast< int >(QwtPlotBoxChart::Rect)
                                          << static_cast< int >(QwtPlotBoxChart::Diamond)
                                          << static_cast< int >(QwtPlotBoxChart::Notch));
    panel->addDoubleProperty(PropBoxExtent, tr("Box Extent")  // cn:箱体宽度
                             ,
                             0.5, 0.1, 10.0, 3);
    panel->addDoubleProperty(PropMinBoxWidth, tr("Min Box Width")  // cn:最小宽度
                             ,
                             0.0, 0.0, 1000.0, 3);
    panel->addDoubleProperty(PropMaxBoxWidth, tr("Max Box Width")  // cn:最大宽度
                             ,
                             0.0, -1000.0, 1000.0, 3);
    panel->addPenProperty(PropPen, tr("Pen")  // cn:轮廓画笔
    );
    panel->addBrushProperty(PropBrush, tr("Brush")  // cn:填充画刷
    );
    panel->endGroup();

    // 须线属性组
    panel->addCollapsibleGroup(tr("Whisker")  // cn:须线
    );
    // QwtPlotBoxChart::WhiskerStyle: NoWhiskers=0, StandardWhisker=1, MinMaxLine=2
    panel->addEnumProperty(PropWhiskerStyle,
                           tr("Whisker Style")  // cn:须线样式
                           ,
                           QStringList() << tr("No Whiskers")  // cn:无须线
                                         << tr("Standard (T-bar)")  // cn:标准(T形)
                                         << tr("Min-Max Line")  // cn:极值线
                           ,
                           QList< int >() << static_cast< int >(QwtPlotBoxChart::NoWhiskers)
                                          << static_cast< int >(QwtPlotBoxChart::StandardWhisker)
                                          << static_cast< int >(QwtPlotBoxChart::MinMaxLine));
    panel->endGroup();

    // 中位数线属性组
    panel->addCollapsibleGroup(tr("Median")  // cn:中位数线
    );
    panel->addBoolProperty(PropMedianVisible, tr("Median Visible")  // cn:中位数线可见
    );
    panel->addPenProperty(PropMedianPen, tr("Median Pen")  // cn:中位数线画笔
    );
    panel->endGroup();

    // 均值属性组
    panel->addCollapsibleGroup(tr("Mean")  // cn:均值
    );
    panel->addBoolProperty(PropMeanVisible, tr("Mean Visible")  // cn:均值标记可见
    );
    panel->endGroup();

    // 离群点属性组
    panel->addCollapsibleGroup(tr("Outliers")  // cn:离群点
    );
    panel->addDoubleProperty(PropOutlierJitter, tr("Outlier Jitter")  // cn:离群点抖动
                             ,
                             0.0, 0.0, 1000.0, 3);
    panel->endGroup();
}

/**
 * @brief 根据中位数线可见性启用/禁用中位数线画笔属性
 * @param visible 是否可见
 */
void DAChartBoxChartSettingPanel::updateMedianPenEnabled(bool visible)
{
    propertyPanel()->setPropertyEnabled(PropMedianPen, visible);
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartBoxChartSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotBoxChart) {
        return;
    }

    QwtPlotBoxChart* box = static_cast< QwtPlotBoxChart* >(item);
    auto panel           = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, box->title().text());
    panel->setDoubleValue(PropZValue, box->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(box->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(box->yAxis()));

    // 箱体属性
    setOrientationValue(PropOrientation, box->orientation());
    panel->setEnumValue(PropBoxStyle, static_cast< int >(box->boxStyle()));
    panel->setDoubleValue(PropBoxExtent, box->boxExtent());
    panel->setDoubleValue(PropMinBoxWidth, box->minBoxWidth());
    panel->setDoubleValue(PropMaxBoxWidth, box->maxBoxWidth());
    panel->setPenValue(PropPen, box->pen());
    panel->setBrushValue(PropBrush, box->brush());

    // 须线属性
    panel->setEnumValue(PropWhiskerStyle, static_cast< int >(box->whiskerStyle()));

    // 中位数线属性
    bool medianVisible = box->isMedianVisible();
    panel->setBoolValue(PropMedianVisible, medianVisible);
    panel->setPenValue(PropMedianPen, box->medianPen());

    // 均值属性
    panel->setBoolValue(PropMeanVisible, box->isMeanVisible());

    // 离群点属性
    panel->setDoubleValue(PropOutlierJitter, box->outlierJitter());

    // 根据中位数线可见性更新画笔属性启用状态(blocker 期间不触发信号)
    updateMedianPenEnabled(medianVisible);
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartBoxChartSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotBoxChart* box = s_cast< QwtPlotBoxChart* >();
    if (nullptr == box) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        box->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        box->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        box->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        box->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropOrientation:
        box->setOrientation(getOrientationValue(PropOrientation));
        break;
    case PropBoxStyle: {
        int styleVal = panel->getEnumValue(PropBoxStyle);
        box->setBoxStyle(static_cast< QwtPlotBoxChart::BoxStyle >(styleVal));
        break;
    }
    case PropBoxExtent:
        box->setBoxExtent(panel->getDoubleValue(PropBoxExtent));
        break;
    case PropMinBoxWidth:
        box->setMinBoxWidth(panel->getDoubleValue(PropMinBoxWidth));
        break;
    case PropMaxBoxWidth:
        box->setMaxBoxWidth(panel->getDoubleValue(PropMaxBoxWidth));
        break;
    case PropPen:
        box->setPen(panel->getPenValue(PropPen));
        break;
    case PropBrush:
        box->setBrush(panel->getBrushValue(PropBrush));
        break;
    case PropWhiskerStyle: {
        int styleVal = panel->getEnumValue(PropWhiskerStyle);
        box->setWhiskerStyle(static_cast< QwtPlotBoxChart::WhiskerStyle >(styleVal));
        break;
    }
    case PropMedianVisible: {
        bool visible = panel->getBoolValue(PropMedianVisible);
        box->setMedianVisible(visible);
        // 可见性切换后更新画笔属性启用状态
        updateMedianPenEnabled(visible);
        break;
    }
    case PropMedianPen:
        box->setMedianPen(panel->getPenValue(PropMedianPen));
        break;
    case PropMeanVisible:
        box->setMeanVisible(panel->getBoolValue(PropMeanVisible));
        break;
    case PropOutlierJitter:
        box->setOutlierJitter(panel->getDoubleValue(PropOutlierJitter));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
