#include "DAChartPlotSettingWidget.h"
#include "ui_DAChartPlotSettingWidget.h"
#include "qwt_text_label.h"
namespace DA
{
DAChartPlotSettingWidget::DAChartPlotSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartPlotSettingWidget)
{
	ui->setupUi(this);
    connect(ui->lineEditTitle, &QLineEdit::textChanged, this, &DAChartPlotSettingWidget::setTitleText);
    connect(ui->lineEditFooter, &QLineEdit::textChanged, this, &DAChartPlotSettingWidget::setFooterText);
    connect(ui->widgetTitleFontEditer, &DAFontEditPannelWidget::currentFontChanged, this, &DAChartPlotSettingWidget::setTitleFont);
    connect(ui->widgetFooterFontEditer,
            &DAFontEditPannelWidget::currentFontChanged,
            this,
            &DAChartPlotSettingWidget::setFooterFont);
    connect(ui->widgetTitleFontEditer,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAChartPlotSettingWidget::setTitleColor);
    connect(ui->widgetFooterFontEditer,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAChartPlotSettingWidget::setFooterColor);
}

DAChartPlotSettingWidget::~DAChartPlotSettingWidget()
{
	delete ui;
}

void DAChartPlotSettingWidget::setChartWidget(DAChartWidget* w)
{
	mChartPlot = w;
	if (!w) {
		return;
	}
    ui->widgetXBottomAxisSetting->setChart(w, QwtPlot::xBottom);
    ui->widgetXTopAxisSetting->setChart(w, QwtPlot::xTop);
    ui->widgetYLeftAxisSetting->setChart(w, QwtPlot::yLeft);
    ui->widgetYRightAxisSetting->setChart(w, QwtPlot::yRight);

    ui->groupBoxXBottom->setCollapsed(!(w->axisEnabled(QwtPlot::xBottom)));
    ui->groupBoxXTop->setCollapsed(!(w->axisEnabled(QwtPlot::xTop)));
    ui->groupBoxYLeft->setCollapsed(!(w->axisEnabled(QwtPlot::yLeft)));
    ui->groupBoxYRight->setCollapsed(!(w->axisEnabled(QwtPlot::yRight)));

    QwtText tt = w->title();
    QSignalBlocker b1(ui->lineEditTitle), b2(ui->widgetTitleFontEditer);
    ui->lineEditTitle->setText(tt.text());
    ui->widgetTitleFontEditer->setCurrentFont(tt.font());
    ui->widgetTitleFontEditer->setCurrentFontColor(tt.color());
    tt = w->footer();
    QSignalBlocker b3(ui->lineEditFooter), b4(ui->widgetFooterFontEditer);
    ui->lineEditFooter->setText(tt.text());
    ui->widgetFooterFontEditer->setCurrentFont(tt.font());
    ui->widgetFooterFontEditer->setCurrentFontColor(tt.color());
}

DAChartWidget* DAChartPlotSettingWidget::getChartWidget() const
{
    return mChartPlot;
}

void DAChartPlotSettingWidget::setTitleText(const QString& t)
{
    if (!mChartPlot) {
        return;
    }
    QwtText tt = mChartPlot->title();
    tt.setText(t);
    mChartPlot->setTitle(tt);
}

void DAChartPlotSettingWidget::setTitleFont(const QFont& f)
{
    if (!mChartPlot) {
        return;
    }
    QwtText tt = mChartPlot->title();
    tt.setFont(f);
    mChartPlot->setTitle(tt);
}

void DAChartPlotSettingWidget::setTitleColor(const QColor& c)
{
    if (!mChartPlot) {
        return;
    }
    QwtText tt = mChartPlot->title();
    tt.setColor(c);
    mChartPlot->setTitle(tt);
}

void DAChartPlotSettingWidget::setFooterText(const QString& t)
{
    if (!mChartPlot) {
        return;
    }
    QwtText tt = mChartPlot->footer();
    tt.setText(t);
    mChartPlot->setFooter(tt);
}

void DAChartPlotSettingWidget::setFooterFont(const QFont& f)
{
    if (!mChartPlot) {
        return;
    }
    QwtText tt = mChartPlot->footer();
    tt.setFont(f);
    mChartPlot->setFooter(tt);
}

void DAChartPlotSettingWidget::setFooterColor(const QColor& c)
{
    if (!mChartPlot) {
        return;
    }
    QwtText tt = mChartPlot->footer();
    tt.setColor(c);
    mChartPlot->setFooter(tt);
}

}  // end da
