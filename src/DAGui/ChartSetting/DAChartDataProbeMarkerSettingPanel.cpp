#include "DAChartDataProbeMarkerSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include "qwt_text.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartDataProbeMarkerSettingPanel::DAChartDataProbeMarkerSettingPanel(QWidget* parent)
    : DAChartItemSettingPanel(parent)
{
    connect(this,
            &DAChartItemSettingPanel::propertyValueChanged,
            this,
            &DAChartDataProbeMarkerSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartDataProbeMarkerSettingPanel::~DAChartDataProbeMarkerSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性（探针名称）
 * - ZValue: 双精度属性
 * - ProbeValue: 双精度属性（探针位置值，垂直探针为X值，水平探针为Y值）
 * - LabelVisible: 布尔属性（标签是否可见）
 * - LabelPosition: 枚举属性（标签位置：上/左 或 下/右）
 * - LabelStyle: 枚举属性（标签样式：纯文本、圆角矩形、矩形、椭圆）
 * - ProbeColor: 颜色属性（探针颜色）
 */
void DAChartDataProbeMarkerSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    // 基础属性组
    panel->addCollapsibleGroup(tr("Basic")  // cn:基础
    );
    panel->addStringProperty(PropTitle, tr("Title")  // cn:标题
    );
    panel->addDoubleProperty(PropZValue, tr("Z Value")  // cn:Z值
    );
    panel->addDoubleProperty(PropProbeValue,
                             tr("Probe Value")  // cn:探针值
                             ,
                             0.0,
                             -1e15,
                             1e15,
                             6);
    panel->endGroup();

    // 标签属性组
    panel->addCollapsibleGroup(tr("Label")  // cn:标签
    );
    panel->addBoolProperty(PropLabelVisible, tr("Label Visible")  // cn:标签可见
    );
    panel->addEnumProperty(PropLabelPosition,
                           tr("Label Position")  // cn:标签位置
                           ,
                           QStringList() << tr("Top / Left")      // cn:上/左
                                         << tr("Bottom / Right")  // cn:下/右
                           ,
                           QList< int >() << 0 << 1);
    panel->addEnumProperty(PropLabelStyle,
                           tr("Label Style")  // cn:标签样式
                           ,
                           QStringList() << tr("Plain Text")    // cn:纯文本
                                         << tr("Rounded Rect")  // cn:圆角矩形
                                         << tr("Rectangle")     // cn:矩形
                                         << tr("Ellipse")       // cn:椭圆
                           ,
                           QList< int >() << 0 << 1 << 2 << 3);
    panel->endGroup();

    // 外观属性组
    panel->addCollapsibleGroup(tr("Appearance")  // cn:外观
    );
    panel->addColorProperty(PropProbeColor, tr("Probe Color")  // cn:探针颜色
    );
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartDataProbeMarkerSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != DADataProbeMarker::Rtti_DataProbeMarker) {
        return;
    }

    DADataProbeMarker* probe = static_cast< DADataProbeMarker* >(item);

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(propertyPanel());

    // 基础属性
    propertyPanel()->setStringValue(PropTitle, probe->probeName().text());
    propertyPanel()->setDoubleValue(PropZValue, probe->z());
    propertyPanel()->setDoubleValue(PropProbeValue, probe->probeValue());

    // 标签属性
    propertyPanel()->setBoolValue(PropLabelVisible, probe->isLabelVisible());
    propertyPanel()->setEnumValue(PropLabelPosition, static_cast< int >(probe->labelPosition()));
    propertyPanel()->setEnumValue(PropLabelStyle, static_cast< int >(probe->labelStyle()));

    // 外观属性
    propertyPanel()->setColorValue(PropProbeColor, probe->probeColor());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartDataProbeMarkerSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    DADataProbeMarker* probe = s_cast< DADataProbeMarker* >();
    if (nullptr == probe) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        probe->setProbeName(QwtText(panel->getStringValue(PropTitle)));
        break;
    case PropZValue:
        probe->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropProbeValue:
        probe->setProbeValue(panel->getDoubleValue(PropProbeValue));
        break;
    case PropLabelPosition:
        probe->setLabelPosition(static_cast< DADataProbeMarker::LabelPosition >(panel->getEnumValue(PropLabelPosition)));
        break;
    case PropLabelStyle:
        probe->setLabelStyle(static_cast< DADataProbeMarker::LabelStyle >(panel->getEnumValue(PropLabelStyle)));
        break;
    case PropLabelVisible:
        probe->setLabelVisible(panel->getBoolValue(PropLabelVisible));
        break;
    case PropProbeColor:
        probe->setProbeColor(panel->getColorValue(PropProbeColor));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
