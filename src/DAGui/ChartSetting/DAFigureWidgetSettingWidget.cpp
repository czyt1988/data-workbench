#include "DAFigureWidgetSettingWidget.h"
#include "ui_DAFigureWidgetSettingWidget.h"
#include <QPointer>
// DA Figure
#include "DAFigureWidget.h"
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
    updateUI(fig);
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

void DAFigureWidgetSettingWidget::updateUI()
{
}

}  // end DA
