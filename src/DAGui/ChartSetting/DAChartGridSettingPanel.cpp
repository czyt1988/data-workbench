#include "DAChartGridSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include <QSignalBlocker>
#include "qwt_text.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartGridSettingPanel::DAChartGridSettingPanel(QWidget* parent)
    : DAChartItemSettingPanel(parent)
{
    // 连接属性值变化信号
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &DAChartGridSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartGridSettingPanel::~DAChartGridSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - XAxis: 坐标轴属性(QComBoX)
 * - YAxis: 坐标轴属性(QComBoX)
 * - MajorPen: 笔属性
 * - MinorPen: 笔属性
 */
void DAChartGridSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    // 基础属性组
    panel->addCollapsibleGroup(tr("Basic"));
    panel->addStringProperty(PropTitle, tr("Title"));
    panel->addDoubleProperty(PropZValue, tr("Z Value"));

    // 坐标轴属性组
    panel->addCollapsibleGroup(tr("Axis"));
    addAxisProperty(PropXAxis, tr("X Axis"), false);
    addAxisProperty(PropYAxis, tr("Y Axis"), true);

    // 线条样式属性组
    panel->addCollapsibleGroup(tr("Line Style"));
    panel->addPenProperty(PropMajorPen, tr("Major Pen"));
    panel->addPenProperty(PropMinorPen, tr("Minor Pen"));
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartGridSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotGrid) {
        return;
    }

    QwtPlotGrid* grid = static_cast< QwtPlotGrid* >(item);

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(propertyPanel());

    // 基础属性
    propertyPanel()->setStringValue(PropTitle, grid->title().text());
    propertyPanel()->setDoubleValue(PropZValue, grid->z());

    // 坐标轴属性
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(grid->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(grid->yAxis()));

    // 笔属性
    propertyPanel()->setPenValue(PropMajorPen, grid->majorPen());
    propertyPanel()->setPenValue(PropMinorPen, grid->minorPen());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartGridSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotGrid* grid = s_cast< QwtPlotGrid* >();
    if (nullptr == grid) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        grid->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        grid->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        grid->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        grid->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropMajorPen:
        grid->setMajorPen(panel->getPenValue(PropMajorPen));
        break;
    case PropMinorPen:
        grid->setMinorPen(panel->getPenValue(PropMinorPen));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
