#include "DAChartCanvasSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include <QSignalBlocker>
#include <QComboBox>
#include <QVBoxLayout>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartCanvasSettingPanel::DAChartCanvasSettingPanel(QWidget* parent)
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
            this, &DAChartCanvasSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChartCanvasSettingPanel::propertyValueChanged, this, &DAChartCanvasSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DAChartCanvasSettingPanel::~DAChartCanvasSettingPanel()
{
}

/**
 * @brief 获取通用属性面板指针
 * @return DAPropertyPanelWidget指针
 */
DAPropertyPanelContainerWidget* DAChartCanvasSettingPanel::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 设置目标QwtPlot
 * @param plot 目标图表
 */
void DAChartCanvasSettingPanel::setTarget(QwtPlot* plot)
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
QwtPlot* DAChartCanvasSettingPanel::target() const
{
    return mPlot.data();
}

/**
 * @brief 更新界面显示
 *
 * 从QwtPlotCanvas读取画布状态写入面板。
 */
void DAChartCanvasSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    QwtPlotCanvas* canvas = nullptr;
    if (mPlot) {
        canvas = qobject_cast< QwtPlotCanvas* >(mPlot->canvas());
    }

    if (canvas) {
        mPanel->setBrushValue(PID_BackgroundBrush, canvas->palette().brush(QPalette::Window));
        mPanel->setIntValue(PID_BorderWidth, canvas->lineWidth());
        mPanel->setPenValue(PID_BorderPen, canvas->palette().brush(QPalette::WindowText).color());

        // 设置FrameStyle下拉框
        DAPropertyItemWidget* item = mPanel->getPropertyItem(PID_FrameStyle);
        if (item) {
            QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
            if (combo) {
                QSignalBlocker comboBlocker(combo);
                int index = combo->findData(static_cast< int >(canvas->frameShape()));
                if (index >= 0) {
                    combo->setCurrentIndex(index);
                }
            }
        }
    } else {
        mPanel->setBrushValue(PID_BackgroundBrush, QBrush());
        mPanel->setIntValue(PID_BorderWidth, 0);
        mPanel->setPenValue(PID_BorderPen, QPen());
    }
}

/**
 * @brief 触发重绘
 */
void DAChartCanvasSettingPanel::replot()
{
    if (mPlot) {
        mPlot->replot();
    }
}

/**
 * @brief 构建属性面板
 *
 * 添加以下属性：
 * - 背景组: 背景画刷
 * - 边框组: 边框宽度、边框画笔
 * - 样式组: 边框样式
 */
void DAChartCanvasSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addCollapsibleGroup(tr("Background"));
    panel->addBrushProperty(PID_BackgroundBrush, tr("Background Brush"));

    panel->addCollapsibleGroup(tr("Border"));
    panel->addIntProperty(PID_BorderWidth, tr("Border Width"), 0, 0, 20);
    panel->addPenProperty(PID_BorderPen, tr("Border Pen"));

    panel->addCollapsibleGroup(tr("Style"));
    // FrameStyle: 使用QComboBox填充QFrame::Shape枚举
    QComboBox* frameStyleCombo = new QComboBox(this);
    frameStyleCombo->addItem(tr("No Frame"), static_cast< int >(QFrame::NoFrame));
    frameStyleCombo->addItem(tr("Box"), static_cast< int >(QFrame::Box));
    frameStyleCombo->addItem(tr("Panel"), static_cast< int >(QFrame::Panel));
    frameStyleCombo->addItem(tr("Styled Panel"), static_cast< int >(QFrame::StyledPanel));
    frameStyleCombo->addItem(tr("Win Panel"), static_cast< int >(QFrame::WinPanel));

    connect(frameStyleCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
            panel, &DAPropertyPanelContainerWidget::propertyValueChanged);

    panel->addProperty(PID_FrameStyle, tr("Frame Shape"), frameStyleCombo);
}

/**
 * @brief 转发属性值变化信号
 * @param propertyId 属性ID
 */
void DAChartCanvasSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 * @param propertyId 属性ID
 */
void DAChartCanvasSettingPanel::onPropertyValueChanged(int propertyId)
{
    QwtPlotCanvas* canvas = nullptr;
    if (mPlot) {
        canvas = qobject_cast< QwtPlotCanvas* >(mPlot->canvas());
    }
    if (!canvas) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PID_BackgroundBrush: {
        QPalette pal = canvas->palette();
        pal.setBrush(QPalette::Window, panel->getBrushValue(PID_BackgroundBrush));
        canvas->setPalette(pal);
        break;
    }
    case PID_BorderWidth: {
        canvas->setLineWidth(panel->getIntValue(PID_BorderWidth));
        break;
    }
    case PID_BorderPen: {
        QPalette pal = canvas->palette();
        pal.setBrush(QPalette::WindowText, panel->getPenValue(PID_BorderPen).brush());
        canvas->setPalette(pal);
        break;
    }
    case PID_FrameStyle: {
        DAPropertyItemWidget* item = panel->getPropertyItem(PID_FrameStyle);
        if (item) {
            QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
            if (combo) {
                QFrame::Shape shape = static_cast< QFrame::Shape >(combo->currentData().toInt());
                canvas->setFrameShape(shape);
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
