#include "DAPlotOperateWidget.h"
#include "ui_DAPlotOperateWidget.h"
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
    DA_IMPL_PUBLIC(DAPlotOperateWidget)
public:
    DAChartOperateWidgetPrivate(DAPlotOperateWidget* p);

public:
    std::unique_ptr< DAFigureFactory > mFigureFactory;
};
DAChartOperateWidgetPrivate::DAChartOperateWidgetPrivate(DAPlotOperateWidget* p) : q_ptr(p)
{
    mFigureFactory = std::make_unique< DAFigureFactory >();
}

//===================================================
// DAChartOperateWidget
//===================================================
DAPlotOperateWidget::DAPlotOperateWidget(QWidget* parent)
    : DAAbstractOperateWidget(parent), d_ptr(new DAChartOperateWidgetPrivate(this)), ui(new Ui::DAPlotOperateWidget)
{
    ui->setupUi(this);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAPlotOperateWidget::onTabWidgetCurrentChanged);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &DAPlotOperateWidget::onTabCloseRequested);
}

DAPlotOperateWidget::~DAPlotOperateWidget()
{
    delete ui;
}

// 获取窗口数量
int DAPlotOperateWidget::getFigureCount() const
{
    return ui->tabWidget->count();
}

/**
 * @brief 安装FigureFactory，DAChartOperateWidget负责工厂的销毁
 *
 * 工厂在createFigure中调用，在某些情况下，用户可以临时设置一个新工厂，生成一个特殊的figure，再设置回默认的工厂
 * @sa takeFactory
 * @param factory
 */
void DAPlotOperateWidget::setupFigureFactory(DAFigureFactory* factory)
{
    d_ptr->mFigureFactory.reset(factory);
}

/**
 * @brief 拿出之前的工厂
 * @note 此函数调用后必须@ref setupFigureFactory 否则会异常
 * @return
 */
DAFigureFactory* DAPlotOperateWidget::takeFactory()
{
    return d_ptr->mFigureFactory.release();
}

/**
 * @brief 获取工厂
 * @return
 */
DAFigureFactory* DAPlotOperateWidget::getFigureFactory() const
{
    return d_ptr->mFigureFactory.get();
}

/**
 * @brief 创建一个绘图
 *
 * @note 重载此函数，如果没有调用DAChartOperateWidget::createFigure，必须调用initFigureConnect(fig);来初始化创建的fig，同时也要发射信号figureCreated
 * @return
 */
DAFigureScrollArea* DAPlotOperateWidget::createFigure(const QString& name)
{
    ++g_figure_cnt;
    QString t = name;
    if (name.isEmpty()) {
        t = tr("figure-%1").arg(g_figure_cnt);
    }
    DAFigureScrollArea* fig = getFigureFactory()->createFigure();
    fig->setWindowTitle(t);

    // ui->tabWidget->addTab会触发currentFigureChanged信号，这里会发射figureCreated，不触发currentFigureChanged信号
    QSignalBlocker b(ui->tabWidget);
    ui->tabWidget->addTab(fig, t);
    initFigureConnect(fig);
    Q_EMIT figureCreated(fig);
    return fig;
}

/**
 * @brief 获取所有的绘图
 * @return
 */
QList< DAFigureScrollArea* > DAPlotOperateWidget::getFigureList() const
{
    QList< DAFigureScrollArea* > res;
    int count = getFigureCount();
    for (int i = 0; i < count; ++i) {
        DAFigureScrollArea* fig = getFigure(i);
        if (fig) {
            res.append(fig);
        }
    }
    return res;
}

/**
 * @brief 获取当前的fig，如果没有返回nullptr
 * @return
 */
DAFigureScrollArea* DAPlotOperateWidget::getCurrentFigure() const
{
    return qobject_cast< DAFigureScrollArea* >(ui->tabWidget->currentWidget());
}

/**
 * @brief like matlab/matplotlib gcf api
 * @return
 */
DAFigureScrollArea* DAPlotOperateWidget::gcf() const
{
    return getCurrentFigure();
}

/**
 * @brief 把绘图设置为当前绘图
 * @param index
 */
void DAPlotOperateWidget::setCurrentFigure(int index)
{
    ui->tabWidget->setCurrentIndex(index);
}

/**
 * @brief 根据索引获取fig
 * @param index
 * @return
 */
DAFigureScrollArea* DAPlotOperateWidget::getFigure(int index) const
{
    return qobject_cast< DAFigureScrollArea* >(ui->tabWidget->widget(index));
}

/**
 * @brief 获取fig的命名
 * @param index
 * @return
 */
QString DAPlotOperateWidget::getFigureName(int index) const
{
    return ui->tabWidget->tabText(index);
}

QString DAPlotOperateWidget::getFigureName(DAFigureScrollArea* f) const
{
    int index = getFigureIndex(f);
    if (index < 0) {
        return QString();
    }
    return getFigureName(index);
}

/**
 * @brief 设置绘图名称
 * @param index
 * @param name
 */
void DAPlotOperateWidget::setFigureName(int index, const QString& name)
{
    ui->tabWidget->setTabText(index, name);
}

void DAPlotOperateWidget::setFigureName(DAFigureScrollArea* f, const QString& name)
{
    int index = getFigureIndex(f);
    if (index < 0) {
        return;
    }
    setFigureName(index, name);
}

/**
 * @brief 获取fig在DAChartOperateWidget的索引
 * @param f
 * @return
 */
int DAPlotOperateWidget::getFigureIndex(DAFigureScrollArea* f) const
{
    return ui->tabWidget->indexOf(f);
}

/**
 * @brief  删除窗口
 * @param f
 * @param deleteFigure 如果为true，将会把窗口也删除，默认为true
 */
void DAPlotOperateWidget::removeFigure(DAFigureScrollArea* f, bool deleteFigure)
{
    if (!f) {
        return;
    }
    int index = getFigureIndex(f);
    if (index < 0) {
        return;
    }
    Q_EMIT figureRemoving(f);
    ui->tabWidget->removeTab(index);
    if (deleteFigure) {
        f->deleteLater();
    }
}

/**
 * @brief 获取当前的chart，如果没有返回nullptr
 * @return
 */
QIM::QImPlotNode* DAPlotOperateWidget::getCurrentPlot() const
{
    DAFigureScrollArea* fig = getCurrentFigure();
    if (fig) {
        return fig->getCurrentPlot();
    }
    return nullptr;
}

/**
 * @brief like matlab/matplotlib gca api
 * @return
 */
QIM::QImPlotNode* DAPlotOperateWidget::gca() const
{
    return getCurrentPlot();
}

/**
 * @brief 获取当前选中绘图的所有图表
 * @return
 */
QList< QIM::QImPlotNode* > DAPlotOperateWidget::getCurrentCharts() const
{
    DAFigureScrollArea* fig = getCurrentFigure();
    if (fig) {
        return fig->plotNodes();
    }
    return {};
}

/**
 * @brief 获取当前选中绘图的所有图表
 * @return
 */
QList< QIM::QImPlotNode* > DAPlotOperateWidget::gcas() const
{
    return getCurrentCharts();
}

QUndoStack* DAPlotOperateWidget::getUndoStack()
{
    DAFigureScrollArea* fig = getCurrentFigure();
    if (fig) {
        return fig->undoStack();
    }
    return nullptr;
}

/**
 * @brief 清除所有绘图
 */
void DAPlotOperateWidget::clear()
{
    const QList< DAFigureScrollArea* > figs = getFigureList();
    for (DAFigureScrollArea* fig : figs) {
        removeFigure(fig, true);
    }
    ui->tabWidget->clear();
}

/**
 * @brief 初始化figure的连接
 *
 * 这个函数用于重载createFigure函数时创建fig后绑定槽函数到DAChartOperateWidget用
 * @param fig
 */
void DAPlotOperateWidget::initFigureConnect(DAFigureScrollArea* fig)
{
    // 信号转发
    connect(fig, &DAFigureScrollArea::plotAdded, this, &DAPlotOperateWidget::plotAdded);
    connect(fig, &DAFigureScrollArea::plotRemoved, this, &DAPlotOperateWidget::plotRemoved);
    connect(fig, &DAFigureScrollArea::currentPlotChanged, this, &DAPlotOperateWidget::currentPlotChanged);
    connect(fig, &DAFigureScrollArea::windowTitleChanged, this, &DAPlotOperateWidget::onFigureTitleChanged);
}

/**
 * @brief tab窗口改变
 * @param index
 */
void DAPlotOperateWidget::onTabWidgetCurrentChanged(int index)
{
    DAFigureScrollArea* fig = getFigure(index);
    if (nullptr == fig) {
        // 这个是删除最后一个绘图
        // qCritical() << tr("chart operate widget's tab changed,but can not find figure");  // cn:绘图操作窗口的标签改变信号中，无法通过标签索引找到对应的绘图
        return;
    }
    auto un = fig->undoStack();
    if (un) {
        if (!un->isActive()) {
            un->setActive(true);
        }
    }
    Q_EMIT currentFigureChanged(fig, index);
}

/**
 * @brief tab窗口关闭
 * @param index
 */
void DAPlotOperateWidget::onTabCloseRequested(int index)
{
    DAFigureScrollArea* fig = getFigure(index);
    if (!fig) {
        return;
    }
    QMessageBox::StandardButton btn =
        QMessageBox::question(this, tr("question"), tr("Whether to close the figure widget"));
    if (QMessageBox::Yes != btn) {
        return;
    }
    // 这里不能直接调用removeFigure，removeFigure里面会判断tab
    removeFigure(fig, true);
}

/**
 * @brief 绘图的标题改变槽函数
 * @param t
 */
void DAPlotOperateWidget::onFigureTitleChanged(const QString& t)
{
    DAFigureScrollArea* fig = qobject_cast< DAFigureScrollArea* >(sender());
    if (fig) {
        int i = getFigureIndex(fig);
        if (i >= 0) {
            ui->tabWidget->setTabText(i, t);
        }
        Q_EMIT figureTitleChanged(fig, t);
    }
}

}  // end DA
