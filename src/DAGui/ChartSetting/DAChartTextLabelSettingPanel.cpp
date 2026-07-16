#include "DAChartTextLabelSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartTextLabelSettingPanel::DAChartTextLabelSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this, &DAChartItemSettingPanel::propertyValueChanged, this, &DAChartTextLabelSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartTextLabelSettingPanel::~DAChartTextLabelSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性
 * - ZValue: 双精度属性
 * - XAxis/YAxis: 坐标轴属性
 * - Text: 字符串属性
 * - Font: 字体属性
 * - TextColor: 颜色属性
 * - TextAlignment: 对齐属性
 * - Margin: 整数属性
 * - BorderRadius: 双精度属性(0.0-50.0)
 * - BackgroundBrush: 画刷属性
 */
void DAChartTextLabelSettingPanel::buildPropertyPanel()
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

    // 文本属性组
    panel->addCollapsibleGroup(tr("Text")  // cn:文本
    );
    panel->addStringProperty(PropText, tr("Text Content")  // cn:文本内容
    );
    panel->addFontProperty(PropFont, tr("Font")  // cn:字体
    );
    panel->addColorProperty(PropTextColor, tr("Text Color")  // cn:文字颜色
    );
    panel->addAlignmentProperty(PropTextAlignment, tr("Alignment")  // cn:对齐方式
    );
    panel->addIntProperty(PropMargin, tr("Margin")  // cn:外边距
                          ,
                          0, 0, 1000);
    panel->endGroup();

    // 背景属性组
    panel->addCollapsibleGroup(tr("Background")  // cn:背景
    );
    panel->addDoubleProperty(PropBorderRadius, tr("Border Radius")  // cn:边框圆角
                             ,
                             0.0, 0.0, 50.0, 3);
    panel->addBrushProperty(PropBackgroundBrush, tr("Background Brush")  // cn:背景画刷
    );
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartTextLabelSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != QwtPlotItem::Rtti_PlotTextLabel) {
        return;
    }

    QwtPlotTextLabel* label = static_cast< QwtPlotTextLabel* >(item);
    auto panel              = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, label->title().text());
    panel->setDoubleValue(PropZValue, label->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(label->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(label->yAxis()));

    // 文本属性(从QwtText提取)
    QwtText txt = label->text();
    panel->setStringValue(PropText, txt.text());
    panel->setFontValue(PropFont, txt.font());
    panel->setColorValue(PropTextColor, txt.color());
    panel->setAlignmentValue(PropTextAlignment, static_cast< Qt::Alignment >(txt.renderFlags()));
    panel->setIntValue(PropMargin, label->margin());

    // 背景属性
    panel->setDoubleValue(PropBorderRadius, txt.borderRadius());
    panel->setBrushValue(PropBackgroundBrush, txt.backgroundBrush());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartTextLabelSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    QwtPlotTextLabel* label = s_cast< QwtPlotTextLabel* >();
    if (nullptr == label) {
        return;
    }

    auto panel = propertyPanel();

    // QwtText相关属性需先获取副本,修改后写回
    if (propertyId == PropFont || propertyId == PropTextColor || propertyId == PropTextAlignment
        || propertyId == PropBorderRadius || propertyId == PropBackgroundBrush || propertyId == PropText) {
        QwtText txt = label->text();
        switch (propertyId) {
        case PropText:
            txt.setText(panel->getStringValue(PropText));
            break;
        case PropFont:
            txt.setFont(panel->getFontValue(PropFont));
            break;
        case PropTextColor:
            txt.setColor(panel->getColorValue(PropTextColor));
            break;
        case PropTextAlignment: {
            // 保留非对齐位(如 Qt::TextWordWrap),仅覆盖水平/垂直对齐
            int flags = txt.renderFlags();
            flags &= ~(Qt::AlignHorizontal_Mask | Qt::AlignVertical_Mask);
            flags |= int(panel->getAlignmentValue(PropTextAlignment));
            txt.setRenderFlags(flags);
            break;
        }
        case PropBorderRadius:
            txt.setBorderRadius(panel->getDoubleValue(PropBorderRadius));
            break;
        case PropBackgroundBrush:
            txt.setBackgroundBrush(panel->getBrushValue(PropBackgroundBrush));
            break;
        default:
            break;
        }
        label->setText(txt);
    } else {
        switch (propertyId) {
        case PropTitle:
            label->setTitle(panel->getStringValue(PropTitle));
            break;
        case PropZValue:
            label->setZ(panel->getDoubleValue(PropZValue));
            break;
        case PropXAxis:
            label->setXAxis(static_cast< QwtAxisId >(getAxisValue(PropXAxis)));
            break;
        case PropYAxis:
            label->setYAxis(static_cast< QwtAxisId >(getAxisValue(PropYAxis)));
            break;
        case PropMargin:
            label->setMargin(panel->getIntValue(PropMargin));
            break;
        default:
            break;
        }
    }

    replot();
}

}  // end namespace DA
