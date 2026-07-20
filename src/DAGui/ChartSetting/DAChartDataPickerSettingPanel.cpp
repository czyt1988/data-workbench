#include "DAChartDataPickerSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
#include "qwt_plot.h"
#include "qwt_plot_series_data_picker.h"
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace DA
{

DAChartDataPickerSettingPanel::DAChartDataPickerSettingPanel(QWidget* parent) : QWidget(parent), mPanel(nullptr)
{
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged, this, &DAChartDataPickerSettingPanel::onPanelPropertyValueChanged);
    connect(this, &DAChartDataPickerSettingPanel::propertyValueChanged, this, &DAChartDataPickerSettingPanel::onPropertyValueChanged);

    buildPropertyPanel();
}

DAChartDataPickerSettingPanel::~DAChartDataPickerSettingPanel()
{
}

DAPropertyPanelContainerWidget* DAChartDataPickerSettingPanel::propertyPanel() const
{
    return mPanel;
}

void DAChartDataPickerSettingPanel::setTarget(QwtPlot* plot)
{
    if (mPlot == plot) {
        return;
    }
    mPlot = plot;
    updateUI();
}

QwtPlot* DAChartDataPickerSettingPanel::target() const
{
    return mPlot.data();
}

void DAChartDataPickerSettingPanel::updateUI()
{
    QSignalBlocker blocker(mPanel);

    DAChartWidget* chart = qobject_cast< DAChartWidget* >(mPlot.data());
    if (!chart) {
        mPanel->setEnumValue(PID_PickerMode, 0);
        mPanel->setBoolValue(PID_ShowXValue, true);
        mPanel->setBoolValue(PID_PickerGroupEnabled, false);
        return;
    }

    QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
    if (!picker || !chart->isDataPickingEnabled()) {
        mPanel->setEnumValue(PID_PickerMode, 0);
    } else if (chart->isYValuePickingEnabled()) {
        mPanel->setEnumValue(PID_PickerMode, 1);
    } else if (chart->isXYValuePickingEnabled()) {
        mPanel->setEnumValue(PID_PickerMode, 2);
    } else {
        mPanel->setEnumValue(PID_PickerMode, 0);
    }

    if (picker) {
        mPanel->setBoolValue(PID_ShowXValue, picker->isEnableShowXValue());
        mPanel->setEnumValue(PID_TextPlacement, static_cast< int >(picker->textArea()));
        mPanel->setEnumValue(PID_InterpolationMode, static_cast< int >(picker->interpolationMode()));
        mPanel->setBoolValue(PID_DrawFeaturePoint, picker->isEnableDrawFeaturePoint());
        mPanel->setIntValue(PID_FeaturePointSize, picker->drawFeaturePointSize());
        mPanel->setIntValue(PID_NearestSearchWindowSize, picker->nearestSearchWindowSize());
        mPanel->setBrushValue(PID_TextBackgroundBrush, picker->textBackgroundBrush());
        mPanel->setAlignmentValue(PID_TextAlignment, picker->textAlignment());
        mPanel->setIntValue(PID_TextOffsetX, picker->textTrackerOffset().x());
        mPanel->setIntValue(PID_TextOffsetY, picker->textTrackerOffset().y());
    } else {
        mPanel->setBoolValue(PID_ShowXValue, true);
        mPanel->setEnumValue(PID_TextPlacement, 1);
        mPanel->setEnumValue(PID_InterpolationMode, 0);
        mPanel->setBoolValue(PID_DrawFeaturePoint, true);
        mPanel->setIntValue(PID_FeaturePointSize, 8);
        mPanel->setIntValue(PID_NearestSearchWindowSize, 5);
        mPanel->setBrushValue(PID_TextBackgroundBrush, QBrush());
        mPanel->setAlignmentValue(PID_TextAlignment, Qt::AlignLeft | Qt::AlignTop);
        mPanel->setIntValue(PID_TextOffsetX, 10);
        mPanel->setIntValue(PID_TextOffsetY, 10);
    }

    if (DAFigureWidget* fig = chart->figureWidget()) {
        mPanel->setBoolValue(PID_PickerGroupEnabled, fig->isDataPickerGroupEnabled());
    } else {
        mPanel->setBoolValue(PID_PickerGroupEnabled, false);
    }
}

void DAChartDataPickerSettingPanel::replot()
{
    if (mPlot) {
        mPlot->replot();
    }
}

void DAChartDataPickerSettingPanel::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addCollapsibleGroup(tr("Basic")  // cn:基础
    );
    panel->addEnumProperty(PID_PickerMode,
                           tr("Picker Mode")  // cn:拾取模式
                           ,
                           QStringList() << tr("Off")  // cn:关闭
                                          << tr("Y Value")  // cn:Y值拾取
                                          << tr("XY Value")  // cn:XY值拾取
                           ,
                           QList< int >() << 0 << 1 << 2,
                           0);
    panel->addBoolProperty(PID_ShowXValue, tr("Show X Value")  // cn:显示X值
                           ,
                           true);
    panel->addEnumProperty(PID_TextPlacement,
                           tr("Text Placement")  // cn:文字位置
                           ,
                           QStringList() << tr("Auto")  // cn:自动
                                          << tr("Follow Top")  // cn:跟随顶部
                                          << tr("Follow Bottom")  // cn:跟随底部
                                          << tr("Follow Mouse")  // cn:跟随鼠标
                                          << tr("Canvas Top Right")  // cn:画布右上
                                          << tr("Canvas Top Left")  // cn:画布左上
                                          << tr("Canvas Bottom Right")  // cn:画布右下
                                          << tr("Canvas Bottom Left")  // cn:画布左下
                                          << tr("Canvas Top Auto")  // cn:画布顶部自动
                                          << tr("Canvas Bottom Auto")  // cn:画布底部自动
                           ,
                           QList< int >() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9,
                           1);
    panel->addEnumProperty(PID_InterpolationMode,
                           tr("Interpolation")  // cn:插值模式
                           ,
                           QStringList() << tr("None")  // cn:无插值
                                          << tr("Linear")  // cn:线性插值
                           ,
                           QList< int >() << 0 << 1,
                           0);
    panel->endGroup();

    panel->addCollapsibleGroup(tr("Feature Point")  // cn:特征点
    );
    panel->addBoolProperty(PID_DrawFeaturePoint, tr("Draw Feature Point")  // cn:绘制特征点
                            ,
                            true);
    panel->addIntProperty(PID_FeaturePointSize, tr("Feature Point Size")  // cn:特征点大小
                           ,
                           8, 1, 30);
    panel->addIntProperty(PID_NearestSearchWindowSize, tr("Nearest Search Window")  // cn:搜索窗口大小
                           ,
                           5, 1, 100);
    panel->endGroup();

    panel->addCollapsibleGroup(tr("Text Style")  // cn:文字样式
    );
    panel->addBrushProperty(PID_TextBackgroundBrush, tr("Background Brush")  // cn:背景画刷
    );
    panel->addAlignmentProperty(PID_TextAlignment, tr("Text Alignment")  // cn:文字对齐
                                 ,
                                 Qt::AlignLeft | Qt::AlignTop);
    panel->addIntProperty(PID_TextOffsetX, tr("Text Offset X")  // cn:文字偏移X
                           ,
                           10, -200, 200);
    panel->addIntProperty(PID_TextOffsetY, tr("Text Offset Y")  // cn:文字偏移Y
                           ,
                           10, -200, 200);
    panel->endGroup();

    panel->addCollapsibleGroup(tr("Linkage")  // cn:联动
    );
    panel->addBoolProperty(PID_PickerGroupEnabled, tr("Picker Group Enabled")  // cn:拾取器联动
                            ,
                            false);
    panel->endGroup();
}

void DAChartDataPickerSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

void DAChartDataPickerSettingPanel::onPropertyValueChanged(int propertyId)
{
    DAChartWidget* chart = qobject_cast< DAChartWidget* >(mPlot.data());
    if (!chart) {
        return;
    }

    auto panel = propertyPanel();

    switch (propertyId) {
    case PID_PickerMode: {
        int mode = panel->getEnumValue(PID_PickerMode);
        if (mode == 0) {
            if (chart->isYValuePickingEnabled()) {
                chart->enableYValuePicking(false);
            }
            if (chart->isXYValuePickingEnabled()) {
                chart->enableXYValuePicking(false);
            }
        } else if (mode == 1) {
            chart->enableYValuePicking(true);
        } else if (mode == 2) {
            chart->enableXYValuePicking(true);
        }
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setEnableShowXValue(panel->getBoolValue(PID_ShowXValue));
            picker->setTextArea(static_cast< QwtPlotSeriesDataPicker::TextPlacement >(panel->getEnumValue(PID_TextPlacement)));
            picker->setInterpolationMode(static_cast< QwtPlotSeriesDataPicker::InterpolationMode >(panel->getEnumValue(PID_InterpolationMode)));
            picker->setEnableDrawFeaturePoint(panel->getBoolValue(PID_DrawFeaturePoint));
            picker->setDrawFeaturePointSize(panel->getIntValue(PID_FeaturePointSize));
            picker->setNearestSearchWindowSize(panel->getIntValue(PID_NearestSearchWindowSize));
            picker->setTextBackgroundBrush(panel->getBrushValue(PID_TextBackgroundBrush));
            picker->setTextAlignment(panel->getAlignmentValue(PID_TextAlignment));
            picker->setTextTrackerOffset(QPoint(panel->getIntValue(PID_TextOffsetX), panel->getIntValue(PID_TextOffsetY)));
        }
        break;
    }
    case PID_ShowXValue: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setEnableShowXValue(panel->getBoolValue(PID_ShowXValue));
        }
        break;
    }
    case PID_TextPlacement: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setTextArea(static_cast< QwtPlotSeriesDataPicker::TextPlacement >(panel->getEnumValue(PID_TextPlacement)));
        }
        break;
    }
    case PID_InterpolationMode: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setInterpolationMode(static_cast< QwtPlotSeriesDataPicker::InterpolationMode >(panel->getEnumValue(PID_InterpolationMode)));
        }
        break;
    }
    case PID_DrawFeaturePoint: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setEnableDrawFeaturePoint(panel->getBoolValue(PID_DrawFeaturePoint));
        }
        break;
    }
    case PID_FeaturePointSize: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setDrawFeaturePointSize(panel->getIntValue(PID_FeaturePointSize));
        }
        break;
    }
    case PID_NearestSearchWindowSize: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setNearestSearchWindowSize(panel->getIntValue(PID_NearestSearchWindowSize));
        }
        break;
    }
    case PID_TextBackgroundBrush: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setTextBackgroundBrush(panel->getBrushValue(PID_TextBackgroundBrush));
        }
        break;
    }
    case PID_TextAlignment: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            picker->setTextAlignment(panel->getAlignmentValue(PID_TextAlignment));
        }
        break;
    }
    case PID_TextOffsetX: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            QPoint offset = picker->textTrackerOffset();
            offset.setX(panel->getIntValue(PID_TextOffsetX));
            picker->setTextTrackerOffset(offset);
        }
        break;
    }
    case PID_TextOffsetY: {
        QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
        if (picker) {
            QPoint offset = picker->textTrackerOffset();
            offset.setY(panel->getIntValue(PID_TextOffsetY));
            picker->setTextTrackerOffset(offset);
        }
        break;
    }
    case PID_PickerGroupEnabled: {
        if (DAFigureWidget* fig = chart->figureWidget()) {
            fig->setDataPickerGroupEnabled(panel->getBoolValue(PID_PickerGroupEnabled));
        }
        break;
    }
    default:
        break;
    }

    replot();
}

}  // namespace DA
