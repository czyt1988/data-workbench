#include "DAChartAxisSetWidget.h"
#include <QtMath>
#include "ui_DAChartAxisSetWidget.h"
#include "qwt_plot.h"
#include "qwt_scale_div.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_widget.h"
#include "qwt_date_scale_draw.h"
#include <QButtonGroup>
#include "DAChartUtil.h"
#include <QDebug>
#include "DASignalBlockers.hpp"

#ifndef DAChartAxisSetWidget_DEBUG_PRINT
#define DAChartAxisSetWidget_DEBUG_PRINT 1
#endif
namespace DA
{

DAChartAxisSetWidget::DAChartAxisSetWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartAxisSetWidget), m_plot(nullptr), m_axisID(QwtPlot::axisCnt)
{
    ui->setupUi(this);
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->addButton(ui->radioButtonNormal, NormalScale);
    m_buttonGroup->addButton(ui->radioButtonTimeScale, DateTimeScale);
    ui->radioButtonNormal->setChecked(true);
    ui->dateTimeScaleSetWidget->hide();

    connect(ui->lineEditTitle, &QLineEdit::textChanged, this, &DAChartAxisSetWidget::onLineEditTextChanged);
    connect(ui->fontSetWidget, &DAFontEditPannelWidget::currentFontChanged, this, &DAChartAxisSetWidget::onAxisFontChanged);
    connect(ui->fontSetWidget,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAChartAxisSetWidget::onAxisFontColorChanged);
    connect(ui->aligmentSetWidget,
            &DAAligmentEditWidget::alignmentChanged,
            this,
            &DAChartAxisSetWidget::onAxisLabelAligmentChanged);
    connect(ui->doubleSpinBoxRotation,
            static_cast< void (QDoubleSpinBox::*)(double v) >(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartAxisSetWidget::onAxisLabelRotationChanged);
    connect(ui->spinBoxMargin,
            static_cast< void (QSpinBox::*)(int v) >(&QSpinBox::valueChanged),
            this,
            &DAChartAxisSetWidget::onAxisMarginValueChanged);
    connect(ui->doubleSpinBoxMax,
            static_cast< void (QDoubleSpinBox::*)(double v) >(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartAxisSetWidget::onAxisMaxScaleChanged);
    connect(ui->doubleSpinBoxMin,
            static_cast< void (QDoubleSpinBox::*)(double v) >(&QDoubleSpinBox::valueChanged),
            this,
            &DAChartAxisSetWidget::onAxisMinScaleChanged);
    Qt5Qt6Compat_Connect_ButtonGroupClicked_int(m_buttonGroup, DAChartAxisSetWidget::onScaleStyleChanged);
    connect(ui->checkBoxEnable, &QAbstractButton::clicked, this, &DAChartAxisSetWidget::onCheckBoxEnableCliecked);
}

DAChartAxisSetWidget::~DAChartAxisSetWidget()
{
    delete ui;
}

void DAChartAxisSetWidget::onCheckBoxEnableCliecked(bool on)
{
    enableWidget(on);
    if (m_plot) {
        m_plot->enableAxis(m_axisID, on);
    }
}

void DAChartAxisSetWidget::onLineEditTextChanged(const QString& text)
{
    if (m_plot) {
        DAChartUtil::setAxisTitle(m_plot, m_axisID, text);
    }
}

void DAChartAxisSetWidget::onAxisFontChanged(const QFont& font)
{
    if (m_plot) {
        DAChartUtil::setAxisFont(m_plot, m_axisID, font);
    }
}

void DAChartAxisSetWidget::onAxisFontColorChanged(const QColor& color)
{
    if (m_plot) {
        DAChartUtil::setAxisFontColor(m_plot, m_axisID, color);
    }
}

void DAChartAxisSetWidget::onAxisLabelAligmentChanged(Qt::Alignment al)
{
    if (m_plot) {
        DAChartUtil::setAxisLabelAlignment(m_plot, m_axisID, al);
    }
}

void DAChartAxisSetWidget::onAxisLabelRotationChanged(double v)
{
    if (m_plot) {
        DAChartUtil::setAxisLabelRotation(m_plot, m_axisID, v);
    }
}


void DAChartAxisSetWidget::onAxisMarginValueChanged(int v)
{
    if (m_plot) {
        DAChartUtil::setAxisMargin(m_plot, m_axisID, v);
    }
}

void DAChartAxisSetWidget::onAxisMaxScaleChanged(double v)
{
#if DAChartAxisSetWidget_DEBUG_PRINT
    qDebug() << "DAChartAxisSetWidget::onAxisMaxScaleChanged";
#endif
    if (m_plot) {
        QwtInterval inv = m_plot->axisInterval(m_axisID);
        if (qFuzzyCompare(inv.maxValue(), v)) {
            // 数值相等，没必要设置
            return;
        }
        m_plot->setAxisScale(m_axisID, inv.minValue(), v);
    }
}

void DAChartAxisSetWidget::onAxisMinScaleChanged(double v)
{
#if DAChartAxisSetWidget_DEBUG_PRINT
    qDebug() << "DAChartAxisSetWidget::onAxisMinScaleChanged";
#endif
    if (m_plot) {
        QwtInterval inv = m_plot->axisInterval(m_axisID);
        if (qFuzzyCompare(inv.minValue(), v)) {
            // 数值相等，没必要设置
            return;
        }
        m_plot->setAxisScale(m_axisID, v, inv.maxValue());
    }
}

void DAChartAxisSetWidget::onScaleDivChanged()
{
#if DAChartAxisSetWidget_DEBUG_PRINT
    qDebug() << "DAChartAxisSetWidget::onScaleDivChanged";
#endif
    if (nullptr == m_plot) {
        return;
    }
    QwtInterval inv = m_plot->axisInterval(m_axisID);
    // onScaleDivChanged是QwtScaleWidget::scaleDivChanged触发的，这里避免重复设置，把doubleSpinBoxMin，doubleSpinBoxMax的信号触发取消
    DASignalBlockers blocks(ui->doubleSpinBoxMin, ui->doubleSpinBoxMax);
    ui->doubleSpinBoxMin->setValue(inv.minValue());
    ui->doubleSpinBoxMax->setValue(inv.maxValue());
}

void DAChartAxisSetWidget::onScaleStyleChanged(int id)
{
#if DAChartAxisSetWidget_DEBUG_PRINT
    qDebug() << "DAChartAxisSetWidget::onScaleStyleChanged=" << id;
#endif
    if (NormalScale == id) {
        ui->dateTimeScaleSetWidget->hide();
        if (m_plot) {
            DAChartUtil::setAxisNormalScale(m_plot, m_axisID);
        }
    } else {
        if (DateTimeScale == id) {
            ui->dateTimeScaleSetWidget->show();
            if (m_plot) {
                QString format = ui->dateTimeScaleSetWidget->getTimeFormat();
                DAChartUtil::setAxisDateTimeScale(m_plot, m_axisID, format);
            }
        }
    }
}

void DAChartAxisSetWidget::updateUI(QwtPlot* chart, int axisID)
{
#if DAChartAxisSetWidget_DEBUG_PRINT
    qDebug() << "DAChartAxisSetWidget::updateUI,axisID=" << axisID;
#endif
    enableWidget(nullptr != chart);
    if (nullptr == chart) {
        resetAxisValue();
        return;
    }
    DASignalBlockers block(ui->checkBoxEnable,
                           ui->lineEditTitle,
                           ui->fontSetWidget,
                           ui->doubleSpinBoxMin,
                           ui->doubleSpinBoxMax,
                           ui->spinBoxMargin,
                           ui->radioButtonTimeScale,
                           ui->dateTimeScaleSetWidget);
    ui->checkBoxEnable->setChecked(chart->axisEnabled(axisID));
    ui->lineEditTitle->setText(chart->axisTitle(axisID).text());
    ui->fontSetWidget->setCurrentFont(chart->axisFont(axisID));

    QwtInterval inv = chart->axisInterval(axisID);
    ui->doubleSpinBoxMin->setValue(inv.minValue());
    ui->doubleSpinBoxMax->setValue(inv.maxValue());
    ui->doubleSpinBoxMin->setDecimals(inv.minValue() < 0.01 ? 5 : 2);  // 显示小数点的位数调整
    ui->doubleSpinBoxMax->setDecimals(inv.minValue() < 0.01 ? 5 : 2);

    QwtScaleWidget* ax = chart->axisWidget(axisID);
    if (nullptr == ax) {
        ui->radioButtonNormal->setChecked(true);
        return;
    }

    QwtScaleDraw* sd = ax->scaleDraw();
    if (sd) {
        ui->doubleSpinBoxRotation->setValue(sd->labelRotation());
        ui->aligmentSetWidget->setCurrentAlignment(sd->labelAlignment());
    }
    ui->spinBoxMargin->setValue(ax->margin());
    QwtDateScaleDraw* dsd = dynamic_cast< QwtDateScaleDraw* >(sd);

    if (dsd) {
        ui->radioButtonTimeScale->setChecked(true);
        ui->dateTimeScaleSetWidget->setTimeFormatText(dsd->dateFormat(QwtDate::Second));
    } else {
        ui->radioButtonNormal->setChecked(true);
    }
}

void DAChartAxisSetWidget::resetAxisValue()
{
    ui->lineEditTitle->setText("");
    ui->fontSetWidget->setCurrentFont(QFont());
    ui->doubleSpinBoxMin->setValue(0);
    ui->doubleSpinBoxMax->setValue(0);
    ui->radioButtonNormal->setChecked(true);
    ui->doubleSpinBoxRotation->setValue(0);
    ui->labelAligment->setAlignment(Qt::AlignLeft);
    ui->spinBoxMargin->setValue(0);
    ui->radioButtonTimeScale->setChecked(false);
    ui->dateTimeScaleSetWidget->setTimeFormatText("");
}

/**
 * @brief 设置是否允许
 * @param on
 */
void DAChartAxisSetWidget::setEnableAxis(bool on)
{
    ui->checkBoxEnable->setChecked(on);
}

bool DAChartAxisSetWidget::isEnableAxis() const
{
    return ui->checkBoxEnable->isChecked();
}

void DAChartAxisSetWidget::enableWidget(bool enable)
{
    ui->lineEditTitle->setEnabled(enable);
    ui->fontSetWidget->setEnabled(enable);
    ui->doubleSpinBoxMin->setEnabled(enable);
    ui->doubleSpinBoxMax->setEnabled(enable);
    ui->radioButtonNormal->setEnabled(enable);
    ui->doubleSpinBoxRotation->setEnabled(enable);
    ui->labelAligment->setEnabled(enable);
    ui->spinBoxMargin->setEnabled(enable);
    ui->radioButtonTimeScale->setEnabled(enable);
    ui->dateTimeScaleSetWidget->setEnabled(enable);
}

/**
 * @brief 设置启用axis checkbox的图标
 * @param icon
 */
void DAChartAxisSetWidget::setEnableCheckBoxIcon(const QIcon& icon)
{
    ui->checkBoxEnable->setIcon(icon);
}

QIcon DAChartAxisSetWidget::getEnableCheckBoxIcon() const
{
    return ui->checkBoxEnable->icon();
}

void DAChartAxisSetWidget::bindTarget()
{
    if (nullptr == m_plot) {
        return;
    }
    QwtScaleWidget* aw = m_plot->axisWidget(m_axisID);
    if (aw) {
        connect(aw, &QwtScaleWidget::scaleDivChanged, this, &DAChartAxisSetWidget::onScaleDivChanged);
    }
}

void DAChartAxisSetWidget::unbindTarget()
{
    if (nullptr == m_plot) {
        return;
    }
    QwtScaleWidget* aw = m_plot->axisWidget(m_axisID);
    if (aw) {
        disconnect(aw, &QwtScaleWidget::scaleDivChanged, this, &DAChartAxisSetWidget::onScaleDivChanged);
    }
}

QwtPlot* DAChartAxisSetWidget::getPlot() const
{
    return m_plot;
}

void DAChartAxisSetWidget::setPlot(QwtPlot* chart, int axisID)
{
    if (m_plot && m_plot != chart) {
        unbindTarget();
    }

    m_plot = nullptr;  // 先设置为null，使得槽函数不动作
    updateUI(chart, axisID);
    m_plot   = chart;
    m_axisID = axisID;
    bindTarget();
}

void DAChartAxisSetWidget::updateUI()
{
    updateUI(m_plot, m_axisID);
}

}  // end DA
