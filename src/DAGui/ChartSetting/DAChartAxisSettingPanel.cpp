#include "DAChartAxisSettingPanel.h"
#include "DAPropertyPanelWidget.h"
#include "DAChartUtil.h"
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_draw.h"
#include "qwt_date_scale_draw.h"
#include <QSignalBlocker>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "DASignalBlockers.hpp"

namespace DA
{

/**
 * @brief 构造函数
 * @param axisId 坐标轴ID
 * @param parent 父控件
 */
DAChartAxisSettingPanel::DAChartAxisSettingPanel(QwtAxis::Position axisId, QWidget* parent)
    : QWidget(parent)
    , mPanel(nullptr)
    , mPlot(nullptr)
    , mAxisId(axisId)
    , mScaleStyleButtonGroup(nullptr)
{
    // 创建DAPropertyPanelWidget并设为自身主布局
    mPanel = new DAPropertyPanelWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    // 连接propertyValueChanged信号
    connect(mPanel, &DAPropertyPanelWidget::propertyValueChanged,
            this, &DAChartAxisSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChartAxisSettingPanel::propertyValueChanged, this, &DAChartAxisSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartAxisSettingPanel::~DAChartAxisSettingPanel()
{
}

/**
 * @brief 获取通用属性面板指针
 * @return DAPropertyPanelWidget指针
 */
DAPropertyPanelWidget* DAChartAxisSettingPanel::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 设置目标QwtPlot
 * @param plot 目标图表
 */
void DAChartAxisSettingPanel::setTarget(QwtPlot* plot)
{
    if (mPlot == plot) {
        return;
    }
    mPlot = plot;
    updateUI();
}

/**
 * @brief 获取目标QwtPlot
 * @return QwtPlot指针
 */
QwtPlot* DAChartAxisSettingPanel::target() const
{
    return mPlot.data();
}

/**
 * @brief 获取当前坐标轴ID
 * @return 坐标轴位置
 */
QwtAxis::Position DAChartAxisSettingPanel::axisId() const
{
    return mAxisId;
}

/**
 * @brief 更新界面显示
 *
 * 从QwtPlot和QwtScaleWidget读取坐标轴状态写入面板。
 */
void DAChartAxisSettingPanel::updateUI()
{
    DASignalBlockers block(mPanel);

    if (!mPlot) {
        mPanel->setBoolValue(PID_EnableAxis, false);
        mPanel->setStringValue(PID_LabelText, QString());
        mPanel->setFontValue(PID_LabelFont, QFont());
        mPanel->setColorValue(PID_LabelFontColor, Qt::black);
        mPanel->setDoubleValue(PID_MinScale, 0.0);
        mPanel->setDoubleValue(PID_MaxScale, 0.0);
        mPanel->setIntValue(PID_Margin, 0);
        mPanel->setDoubleValue(PID_LabelRotation, 0.0);
        return;
    }

    QwtScaleWidget* scaleWidget = mPlot->axisWidget(mAxisId);

    mPanel->setBoolValue(PID_EnableAxis, mPlot->axisEnabled(mAxisId));
    mPanel->setStringValue(PID_LabelText, mPlot->axisTitle(mAxisId).text());
    mPanel->setFontValue(PID_LabelFont, mPlot->axisFont(mAxisId));

    if (scaleWidget) {
        mPanel->setColorValue(PID_LabelFontColor, scaleWidget->textColor());
    } else {
        mPanel->setColorValue(PID_LabelFontColor, Qt::black);
    }

    // 刻度范围
    QwtInterval inv = mPlot->axisInterval(mAxisId);
    mPanel->setDoubleValue(PID_MinScale, inv.minValue());
    mPanel->setDoubleValue(PID_MaxScale, inv.maxValue());

    if (scaleWidget) {
        // 边距
        mPanel->setIntValue(PID_Margin, scaleWidget->margin());

        // 对齐和旋转
        QwtScaleDraw* sd = scaleWidget->scaleDraw();
        if (sd) {
            mPanel->setDoubleValue(PID_LabelRotation, sd->labelRotation());
        }
    }

    // 刻度样式
    bool isDateTime = false;
    if (scaleWidget && scaleWidget->scaleDraw()) {
        isDateTime = (dynamic_cast< QwtDateScaleDraw* >(scaleWidget->scaleDraw()) != nullptr);
    }
    setScaleStyleValue(isDateTime ? DateTimeScale : NormalScale);
}

/**
 * @brief 触发重绘
 */
void DAChartAxisSettingPanel::replot()
{
    if (mPlot) {
        mPlot->replot();
    }
}

/**
 * @brief 设置刻度样式值
 * @param style 刻度样式
 */
void DAChartAxisSettingPanel::setScaleStyleValue(int style)
{
    if (!mScaleStyleButtonGroup) {
        return;
    }
    bool wasBlocked = mScaleStyleButtonGroup->blockSignals(true);
    auto buttons = mScaleStyleButtonGroup->buttons();
    for (auto btn : buttons) {
        int btnId = mScaleStyleButtonGroup->id(btn);
        if (btnId == style) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            // In Qt5, QAbstractButton::setChecked is available
            btn->setChecked(true);
#else
            btn->setChecked(true);
#endif
            break;
        }
    }
    mScaleStyleButtonGroup->blockSignals(wasBlocked);
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - 启用组: 坐标轴启用开关
 * - 标签组: 标签文本、字体、颜色、对齐、旋转
 * - 刻度组: 边距、最小刻度、最大刻度、刻度样式
 */
void DAChartAxisSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addGroupLabel(tr("Enable"));
    panel->addBoolProperty(PID_EnableAxis, tr("Enable Axis"));

    panel->addGroupLabel(tr("Label"));
    panel->addStringProperty(PID_LabelText, tr("Label Text"));
    panel->addFontProperty(PID_LabelFont, tr("Label Font"));
    panel->addColorProperty(PID_LabelFontColor, tr("Label Font Color"));
    panel->addAlignmentProperty(PID_LabelAlignment, tr("Label Alignment"));
    panel->addDoubleProperty(PID_LabelRotation, tr("Label Rotation"), 0.0, -360.0, 360.0, 1);

    panel->addGroupLabel(tr("Scale"));
    panel->addIntProperty(PID_Margin, tr("Margin"), 0, -20, 100);
    panel->addDoubleProperty(PID_MinScale, tr("Min Scale"), 0.0, -1e15, 1e15, 5);
    panel->addDoubleProperty(PID_MaxScale, tr("Max Scale"), 0.0, -1e15, 1e15, 5);

    // 刻度样式: Normal/DateTime 两个RadioButton
    QWidget* scaleStyleContainer = new QWidget(this);
    QHBoxLayout* hLayout = new QHBoxLayout(scaleStyleContainer);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(8);

    QRadioButton* rbNormal = new QRadioButton(tr("Normal"), scaleStyleContainer);
    QRadioButton* rbDateTime = new QRadioButton(tr("DateTime"), scaleStyleContainer);
    rbNormal->setChecked(true);

    mScaleStyleButtonGroup = new QButtonGroup(scaleStyleContainer);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mScaleStyleButtonGroup->addButton(rbNormal, NormalScale);
    mScaleStyleButtonGroup->addButton(rbDateTime, DateTimeScale);
#else
    mScaleStyleButtonGroup->setId(rbNormal, NormalScale);
    mScaleStyleButtonGroup->setId(rbDateTime, DateTimeScale);
    mScaleStyleButtonGroup->addButton(rbNormal);
    mScaleStyleButtonGroup->addButton(rbDateTime);
#endif

    hLayout->addWidget(rbNormal);
    hLayout->addWidget(rbDateTime);
    hLayout->addStretch();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(mScaleStyleButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int) {
        onPanelPropertyValueChanged(PID_ScaleStyle);
    });
#else
    connect(mScaleStyleButtonGroup, &QButtonGroup::idClicked, this, [this](int) {
        onPanelPropertyValueChanged(PID_ScaleStyle);
    });
#endif

    panel->addProperty(PID_ScaleStyle, tr("Scale Style"), scaleStyleContainer);
}

/**
 * @brief 转发属性值变化信号
 * @param propertyId 属性ID
 */
void DAChartAxisSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartAxisSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mPlot) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PID_EnableAxis: {
        bool enabled = panel->getBoolValue(PID_EnableAxis);
        mPlot->enableAxis(mAxisId, enabled);
        break;
    }
    case PID_LabelText: {
        QString text = panel->getStringValue(PID_LabelText);
        QwtText title = mPlot->axisTitle(mAxisId);
        title.setText(text);
        mPlot->setAxisTitle(mAxisId, title);
        break;
    }
    case PID_LabelFont: {
        QFont font = panel->getFontValue(PID_LabelFont);
        mPlot->setAxisFont(mAxisId, font);
        break;
    }
    case PID_LabelFontColor: {
        QColor color = panel->getColorValue(PID_LabelFontColor);
        QwtScaleWidget* scaleWidget = mPlot->axisWidget(mAxisId);
        if (scaleWidget) {
            scaleWidget->setTextColor(color);
        }
        break;
    }
    case PID_LabelAlignment: {
        Qt::Alignment alignment = panel->getAlignmentValue(PID_LabelAlignment);
        QwtScaleWidget* scaleWidget = mPlot->axisWidget(mAxisId);
        if (scaleWidget && scaleWidget->scaleDraw()) {
            scaleWidget->scaleDraw()->setLabelAlignment(alignment);
        }
        break;
    }
    case PID_LabelRotation: {
        double rotation = panel->getDoubleValue(PID_LabelRotation);
        QwtScaleWidget* scaleWidget = mPlot->axisWidget(mAxisId);
        if (scaleWidget && scaleWidget->scaleDraw()) {
            scaleWidget->scaleDraw()->setLabelRotation(rotation);
        }
        break;
    }
    case PID_Margin: {
        int margin = panel->getIntValue(PID_Margin);
        QwtScaleWidget* scaleWidget = mPlot->axisWidget(mAxisId);
        if (scaleWidget) {
            scaleWidget->setMargin(margin);
        }
        break;
    }
    case PID_MinScale: {
        QwtInterval inv = mPlot->axisInterval(mAxisId);
        double min = panel->getDoubleValue(PID_MinScale);
        mPlot->setAxisScale(mAxisId, min, inv.maxValue());
        break;
    }
    case PID_MaxScale: {
        QwtInterval inv = mPlot->axisInterval(mAxisId);
        double max = panel->getDoubleValue(PID_MaxScale);
        mPlot->setAxisScale(mAxisId, inv.minValue(), max);
        break;
    }
    case PID_ScaleStyle: {
        if (mScaleStyleButtonGroup) {
            int style = mScaleStyleButtonGroup->checkedId();
            if (style == DateTimeScale) {
                // DateTime刻度 - 使用默认格式（可扩展）
                DAChartUtil::setAxisDateTimeScale(mPlot, mAxisId, "yyyy-MM-dd");
            } else {
                // 设置普通刻度
                DAChartUtil::setAxisNormalScale(mPlot, mAxisId);
            }
        }
        break;
    }
    default:
        break;
    }

    replot();
}

}  // end namespace DA
