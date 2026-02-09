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
}

DAChartPlotSettingWidget::~DAChartPlotSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置chart，可传入空指针
 * @param w
 */
void DAChartPlotSettingWidget::setPlot(QwtPlot* w)
{
    if (mChartPlot == w) {
        return;
    }
    mChartPlot = w;
    updateUI();
}

QwtPlot* DAChartPlotSettingWidget::getPlot() const
{
    return mChartPlot.data();
}

/**
 * @brief 更新界面
 */
void DAChartPlotSettingWidget::updateUI()
{

    QwtPlot* plot = getPlot();

    QwtText tt;
    if (plot) {
        tt = plot->title();
    }
    DASignalBlockers block(ui->lineEditTitle, ui->widgetTitleFontEditer, ui->lineEditFooter, ui->widgetFooterFontEditer);
    ui->lineEditTitle->setText(tt.text());
    ui->widgetTitleFontEditer->setCurrentFont(tt.font());
    ui->widgetTitleFontEditer->setCurrentFontColor(tt.color());

    if (plot) {
        tt = plot->footer();
    }
    ui->lineEditFooter->setText(tt.text());
    ui->widgetFooterFontEditer->setCurrentFont(tt.font());
    ui->widgetFooterFontEditer->setCurrentFontColor(tt.color());
}

void DAChartPlotSettingWidget::onTitleTextChanged(const QString& t)
{
    QwtPlot* plot = getPlot();
    if (!plot) {
        return;
    }
    QwtText tt = plot->title();
    if (tt.text() == t) {
        return;
    }
    tt.setText(t);
    plot->setTitle(tt);
}

void DAChartPlotSettingWidget::onTitleFontChanged(const QFont& f)
{
    QwtPlot* plot = getPlot();
    if (!plot) {
        return;
    }
    QwtText tt = plot->title();
    if (tt.font() == f) {
        return;
    }
    tt.setFont(f);
    plot->setTitle(tt);
}

void DAChartPlotSettingWidget::onTitleColorChanged(const QColor& c)
{
    QwtPlot* plot = getPlot();
    if (!plot) {
        return;
    }
    QwtText tt = plot->title();
    if (tt.color() == c) {
        return;
    }
    tt.setColor(c);
    plot->setTitle(tt);
}

void DAChartPlotSettingWidget::onFooterTextChanged(const QString& t)
{
    QwtPlot* w = getPlot();
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
    QwtPlot* w = getPlot();
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
    QwtPlot* w = getPlot();
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
