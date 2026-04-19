#include "DAChartPlotSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "qwt_text.h"
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartPlotSettingPanel::DAChartPlotSettingPanel(QWidget* parent) : QWidget(parent), mPanel(nullptr)
{
    // 创建DAPropertyPanelWidget并设为自身主布局
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    // 连接propertyValueChanged信号
    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged, this, &DAChartPlotSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChartPlotSettingPanel::propertyValueChanged, this, &DAChartPlotSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartPlotSettingPanel::~DAChartPlotSettingPanel()
{
}

/**
 * @brief 获取通用属性面板指针
 * @return DAPropertyPanelWidget指针
 */
DAPropertyPanelContainerWidget* DAChartPlotSettingPanel::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 设置目标QwtPlot
 * @param plot 目标图表
 */
void DAChartPlotSettingPanel::setTarget(QwtPlot* plot)
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
QwtPlot* DAChartPlotSettingPanel::target() const
{
    return mPlot.data();
}

/**
 * @brief 更新界面显示
 *
 * 从QwtPlot读取标题、脚注等状态写入面板。
 */
void DAChartPlotSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    QwtText titleText;
    QwtText footerText;
    if (mPlot) {
        titleText  = mPlot->title();
        footerText = mPlot->footer();
    }

    mPanel->setStringValue(PID_TitleText, titleText.text());
    mPanel->setFontValue(PID_TitleFont, titleText.font());
    mPanel->setColorValue(PID_TitleColor, titleText.color());

    mPanel->setStringValue(PID_FooterText, footerText.text());
    mPanel->setFontValue(PID_FooterFont, footerText.font());
    mPanel->setColorValue(PID_FooterColor, footerText.color());
}

/**
 * @brief 触发重绘
 */
void DAChartPlotSettingPanel::replot()
{
    if (mPlot) {
        mPlot->replot();
    }
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - 标题组: 标题文本、标题字体、标题颜色
 * - 脚注组: 脚注文本、脚注字体、脚注颜色
 */
void DAChartPlotSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addCollapsibleGroup(tr("Title"));
    panel->addStringProperty(PID_TitleText, tr("Title Text"));
    panel->addFontProperty(PID_TitleFont, tr("Title Font"));
    panel->addColorProperty(PID_TitleColor, tr("Title Color"));
    panel->endGroup();

    panel->addCollapsibleGroup(tr("Footer"));
    panel->addStringProperty(PID_FooterText, tr("Footer Text"));
    panel->addFontProperty(PID_FooterFont, tr("Footer Font"));
    panel->addColorProperty(PID_FooterColor, tr("Footer Color"));
    panel->endGroup();
}

/**
 * @brief 转发属性值变化信号
 * @param propertyId 属性ID
 */
void DAChartPlotSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartPlotSettingPanel::onPropertyValueChanged(int propertyId)
{
    if (!mPlot) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PID_TitleText: {
        QwtText tt = mPlot->title();
        tt.setText(panel->getStringValue(PID_TitleText));
        mPlot->setTitle(tt);
        break;
    }
    case PID_TitleFont: {
        QwtText tt = mPlot->title();
        tt.setFont(panel->getFontValue(PID_TitleFont));
        mPlot->setTitle(tt);
        break;
    }
    case PID_TitleColor: {
        QwtText tt = mPlot->title();
        tt.setColor(panel->getColorValue(PID_TitleColor));
        mPlot->setTitle(tt);
        break;
    }
    case PID_FooterText: {
        QwtText tt = mPlot->footer();
        tt.setText(panel->getStringValue(PID_FooterText));
        mPlot->setFooter(tt);
        break;
    }
    case PID_FooterFont: {
        QwtText tt = mPlot->footer();
        tt.setFont(panel->getFontValue(PID_FooterFont));
        mPlot->setFooter(tt);
        break;
    }
    case PID_FooterColor: {
        QwtText tt = mPlot->footer();
        tt.setColor(panel->getColorValue(PID_FooterColor));
        mPlot->setFooter(tt);
        break;
    }
    default:
        break;
    }

    replot();
}

}  // end namespace DA
