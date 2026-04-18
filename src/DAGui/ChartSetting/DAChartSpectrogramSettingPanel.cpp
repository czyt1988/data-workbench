#include "DAChartSpectrogramSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_color_map.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartSpectrogramSettingPanel::DAChartSpectrogramSettingPanel(QWidget* parent)
    : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged,
            this, &DAChartSpectrogramSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartSpectrogramSettingPanel::~DAChartSpectrogramSettingPanel()
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
 * - DisplayMode: 枚举属性(ImageMode=1/ContourMode=2)
 * - FromColor: 颜色属性（线性颜色映射的起始颜色）
 * - ToColor: 颜色属性（线性颜色映射的结束颜色）
 * - ContourPen: 笔属性（仅ContourMode时启用）
 */
void DAChartSpectrogramSettingPanel::buildPropertyPanel()
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

    // 显示属性组
    panel->addCollapsibleGroup(tr("Display"));
    // QwtPlotSpectrogram::DisplayMode: ImageMode=1, ContourMode=2
    panel->addEnumProperty(PropDisplayMode, tr("Display Mode"),
                           QStringList() << tr("Image Mode") << tr("Contour Mode"),
                           QList< int >() << static_cast< int >(QwtPlotSpectrogram::ImageMode)
                                                << static_cast< int >(QwtPlotSpectrogram::ContourMode));

    // 颜色属性组
    panel->addCollapsibleGroup(tr("Color"));
    panel->addColorProperty(PropFromColor, tr("From Color"));
    panel->addColorProperty(PropToColor, tr("To Color"));

    // 轮廓线属性组
    panel->addCollapsibleGroup(tr("Contour"));
    panel->addPenProperty(PropContourPen, tr("Contour Pen"));
    panel->setPropertyEnabled(PropContourPen, false);
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartSpectrogramSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotSpectrogram) {
        return;
    }

    QwtPlotSpectrogram* spectrogram = static_cast< QwtPlotSpectrogram* >(item);
    auto panel = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, spectrogram->title().text());
    panel->setDoubleValue(PropZValue, spectrogram->z());

    // 坐标轴属性
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(spectrogram->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(spectrogram->yAxis()));

    // 显示模式
    bool isContour = spectrogram->testDisplayMode(QwtPlotSpectrogram::ContourMode);
    bool isImage = spectrogram->testDisplayMode(QwtPlotSpectrogram::ImageMode);
    int displayModeIndex = isContour ? 1 : (isImage ? 0 : -1);
    panel->setEnumValue(PropDisplayMode, displayModeIndex);

    // 更新轮廓线笔的启用状态
    panel->setPropertyEnabled(PropContourPen, isContour);
    panel->setPenValue(PropContourPen, spectrogram->defaultContourPen());

    // 颜色属性（从colorMap中提取）
    const QwtColorMap* currentMap = spectrogram->colorMap();
    const QwtLinearColorMap* linearMap = dynamic_cast< const QwtLinearColorMap* >(currentMap);
    if (linearMap) {
        panel->setColorValue(PropFromColor, linearMap->color1());
        panel->setColorValue(PropToColor, linearMap->color2());
    } else {
        panel->setColorValue(PropFromColor, Qt::blue);
        panel->setColorValue(PropToColor, Qt::red);
    }
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartSpectrogramSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotSpectrogram* spectrogram = s_cast< QwtPlotSpectrogram* >();
    if (nullptr == spectrogram) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        spectrogram->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        spectrogram->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropXAxis:
        spectrogram->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
        break;
    case PropYAxis:
        spectrogram->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
        break;
    case PropDisplayMode: {
        int modeVal = panel->getEnumValue(PropDisplayMode);
        // 先关闭所有模式，再设置选中的模式
        spectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, false);
        spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, false);
        if (modeVal == 0) {
            spectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
        } else if (modeVal == 1) {
            spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, true);
        }
        // 根据显示模式更新轮廓线笔的启用状态
        panel->setPropertyEnabled(PropContourPen, (modeVal == 1));
        break;
    }
    case PropFromColor:
    case PropToColor: {
        QColor fromColor = panel->getColorValue(PropFromColor);
        QColor toColor = panel->getColorValue(PropToColor);
        spectrogram->setColorMap(new QwtLinearColorMap(fromColor, toColor));
        break;
    }
    case PropContourPen:
        spectrogram->setDefaultContourPen(panel->getPenValue(PropContourPen));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
