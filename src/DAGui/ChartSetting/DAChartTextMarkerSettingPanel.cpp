#include "DAChartTextMarkerSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAChartTextEditorPopup.h"
#include <QSignalBlocker>
#include <QPushButton>
#include <QToolButton>
#include "qwt_text.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartTextMarkerSettingPanel::DAChartTextMarkerSettingPanel(QWidget* parent) : DAChartItemSettingPanel(parent)
{
    connect(this,
            &DAChartItemSettingPanel::propertyValueChanged,
            this,
            &DAChartTextMarkerSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartTextMarkerSettingPanel::~DAChartTextMarkerSettingPanel()
{
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - Title: 字符串属性（树节点标题）
 * - ZValue: 双精度属性
 * - XAxis/YAxis: 坐标轴属性
 * - X/Y: 锚点数据坐标
 * - EditText: 富文本编辑入口按钮
 * - Font: 默认字体属性
 * - TextColor: 默认文字颜色属性
 * - LabelAlignment: 标签对齐属性（文本相对锚点位置）
 * - BorderRadius: 背景边框圆角
 * - BackgroundBrush: 背景画刷
 * - Spacing: 标签间距
 */
void DAChartTextMarkerSettingPanel::buildPropertyPanel()
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
    panel->addDoubleProperty(PropX, tr("Anchor X")  // cn:锚点X
                             ,
                             0.0,
                             -1e15,
                             1e15,
                             6);
    panel->addDoubleProperty(PropY, tr("Anchor Y")  // cn:锚点Y
                             ,
                             0.0,
                             -1e15,
                             1e15,
                             6);
    panel->endGroup();

    // 文本属性组
    panel->addCollapsibleGroup(tr("Text")  // cn:文本
    );
    {
        mEditTextButton = new QToolButton(this);
        mEditTextButton->setText(tr("Edit Rich Text...")  // cn:编辑富文本...
        );
        connect(mEditTextButton, &QToolButton::clicked, this, &DAChartTextMarkerSettingPanel::onEditTextButtonClicked);
        panel->addProperty(PropEditText, tr("Content")  // cn:内容
                           ,
                           mEditTextButton);
    }
    panel->addFontProperty(PropFont, tr("Font")  // cn:字体
    );
    panel->addColorProperty(PropTextColor, tr("Text Color")  // cn:文字颜色
    );
    panel->addAlignmentProperty(PropLabelAlignment, tr("Alignment")  // cn:对齐方式
    );
    panel->endGroup();

    // 背景属性组
    panel->addCollapsibleGroup(tr("Background")  // cn:背景
    );
    panel->addDoubleProperty(PropBorderRadius, tr("Border Radius")  // cn:边框圆角
                             ,
                             0.0,
                             0.0,
                             50.0,
                             3);
    panel->addBrushProperty(PropBackgroundBrush, tr("Background Brush")  // cn:背景画刷
    );
    panel->endGroup();

    // 布局属性组
    panel->addCollapsibleGroup(tr("Layout")  // cn:布局
    );
    panel->addIntProperty(PropSpacing, tr("Spacing")  // cn:间距
                          ,
                          0,
                          0,
                          100);
    panel->endGroup();
}

/**
 * @brief 从QwtPlotItem更新界面
 * @param item 图表项
 */
void DAChartTextMarkerSettingPanel::updateUI(QwtPlotItem* item)
{
    if (nullptr == item) {
        return;
    }
    if (item->rtti() != DAChartTextMarker::Rtti_TextMarker) {
        return;
    }

    DAChartTextMarker* marker = static_cast< DAChartTextMarker* >(item);
    auto panel                = propertyPanel();

    // 使用QSignalBlocker防止触发信号
    QSignalBlocker blocker(panel);

    // 基础属性
    panel->setStringValue(PropTitle, marker->title().text());
    panel->setDoubleValue(PropZValue, marker->z());
    setAxisValue(PropXAxis, static_cast< QwtAxis::Position >(marker->xAxis()));
    setAxisValue(PropYAxis, static_cast< QwtAxis::Position >(marker->yAxis()));
    panel->setDoubleValue(PropX, marker->xValue());
    panel->setDoubleValue(PropY, marker->yValue());

    // 文本属性(从QwtText提取)
    QwtText txt = marker->label();
    panel->setFontValue(PropFont, txt.font());
    panel->setColorValue(PropTextColor, txt.color());
    panel->setAlignmentValue(PropLabelAlignment, marker->labelAlignment());

    // 背景属性
    panel->setDoubleValue(PropBorderRadius, txt.borderRadius());
    panel->setBrushValue(PropBackgroundBrush, txt.backgroundBrush());

    // 布局属性
    panel->setIntValue(PropSpacing, marker->spacing());
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartTextMarkerSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAAbstractChartItemSettingWidget_ReturnWhenItemNull;
    DAChartTextMarker* marker = s_cast< DAChartTextMarker* >();
    if (nullptr == marker) {
        return;
    }

    auto panel = propertyPanel();

    // QwtText相关属性需先获取副本,修改后写回
    if (propertyId == PropFont || propertyId == PropTextColor || propertyId == PropBorderRadius
        || propertyId == PropBackgroundBrush) {
        QwtText txt = marker->label();
        switch (propertyId) {
        case PropFont:
            txt.setFont(panel->getFontValue(PropFont));
            break;
        case PropTextColor:
            txt.setColor(panel->getColorValue(PropTextColor));
            break;
        case PropBorderRadius:
            txt.setBorderRadius(panel->getDoubleValue(PropBorderRadius));
            break;
        case PropBackgroundBrush:
            txt.setBackgroundBrush(panel->getBrushValue(PropBackgroundBrush));
            break;
        default:
            break;
        }
        marker->setLabel(txt);
    } else {
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
        case PropX:
        case PropY:
            marker->setAnchorPosition(QPointF(panel->getDoubleValue(PropX), panel->getDoubleValue(PropY)));
            break;
        case PropLabelAlignment:
            marker->setLabelAlignment(static_cast< Qt::Alignment >(panel->getAlignmentValue(PropLabelAlignment)));
            break;
        case PropSpacing:
            marker->setSpacing(panel->getIntValue(PropSpacing));
            break;
        default:
            break;
        }
    }

    replot();
}

/**
 * @brief 富文本编辑按钮点击，弹窗编辑现有内容
 */
void DAChartTextMarkerSettingPanel::onEditTextButtonClicked()
{
    if (nullptr == getPlotItem()) {
        return;
    }
    if (getPlotItem()->rtti() != DAChartTextMarker::Rtti_TextMarker) {
        return;
    }
    DAChartTextMarker* marker = s_cast< DAChartTextMarker* >();
    if (nullptr == marker) {
        return;
    }
    DAChartTextEditorPopup* popup = new DAChartTextEditorPopup();
    connect(popup, &DAChartTextEditorPopup::accepted, this, [ this, popup, marker ]() {
        marker->setHtmlText(popup->toHtml());
        replot();
        popup->deleteLater();
    });
    connect(popup, &DAChartTextEditorPopup::rejected, popup, &QObject::deleteLater);
    popup->setHtml(marker->htmlText());
    // 在按钮附近弹出
    popup->popupAt(mEditTextButton->mapToGlobal(QPoint(mEditTextButton->width() / 2, mEditTextButton->height() / 2)));
}

/**
 * @brief 获取当前文本标注
 * @return 当前图元转换为DAChartTextMarker，类型不匹配返回nullptr
 */
DAChartTextMarker* DAChartTextMarkerSettingPanel::currentTextMarker() const
{
    QwtPlotItem* item = getPlotItem();
    if (nullptr == item) {
        return nullptr;
    }
    if (item->rtti() != DAChartTextMarker::Rtti_TextMarker) {
        return nullptr;
    }
    return static_cast< DAChartTextMarker* >(item);
}

}  // end namespace DA
