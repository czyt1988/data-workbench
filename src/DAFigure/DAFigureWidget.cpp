#include "DAFigureWidget.h"
#include <QApplication>
#include <QMessageBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QAction>
#include <QMimeData>
#include <QPaintEvent>
#include <QCoreApplication>
#include <QScopedPointer>
#include <QChildEvent>
#include <QCursor>
#include <QPainter>
#include <QVBoxLayout>
#include <QUndoStack>
#include <QDebug>
#include <QScopedPointer>
#include <QPointer>
#include <QKeyEvent>
#include <QClipboard>
// chart
#include "DAChartUtil.h"
#include "DAChartWidget.h"
#include "DAChartSerialize.h"
#include "DAFigureWidgetOverlay.h"
#include "DAFigureWidgetCommands.h"
// qwt
#include "qwt_figure.h"
#include "qwt_figure_layout.h"
#include "qwt_scale_draw.h"

#ifndef DAFigureWidget_DEBUG_PRINT
#define DAFigureWidget_DEBUG_PRINT 1
#endif
namespace DA
{
const QRectF c_figurewidget_default_size = QRectF(0.05, 0.05, 0.9, 0.9);
//===================================================
// DAFigureWidgetPrivate
//===================================================

class DAFigureWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureWidget)
public:
    QPointer< DAFigureWidgetOverlay > mChartEditorOverlay;  ///< 编辑模式
    QBrush mBackgroundBrush;                                ///< 背景
    QUndoStack mUndoStack;                                  ///<
    QScopedPointer< DAChartFactory > mFactory;              ///< 绘图创建的工厂
    DAColorTheme mColorTheme;  ///< 主题，注意，这里不要用DAColorTheme mColorTheme { DAColorTheme::ColorTheme_Archambault }这样的初始化，会被当作std::initializer_list< QColor >捕获
public:
    PrivateData(DAFigureWidget* p) : q_ptr(p), mColorTheme(DAColorTheme::Style_Matplotlib_Tab10)
    {
        mFactory.reset(new DAChartFactory());
    }

    void retranslateUi()
    {
        q_ptr->setWindowTitle(QApplication::translate("DAFigureWidget", "Figure", 0));
    }
};

//===================================================
// DAFigureWidget
//===================================================
DAFigureWidget::DAFigureWidget(QWidget* parent) : QScrollArea(parent), DA_PIMPL_CONSTRUCT
{
    init();
}

DAFigureWidget::~DAFigureWidget()
{
}

/**
 * @brief 获取绘图窗口
 * @return
 */
QwtFigure* DAFigureWidget::figure() const
{
    return qobject_cast< QwtFigure* >(widget());
}

void DAFigureWidget::init()
{
    setWindowIcon(QIcon(":/DAFigure/icon/figure.svg"));
    setFocusPolicy(Qt::ClickFocus);
    setBackgroundColor(QColor(255, 255, 255));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWidgetResizable(true);
    setAlignment(Qt::AlignCenter);  // 居中显示
    setMinimumWidth(100);
    setMinimumHeight(50);
    QwtFigure* figure = new QwtFigure();
    setWidget(figure);
    static int s_figure_count = 0;
    ++s_figure_count;
    setWindowTitle(QString("figure-%1").arg(s_figure_count));

    connect(figure, &QwtFigure::axesAdded, this, &DAFigureWidget::onAxesAdded);
    connect(figure, &QwtFigure::axesRemoved, this, &DAFigureWidget::onAxesRemoved);
    connect(figure, &QwtFigure::currentAxesChanged, this, &DAFigureWidget::onCurrentAxesChanged);
}

DAChartFactory* DAFigureWidget::getChartFactory() const
{
    return d_ptr->mFactory.get();
}

/**
 * @brief 设置ChartFactory
 * @param fac
 */
void DAFigureWidget::setupChartFactory(DAChartFactory* fac)
{
    d_ptr->mFactory.reset(fac);
}

/**
 * @brief 添加一个chart
 *
 * 默认的位置占比为0.05f, 0.05f, 0.9f, 0.9f
 * @return  返回2D绘图的指针
 */
DAChartWidget* DAFigureWidget::createChart()
{
    return (createChart(c_figurewidget_default_size));
}

/**
 * @brief 创建绘图
 * @param versatileSize
 * @param relativePos
 * @return
 */
DAChartWidget* DAFigureWidget::createChart(const QRectF& versatileSize)
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);

    DAChartWidget* chart = d_ptr->mFactory->createChart(this);
    addChart(chart, versatileSize);

    // 对于有Overlay，需要把Overlay提升到最前面，否则会被覆盖
    if (d_ptr->mChartEditorOverlay) {
        d_ptr->mChartEditorOverlay->raise();  // 同时提升最前
    }
    return chart;
}

/**
 * @brief 添加一个chart，指定位置占比
 * @param xVersatile
 * @param yPresent
 * @param wPresent
 * @param hPresent
 * @return
 */
DAChartWidget* DAFigureWidget::createChart(float xVersatile, float yVersatile, float wVersatile, float hVersatile)
{

    return createChart(QRectF(xVersatile, yVersatile, wVersatile, hVersatile));
}

/**
 * @brief 移除chart，但不会delete
 * @param chart
 */
void DAFigureWidget::removeChart(DAChartWidget* chart)
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    fig->removeAxes(chart);
}

void DAFigureWidget::removeChart_(DAChartWidget* chart)
{
    d_ptr->mUndoStack.push(new DAFigureWidgetCommandRemoveChart(this, chart));
}

/**
 * @brief 支持redo/undo的createchart
 * @return
 */
DAChartWidget* DAFigureWidget::createChart_()
{
    return createChart_(c_figurewidget_default_size);
}

/**
 * @brief 支持redo/undo的createchart
 * @param versatileSize
 * @param yPresent
 * @param wVersatile
 * @param hPresent
 * @return
 */
DAChartWidget* DAFigureWidget::createChart_(const QRectF& versatileSize)
{
    DAFigureWidgetCommandCreateChart* cmd = new DAFigureWidgetCommandCreateChart(this, versatileSize);
    d_ptr->mUndoStack.push(cmd);
    // 必须先push再获取chart
    return cmd->getChartWidget();
}

/**
 * @brief 添加一个chart，指定位置占比
 * @param chart 绘图
 * @param xPresent
 * @param yPresent
 * @param wVersatile
 * @param hPresent
 * @return
 */
void DAFigureWidget::addChart(DAChartWidget* chart, qreal xVersatile, qreal yVersatile, qreal wVersatile, qreal hVersatile)
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    // 将会发射QwtFigure::axesAdded信号
    fig->addAxes(chart, xVersatile, yVersatile, wVersatile, hVersatile);
    //! 不清楚为何如果不加这句话，坐标轴的轴线不绘制出来
    /*
    //! TODO:未来需要确认是什么原因导致的
    chart->axisWidget(QwtAxis::XBottom)->setScaleDraw(new QwtScaleDraw());
    chart->axisWidget(QwtAxis::XTop)->setScaleDraw(new QwtScaleDraw());
    chart->axisWidget(QwtAxis::YRight)->setScaleDraw(new QwtScaleDraw());
    chart->axisWidget(QwtAxis::YLeft)->setScaleDraw(new QwtScaleDraw());
    */
    // 由于使用了layout管理，因此要显示调用show
    chart->show();
}

void DAFigureWidget::addChart(DAChartWidget* chart, const QRectF& versatileSize)
{
    addChart(chart, versatileSize.x(), versatileSize.y(), versatileSize.width(), versatileSize.height());
}

/**
 * @brief 获取所有的绘图
 * @return
 */
QList< DAChartWidget* > DAFigureWidget::getCharts() const
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    QList< DAChartWidget* > res;
    const QList< QwtPlot* > plots = fig->allAxes();

    for (QwtPlot* p : plots) {
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(p);
        if (chart) {
            res.append(chart);
        }
    }
    return (res);
}

/**
 * @brief 当前的图表的指针
 * @return 当没有2d绘图时返回nullptr
 */
DAChartWidget* DAFigureWidget::getCurrentChart() const
{
    return qobject_cast< DAChartWidget* >(figure()->currentAxes());
}

/**
 * @brief like matlab/matplotlib api gca
 * @return
 */
DAChartWidget* DAFigureWidget::gca() const
{
    return getCurrentChart();
}

/**
 * @brief 清空同时删除
 */
void DAFigureWidget::clear()
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    fig->clear();
}

/**
 * @brief 设置figure背景
 * @param brush
 */
void DAFigureWidget::setBackgroundColor(const QBrush& brush)
{
    d_ptr->mBackgroundBrush = brush;
    update();
}

/**
 * @brief 设置figure背景
 * @param clr
 */
void DAFigureWidget::setBackgroundColor(const QColor& clr)
{
    d_ptr->mBackgroundBrush.setStyle(Qt::SolidPattern);
    d_ptr->mBackgroundBrush.setColor(clr);
    update();
}

/**
 * @brief 获取背景颜色
 * @return
 */
const QBrush& DAFigureWidget::getBackgroundColor() const
{
    return (d_ptr->mBackgroundBrush);
}

/**
 * @brief 设置当前的chart
 * @param p 如果p和当前的currentChart一样，不做任何动作
 * @return 如果成功设置返回true，如果当前窗口已经是p，则返回true，但不会发射currentWidgetChanged信号
 * @sa currentWidgetChanged
 */
void DAFigureWidget::setCurrentChart(QwtPlot* p)
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    fig->setCurrentAxes(p);
}

/**
 * @brief 获取当前的chart，如果没有current chart，或figure不存在chart，
 * 则创建一个新chart，此函数不返回nullptr
 * @return
 */
DAChartWidget* DAFigureWidget::currentChart()
{
    DAChartWidget* w = getCurrentChart();
    if (w) {
        return w;
    }
    // 到这里说明没有chart
    QList< DAChartWidget* > cs = getCharts();
    if (!cs.empty()) {
        return cs.first();
    }
    return createChart();
}

/**
 * @brief 通过item查找对应的SAChart2D，如果没有返回nullptr
 * @param item
 * @return 如果没有返回nullptr
 */
DAChartWidget* DAFigureWidget::findChartFromItem(QwtPlotItem* item) const
{
    QList< DAChartWidget* > charts = getCharts();

    for (DAChartWidget* w : qAsConst(charts)) {
        QwtPlotItemList items = w->itemList();
        if (items.contains(item)) {
            return (w);
        }
    }
    return (nullptr);
}

///
/// \brief 是否开始子窗口编辑模式
/// \param enable
/// \param ptr 通过此参数可以指定自定义的编辑器，若为nullptr，将使用默认的编辑器，此指针的管理权将移交SAFigureWindow
///
void DAFigureWidget::setSubChartEditorEnable(bool enable)
{
#if DAFigureWidget_DEBUG_PRINT
    qDebug() << "DAFigureWidget::setSubChartEditorEnable=" << enable;
#endif
    if (enable) {
        if (nullptr == d_ptr->mChartEditorOverlay) {
            d_ptr->mChartEditorOverlay = new DAFigureWidgetOverlay(figure());
            connect(d_ptr->mChartEditorOverlay,
                    &DAFigureWidgetOverlay::widgetNormGeometryChanged,
                    this,
                    &DAFigureWidget::onWidgetGeometryChanged);
            connect(d_ptr->mChartEditorOverlay,
                    &DAFigureWidgetOverlay::activeWidgetChanged,
                    this,
                    &DAFigureWidget::onOverlayActiveWidgetChanged);
            d_ptr->mChartEditorOverlay->show();
            d_ptr->mChartEditorOverlay->raise();  // 同时提升最前
        } else {
            if (d_ptr->mChartEditorOverlay->isHidden()) {
                d_ptr->mChartEditorOverlay->show();
                d_ptr->mChartEditorOverlay->raise();  // 同时提升最前
            }
        }
    } else {
        if (d_ptr->mChartEditorOverlay) {
            delete d_ptr->mChartEditorOverlay;
            d_ptr->mChartEditorOverlay = nullptr;
        }
    }
}

///
/// \brief 获取子窗口编辑器指针，若没有此编辑器，返回nullptr
///
/// 此指针的管理权在SAFigureWindow上，不要在外部对此指针进行释放
/// \return
///
DAFigureWidgetOverlay* DAFigureWidget::getSubChartEditor() const
{
    return (d_ptr->mChartEditorOverlay);
}

/**
 * @brief SAFigureWindow::isSubWindowEditingMode
 * @return
 */
bool DAFigureWidget::isEnableSubChartEditor() const
{
    if (d_ptr->mChartEditorOverlay) {
        return (d_ptr->mChartEditorOverlay->isVisible());
    }
    return (false);
}

/**
 * @brief 获取图表的数量
 * @return
 */
int DAFigureWidget::getChartCount() const
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    const QList< QwtPlot* > plots = fig->allAxes();
    return (plots.size());
}

/**
 * @brief 获取默认的绘图颜色
 *
 * 每次调用figure的绘图相关函数，会调用getDefaultColor获取默认颜色为曲线填充，
 * 默认会根据颜色主题来变换颜色，也可以继承此函数，让figure每次给出自定义的颜色
 * @return
 */
QColor DAFigureWidget::getDefaultColor() const
{
    return (d_ptr->mColorTheme)++;
}

void DAFigureWidget::setColorTheme(const DAColorTheme& th)
{
    d_ptr->mColorTheme = th;
    // 同步应用样式
    const QList< QwtPlot* > plots = figure()->allAxes();
    for (QwtPlot* plot : plots) {
        const QList< QwtPlot* > plotWithparasite = plot->plotList();
        DAColorTheme theme                       = th;
        if (theme.size() <= 0) {
            theme = DAColorTheme(DAColorTheme::Style_Cassatt1);
        }
        for (QwtPlot* p : plotWithparasite) {
            const QwtPlotItemList items = p->itemList();
            for (QwtPlotItem* item : items) {
                if (!DAChartUtil::isPlotGraphicsItem(item)) {
                    continue;
                }
                DAChartUtil::setPlotItemColor(item, ++theme);
            }
        }
    }
    figure()->replotAll();
}

DAColorTheme DAFigureWidget::getColorTheme() const
{
    return d_ptr->mColorTheme;
}

const DAColorTheme& DAFigureWidget::colorTheme() const
{
    return d_ptr->mColorTheme;
}

DAColorTheme& DAFigureWidget::colorTheme()
{
    return d_ptr->mColorTheme;
}

void DAFigureWidget::copyToClipboard()
{
    // 捕获当前窗口的截图
    QPixmap screenshot = figure()->grab();
    // 将截图放到剪贴板
    QApplication::clipboard()->setPixmap(screenshot);
}

QRectF DAFigureWidget::axesNormRect(QwtPlot* plot) const
{
    return figure()->axesNormRect(plot);
}

QRectF DAFigureWidget::widgetNormRect(QWidget* w) const
{
    return figure()->widgetNormRect(w);
}

void DAFigureWidget::addWidget(QWidget* widget, qreal left, qreal top, qreal width, qreal height)
{
    figure()->addWidget(widget, left, top, width, height);
}

void DAFigureWidget::addWidget(QWidget* widget, int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace)
{
    figure()->addWidget(widget, rowCnt, colCnt, row, col, rowSpan, colSpan, wspace, hspace);
}

void DAFigureWidget::setWidgetNormPos(QWidget* widget, const QRectF& rect)
{
    figure()->setWidgetNormPos(widget, rect);
}
/**
 * @brief 获取在此坐标下的绘图，如果此坐标下没有，则返回nullptr，存在寄生轴情况只返回宿主轴
 * @param pos 相对于DAFigureWidget的位置
 * @return
 */
QwtPlot* DAFigureWidget::plotUnderPos(const QPoint& pos) const
{
    // 先要把pos映射到figure
    QwtFigure* fig  = figure();
    QPoint posOfFig = fig->mapFromParent(pos);
    return figure()->plotUnderPos(posOfFig);
}

void DAFigureWidget::setFaceBrush(const QBrush& brush)
{
    QwtFigure* fig = figure();
    fig->setFaceBrush(brush);
    fig->update();
}

QBrush DAFigureWidget::getFaceBrush() const
{
    return figure()->faceBrush();
}

/**
 * @brief 支持redo/undo的添加item
 *
 * 等同addItem_(gca(),item)
 * @param item
 * @return 如果没有加入成功，返回false
 */
bool DAFigureWidget::addItem_(QwtPlotItem* item)
{
    DAChartWidget* chart = gca();
    if (!chart) {
        return false;
    }
    addItem_(chart, item);
    return true;
}

/**
 * @brief 支持redo/undo的添加item
 * @param chart
 * @param item
 */
void DAFigureWidget::addItem_(DAChartWidget* chart, QwtPlotItem* item)
{
    push(new DAFigureWidgetCommandAttachItem(this, chart, item, false));
}

/**
 * @brief 支持redo/undo的addCurve，等同于gca()->addCurve
 * @param xyDatas
 * @return 如果添加失败，返回一个nullptr
 */
QwtPlotCurve* DAFigureWidget::addCurve_(const QVector< QPointF >& xyDatas)
{
    DAChartWidget* chart = gca();
    if (!chart) {
        return nullptr;
    }
    QwtPlotCurve* item = chart->addCurve(xyDatas);
    DAChartUtil::setPlotItemColor(item, getDefaultColor());
    addItem_(chart, item);
    return item;
}

/**
 * @brief 支持redo/undo的addScatter，等同于gca()->addCurve
 * @param xyDatas
 * @return 如果添加失败，返回一个nullptr
 */
QwtPlotCurve* DAFigureWidget::addScatter_(const QVector< QPointF >& xyDatas)
{
    DAChartWidget* chart = gca();
    if (!chart) {
        return nullptr;
    }
    QwtPlotCurve* item = chart->addScatter(xyDatas);
    DAChartUtil::setPlotItemColor(item, getDefaultColor());
    addItem_(chart, item);
    return item;
}

/**
 * @brief 支持redo/undo的addBar，等同于gca()->addBar
 * @param xyDatas
 * @return 如果添加失败，返回一个nullptr
 */
QwtPlotBarChart* DAFigureWidget::addBar_(const QVector< QPointF >& xyDatas)
{
    DAChartWidget* chart = gca();
    if (!chart) {
        return nullptr;
    }
    QwtPlotBarChart* item = chart->addBarChart(xyDatas);
    DAChartUtil::setPlotItemColor(item, getDefaultColor());
    addItem_(chart, item);
    return item;
}

/**
 * @brief 支持redo/undo的addBar，等同于gca()->addBar
 * @param xyDatas
 * @return 如果添加失败，返回一个nullptr
 */
QwtPlotIntervalCurve* DAFigureWidget::addErrorBar_(const QVector< double >& values,
                                                   const QVector< double >& mins,
                                                   const QVector< double >& maxs)
{
    if (DAChartWidget* chart = gca()) {
        QwtPlotIntervalCurve* item = chart->addIntervalCurve(values, mins, maxs);
        DAChartUtil::setPlotItemColor(item, getDefaultColor());
        addItem_(chart, item);
        return item;
    }
    return nullptr;
}

/**
 * @brief 推入一个命令
 * @param cmd
 */
void DAFigureWidget::push(QUndoCommand* cmd)
{
    d_ptr->mUndoStack.push(cmd);
}

/**
 * @brief 获取内部的undoStack
 * @return
 */
QUndoStack* DAFigureWidget::getUndoStack()
{
    return &(d_ptr->mUndoStack);
}

void DAFigureWidget::keyPressEvent(QKeyEvent* e)
{
    QKeySequence keySeq(e->key() | e->modifiers());
    if (keySeq == QKeySequence::Copy) {
        // 同上
        copyToClipboard();
        e->accept();
        return;
    }
    QScrollArea::keyPressEvent(e);
}

/**
 * @brief 返回当前光标下的widget
 * @return 如果当前没有返回nullptr
 */
QWidget* DAFigureWidget::getUnderCursorWidget() const
{
    QPoint p = mapFromGlobal(QCursor::pos());
    return (childAt(p));
}

/**
 * @brief 返回在当前光标下的2D图
 * @return 如果当前没有返回nullptr
 */
DAChartWidget* DAFigureWidget::getUnderCursorChart() const
{
    QWidget* w = getUnderCursorWidget();
    return qobject_cast< DAChartWidget* >(w);
}

/**
 * @brief DAFigureWidgetChartRubberbandEditOverlay导致的尺寸变化
 * @param w 子窗体
 * @param oldGeometry 旧尺寸
 * @param newGeometry 新尺寸
 * @note QwtFigureWidgetOverlay并不会直接改变尺寸，因此尺寸的改变主要在管理窗口中执行，这是为了能让它有更大的自由度，例如需要做回退功能
 */
void DAFigureWidget::onWidgetGeometryChanged(QWidget* w, const QRectF& oldNormGeo, const QRectF& newNormGeo)
{
    Q_UNUSED(oldNormGeo);
    DAFigureWidgetCommandResizeWidget* cmd = new DAFigureWidgetCommandResizeWidget(this, w, oldNormGeo, newNormGeo);
    push(cmd);
    // 由于设置geo会有一定误差，因此，这里需要更新一下overlay
    if (d_ptr->mChartEditorOverlay) {
        d_ptr->mChartEditorOverlay->updateOverlay();
    }
}

/**
 * @brief DAFigureOverlayChartEditor的激活窗口变化
 *
 * 此槽函数用于改变当前的chart
 * @param oldActive
 * @param newActive
 */
void DAFigureWidget::onOverlayActiveWidgetChanged(QWidget* oldActive, QWidget* newActive)
{
    Q_UNUSED(oldActive);
    DAChartWidget* c = qobject_cast< DAChartWidget* >(newActive);
    if (c) {
        setCurrentChart(c);
    }
}

void DAFigureWidget::onAxesAdded(QwtPlot* newAxes)
{
    if (DAChartWidget* c = qobject_cast< DAChartWidget* >(newAxes)) {
        Q_EMIT chartAdded(c);
    }
}

void DAFigureWidget::onAxesRemoved(QwtPlot* removedAxes)
{
    if (DAChartWidget* c = qobject_cast< DAChartWidget* >(removedAxes)) {
        Q_EMIT chartRemoved(c);
    }
}

void DAFigureWidget::onCurrentAxesChanged(QwtPlot* plot)
{
    DAChartWidget* chartWidget = plot ? qobject_cast< DAChartWidget* >(plot) : nullptr;
    // 如果有子窗口编辑器，把编辑器的激活窗口改变
    if (d_ptr->mChartEditorOverlay) {
        d_ptr->mChartEditorOverlay->setActiveWidget(plot);
    }
    Q_EMIT currentChartChanged(chartWidget);
}

QDataStream& operator<<(QDataStream& out, const DAFigureWidget* p)
{
    const uint32_t magicStart = 0x1314abc;

    out << magicStart << p->saveGeometry();
    QList< DAChartWidget* > charts = p->getCharts();
    QList< QRectF > pos;
    QwtFigure* fig = p->figure();
    if (fig) {
        for (int i = 0; i < charts.size(); ++i) {
            pos.append(fig->axesNormRect(charts[ i ]));
        }
        out << pos;
        for (int i = 0; i < charts.size(); ++i) {
            out << charts[ i ];
        }
    }

    return (out);
}

QDataStream& operator>>(QDataStream& in, DAFigureWidget* p)
{
    const uint32_t magicStart = 0x1314abc;
    int tmp;

    in >> tmp;
    if (tmp != magicStart) {
        throw DABadSerializeExpection("DAFigureWidget get invalid magic strat code");  // cn: DAFigureWidget的文件头异常
        return (in);
    }
    QByteArray geometryData, stateData;

    in >> geometryData;
    p->restoreGeometry(geometryData);
    QList< QRectF > pos;

    in >> pos;
    try {
        for (int i = 0; i < pos.size(); ++i) {
            const QRectF& r = pos[ i ];
            auto chart      = p->createChart(r.x(), r.y(), r.width(), r.height());
            std::unique_ptr< DAChartWidget > chart_guard(chart);
            in >> chart;
            chart->show();
            chart_guard.release();
        }
    } catch (const DABadSerializeExpection& exp) {
        throw exp;
    }
    return (in);
}

}
