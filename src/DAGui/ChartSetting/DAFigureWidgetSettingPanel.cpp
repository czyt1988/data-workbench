#include "DAFigureWidgetSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAFigureWidget.h"
#include "qwt_figure.h"
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAFigureWidgetSettingPanel::DAFigureWidgetSettingPanel(QWidget* parent)
    : QWidget(parent)
    , mPanel(nullptr)
{
    // 创建DAPropertyPanelWidget并设为自身主布局
    mPanel = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    // 连接propertyValueChanged信号
    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged,
            this, &DAFigureWidgetSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAFigureWidgetSettingPanel::propertyValueChanged, this, &DAFigureWidgetSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAFigureWidgetSettingPanel::~DAFigureWidgetSettingPanel()
{
}

/**
 * @brief 获取通用属性面板指针
 * @return DAPropertyPanelWidget指针
 */
DAPropertyPanelContainerWidget* DAFigureWidgetSettingPanel::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 设置目标DAFigureWidget
 * @param fig 目标Figure
 */
void DAFigureWidgetSettingPanel::setTarget(DAFigureWidget* fig)
{
    if (mFigure == fig) {
        return;
    }
    mFigure = fig;
    updateUI();
}

/**
 * @brief 获取目标DAFigureWidget
 * @return DAFigureWidget指针
 */
DAFigureWidget* DAFigureWidgetSettingPanel::target() const
{
    return mFigure.data();
}

/**
 * @brief 更新界面显示
 *
 * 从DAFigureWidget和QwtFigure读取尺寸和背景状态写入面板。
 */
void DAFigureWidgetSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    QwtFigure* qwtfig = nullptr;
    if (mFigure) {
        qwtfig = mFigure->figure();
    }

    if (qwtfig) {
        mPanel->setIntValue(PID_MinWidth, qwtfig->minimumWidth());
        mPanel->setIntValue(PID_MinHeight, qwtfig->minimumHeight());
        mPanel->setIntValue(PID_MaxWidth, qwtfig->maximumWidth());
        mPanel->setIntValue(PID_MaxHeight, qwtfig->maximumHeight());
    } else {
        mPanel->setIntValue(PID_MinWidth, 0);
        mPanel->setIntValue(PID_MinHeight, 0);
        mPanel->setIntValue(PID_MaxWidth, 0);
        mPanel->setIntValue(PID_MaxHeight, 0);
    }

    if (mFigure) {
        mPanel->setBrushValue(PID_BackgroundBrush, mFigure->getFaceBrush());
    } else {
        mPanel->setBrushValue(PID_BackgroundBrush, QBrush());
    }
}

/**
 * @brief 触发重绘
 */
void DAFigureWidgetSettingPanel::replot()
{
    if (mFigure) {
        mFigure->update();
    }
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - 尺寸组: 最小宽度、最小高度、最大宽度、最大高度
 * - 背景组: 背景画刷
 */
void DAFigureWidgetSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addCollapsibleGroup(tr("Size"));
    panel->addIntProperty(PID_MinWidth, tr("Min Width"), 0, 0, 999999999);
    panel->addIntProperty(PID_MinHeight, tr("Min Height"), 0, 0, 999999999);
    panel->addIntProperty(PID_MaxWidth, tr("Max Width"), 0, 0, 999999999);
    panel->addIntProperty(PID_MaxHeight, tr("Max Height"), 0, 0, 999999999);

    panel->addCollapsibleGroup(tr("Background"));
    panel->addBrushProperty(PID_BackgroundBrush, tr("Background Brush"));
}

/**
 * @brief 转发属性值变化信号
 * @param propertyId 属性ID
 */
void DAFigureWidgetSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAFigureWidgetSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mFigure) {
        return;
    }

    QwtFigure* qwtfig = mFigure->figure();
    auto panel = propertyPanel();

    switch (propertyId) {
    case PID_MinWidth: {
        if (qwtfig) {
            qwtfig->setMinimumWidth(panel->getIntValue(PID_MinWidth));
        }
        break;
    }
    case PID_MinHeight: {
        if (qwtfig) {
            qwtfig->setMinimumHeight(panel->getIntValue(PID_MinHeight));
        }
        break;
    }
    case PID_MaxWidth: {
        if (qwtfig) {
            qwtfig->setMaximumWidth(panel->getIntValue(PID_MaxWidth));
        }
        break;
    }
    case PID_MaxHeight: {
        if (qwtfig) {
            qwtfig->setMaximumHeight(panel->getIntValue(PID_MaxHeight));
        }
        break;
    }
    case PID_BackgroundBrush: {
        mFigure->setFaceBrush(panel->getBrushValue(PID_BackgroundBrush));
        break;
    }
    default:
        break;
    }

    replot();
}

}  // end namespace DA
