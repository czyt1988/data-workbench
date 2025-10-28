#include "DAChartPlotSettingWidget.h"
#include "ui_DAChartPlotSettingWidget.h"
#include "qwt_text_label.h"
#include <QIcon>
#include <QDebug>
#include "DASignalBlockers.hpp"
namespace DA
{
DAChartPlotSettingWidget::DAChartPlotSettingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartPlotSettingWidget)
{
	ui->setupUi(this);
    connect(ui->lineEditTitle, &QLineEdit::textChanged, this, &DAChartPlotSettingWidget::onTitleTextChanged);
    connect(ui->lineEditFooter, &QLineEdit::textChanged, this, &DAChartPlotSettingWidget::onFooterTextChanged);
    connect(ui->widgetTitleFontEditer,
            &DAFontEditPannelWidget::currentFontChanged,
            this,
            &DAChartPlotSettingWidget::onTitleFontChanged);
	connect(ui->widgetFooterFontEditer,
            &DAFontEditPannelWidget::currentFontChanged,
            this,
            &DAChartPlotSettingWidget::onFooterFontChanged);
	connect(ui->widgetTitleFontEditer,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAChartPlotSettingWidget::onTitleColorChanged);
	connect(ui->widgetFooterFontEditer,
            &DAFontEditPannelWidget::currentFontColorChanged,
            this,
            &DAChartPlotSettingWidget::onFooterColorChanged);
	ui->widgetXBottomAxisSetting->setEnableCheckBoxIcon(QIcon(":/DAGui/ChartSetting/icon/axisXBottom.svg"));
	ui->widgetXTopAxisSetting->setEnableCheckBoxIcon(QIcon(":/DAGui/ChartSetting/icon/axisXTop.svg"));
	ui->widgetYLeftAxisSetting->setEnableCheckBoxIcon(QIcon(":/DAGui/ChartSetting/icon/axisYLeft.svg"));
	ui->widgetYRightAxisSetting->setEnableCheckBoxIcon(QIcon(":/DAGui/ChartSetting/icon/axisYRight.svg"));
}

DAChartPlotSettingWidget::~DAChartPlotSettingWidget()
{
	delete ui;
}

/**
 * @brief 设置chart，可传入空指针
 * @param w
 */
void DAChartPlotSettingWidget::setChartWidget(DAChartWidget* w)
{
	mChartPlot = w;
    updateUI();
}

DAChartWidget* DAChartPlotSettingWidget::getChartWidget() const
{
    return mChartPlot.data();
}

/**
 * @brief 更新界面
 */
void DAChartPlotSettingWidget::updateUI()
{

    DAChartWidget* w = getChartWidget();
    ui->widgetXBottomAxisSetting->setChart(w, QwtPlot::xBottom);
    ui->widgetXTopAxisSetting->setChart(w, QwtPlot::xTop);
    ui->widgetYLeftAxisSetting->setChart(w, QwtPlot::yLeft);
    ui->widgetYRightAxisSetting->setChart(w, QwtPlot::yRight);

    ui->groupBoxXBottom->setCollapsed(!w || !(w->axisEnabled(QwtPlot::xBottom)));
    ui->groupBoxXTop->setCollapsed(!w || !(w->axisEnabled(QwtPlot::xTop)));
    ui->groupBoxYLeft->setCollapsed(!w || !(w->axisEnabled(QwtPlot::yLeft)));
    ui->groupBoxYRight->setCollapsed(!w || !(w->axisEnabled(QwtPlot::yRight)));

    QwtText tt;
    if (w) {
        tt = w->title();
    }
    DASignalBlockers block(ui->lineEditTitle, ui->widgetTitleFontEditer, ui->lineEditFooter, ui->widgetFooterFontEditer);
    ui->lineEditTitle->setText(tt.text());
    ui->widgetTitleFontEditer->setCurrentFont(tt.font());
    ui->widgetTitleFontEditer->setCurrentFontColor(tt.color());

    if (w) {
        tt = w->footer();
    }
    ui->lineEditFooter->setText(tt.text());
    ui->widgetFooterFontEditer->setCurrentFont(tt.font());
    ui->widgetFooterFontEditer->setCurrentFontColor(tt.color());
}

void DAChartPlotSettingWidget::onTitleTextChanged(const QString& t)
{
    DAChartWidget* w = getChartWidget();
    if (!w) {
        return;
    }
    QwtText tt = w->title();
    if (tt.text() == t) {
        return;
    }
	tt.setText(t);
    w->setTitle(tt);
}

void DAChartPlotSettingWidget::onTitleFontChanged(const QFont& f)
{
    DAChartWidget* w = getChartWidget();
    if (!w) {
        return;
    }
    QwtText tt = w->title();
    if (tt.font() == f) {
        return;
    }
	tt.setFont(f);
    w->setTitle(tt);
}

void DAChartPlotSettingWidget::onTitleColorChanged(const QColor& c)
{
    DAChartWidget* w = getChartWidget();
    if (!w) {
        return;
    }
    QwtText tt = w->title();
    if (tt.color() == c) {
        return;
    }
	tt.setColor(c);
    w->setTitle(tt);
}

void DAChartPlotSettingWidget::onFooterTextChanged(const QString& t)
{
    DAChartWidget* w = getChartWidget();
    if (!w) {
        return;
    }
    QwtText tt = w->footer();
    if (tt.text() == t) {
        return;
    }
	tt.setText(t);
    w->setFooter(tt);
}

void DAChartPlotSettingWidget::onFooterFontChanged(const QFont& f)
{
    DAChartWidget* w = getChartWidget();
    if (!w) {
        return;
    }
    QwtText tt = w->footer();
    if (tt.font() == f) {
        return;
    }
	tt.setFont(f);
    w->setFooter(tt);
}

void DAChartPlotSettingWidget::onFooterColorChanged(const QColor& c)
{
    DAChartWidget* w = getChartWidget();
    if (!w) {
        return;
    }
    QwtText tt = w->footer();
    if (tt.color() == c) {
        return;
    }
	tt.setColor(c);
    w->setFooter(tt);
}

}  // end da
