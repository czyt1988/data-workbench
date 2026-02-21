#include "DAFigureWidgetSettingWidget.h"
#include "ui_DAFigureWidgetSettingWidget.h"
#include <QPointer>
// DA Figure
#include "DAFigureWidget.h"
#include "DASignalBlockers.hpp"
// Qwt
#include "qwt_figure.h"
namespace DA
{

class DAFigureWidgetSettingWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureWidgetSettingWidget)
public:
    PrivateData(DAFigureWidgetSettingWidget* p);

public:
    QPointer< DAFigureWidget > figure;
};

DAFigureWidgetSettingWidget::PrivateData::PrivateData(DAFigureWidgetSettingWidget* p) : q_ptr(p)
{
}

//===============================================================
// DAFigureWidgetSettingWidget
//===============================================================

DAFigureWidgetSettingWidget::DAFigureWidgetSettingWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAFigureWidgetSettingWidget)
{
    ui->setupUi(this);
    connect(ui->spinBoxMinWidth,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAFigureWidgetSettingWidget::onSpinBoxMinWidthValueChanged);
    connect(ui->spinBoxMinHeight,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAFigureWidgetSettingWidget::onSpinBoxMinHeightValueChanged);
    connect(ui->spinBoxMaxWidth,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAFigureWidgetSettingWidget::onSpinBoxMaxWidthValueChanged);
    connect(ui->spinBoxMaxHeight,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DAFigureWidgetSettingWidget::onSpinBoxMaxHeightValueChanged);
    connect(ui->widgetBrushEditor,
            &DABrushEditWidget::brushChanged,
            this,
            &DAFigureWidgetSettingWidget::onWidgetBrushEditorBrushChanged);
}

DAFigureWidgetSettingWidget::~DAFigureWidgetSettingWidget()
{
    delete ui;
}

void DAFigureWidgetSettingWidget::setFigure(DAFigureWidget* fig)
{
    DA_D(d);
    if (fig == d->figure) {
        return;
    }
    if (d->figure) {
        unbindFigure(d->figure);
    }
    d_ptr->figure = fig;
    bindFigure(fig);
    updateUI();
}

DAFigureWidget* DAFigureWidgetSettingWidget::getFigure() const
{
    return d_ptr->figure.data();
}

void DAFigureWidgetSettingWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DAFigureWidgetSettingWidget::bindFigure(DAFigureWidget* fig)
{
    Q_UNUSED(fig);
}

void DAFigureWidgetSettingWidget::unbindFigure(DAFigureWidget* fig)
{
    Q_UNUSED(fig);
}

void DAFigureWidgetSettingWidget::onSpinBoxMinWidthValueChanged(int v)
{
    DAFigureWidget* fig = getFigure();
    if (!fig) {
        return;
    }
    QwtFigure* qwtfig = fig->figure();
    if (!qwtfig) {
        return;
    }
    qwtfig->setMinimumWidth(v);
}

void DAFigureWidgetSettingWidget::onSpinBoxMinHeightValueChanged(int v)
{
    DAFigureWidget* fig = getFigure();

    if (!fig) {
        return;
    }
    QwtFigure* qwtfig = fig->figure();
    if (!qwtfig) {
        return;
    }
    qwtfig->setMinimumHeight(v);
}

void DAFigureWidgetSettingWidget::onSpinBoxMaxWidthValueChanged(int v)
{
    DAFigureWidget* fig = getFigure();
    if (!fig) {
        return;
    }
    QwtFigure* qwtfig = fig->figure();
    if (!qwtfig) {
        return;
    }
    qwtfig->setMaximumWidth(v);
}

void DAFigureWidgetSettingWidget::onSpinBoxMaxHeightValueChanged(int v)
{
    DAFigureWidget* fig = getFigure();
    if (!fig) {
        return;
    }
    QwtFigure* qwtfig = fig->figure();
    if (!qwtfig) {
        return;
    }
    qwtfig->setMaximumHeight(v);
}

void DAFigureWidgetSettingWidget::onWidgetBrushEditorBrushChanged(const QBrush& brush)
{
    DAFigureWidget* fig = getFigure();
    if (!fig) {
        return;
    }
    fig->setFaceBrush(brush);
}

void DAFigureWidgetSettingWidget::updateUI()
{
    DAFigureWidget* fig = getFigure();
    DASignalBlockers block(ui->spinBoxMinWidth,
                           ui->spinBoxMinHeight,
                           ui->spinBoxMaxWidth,
                           ui->spinBoxMaxHeight,
                           ui->widgetBrushEditor);
    QwtFigure* qwtfig = nullptr;
    if (fig) {
        qwtfig = fig->figure();
    }
    ui->spinBoxMinWidth->setValue(qwtfig ? qwtfig->minimumWidth() : 0);
    ui->spinBoxMinHeight->setValue(qwtfig ? qwtfig->minimumHeight() : 0);
    ui->spinBoxMaxWidth->setValue(qwtfig ? qwtfig->maximumWidth() : 0);
    ui->spinBoxMaxHeight->setValue(qwtfig ? qwtfig->maximumHeight() : 0);
    ui->widgetBrushEditor->setCurrentBrush(fig ? fig->getFaceBrush() : QBrush());
}

void DAFigureWidgetSettingWidget::applySetting(DAFigureWidget* fig)
{
    if (!fig) {
        return;
    }
    QwtFigure* qwtfig = fig->figure();
    if (qwtfig) {
        qwtfig->setMinimumWidth(ui->spinBoxMinWidth->value());
        qwtfig->setMinimumHeight(ui->spinBoxMinHeight->value());
        qwtfig->setMaximumWidth(ui->spinBoxMaxWidth->value());
        qwtfig->setMaximumHeight(ui->spinBoxMaxHeight->value());
    }
    fig->setFaceBrush(ui->widgetBrushEditor->getCurrentBrush());
}

}  // end DA
