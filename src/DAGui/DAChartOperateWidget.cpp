#include "DAChartOperateWidget.h"
#include "ui_DAChartOperateWidget.h"
// stl
#include <memory>
// Qt
#include <QUndoStack>
#include <QMessageBox>
#include <QDebug>
// DAFigure
#include "DAFigureFactory.h"
namespace DA
{
int g_figure_cnt = 0;  ///< 绘图的数量，仅限当前程序创建计数
//==============================================================
// DAChartOperateWidgetPrivate
//==============================================================
class DAChartOperateWidgetPrivate
{
    DA_IMPL_PUBLIC(DAChartOperateWidget)
public:
    DAChartOperateWidgetPrivate(DAChartOperateWidget* p);

public:
    std::unique_ptr< DAFigureFactory > _figureFactory;
};
DAChartOperateWidgetPrivate::DAChartOperateWidgetPrivate(DAChartOperateWidget* p) : q_ptr(p)
{
    _figureFactory = std::make_unique< DAFigureFactory >();
}

//===================================================
// DAChartOperateWidget
//===================================================
DAChartOperateWidget::DAChartOperateWidget(QWidget* parent)
    : QWidget(parent), d_ptr(new DAChartOperateWidgetPrivate(this)), ui(new Ui::DAChartOperateWidget)
{
    ui->setupUi(this);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAChartOperateWidget::onTabWidgetCurrentChanged);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &DAChartOperateWidget::onTabCloseRequested);
}

DAChartOperateWidget::~DAChartOperateWidget()
{
    delete ui;
}

/**
 * @brief 安装FigureFactory，针对继承的DAFigureWidget需要安装新的工厂,DAChartOperateWidget负责工厂的销毁
 * @param factory
 */
void DAChartOperateWidget::setupFigureFactory(DAFigureFactory* factory)
{
    d_ptr->_figureFactory.reset(factory);
}

/**
 * @brief 创建一个绘图
 * @return
 */
DAFigureWidget* DAChartOperateWidget::createFigure()
{
    ++g_figure_cnt;
    return createFigure(tr("figure-%1").arg(g_figure_cnt));
}

/**
 * @brief 创建一个绘图
 * @return
 */
DAFigureWidget* DAChartOperateWidget::createFigure(const QString& title)
{
    DAFigureWidget* fig = d_ptr->_figureFactory->createFigure();
    ui->tabWidget->addTab(fig, title);
    //信号转发
    connect(fig, &DAFigureWidget::chartAdded, this, &DAChartOperateWidget::chartAdded);
    connect(fig, &DAFigureWidget::chartWillRemove, this, &DAChartOperateWidget::chartWillRemove);
    connect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartOperateWidget::currentChartChanged);
    emit figureCreated(fig);
    return fig;
}

/**
 * @brief 获取当前的fig，如果没有返回nullptr
 * @return
 */
DAFigureWidget* DAChartOperateWidget::getCurrentFigure() const
{
    return qobject_cast< DAFigureWidget* >(ui->tabWidget->currentWidget());
}

/**
 * @brief 根据索引获取fig
 * @param index
 * @return
 */
DAFigureWidget* DAChartOperateWidget::getFigure(int index) const
{
    return qobject_cast< DAFigureWidget* >(ui->tabWidget->widget(index));
}

/**
 * @brief 获取fig在DAChartOperateWidget的索引
 * @param f
 * @return
 */
int DAChartOperateWidget::getFigureIndex(DAFigureWidget* f)
{
    return ui->tabWidget->indexOf(f);
}

/**
 * @brief 获取当前的chart，如果没有返回nullptr
 * @return
 */
DAChartWidget* DAChartOperateWidget::getCurrentChart() const
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        return fig->getCurrentChart();
    }
    return nullptr;
}

/**
 * @brief tab窗口改变
 * @param index
 */
void DAChartOperateWidget::onTabWidgetCurrentChanged(int index)
{
    DAFigureWidget* fig = getFigure(index);
    if (nullptr == fig) {
        qCritical() << tr("chart operate widget's tab changed,but can not find figure");  // cn:绘图操作窗口的标签改变信号中，无法通过标签索引找到对应的绘图
        return;
    }
    fig->getUndoStack()->setActive();
    emit currentFigureChanged(fig, index);
}

/**
 * @brief tab窗口关闭
 * @param index
 */
void DAChartOperateWidget::onTabCloseRequested(int index)
{
    QMessageBox::StandardButton btn = QMessageBox::question(this, tr("question"), tr("Whether to close the figure widget"));
    if (QMessageBox::Yes != btn) {
        return;
    }
    QWidget* w          = ui->tabWidget->widget(index);
    DAFigureWidget* fig = qobject_cast< DAFigureWidget* >(w);
    if (fig) {
        emit figureCloseing(fig);
    }
    ui->tabWidget->removeTab(index);
    w->deleteLater();
}

}  // end DA
