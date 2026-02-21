#include "DAPlotNodeSettingWidget.h"
#include "ui_DAPlotNodeSettingWidget.h"
#include <QIcon>
#include <QDebug>
#include "DASignalBlockers.hpp"
// qim
#include "plot/QImPlotNode.h"
namespace DA
{
DAPlotNodeSettingWidget::DAPlotNodeSettingWidget(QWidget* parent) : QWidget(parent), ui(new Ui::DAPlotNodeSettingWidget)
{
    ui->setupUi(this);
    connect(ui->lineEditTitle, &QLineEdit::textChanged, this, &DAPlotNodeSettingWidget::onTitleTextChanged);
    connect(ui->lineEditFooter, &QLineEdit::textChanged, this, &DAPlotNodeSettingWidget::onFooterTextChanged);
    connect(ui->widgetTitleFontEditer, &DAFontEditPannelWidget::currentFontChanged, this, &DAPlotNodeSettingWidget::onTitleFontChanged);
    connect(ui->widgetFooterFontEditer, &DAFontEditPannelWidget::currentFontChanged, this, &DAPlotNodeSettingWidget::onFooterFontChanged);
    connect(ui->widgetTitleFontEditer, &DAFontEditPannelWidget::currentFontColorChanged, this, &DAPlotNodeSettingWidget::onTitleColorChanged);
    connect(
        ui->widgetFooterFontEditer, &DAFontEditPannelWidget::currentFontColorChanged, this, &DAPlotNodeSettingWidget::onFooterColorChanged
    );
}

DAPlotNodeSettingWidget::~DAPlotNodeSettingWidget()
{
    delete ui;
}

/**
 * @brief 设置chart，可传入空指针
 * @param w
 */
void DAPlotNodeSettingWidget::setPlotNode(QIM::QImPlotNode* w)
{
    if (mChartPlot == w) {
        return;
    }
    mChartPlot = w;
    updateUI();
}

QIM::QImPlotNode* DAPlotNodeSettingWidget::getPlotNode() const
{
    return mChartPlot.data();
}

/**
 * @brief 更新界面
 */
void DAPlotNodeSettingWidget::updateUI()
{
    QIM::QImPlotNode* plot = getPlotNode();
    DASignalBlockers block(ui->lineEditTitle, ui->widgetTitleFontEditer, ui->lineEditFooter, ui->widgetFooterFontEditer);
    if (plot) {
        ui->lineEditTitle->setText(plot->title());
    } else {
        ui->lineEditTitle->setText(QString());
    }

    ui->widgetTitleFontEditer->setCurrentFont(tt.font());
    ui->widgetTitleFontEditer->setCurrentFontColor(tt.color());

    if (plot) {
        tt = plot->footer();
    }
    ui->lineEditFooter->setText(tt.text());
    ui->widgetFooterFontEditer->setCurrentFont(tt.font());
    ui->widgetFooterFontEditer->setCurrentFontColor(tt.color());
}

void DAPlotNodeSettingWidget::onTitleTextChanged(const QString& t)
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

void DAPlotNodeSettingWidget::onTitleFontChanged(const QFont& f)
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

void DAPlotNodeSettingWidget::onTitleColorChanged(const QColor& c)
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

void DAPlotNodeSettingWidget::onFooterTextChanged(const QString& t)
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

void DAPlotNodeSettingWidget::onFooterFontChanged(const QFont& f)
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

void DAPlotNodeSettingWidget::onFooterColorChanged(const QColor& c)
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
