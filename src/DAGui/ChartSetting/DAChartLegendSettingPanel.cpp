#include "DAChartLegendSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QSignalBlocker>
#include "qwt_text.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartLegendSettingPanel::DAChartLegendSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartLegendSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartLegendSettingPanel::~DAChartLegendSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - Alignment: 对齐属性
 * - HorizontalOffset: 整数属性
 * - VerticalOffset: 整数属性
 * - Margin: 整数属性
 * - Spacing: 整数属性
 * - ItemMargin: 整数属性
 * - ItemSpacing: 整数属性
 * - MaxColumns: 整数属性
 * - BorderRadius: 双精度属性
 * - BorderPen: 笔属性
 * - Font: 字体属性
 * - FontColor: 颜色属性（从textPen获取）
 * - BackgroundBrush: 画刷属性
 */
void DAChartLegendSettingPanel::buildPropertyPanel()
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

    // 位置属性组
    panel->addCollapsibleGroup(tr("Position")  // cn:位置
    );
    panel->addAlignmentProperty(PropAlignment, tr("Alignment")  // cn:对齐
    );
    panel->addIntProperty(PropHorizontalOffset, tr("Horizontal Offset")  // cn:水平偏移
                          ,
                          0, -1000, 1000);
    panel->addIntProperty(PropVerticalOffset, tr("Vertical Offset")  // cn:垂直偏移
                          ,
                          0, -1000, 1000);
    panel->endGroup();

    // 间距属性组
    panel->addCollapsibleGroup(tr("Spacing")  // cn:间距
    );
    panel->addIntProperty(PropMargin, tr("Margin")  // cn:边距
                          ,
                          0, 0, 1000);
    panel->addIntProperty(PropSpacing, tr("Spacing")  // cn:间距
                          ,
                          0, 0, 1000);
    panel->addIntProperty(PropItemMargin, tr("Item Margin")  // cn:项边距
                          ,
                          0, 0, 1000);
    panel->addIntProperty(PropItemSpacing, tr("Item Spacing")  // cn:项间距
                          ,
                          0, 0, 1000);
    panel->addIntProperty(PropMaxColumns, tr("Max Columns")  // cn:最大列数
                          ,
                          0, 0, 100);
    panel->endGroup();

    // 外观属性组
    panel->addCollapsibleGroup(tr("Appearance")  // cn:外观
    );
    panel->addDoubleProperty(PropBorderRadius, tr("Border Radius")  // cn:边框圆角
                             ,
                             0.0, 0.0, 100.0);
    panel->addPenProperty(PropBorderPen, tr("Border Pen")  // cn:边框画笔
    );
    panel->addFontProperty(PropFont, tr("Font")  // cn:字体
    );
    panel->addColorProperty(PropFontColor, tr("Font Color")  // cn:字体颜色
    );
    panel->addBrushProperty(PropBackgroundBrush, tr("Background Brush")  // cn:背景画刷
    );
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartLegendSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotLegend) {
        return;
    }

    QwtPlotLegendItem* legend = static_cast< QwtPlotLegendItem* >(item);

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(propertyPanel());

    // 基础属性
    propertyPanel()->setStringValue(PropTitle, legend->title().text());
    propertyPanel()->setDoubleValue(PropZValue, legend->z());

    // 位置属性
    propertyPanel()->setAlignmentValue(PropAlignment, legend->alignmentInCanvas());
    propertyPanel()->setIntValue(PropHorizontalOffset, legend->offsetInCanvas(Qt::Horizontal));
    propertyPanel()->setIntValue(PropVerticalOffset, legend->offsetInCanvas(Qt::Vertical));

    // 间距属性
    propertyPanel()->setIntValue(PropMargin, legend->margin());
    propertyPanel()->setIntValue(PropSpacing, legend->spacing());
    propertyPanel()->setIntValue(PropItemMargin, legend->itemMargin());
    propertyPanel()->setIntValue(PropItemSpacing, legend->itemSpacing());
    propertyPanel()->setIntValue(PropMaxColumns, static_cast< int >(legend->maxColumns()));

    // 外观属性
    propertyPanel()->setDoubleValue(PropBorderRadius, legend->borderRadius());
    propertyPanel()->setPenValue(PropBorderPen, legend->borderPen());
    propertyPanel()->setFontValue(PropFont, legend->font());
    propertyPanel()->setColorValue(PropFontColor, legend->textPen().color());
    propertyPanel()->setBrushValue(PropBackgroundBrush, legend->backgroundBrush());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartLegendSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotLegendItem* legend = s_cast< QwtPlotLegendItem* >();
    if (nullptr == legend) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PropTitle:
        legend->setTitle(panel->getStringValue(PropTitle));
        break;
    case PropZValue:
        legend->setZ(panel->getDoubleValue(PropZValue));
        break;
    case PropAlignment: {
        Qt::Alignment al = panel->getAlignmentValue(PropAlignment);
        legend->setAlignmentInCanvas(al);
        break;
    }
    case PropHorizontalOffset:
        legend->setOffsetInCanvas(Qt::Horizontal, panel->getIntValue(PropHorizontalOffset));
        break;
    case PropVerticalOffset:
        legend->setOffsetInCanvas(Qt::Vertical, panel->getIntValue(PropVerticalOffset));
        break;
    case PropMargin:
        legend->setMargin(panel->getIntValue(PropMargin));
        break;
    case PropSpacing:
        legend->setSpacing(panel->getIntValue(PropSpacing));
        break;
    case PropItemMargin:
        legend->setItemMargin(panel->getIntValue(PropItemMargin));
        break;
    case PropItemSpacing:
        legend->setItemSpacing(panel->getIntValue(PropItemSpacing));
        break;
    case PropMaxColumns: {
        unsigned int maxCol = static_cast< unsigned int >(panel->getIntValue(PropMaxColumns));
        legend->setMaxColumns(maxCol);
        break;
    }
    case PropBorderRadius:
        legend->setBorderRadius(panel->getDoubleValue(PropBorderRadius));
        break;
    case PropBorderPen:
        legend->setBorderPen(panel->getPenValue(PropBorderPen));
        break;
    case PropFont:
        legend->setFont(panel->getFontValue(PropFont));
        break;
    case PropFontColor: {
        QColor fc = panel->getColorValue(PropFontColor);
        QPen tp   = legend->textPen();
        tp.setColor(fc);
        legend->setTextPen(tp);
        break;
    }
    case PropBackgroundBrush:
        legend->setBackgroundBrush(panel->getBrushValue(PropBackgroundBrush));
        break;
    default:
        break;
    }

    replot();
}

}  // end namespace DA
