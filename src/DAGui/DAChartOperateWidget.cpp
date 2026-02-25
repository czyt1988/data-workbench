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
    std::unique_ptr< DAFigureFactory > mFigureFactory;
};
DAChartOperateWidgetPrivate::DAChartOperateWidgetPrivate(DAChartOperateWidget* p) : q_ptr(p)
{
    mFigureFactory = std::make_unique< DAFigureFactory >();
}

//===================================================
// DAChartOperateWidget
//===================================================
DAChartOperateWidget::DAChartOperateWidget(QWidget* parent)
    : DAAbstractOperateWidget(parent), d_ptr(new DAChartOperateWidgetPrivate(this)), ui(new Ui::DAChartOperateWidget)
{
    ui->setupUi(this);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAChartOperateWidget::onTabWidgetCurrentChanged);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &DAChartOperateWidget::onTabCloseRequested);
}

DAChartOperateWidget::~DAChartOperateWidget()
{
    delete ui;
}

// 获取窗口数量
int DAChartOperateWidget::getFigureCount() const
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
void DAChartOperateWidget::setupFigureFactory(DAFigureFactory* factory)
{
    d_ptr->mFigureFactory.reset(factory);
}

/**
 * @brief 拿出之前的工厂
 * @note 此函数调用后必须@ref setupFigureFactory 否则会异常
 * @return
 */
DAFigureFactory* DAChartOperateWidget::takeFactory()
{
    return d_ptr->mFigureFactory.release();
}

/**
 * @brief 获取工厂
 * @return
 */
DAFigureFactory* DAChartOperateWidget::getFigureFactory() const
{
    return d_ptr->mFigureFactory.get();
}

/**
 * @brief 创建一个绘图
 *
 * @note 重载此函数，如果没有调用DAChartOperateWidget::createFigure，必须调用initFigureConnect(fig);来初始化创建的fig，同时也要发射信号figureCreated
 * @return
 */
DAFigureWidget* DAChartOperateWidget::createFigure(const QString& name)
{
    ++g_figure_cnt;
    QString t = name;
    if (name.isEmpty()) {
        t = tr("figure-%1").arg(g_figure_cnt);
    }
    DAFigureWidget* fig = getFigureFactory()->createFigure();
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
QList< DAFigureWidget* > DAChartOperateWidget::getFigureList() const
{
    QList< DAFigureWidget* > res;
    int count = getFigureCount();
    for (int i = 0; i < count; ++i) {
        DAFigureWidget* fig = getFigure(i);
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
DAFigureWidget* DAChartOperateWidget::getCurrentFigure() const
{
    return qobject_cast< DAFigureWidget* >(ui->tabWidget->currentWidget());
}

/**
 * @brief like matlab/matplotlib gcf api
 * @return
 */
DAFigureWidget* DAChartOperateWidget::gcf() const
{
    return getCurrentFigure();
}

/**
 * @brief 把绘图设置为当前绘图
 * @param index
 */
void DAChartOperateWidget::setCurrentFigure(int index)
{
    ui->tabWidget->setCurrentIndex(index);
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
 * @brief 查找绘图
 * @param id
 * @return
 */
DAFigureWidget* DAChartOperateWidget::findFigure(const QString& id) const
{
    const QList< DAFigureWidget* > figs = getFigureList();
    for (DAFigureWidget* fig : figs) {
        if (fig->getFigureId() == id) {
            return fig;
        }
    }
    return nullptr;
}

/**
 * @brief 获取fig的命名
 * @param index
 * @return
 */
QString DAChartOperateWidget::getFigureName(int index) const
{
    return ui->tabWidget->tabText(index);
}

QString DAChartOperateWidget::getFigureName(DAFigureWidget* f) const
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
void DAChartOperateWidget::setFigureName(int index, const QString& name)
{
    ui->tabWidget->setTabText(index, name);
}

void DAChartOperateWidget::setFigureName(DAFigureWidget* f, const QString& name)
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
int DAChartOperateWidget::getFigureIndex(DAFigureWidget* f) const
{
    return ui->tabWidget->indexOf(f);
}

/**
 * @brief  删除窗口
 * @param f
 * @param deleteFigure 如果为true，将会把窗口也删除，默认为true
 */
void DAChartOperateWidget::removeFigure(DAFigureWidget* f, bool deleteFigure)
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
DAChartWidget* DAChartOperateWidget::getCurrentChart() const
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        return fig->getCurrentChart();
    }
    return nullptr;
}

/**
 * @brief like matlab/matplotlib gca api
 * @return
 */
DAChartWidget* DAChartOperateWidget::gca() const
{
    return getCurrentChart();
}

/**
 * @brief 获取当前选中绘图的所有图表
 * @return
 */
QList< DAChartWidget* > DAChartOperateWidget::getCurrentCharts() const
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        return fig->getCharts();
    }
    return QList< DAChartWidget* >();
}

/**
 * @brief 获取当前选中绘图的所有图表
 * @return
 */
QList< DAChartWidget* > DAChartOperateWidget::gcas() const
{
    return getCurrentCharts();
}

QUndoStack* DAChartOperateWidget::getUndoStack()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        return fig->getUndoStack();
    }
    return nullptr;
}

/**
 * @brief 清除所有绘图
 */
void DAChartOperateWidget::clear()
{
    const QList< DAFigureWidget* > figs = getFigureList();
    for (DAFigureWidget* fig : figs) {
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
void DAChartOperateWidget::initFigureConnect(DAFigureWidget* fig)
{
    // 信号转发
    connect(fig, &DAFigureWidget::chartAdded, this, &DAChartOperateWidget::chartAdded);
    connect(fig, &DAFigureWidget::chartRemoved, this, &DAChartOperateWidget::chartRemoved);
    connect(fig, &DAFigureWidget::currentChartChanged, this, &DAChartOperateWidget::currentChartChanged);
    connect(fig, &DAFigureWidget::windowTitleChanged, this, &DAChartOperateWidget::onFigureTitleChanged);
}

/**
 * @brief tab窗口改变
 * @param index
 */
void DAChartOperateWidget::onTabWidgetCurrentChanged(int index)
{
    DAFigureWidget* fig = getFigure(index);
    if (nullptr == fig) {
        // 这个是删除最后一个绘图
        // qCritical() << tr("chart operate widget's tab changed,but can not find figure");  // cn:绘图操作窗口的标签改变信号中，无法通过标签索引找到对应的绘图
        return;
    }
    auto un = fig->getUndoStack();
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
void DAChartOperateWidget::onTabCloseRequested(int index)
{
    DAFigureWidget* fig = getFigure(index);
    if (!fig) {
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::question(this, tr("question"), tr("Whether to close the figure widget"));
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
void DAChartOperateWidget::onFigureTitleChanged(const QString& t)
{
    DAFigureWidget* fig = qobject_cast< DAFigureWidget* >(sender());
    if (fig) {
        int i = getFigureIndex(fig);
        if (i >= 0) {
            ui->tabWidget->setTabText(i, t);
        }
        Q_EMIT figureTitleChanged(fig, t);
    }
}

}  // end DA
