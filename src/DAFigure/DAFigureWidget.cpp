#include "DAFigureWidget.h"
#include <QtWidgets/QApplication>
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
// chart
#include "DAChartWidget.h"
#include "DAChartSerialize.h"
#include "DAFigureContainer.h"
#include "DAFigureWidgetOverlayChartEditor.h"
namespace DA
{
const float c_default_x = 0.05f;
const float c_default_y = 0.05f;
const float c_default_w = 0.9f;
const float c_default_h = 0.9f;
//===================================================
// command
//===================================================

/**
 * @brief DAFigureWidget命令的基本体
 */
class DAFigureWidgetCommandBase : public QUndoCommand
{
public:
    DAFigureWidgetCommandBase(DAFigureWidget* fig, QUndoCommand* par = nullptr) : QUndoCommand(par), mFig(fig)
    {
    }
    DAFigureWidget* figure()
    {
        return mFig;
    }

public:
    DAFigureWidget* mFig;
};

/**
 * @brief 创建绘图
 */
class DAFigureWidgetCommandCreateChart : public DAFigureWidgetCommandBase
{
public:
    DAFigureWidgetCommandCreateChart(DAFigureWidget* fig, float xPresent, float yPresent, float wPresent, float hPresent, QUndoCommand* par = nullptr)
        : DAFigureWidgetCommandBase(fig, par)
        , mChart(nullptr)
        , mXPresent(xPresent)
        , mYPresent(yPresent)
        , mWPresent(wPresent)
        , mHPresent(hPresent)
        , mNeedDelete(false)
    {
        setText(QObject::tr("create chart"));  // cn:创建绘图
    }
    ~DAFigureWidgetCommandCreateChart()
    {
        if (mNeedDelete) {
            if (mChart) {
                mChart->deleteLater();
            }
        }
    }
    void redo() override
    {
        mNeedDelete = false;
        if (mChart) {
            figure()->addChart(mChart, mXPresent, mYPresent, mWPresent, mHPresent);
        } else {
            mChart = figure()->createChart(mXPresent, mYPresent, mWPresent, mHPresent);
            mChart->setXLabel("x");
            mChart->setYLabel("y");
        }
    }
    void undo() override
    {
        mNeedDelete = true;
        figure()->removeChart(mChart);
    }

public:
    DAChartWidget* mChart;
    float mXPresent;
    float mYPresent;
    float mWPresent;
    float mHPresent;
    bool mNeedDelete;
};

/**
 * @brief 设置绘图中窗体的尺寸
 */
class DAFigureWidgetCommandResizeWidget : public DAFigureWidgetCommandBase
{
public:
    DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig, QWidget* w, const QRectF& oldPresent, const QRectF& newPresent, QUndoCommand* par = nullptr)
        : DAFigureWidgetCommandBase(fig, par), mWidget(w), mOldPresent(oldPresent), mNewPresent(newPresent)
    {
        setText(QObject::tr("set figure widget size"));  // cn:设置绘图中窗体的尺寸
    }
    void redo() override
    {
        figure()->setWidgetPosPercent(mWidget, mNewPresent);
    }
    void undo() override
    {
        figure()->setWidgetPosPercent(mWidget, mOldPresent);
    }

public:
    QWidget* mWidget;
    QRectF mOldPresent;
    QRectF mNewPresent;
};

//===================================================
// DAFigureWidgetPrivate
//===================================================

class DAFigureWidgetPrivate
{
    DA_IMPL_PUBLIC(DAFigureWidget)
public:
    DAChartWidget* _currentChart;
    DAFigureWidgetOverlayChartEditor* _chartEditorOverlay;  ///< 编辑模式
    QBrush _backgroundBrush;                                ///< 背景
    QUndoStack _undoStack;                                  ///<

    DAFigureWidgetPrivate(DAFigureWidget* p) : q_ptr(p), _currentChart(nullptr), _chartEditorOverlay(nullptr)
    {
    }

    void setupUI()
    {
        q_ptr->setBackgroundColor(QColor(255, 255, 255));
        q_ptr->setWindowIcon(QIcon(":/da-figure/icon/figure.svg"));
    }

    void retranslateUi()
    {
        q_ptr->setWindowTitle(QApplication::translate("DAFigureWidget", "Figure", 0));
    }
};

//===================================================
// DAFigureWidget
//===================================================
DAFigureWidget::DAFigureWidget(QWidget* parent) : DAFigureContainer(parent), d_ptr(new DAFigureWidgetPrivate(this))
{
    d_ptr->setupUI();
    setFocusPolicy(Qt::ClickFocus);
    static int s_figure_count = 0;

    ++s_figure_count;
    setWindowTitle(QString("figure-%1").arg(s_figure_count));
    setMinimumWidth(100);
    setMinimumHeight(50);
}

DAFigureWidget::~DAFigureWidget()
{
    // qDebug() << "SAFigureWindow destroy";
}

/**
 * @brief 添加一个chart
 *
 * 默认的位置占比为0.05f, 0.05f, 0.9f, 0.9f
 * @return  返回2D绘图的指针
 */
DAChartWidget* DAFigureWidget::createChart()
{
    return (createChart(c_default_x, c_default_y, c_default_w, c_default_h));
}

/**
 * @brief 添加一个chart，指定位置占比
 * @param xPresent
 * @param yPresent
 * @param wPresent
 * @param hPresent
 * @return
 */
DAChartWidget* DAFigureWidget::createChart(float xPresent, float yPresent, float wPresent, float hPresent)
{
    DAChartWidget* chart = new DAChartWidget(this);
    addChart(chart, xPresent, yPresent, wPresent, hPresent);
    //不加这句话，有时候不显示出来
    chart->show();
    //对于有Overlay，需要把Overlay提升到最前面，否则会被覆盖
    if (d_ptr->_chartEditorOverlay) {
        d_ptr->_chartEditorOverlay->raise();  //同时提升最前
    }
    return chart;
}

/**
 * @brief 移除chart，但不会delete
 * @param chart
 */
void DAFigureWidget::removeChart(DAChartWidget* chart)
{
    removeWidget(chart);
}

/**
 * @brief 支持redo/undo的createchart
 * @return
 */
DAChartWidget* DAFigureWidget::createChart_()
{
    return createChart_(c_default_x, c_default_y, c_default_w, c_default_h);
}

/**
 * @brief 支持redo/undo的createchart
 * @param xPresent
 * @param yPresent
 * @param wPresent
 * @param hPresent
 * @return
 */
DAChartWidget* DAFigureWidget::createChart_(float xPresent, float yPresent, float wPresent, float hPresent)
{
    DAFigureWidgetCommandCreateChart* cmd = new DAFigureWidgetCommandCreateChart(this, xPresent, yPresent, wPresent, hPresent);
    d_ptr->_undoStack.push(cmd);
    //必须先push再获取chart
    return cmd->mChart;
}

/**
 * @brief 添加一个chart，指定位置占比
 * @param chart 绘图
 * @param xPresent
 * @param yPresent
 * @param wPresent
 * @param hPresent
 * @return
 */
void DAFigureWidget::addChart(DAChartWidget* chart, float xPresent, float yPresent, float wPresent, float hPresent)
{
    addWidget(chart, xPresent, yPresent, wPresent, hPresent);
    d_ptr->_currentChart = chart;
    emit chartAdded(chart);
    setFocusProxy(chart);
}

/**
 * @brief 获取所有的绘图
 * @return
 */
QList< DAChartWidget* > DAFigureWidget::getCharts() const
{
    QList< DAChartWidget* > res;
    QList< QWidget* > widgets = getWidgetList();

    for (QWidget* w : qAsConst(widgets)) {
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(w);
        if (chart) {
            res.append(chart);
        }
    }
    return (res);
}

/**
 * @brief 获取所有的绘图,绘图有序
 * @return
 */
QList< DAChartWidget* > DAFigureWidget::getChartsOrdered() const
{
    QList< DAChartWidget* > res;
    QList< QWidget* > widgets = getOrderedWidgetList();

    for (QWidget* w : qAsConst(widgets)) {
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(w);
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
    return (d_ptr->_currentChart);
}

/**
 * @brief 清空同时删除
 */
void DAFigureWidget::clearAllCharts()
{
    QList< DAChartWidget* > charts = getCharts();

    while (!charts.isEmpty()) {
        DAChartWidget* p = charts.takeLast();
        emit chartWillRemove(p);
        p->hide();
        p->deleteLater();
    }
}

/**
 * @brief 设置figure背景
 * @param brush
 */
void DAFigureWidget::setBackgroundColor(const QBrush& brush)
{
    d_ptr->_backgroundBrush = brush;
    repaint();
}

/**
 * @brief 设置figure背景
 * @param clr
 */
void DAFigureWidget::setBackgroundColor(const QColor& clr)
{
    d_ptr->_backgroundBrush.setStyle(Qt::SolidPattern);
    d_ptr->_backgroundBrush.setColor(clr);
    repaint();
}

/**
 * @brief 获取背景颜色
 * @return
 */
const QBrush& DAFigureWidget::getBackgroundColor() const
{
    return (d_ptr->_backgroundBrush);
}

/**
 * @brief 设置当前的chart
 * @param p 如果p和当前的currentChart一样，不做任何动作
 * @return 如果成功设置返回true，如果当前窗口已经是p，则返回true，但不会发射currentWidgetChanged信号
 * @sa currentWidgetChanged
 */
bool DAFigureWidget::setCurrentChart(DAChartWidget* p)
{
    if (p == d_ptr->_currentChart) {
        return (true);
    }
    if (!isWidgetInContainer(p)) {
        return (false);
    }
    d_ptr->_currentChart = p;
    // setFocusProxy(p);
    //如果在进行子窗口编辑模式，此时需要重新设置编辑
    if (isEnableChartEditor()) {
        d_ptr->_chartEditorOverlay->setActiveWidget(p);
    }
    emit currentChartChanged(p);

    return (true);
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
void DAFigureWidget::enableChartEditor(bool enable)
{
    if (enable) {
        if (nullptr == d_ptr->_chartEditorOverlay) {
            d_ptr->_chartEditorOverlay = new DAFigureWidgetOverlayChartEditor(this);
            connect(d_ptr->_chartEditorOverlay, &DAFigureWidgetOverlayChartEditor::widgetGeometryChanged, this, &DAFigureWidget::onWidgetGeometryChanged);
            connect(d_ptr->_chartEditorOverlay, &DAFigureWidgetOverlayChartEditor::activeWidgetChanged, this, &DAFigureWidget::onOverlayActiveWidgetChanged);
            d_ptr->_chartEditorOverlay->show();
            d_ptr->_chartEditorOverlay->raise();  //同时提升最前
        } else {
            if (d_ptr->_chartEditorOverlay->isHidden()) {
                d_ptr->_chartEditorOverlay->show();
                d_ptr->_chartEditorOverlay->raise();  //同时提升最前
            }
        }
    } else {
        if (d_ptr->_chartEditorOverlay) {
            delete d_ptr->_chartEditorOverlay;
            d_ptr->_chartEditorOverlay = nullptr;
        }
    }
}

///
/// \brief 获取子窗口编辑器指针，若没有此编辑器，返回nullptr
///
/// 此指针的管理权在SAFigureWindow上，不要在外部对此指针进行释放
/// \return
///
DAFigureWidgetOverlayChartEditor* DAFigureWidget::getChartEditorOverlay() const
{
    return (d_ptr->_chartEditorOverlay);
}

/**
 * @brief SAFigureWindow::isSubWindowEditingMode
 * @return
 */
bool DAFigureWidget::isEnableChartEditor() const
{
    if (d_ptr->_chartEditorOverlay) {
        return (d_ptr->_chartEditorOverlay->isVisible());
    }
    return (false);
}

/**
 * @brief 推入一个命令
 * @param cmd
 */
void DAFigureWidget::push(QUndoCommand* cmd)
{
    d_ptr->_undoStack.push(cmd);
}

/**
 * @brief 获取内部的undoStack
 * @return
 */
QUndoStack* DAFigureWidget::getUndoStack()
{
    return &(d_ptr->_undoStack);
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
 * @brief SAFigureWindow::paintEvent
 * @param e
 */
void DAFigureWidget::paintEvent(QPaintEvent* e)
{
    QPainter p(this);

    p.setBrush(d_ptr->_backgroundBrush);
    p.fillRect(0, 0, width(), height(), d_ptr->_backgroundBrush);
    DAFigureContainer::paintEvent(e);
}

/**
 * @brief DAFigureWidgetChartRubberbandEditOverlay导致的尺寸变化
 * @param w 子窗体
 * @param oldGeometry 旧尺寸
 * @param newGeometry 新尺寸
 */
void DAFigureWidget::onWidgetGeometryChanged(QWidget* w, const QRect& oldGeometry, const QRect& newGeometry)
{
    Q_UNUSED(oldGeometry);
    QRectF oldPresent = getWidgetPosPercent(w);
    QRectF newPresent = calcPercentByGeometryRect(newGeometry);

    DAFigureWidgetCommandResizeWidget* cmd = new DAFigureWidgetCommandResizeWidget(this, w, oldPresent, newPresent);
    push(cmd);
    //由于设置geo会有一定误差，因此，这里需要更新一下overlay
    if (d_ptr->_chartEditorOverlay) {
        d_ptr->_chartEditorOverlay->updateOverlay();
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

QDataStream& operator<<(QDataStream& out, const DAFigureWidget* p)
{
    const uint32_t magicStart = 0x1314abc;

    out << magicStart << p->saveGeometry();
    QList< DAChartWidget* > charts = p->getCharts();
    QList< QRectF > pos;

    for (int i = 0; i < charts.size(); ++i) {
        pos.append(p->getWidgetPosPercent(charts[ i ]));
    }
    out << pos;
    for (int i = 0; i < charts.size(); ++i) {
        out << charts[ i ];
    }
    return (out);
}

QDataStream& operator>>(QDataStream& in, DAFigureWidget* p)
{
    const uint32_t magicStart = 0x1314abc;
    int tmp;

    in >> tmp;
    if (tmp != magicStart) {
        throw DABadSerializeExpection(QObject::tr("DAFigureWidget get invalid magic strat code"));  // cn: DAFigureWidget的文件头异常
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
            QScopedPointer< DAChartWidget > chart(p->createChart(r.x(), r.y(), r.width(), r.height()));
            in >> chart.data();
            chart->show();
            chart.take();
        }
    } catch (const DABadSerializeExpection& exp) {
        throw exp;
    }
    return (in);
}
}
