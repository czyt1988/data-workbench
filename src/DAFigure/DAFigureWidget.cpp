#include "DAFigureWidget.h"
// stl
#include <functional>
// Qt
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
#include <QHash>
#include <QUuid>
#include <QTimer>
// chart
#include "DAChartUtil.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "DAChartSerialize.h"
#include "DAChart3DSerialize.h"
#include "DAFigureWidgetOverlay.h"
#include "DAFigureWidgetCommands.h"
#include "DAChartAxisRangeBinder.h"
#include "DAChartRectRegionSelectEditor.h"
#include "DAChartSelectRegionShapeItem.h"
#include "DAChartEllipseRegionSelectEditor.h"
#include "DAChartPolygonRegionSelectEditor.h"
#include "DAChartItemCreatInteractor.h"
#include "DAChartArrowEditor.h"
#include "DADataProbeMarker.h"
#include "DALogCategory.h"
// qwt
#include "qwt_figure.h"
#include "qwt_color_cycle.h"
#include "qwt_figure_layout.h"
#include "qwt_scale_draw.h"
#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_series_data_picker_group.h"
// qwt3d
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"

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
    QString mId;
    QBrush mBackgroundBrush;                     ///< 背景
    QUndoStack mUndoStack;                       ///<
    std::unique_ptr< DAChartFactory > mFactory;  ///< 绘图创建的工厂
    DAColorTheme mColorTheme;  ///< 主题，注意，这里不要用DAColorTheme mColorTheme { DAColorTheme::ColorTheme_Archambault }这样的初始化，会被当作std::initializer_list< QColor >捕获
    QList< std::shared_ptr< DAChartAxisRangeBinder > > mAxisRangeBinders;
    QwtPlotSeriesDataPickerGroup* mPickerGroup { nullptr };
    DAFigureWidgetOverlay* mChartEditor { nullptr };  ///< 绘图编辑器
    int mProbeNameCounter { 0 };                      ///< 探针命名计数器
    QList< DAChart3DWidget* > m_3dCharts;            ///< 3D chart 列表
    QPointer< DAChart3DWidget > mCurrent3DChart;    ///< 当前选中的 3D chart
public:
    PrivateData(DAFigureWidget* p) : q_ptr(p), mColorTheme(DAColorTheme::Style_Matplotlib_Tab10)
    {
        mFactory.reset(new DAChartFactory());
        mId = QUuid::createUuid().toString();
    }

    void retranslateUi()
    {
        q_ptr->setWindowTitle(QApplication::translate("DAFigureWidget", "Figure", 0));
    }

    // 将当前颜色主题同步为 qwt 的颜色循环并设置到指定绘图上。
    // 之后新附加到该绘图的图元会按主题自动取色（见 QwtPlot::nextColorForItem）。
    void syncColorCycleToChart(QwtPlot* chart) const
    {
        if (!chart) {
            return;
        }
        QList< QColor > cols = mColorTheme.toColorList();
        if (cols.isEmpty()) {
            cols = DAColorTheme(DAColorTheme::Style_Cassatt1).toColorList();
        }
        chart->setColorCycle(QwtColorCycle(QVector< QColor >(cols.cbegin(), cols.cend())));
    }

    std::shared_ptr< DAChartAxisRangeBinder >
    findAxisRangeBinder(QwtPlot* source, QwtAxisId sourceAxisid, QwtPlot* follower, QwtAxisId followerAxisid)
    {
        for (const auto& b : std::as_const(mAxisRangeBinders)) {
            if (b->isSame(source, sourceAxisid, follower, followerAxisid)) {
                return b;
            }
        }
        return nullptr;
    }

    // 嵌套类定义（在类作用域，允许模板）
    template< typename EditorType, typename... Args >
    struct EditorFactory
    {
        DAFigureWidget* fig;
        std::tuple< std::decay_t< Args >... > args;

        EditorFactory(DAFigureWidget* f, Args&&... a) : fig(f), args(std::forward< Args >(a)...)
        {
        }

        DAAbstractChartEditor* create(QwtPlot* plot)
        {
            return createImpl(plot, std::index_sequence_for< Args... > { });
        }

    private:
        template< std::size_t... I >
        EditorType* createImpl(QwtPlot* plot, std::index_sequence< I... >)
        {
            EditorType* editor = new EditorType(plot, std::get< I >(std::move(args))...);

            // 信号连接 - 使用 fig 而不是 this
            DAFigureWidget::connect(
                editor, &EditorType::beginEdit, fig, [ fig = this->fig ]() { fig->emitChartEditorBeginEdit(); });

            DAFigureWidget::connect(editor, &EditorType::finishedEdit, fig, [ fig = this->fig, editor, plot ](bool isCancel) {
                if (isCancel)
                    return;

                QwtPlotItem* item = editor->takeItem();
                if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(plot)) {
                    fig->addItem_(chart, item, true);
                } else {
                    daCritical << tr("Unexpected plotting operation: a chart that does not belong to the DAChartWidget "
                                  "type was added to the figure");  //cn:意外的绘图操作：不属于 DAChartWidget 类型的图表被添加到了 figure 中
                    item->detach();
                    delete item;
                }
                fig->emitChartEditorFinishEdit();
            });

            return editor;
        }
    };

    /**
     * @brief 开始选择编辑器
     *
     * 选择编辑器适合单一点击操作，多次点击操作
     * @tparam EditorType 选择编辑器类型
     * @param 可变参数，为EditorType构造时传入的可变参数
     */
    template< typename EditorType, typename... Args >
    DAFigureChartEditorWidgetOverlay* beginSelectEditor(Args&&... args)
    {
        DAFigureWidget* fig = q_ptr;

        // 修正：显式指定模板参数
        EditorFactory< EditorType, Args... > factory(fig, std::forward< Args >(args)...);

        auto fun = [ factory ](QwtPlot* plot) mutable -> DAAbstractChartEditor* { return factory.create(plot); };

        DAFigureChartEditorWidgetOverlay* figEditor = new DAFigureChartEditorWidgetOverlay(fig->figure(), fun);
        figEditor->setEnabled(true);
        figEditor->show();

        DAFigureWidget::connect(
            figEditor, &DAFigureChartEditorWidgetOverlay::finished, q_ptr, &DAFigureWidget::onFigureChartEditorFinished);
        DAFigureWidget::connect(
            figEditor, &DAFigureWidgetOverlay::activeWidgetChanged, q_ptr, &DAFigureWidget::onOverlayActiveWidgetChanged);
        return figEditor;
    }

    void beginSubChartEditor();
    void beginRectSelectEditor();
    void beginEllipseSelectEditor();
    void beginPolygonSelectEditor();
    void beginHLineMarkerEditor();
    void beginVLineMarkerEditor();
    void beginCrossLineMarkerEditor();
    void beginArrowMarkerEditor();
    void beginVerticalProbeEditor();
    void beginHorizontalProbeEditor();
};

/**
 * @brief 开始子图编辑器
 *
 * 创建子图编辑器覆盖层，用于调整子图的位置和大小
 */
void DAFigureWidget::PrivateData::beginSubChartEditor()
{
    DAFigureWidget* fig = q_ptr;
    mChartEditor       = new DAFigureWidgetOverlay(fig->figure());
    mChartEditor->show();
    mChartEditor->raise();
    fig->emitChartEditorBeginEdit();
    DAFigureWidget::connect(
        mChartEditor, &DAFigureWidgetOverlay::widgetNormGeometryChanged, fig, &DAFigureWidget::onWidgetGeometryChanged);
    DAFigureWidget::connect(
        mChartEditor, &DAFigureWidgetOverlay::activeWidgetChanged, fig, &DAFigureWidget::onOverlayActiveWidgetChanged);
}

/**
 * @brief 开始矩形区域选择编辑器
 */
void DAFigureWidget::PrivateData::beginRectSelectEditor()
{
    mChartEditor = beginSelectEditor< DAChartRectRegionSelectEditor >();
}

/**
 * @brief 开始椭圆区域选择编辑器
 */
void DAFigureWidget::PrivateData::beginEllipseSelectEditor()
{
    mChartEditor = beginSelectEditor< DAChartEllipseRegionSelectEditor >();
}

/**
 * @brief 开始多边形区域选择编辑器
 */
void DAFigureWidget::PrivateData::beginPolygonSelectEditor()
{
    mChartEditor = beginSelectEditor< DAChartPolygonRegionSelectEditor >();
}

/**
 * @brief 开始水平线标记编辑器
 */
void DAFigureWidget::PrivateData::beginHLineMarkerEditor()
{
    mChartEditor = beginSelectEditor< DAChartItemCreatInteractor >(createHLineMarkerPlotItem);
}

/**
 * @brief 开始垂直线标记编辑器
 */
void DAFigureWidget::PrivateData::beginVLineMarkerEditor()
{
    mChartEditor = beginSelectEditor< DAChartItemCreatInteractor >(createVLineMarkerPlotItem);
}

/**
 * @brief 开始十字线标记编辑器
 */
void DAFigureWidget::PrivateData::beginCrossLineMarkerEditor()
{
    mChartEditor = beginSelectEditor< DAChartItemCreatInteractor >(createCrossLineMarkerPlotItem);
}

/**
 * @brief 开始箭头标记编辑器
 */
void DAFigureWidget::PrivateData::beginArrowMarkerEditor()
{
    mChartEditor = beginSelectEditor< DAChartArrowEditor >();
}

/**
 * @brief 开始垂直探针编辑器
 */
void DAFigureWidget::PrivateData::beginVerticalProbeEditor()
{
    mChartEditor = beginSelectEditor< DAChartItemCreatInteractor >(createVerticalDataProbePlotItem);
}

/**
 * @brief 开始水平探针编辑器
 */
void DAFigureWidget::PrivateData::beginHorizontalProbeEditor()
{
    mChartEditor = beginSelectEditor< DAChartItemCreatInteractor >(createHorizontalDataProbePlotItem);
}

//===================================================
// DAFigureWidget
//===================================================
/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DAFigureWidget::DAFigureWidget(QWidget* parent) : QScrollArea(parent), DA_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief 析构函数
 */
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

/**
 * @brief 获取Figure的唯一标识
 * @return Figure的id字符串
 * @sa setFigureId
 */
QString DAFigureWidget::getFigureId() const
{
    return d_ptr->mId;
}

/**
 * @brief 设置Figure的唯一标识
 * @param id Figure的id字符串
 * @sa getFigureId
 */
void DAFigureWidget::setFigureId(const QString& id)
{
    d_ptr->mId = id;
}

/**
 * @brief 初始化函数
 *
 * 设置窗口属性、创建Figure、连接信号槽等初始化操作
 */
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

/**
 * @brief 设置数据拾取器分组
 *
 * 将所有绘图的QwtPlotSeriesDataPicker添加到一个分组中，实现联动拾取
 */
void DAFigureWidget::setupDataPickerGroup()
{
    DA_D(d);
    if (!d->mPickerGroup) {
        d->mPickerGroup = new QwtPlotSeriesDataPickerGroup(this);
    }
    // 把所有绘图的picker添加到分组中
    //  QwtPlotSeriesDataPickerGroup会自动过滤重复添加的picker
    const QList< QwtPlot* > plots = figure()->allAxes();
    for (QwtPlot* plot : plots) {
        DAChartWidget* chart = qobject_cast< DAChartWidget* >(plot);
        if (!chart) {
            continue;
        }
        auto* picker = chart->getDataPicker();
        if (!picker) {
            continue;
        }
        d->mPickerGroup->addPicker(picker);
    }
}

/**
 * @brief 发射图表编辑器开始编辑信号
 * @sa chartEditorStatusChanged
 */
void DAFigureWidget::emitChartEditorBeginEdit()
{
    Q_EMIT chartEditorStatusChanged(BeginEdit);
}

/**
 * @brief 发射图表编辑器结束编辑信号
 * @sa chartEditorStatusChanged
 */
void DAFigureWidget::emitChartEditorFinishEdit()
{
    Q_EMIT chartEditorStatusChanged(EndEdit);
}

/**
 * @brief 获取绘图工厂
 * @return 绘图工厂指针
 * @sa setupChartFactory
 */
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
    // 设置AxisScale，让qwt内部的map数据和坐标系显示一致，注意不要设置为0~1000，坐标轴默认就是0~1000,会跳过设置
    chart->setAxisScale(QwtAxis::XBottom, 0, 800);
    chart->setAxisScale(QwtAxis::YLeft, 0, 500);
    addChart(chart, versatileSize);

    // 对于有Overlay，需要把Overlay提升到最前面，否则会被覆盖
    if (d_ptr->mChartEditor) {
        d_ptr->mChartEditor->raise();  // 同时提升最前
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

/**
 * @brief 支持redo/undo的移除chart
 * @param chart 要移除的绘图
 * @sa removeChart
 */
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
    // 让新绘图继承 figure 的颜色主题（qwt 颜色循环），新附加的图元据此自动取色
    d_ptr->syncColorCycleToChart(chart);
    connect(chart, &DAChartWidget::chartPropertiesChanged, this, &DAFigureWidget::onChartPropertyChanged);
    // 由于使用了layout管理，因此要显示调用show
    chart->show();
}

/**
 * @brief 添加一个chart，指定归一化位置
 * @param chart 绘图
 * @param versatileSize 归一化的位置和大小
 * @sa addChart
 */
void DAFigureWidget::addChart(DAChartWidget* chart, const QRectF& versatileSize)
{
    addChart(chart, versatileSize.x(), versatileSize.y(), versatileSize.width(), versatileSize.height());
}

// ========== 3D chart 管理 ==========

/**
 * @brief 添加一个3D chart
 *
 * 默认的位置占比为0.05f, 0.05f, 0.9f, 0.9f
 * @return 返回3D绘图的指针
 */
DAChart3DWidget* DAFigureWidget::create3DChart()
{
    return create3DChart(c_figurewidget_default_size);
}

/**
 * @brief 创建3D绘图
 * @param versatileSize 归一化位置
 * @return
 */
DAChart3DWidget* DAFigureWidget::create3DChart(const QRectF& versatileSize)
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);

    DAChart3DWidget* chart3d = new DAChart3DWidget(this);
    add3DChart(chart3d, versatileSize);

    // 对于有Overlay，需要把Overlay提升到最前面，否则会被覆盖
    if (d_ptr->mChartEditor) {
        d_ptr->mChartEditor->raise();
    }
    return chart3d;
}

/**
 * @brief 添加一个3D chart，指定位置占比
 */
DAChart3DWidget* DAFigureWidget::create3DChart(float xVersatile, float yVersatile, float wVersatile, float hVersatile)
{
    return create3DChart(QRectF(xVersatile, yVersatile, wVersatile, hVersatile));
}

/**
 * @brief 添加一个3D chart，指定位置占比
 * @param chart3d 3D绘图
 * @param xVersatile
 * @param yVersatile
 * @param wVersatile
 * @param hVersatile
 */
void DAFigureWidget::add3DChart(DAChart3DWidget* chart3d, qreal xVersatile, qreal yVersatile, qreal wVersatile, qreal hVersatile)
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    // 使用 addWidget 添加 3D chart widget（DAChart3DWidget 不是 QwtPlot 子类，不能使用 addAxes）
    fig->addWidget(chart3d, xVersatile, yVersatile, wVersatile, hVersatile);
    // 安装事件过滤器，用于检测3D chart被点击时设置为current3DChart
    chart3d->installEventFilter(this);
    // 监听销毁信号，自动清理引用。
    // 先 disconnect 再 connect，避免 undo/redo 场景下（add3DChart 被多次调用）累积重复连接
    disconnect(chart3d, &QObject::destroyed, this, nullptr);
    connect(chart3d, &QObject::destroyed, this, [this](QObject* obj) {
        DAChart3DWidget* destroyedChart = static_cast< DAChart3DWidget* >(obj);
        d_ptr->m_3dCharts.removeAll(destroyedChart);
        if (d_ptr->mCurrent3DChart == destroyedChart) {
            d_ptr->mCurrent3DChart = nullptr;
            Q_EMIT current3DChartChanged(nullptr);
        }
    });
    d_ptr->m_3dCharts.append(chart3d);
    chart3d->show();
    Q_EMIT chart3DAdded(chart3d);
}

/**
 * @brief 添加一个3D chart，指定归一化位置
 * @param chart3d 3D绘图
 * @param versatileSize 归一化的位置和大小
 * @sa add3DChart
 */
void DAFigureWidget::add3DChart(DAChart3DWidget* chart3d, const QRectF& versatileSize)
{
    add3DChart(chart3d, versatileSize.x(), versatileSize.y(), versatileSize.width(), versatileSize.height());
}

/**
 * @brief 移除3D chart，但不会delete
 * @param chart3d
 */
void DAFigureWidget::remove3DChart(DAChart3DWidget* chart3d)
{
    if (!chart3d) {
        return;
    }
    // 移除事件过滤器
    chart3d->removeEventFilter(this);
    // 断开 destroyed 信号连接（add3DChart 中建立），避免 undo/redo 场景下重复连接累积
    disconnect(chart3d, &QObject::destroyed, this, nullptr);
    // 显式从 QwtFigureLayout 中移除 layout item（参考 QwtFigure::takeAxes() 的做法）
    // QwtFigure 没有提供 removeWidget(QWidget*) 方法，只有 removeAxes(QwtPlot*)
    QwtFigure* fig = figure();
    QLayout* lay = fig->layout();
    if (lay) {
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (!item) {
                continue;
            }
            QWidget* w = item->widget();
            if (w == chart3d) {
                lay->removeItem(item);
                delete item;
                break;
            }
        }
    }
    // 从父窗口分离
    chart3d->setParent(nullptr);
    chart3d->hide();
    // 从跟踪列表中移除
    d_ptr->m_3dCharts.removeAll(chart3d);
    // 清理 current 指针
    if (d_ptr->mCurrent3DChart == chart3d) {
        d_ptr->mCurrent3DChart = nullptr;
        Q_EMIT current3DChartChanged(nullptr);
    }
    Q_EMIT chart3DRemoved(chart3d);
}

/**
 * @brief 支持redo/undo的移除3D chart
 * @param chart3d 要移除的3D绘图
 * @sa remove3DChart
 */
void DAFigureWidget::remove3DChart_(DAChart3DWidget* chart3d)
{
    d_ptr->mUndoStack.push(new DAFigureWidgetCommandRemove3DChart(this, chart3d));
}

/**
 * @brief 支持redo/undo的create3DChart
 * @return
 */
DAChart3DWidget* DAFigureWidget::create3DChart_()
{
    return create3DChart_(c_figurewidget_default_size);
}

/**
 * @brief 支持redo/undo的create3DChart
 * @param versatileSize
 * @return
 */
DAChart3DWidget* DAFigureWidget::create3DChart_(const QRectF& versatileSize)
{
    DAFigureWidgetCommandCreate3DChart* cmd = new DAFigureWidgetCommandCreate3DChart(this, versatileSize);
    d_ptr->mUndoStack.push(cmd);
    return cmd->getChart3DWidget();
}

/**
 * @brief 获取所有的3D绘图
 * @return
 */
QList< DAChart3DWidget* > DAFigureWidget::get3DCharts() const
{
    return d_ptr->m_3dCharts;
}

/**
 * @brief 当前的3D绘图的指针
 * @return 当没有3D绘图时返回nullptr
 */
DAChart3DWidget* DAFigureWidget::getCurrent3DChart() const
{
    return d_ptr->mCurrent3DChart;
}

/**
 * @brief 设置当前的3D绘图
 * @param chart3d 如果和当前的current3DChart一样，不做任何动作
 */
void DAFigureWidget::setCurrent3DChart(DAChart3DWidget* chart3d)
{
    DA_D(d);
    if (d->mCurrent3DChart == chart3d) {
        return;
    }
    d->mCurrent3DChart = chart3d;
    // 通知 overlay 编辑器更新激活窗口（参考 2D 的 onCurrentAxesChanged）
    if (d->mChartEditor) {
        d->mChartEditor->setActiveWidget(chart3d);
    }
    Q_EMIT current3DChartChanged(chart3d);
}

/**
 * @brief 获取当前的3D chart，如果没有current 3D chart，或figure不存在3D chart，
 * 则创建一个新3D chart，此函数不返回nullptr
 * @return
 */
DAChart3DWidget* DAFigureWidget::current3DChart()
{
    DAChart3DWidget* w = getCurrent3DChart();
    if (w) {
        return w;
    }
    // 到这里说明没有current 3D chart
    QList< DAChart3DWidget* > cs = get3DCharts();
    if (!cs.isEmpty()) {
        return cs.first();
    }
    return create3DChart();
}

/**
 * @brief 返回当前光标下的3D图
 * @return 如果当前没有返回nullptr
 */
DAChart3DWidget* DAFigureWidget::getUnderCursor3DChart() const
{
    QWidget* w = getUnderCursorWidget();
    return qobject_cast< DAChart3DWidget* >(w);
}

/**
 * @brief 是否存在这个3D绘图
 * @param chart3d 3D绘图
 * @return
 */
bool DAFigureWidget::has3DChart(DAChart3DWidget* chart3d) const
{
    return d_ptr->m_3dCharts.contains(chart3d);
}

/**
 * @brief 获取3D图表的数量
 * @return
 */
int DAFigureWidget::get3DChartCount() const
{
    return d_ptr->m_3dCharts.size();
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
    // 先清除3D chart
    const QList< DAChart3DWidget* > chart3ds = d_ptr->m_3dCharts;
    for (DAChart3DWidget* chart3d : chart3ds) {
        chart3d->removeEventFilter(this);
        chart3d->disconnect(this);
        // 先从布局中显式移除 layout item（参考 remove3DChart 的做法）
        QLayout* lay = fig->layout();
        if (lay) {
            for (int i = 0; i < lay->count(); ++i) {
                QLayoutItem* item = lay->itemAt(i);
                if (item && item->widget() == chart3d) {
                    lay->removeItem(item);
                    delete item;
                    break;
                }
            }
        }
        chart3d->setParent(nullptr);
        // 信号在 setParent 之后发射，确保接收方查询时 chart 已从布局移除
        Q_EMIT chart3DRemoved(chart3d);
        chart3d->deleteLater();
    }
    d_ptr->m_3dCharts.clear();
    d_ptr->mCurrent3DChart = nullptr;
    // 再清除2D chart
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
    if (!item) {
        return nullptr;
    }
    return qobject_cast< DAChartWidget* >(item->plot());
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
 * @note 自颜色主题迁移到 qwt 的 QwtColorCycle 后，新图元的默认颜色改由
 *       QwtPlot::nextColorForItem 在 attach 时自动分配，本函数已不再被
 *       addCurve_/addScatter_/addBar_/addErrorBar_ 及图表向导调用。保留以兼容
 *       外部继承实现，如需手动取色仍可使用。
 * @return 下一个主题颜色
 */
QColor DAFigureWidget::getDefaultColor() const
{
    return (d_ptr->mColorTheme)++;
}

/**
 * @brief 是否存在这个绘图
 * @param chart 绘图
 * @param includeParasite 是否包括寄生绘图
 * @return
 */
bool DAFigureWidget::hasPlot(QwtPlot* chart, bool includeParasite) const
{
    const QList< QwtPlot* > plots = figure()->allAxes();
    for (QwtPlot* p : plots) {
        if (p == chart) {
            return true;
        }
        if (includeParasite) {
            if (p->isHostPlot()) {
                const QList< QwtPlot* > parasites = p->parasitePlots();
                for (QwtPlot* parasite : parasites) {
                    if (parasite == chart) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/**
 * @brief 绑定坐标轴的范围
 * @param source 主坐标轴绘图
 * @param follower 跟随坐标轴绘图
 * @param axisid 绑定的轴id
 */
bool DAFigureWidget::bindAxisRange(QwtPlot* source, QwtPlot* follower, QwtAxisId axisid)
{
    return bindAxisRange(source, axisid, follower, axisid);
}

/**
 * @brief 绑定坐标轴的范围
 * @param source 主坐标轴绘图
 * @param sourceAxisid 主坐标轴绘图绑定的轴id
 * @param follower 跟随坐标轴绘图
 * @param followerAxisid 跟随坐标轴绘图绑定的轴id
 */
bool DAFigureWidget::bindAxisRange(QwtPlot* source, QwtAxisId sourceAxisid, QwtPlot* follower, QwtAxisId followerAxisid)
{
    if (!QwtAxis::isValid(sourceAxisid) || !QwtAxis::isValid(followerAxisid) || !hasPlot(source) || !hasPlot(follower)) {
        return false;
    }
    if (auto p = d_ptr->findAxisRangeBinder(source, sourceAxisid, follower, followerAxisid)) {
        // 说明已经绑定过
        return false;
    }
    // 构造时自动绑定
    std::shared_ptr< DAChartAxisRangeBinder > binder =
        std::make_shared< DAChartAxisRangeBinder >(source, sourceAxisid, follower, followerAxisid);
    d_ptr->mAxisRangeBinders.push_back(binder);
    return binder->isBinded();
}

/**
 * @brief 解绑坐标轴的范围
 * @param source 主坐标轴绘图
 * @param follower 跟随坐标轴绘图
 * @param axisid 绑定的轴id
 * @return 解绑成功返回true
 * @sa bindAxisRange
 */
bool DAFigureWidget::unbindAxisRange(QwtPlot* source, QwtPlot* follower, QwtAxisId axisid)
{
    return unbindAxisRange(source, axisid, follower, axisid);
}

/**
 * @brief 解绑坐标轴的范围
 * @param source 主坐标轴绘图
 * @param sourceAxisid 主坐标轴绘图绑定的轴id
 * @param follower 跟随坐标轴绘图
 * @param followerAxisid 跟随坐标轴绘图绑定的轴id
 * @return 解绑成功返回true
 * @sa bindAxisRange
 */
bool DAFigureWidget::unbindAxisRange(QwtPlot* source, QwtAxisId sourceAxisid, QwtPlot* follower, QwtAxisId followerAxisid)
{
    if (auto p = d_ptr->findAxisRangeBinder(source, sourceAxisid, follower, followerAxisid)) {
        // 说明已经绑定过
        d_ptr->mAxisRangeBinders.removeAll(p);
        // 解绑
        return p->unbind();
    }
    return false;
}

/**
 * @brief 获取坐标轴绑定信息
 * @return
 */
QList< DAChartAxisRangeBinder* > DAFigureWidget::getBindAxisRangeInfos() const
{
    QList< DAChartAxisRangeBinder* > res;
    for (const auto& b : std::as_const(d_ptr->mAxisRangeBinders)) {
        res.push_back(b.get());
    }
    return res;
}

/**
 * @brief 设置联动数据拾取，联动数据拾取会让当前所有绘图的QwtPlotSeriesDataPicker建立分组，进行联动
 * @param on
 */
void DAFigureWidget::setDataPickerGroupEnabled(bool on)
{
    DA_D(d);
    if (on && !d->mPickerGroup) {
        setupDataPickerGroup();
    }
    if (d->mPickerGroup) {
        d->mPickerGroup->setEnabled(on);
    }
}

/**
 * @brief 联动数据拾取是否启用
 * @return 如果启用返回true
 * @sa setDataPickerGroupEnabled
 */
bool DAFigureWidget::isDataPickerGroupEnabled() const
{
    DA_DC(d);
    return d->mPickerGroup && d->mPickerGroup->isEnabled();
}

/**
 * @brief 获取数据拾取器分组
 * @return 数据拾取器分组指针，如果未设置返回nullptr
 */
QwtPlotSeriesDataPickerGroup* DAFigureWidget::getDataPickerGroup() const
{
    return d_ptr->mPickerGroup;
}

/**
 * @brief 通过id查找plot
 * @param id
 * @param findParasite 是否查找寄生绘图
 * @return  如果找不到返回空指针
 */
QwtPlot* DAFigureWidget::findPlotById(const QString& id, bool findParasite) const
{
    const QList< QwtPlot* > axes = figure()->allAxes();
    for (QwtPlot* p : axes) {
        if (p->plotId() == id) {
            return p;
        }
        if (findParasite && p->isHostPlot()) {
            const QList< QwtPlot* > parasites = p->parasitePlots();
            for (QwtPlot* paras : parasites) {
                if (paras->plotId() == id) {
                    return paras;
                }
            }
        }
    }
    return nullptr;
}

/**
 * @brief 设置颜色主题
 *
 * 设置颜色主题后，会同步应用到所有绘图中的图形项
 * @param th 颜色主题
 * @sa getColorTheme
 */
void DAFigureWidget::setColorTheme(const DAColorTheme& th)
{
    d_ptr->mColorTheme = th;
    // 将主题颜色列表同步为 qwt 的颜色循环并设置到每个绘图上，这样后续新附加的
    // 图元会按主题自动取色（QwtPlot::nextColorForItem）。
    QList< QColor > cols = th.toColorList();
    if (cols.isEmpty()) {
        cols = DAColorTheme(DAColorTheme::Style_Cassatt1).toColorList();
    }
    const QwtColorCycle cycle(QVector< QColor >(cols.cbegin(), cols.cend()));
    // 同步应用样式：把循环设置到每个绘图，并按现有图元顺序重新着色。
    // 取色索引在每组坐标轴内连续递增，与原 ++theme 的行为保持一致。
    const QList< QwtPlot* > plots = figure()->allAxes();
    for (QwtPlot* plot : plots) {
        const QList< QwtPlot* > plotWithparasite = plot->plotList();
        int idx                                 = 0;
        for (QwtPlot* p : plotWithparasite) {
            p->setColorCycle(cycle);
            const QwtPlotItemList items = p->itemList();
            for (QwtPlotItem* item : items) {
                if (!DAChartUtil::isPlotGraphicsItem(item)) {
                    continue;
                }
                DAChartUtil::setPlotItemColor(item, cycle.color(idx++));
            }
        }
    }
    figure()->replotAll();
}

/**
 * @brief 获取颜色主题
 * @return 颜色主题
 * @sa setColorTheme
 */
DAColorTheme DAFigureWidget::getColorTheme() const
{
    return d_ptr->mColorTheme;
}

/**
 * @brief 获取颜色主题的const引用
 * @return 颜色主题的const引用
 * @sa setColorTheme
 */
const DAColorTheme& DAFigureWidget::colorTheme() const
{
    return d_ptr->mColorTheme;
}

/**
 * @brief 获取颜色主题的引用
 * @return 颜色主题的引用
 * @sa setColorTheme
 */
DAColorTheme& DAFigureWidget::colorTheme()
{
    return d_ptr->mColorTheme;
}

/**
 * @brief 将当前Figure的截图复制到剪贴板
 */
void DAFigureWidget::copyToClipboard()
{
    // 捕获当前窗口的截图
    QPixmap screenshot = figure()->grab();
    // 将截图放到剪贴板
    QApplication::clipboard()->setPixmap(screenshot);
}

/**
 * @brief 当前是否有编辑器激活
 * @return
 */
bool DAFigureWidget::isChartEditorActive() const
{
    return (d_ptr->mChartEditor != nullptr);
}

/**
 * @brief 开启图表编辑器
 * @param type 编辑器类型
 */
void DAFigureWidget::beginChartEditor(ChartEditorType type)
{
    DA_D(d);
    if (isChartEditorActive()) {
        endChartEditor();
    }
    switch (type) {
    case SubChartEditor:
        d->beginSubChartEditor();
        break;
    case RectSelectEditor:
        d->beginRectSelectEditor();
        break;
    case EllipseSelectEditor:
        d->beginEllipseSelectEditor();
        break;
    case PolygonSelectEditor:
        d->beginPolygonSelectEditor();
        break;
    case HLineMarker:
        d->beginHLineMarkerEditor();
        break;
    case VLineMarker:
        d->beginVLineMarkerEditor();
        break;
    case CrossMarker:
        d->beginCrossLineMarkerEditor();
        break;
    case ArrowMarker:
        d->beginArrowMarkerEditor();
        break;
    case VerticalDataProbe:
        d->beginVerticalProbeEditor();
        break;
    case HorizontalDataProbe:
        d->beginHorizontalProbeEditor();
        break;
    default:
        daWarning << tr("Unsupported chart editor type: %1").arg(type);  //cn:不支持的图表编辑器类型：%1
        break;
    }
}

/**
 * @brief 关闭图表编辑器
 */
void DAFigureWidget::endChartEditor()
{
    DA_D(d);
    if (d->mChartEditor) {
        d->mChartEditor->hide();
        d->mChartEditor->deleteLater();
        d->mChartEditor = nullptr;
    }
}

/**
 * @brief 获取绘图的归一化矩形区域
 * @param plot 绘图指针
 * @return 归一化的矩形区域
 */
QRectF DAFigureWidget::axesNormRect(QwtPlot* plot) const
{
    return figure()->axesNormRect(plot);
}

/**
 * @brief 获取窗口的归一化矩形区域
 * @param w 窗口指针
 * @return 归一化的矩形区域
 */
QRectF DAFigureWidget::widgetNormRect(QWidget* w) const
{
    return figure()->widgetNormRect(w);
}

/**
 * @brief 添加窗口到Figure中，指定归一化位置
 * @param widget 窗口指针
 * @param left 左边距占比
 * @param top 上边距占比
 * @param width 宽度占比
 * @param height 高度占比
 */
void DAFigureWidget::addWidget(QWidget* widget, qreal left, qreal top, qreal width, qreal height)
{
    figure()->addWidget(widget, left, top, width, height);
}

/**
 * @brief 添加窗口到Figure中，使用网格布局
 * @param widget 窗口指针
 * @param rowCnt 行数
 * @param colCnt 列数
 * @param row 起始行
 * @param col 起始列
 * @param rowSpan 行跨度
 * @param colSpan 列跨度
 * @param wspace 水平间距
 * @param hspace 垂直间距
 */
void DAFigureWidget::addWidget(QWidget* widget, int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace)
{
    figure()->addWidget(widget, rowCnt, colCnt, row, col, rowSpan, colSpan, wspace, hspace);
}

/**
 * @brief 设置窗口的归一化位置
 * @param widget 窗口指针
 * @param rect 归一化的位置和大小
 */
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

/**
 * @brief 设置Figure的表面画刷
 * @param brush 画刷
 * @sa getFaceBrush
 */
void DAFigureWidget::setFaceBrush(const QBrush& brush)
{
    QwtFigure* fig = figure();
    fig->setFaceBrush(brush);
    fig->update();
}

/**
 * @brief 获取Figure的表面画刷
 * @return 画刷
 * @sa setFaceBrush
 */
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
 * @param skipfirstRedo 跳过第一次redo操作，这种是针对当前item已经加入到plot里，只是通过此函数把它加入到redo/undo不进行item->attach操作
 */
void DAFigureWidget::addItem_(DAChartWidget* chart, QwtPlotItem* item, bool skipfirstRedo)
{
    push(new DAFigureWidgetCommandAttachItem(this, chart, item, skipfirstRedo));
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
    // 颜色由 qwt 颜色循环在 attach 时自动分配，无需显式取色
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
    // 颜色由 qwt 颜色循环在 attach 时自动分配，无需显式取色
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
    // 颜色由 qwt 颜色循环在 attach 时自动分配，无需显式取色
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
        // 颜色由 qwt 颜色循环在 attach 时自动分配，无需显式取色
        addItem_(chart, item);
        return item;
    }
    return nullptr;
}

/**
 * @brief 支持redo/undo的添加3D item
 * @param chart3d
 * @param item
 * @param skipfirstRedo 跳过第一次redo操作，针对当前item已经加入到plot的情况
 */
void DAFigureWidget::add3DItem_(DAChart3DWidget* chart3d, Qwt3DPlotItem* item, bool skipfirstRedo)
{
    push(new DAFigureWidgetCommandAttach3DItem(this, chart3d, item, skipfirstRedo));
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

/**
 * @brief 键盘按键事件处理
 * @param e 键盘事件
 */
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
 * @brief 显示事件处理
 *
 * 在窗口显示时对齐所有坐标轴
 * @param e 显示事件
 */
void DAFigureWidget::showEvent(QShowEvent* e)
{
    QScrollArea::showEvent(e);
    // 对齐坐标轴
    QTimer::singleShot(100, this, [ this ]() {
        if (QwtFigure* fig = figure()) {
            fig->applyAllAxisAlignments(true);
        }
    });
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
    if (d_ptr->mChartEditor) {
        d_ptr->mChartEditor->updateOverlay();
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
    if (DAChartWidget* c = qobject_cast< DAChartWidget* >(newActive)) {
        // 2D chart 被激活
        setCurrentChart(c);
    } else if (DAChart3DWidget* c3d = qobject_cast< DAChart3DWidget* >(newActive)) {
        // 3D chart 被激活
        setCurrent3DChart(c3d);
    }
}

/**
 * @brief 添加一个子图的槽函数
 * @param newAxes
 */
void DAFigureWidget::onAxesAdded(QwtPlot* newAxes)
{
    if (DAChartWidget* c = qobject_cast< DAChartWidget* >(newAxes)) {
        Q_EMIT chartAdded(c);
    }
}

/**
 * @brief 删除一个子图的槽函数
 * @param removedAxes
 */
void DAFigureWidget::onAxesRemoved(QwtPlot* removedAxes)
{
    if (DAChartWidget* c = qobject_cast< DAChartWidget* >(removedAxes)) {
        Q_EMIT chartRemoved(c);
    }
}

/**
 * @brief 当前的子图改变的槽函数
 * @param plot
 */
void DAFigureWidget::onCurrentAxesChanged(QwtPlot* plot)
{
    DAChartWidget* chartWidget = plot ? qobject_cast< DAChartWidget* >(plot) : nullptr;
    // 如果有子窗口编辑器，把编辑器的激活窗口改变
    if (d_ptr->mChartEditor) {
        d_ptr->mChartEditor->setActiveWidget(plot);
    }
    Q_EMIT currentChartChanged(chartWidget);
}

/**
 * @brief 图表属性变化的槽函数
 * @param chart 发生变化的图表
 * @param flag 属性变化标志
 */
void DAFigureWidget::onChartPropertyChanged(DAChartWidget* chart, DA::DAChartWidget::ChartPropertyChangeFlags flag)
{
    if (!chart) {
        return;
    }
    if (flag.testFlag(DAChartWidget::DataPickingStateChanged) && isDataPickerGroupEnabled()) {
        if (QwtPlotSeriesDataPickerGroup* group = getDataPickerGroup()) {
            if (chart->isDataPickingEnabled()) {
                QwtPlotSeriesDataPicker* picker = chart->getDataPicker();
                if (!picker) {
                    return;
                }
                group->addPicker(picker);
            }
        }
    }
}

/**
 * @brief 事件过滤器，用于检测3D chart的鼠标点击
 *
 * 当用户点击3D chart时，将其设置为current3DChart
 * @param obj
 * @param event
 * @return
 */
bool DAFigureWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (DAChart3DWidget* chart3d = qobject_cast< DAChart3DWidget* >(obj)) {
            setCurrent3DChart(chart3d);
        }
    }
    return QScrollArea::eventFilter(obj, event);
}

/**
 * @brief 图表编辑器完成的槽函数
 * @param isCancel 是否取消
 */
void DAFigureWidget::onFigureChartEditorFinished(bool isCancel)
{
    Q_UNUSED(isCancel);
    endChartEditor();
}

/**
 * @brief 序列化DAFigureWidget到数据流
 * @param out 输出数据流
 * @param p DAFigureWidget指针
 * @return 输出数据流
 */
QDataStream& operator<<(QDataStream& out, const DAFigureWidget* p)
{
    // 使用新 magic 标识支持 2D/3D 混合格式
    const quint32 magicStart = 0xDA3D0001;

    out << magicStart << p->saveGeometry();
    // 使用 const 声明避免 range-for 触发 Qt COW 深拷贝（AGENTS.md §Qt 容器范围迭代）
    const QList< DAChartWidget* > charts2d = p->getCharts();
    const QList< DAChart3DWidget* > charts3d = p->get3DCharts();
    QwtFigure* fig = p->figure();

    quint32 totalCharts = static_cast< quint32 >(charts2d.size() + charts3d.size());
    out << totalCharts;

    if (fig) {
        // 写入 2D charts
        for (DAChartWidget* chart : charts2d) {
            QRectF rect = fig->axesNormRect(chart);
            out << DA::gc_dafigure_chart2d_mark << rect;
            out << chart;  // 使用 DAChartWidget 的 operator<<
        }
        // 写入 3D charts
        // 注意：DAChart3DWidget 继承 Qwt3DPlot → QOpenGLWidget → QWidget，不继承 QwtPlot，
        // 因此不能使用 axesNormRect(QwtPlot*)，必须使用 widgetNormRect(QWidget*)
        for (DAChart3DWidget* chart3d : charts3d) {
            QRectF rect = fig->widgetNormRect(chart3d);
            out << DA::gc_dafigure_chart3d_mark << rect;
            // DAChart3DWidget 继承 Qwt3DPlot，使用 Qwt3DPlot 的 operator<<
            out << static_cast< const Qwt3DPlot* >(chart3d);
        }
    }

    return (out);
}

/**
 * @brief 从数据流反序列化DAFigureWidget
 * @param in 输入数据流
 * @param p DAFigureWidget指针
 * @return 输入数据流
 */
QDataStream& operator>>(QDataStream& in, DAFigureWidget* p)
{
    quint32 magicStart = 0;
    in >> magicStart;

    if (magicStart == 0x1314abc) {
        // 旧格式（仅 2D），调用旧的反序列化逻辑
        QByteArray geometryData;
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
        return in;
    }

    if (magicStart != 0xDA3D0001) {
        throw DABadSerializeExpection("DAFigureWidget get invalid magic start code");  // cn:DAFigureWidget的文件头异常
    }

    // 新格式（2D + 3D 混合）
    QByteArray geometryData;
    in >> geometryData;
    p->restoreGeometry(geometryData);

    quint32 chartCount = 0;
    in >> chartCount;

    try {
        for (quint32 i = 0; i < chartCount; ++i) {
            quint32 chartTypeMark = 0;
            QRectF rect;
            in >> chartTypeMark >> rect;

            if (chartTypeMark == DA::gc_dafigure_chart2d_mark) {
                // 2D chart
                auto chart = p->createChart(rect.x(), rect.y(), rect.width(), rect.height());
                std::unique_ptr< DAChartWidget > chart_guard(chart);
                in >> chart;
                chart->show();
                chart_guard.release();
            } else if (chartTypeMark == DA::gc_dafigure_chart3d_mark) {
                // 3D chart
                auto chart3d = p->create3DChart(rect.x(), rect.y(), rect.width(), rect.height());
                std::unique_ptr< DAChart3DWidget > chart3d_guard(chart3d);
                // DAChart3DWidget 继承 Qwt3DPlot，使用 Qwt3DPlot 的 operator>>
                in >> static_cast< Qwt3DPlot* >(chart3d);
                chart3d->show();
                chart3d_guard.release();
            } else {
                throw DABadSerializeExpection("Unknown chart type mark in figure data");
            }
        }
    } catch (const DABadSerializeExpection& exp) {
        throw exp;
    }
    return (in);
}

/**
 * \if ENGLISH
 * @brief Generate a unique probe name
 * @return Generated probe name (A, B, C, ..., Z, AA, AB, ...)
 * \endif
 *
 * \if CHINESE
 * @brief 生成唯一的探针名称
 * @return 生成的探针名称 (A, B, C, ..., Z, AA, AB, ...)
 * \endif
 */
QString DAFigureWidget::generateProbeName()
{
    DA_D(d);
    QString name;
    int counter = d->mProbeNameCounter;

    // 循环直到找到不存在的名称
    do {
        name = probeNameFromCounter(counter);
        ++counter;
    } while (isProbeNameExists(name) && counter < 1000);  // 上限保护

    d->mProbeNameCounter = counter;
    return name;
}

/**
 * @brief 根据计数器生成探针名称
 * @param counter 计数器值
 * @return 生成的探针名称 (A, B, C, ..., Z, AA, AB, ...)
 */
QString DAFigureWidget::probeNameFromCounter(int counter) const
{
    if (counter < 26) {
        return QChar('A' + counter);
    }

    QString name;
    counter++;
    while (counter > 0) {
        counter--;
        name.prepend(QChar('A' + (counter % 26)));
        counter /= 26;
    }

    return name;
}

/**
 * \if ENGLISH
 * @brief Check if a probe name already exists
 * @param name Probe name to check
 * @return True if name exists
 * \endif
 *
 * \if CHINESE
 * @brief 检查探针名称是否已存在
 * @param name 要检查的探针名称
 * @return 如果名称已存在返回true
 * \endif
 */
bool DAFigureWidget::isProbeNameExists(const QString& name) const
{
    const QList< DADataProbeMarker* > probes = getProbes();
    for (const DADataProbeMarker* probe : probes) {
        if (probe->probeName() == name) {
            return true;
        }
    }
    return false;
}

/**
 * \if ENGLISH
 * @brief Create a vertical data probe at specified x value
 * @param xValue The x-axis value for the probe
 * @param name Optional custom name for the probe
 * @return Pointer to the created probe
 * \endif
 *
 * \if CHINESE
 * @brief 在指定x值位置创建垂直数据探针
 * @param xValue 探针的x轴值
 * @param name 探针的自定义名称（可选）
 * @return 创建的探针指针
 * \endif
 */
DADataProbeMarker* DAFigureWidget::createVerticalProbe(double xValue, const QString& name)
{
    DAChartWidget* chart = gca();
    if (!chart) {
        return nullptr;
    }

    QString probeName        = name.isEmpty() ? generateProbeName() : name;
    DADataProbeMarker* probe = new DADataProbeMarker(DADataProbeMarker::VerticalProbe, probeName);
    probe->setXValue(xValue);
    probe->attach(chart);
    probe->captureData(true);
    chart->replot();

    return probe;
}

/**
 * \if ENGLISH
 * @brief Create a horizontal data probe at specified y value
 * @param yValue The y-axis value for the probe
 * @param name Optional custom name for the probe
 * @return Pointer to the created probe
 * \endif
 *
 * \if CHINESE
 * @brief 在指定y值位置创建水平数据探针
 * @param yValue 探针的y轴值
 * @param name 探针的自定义名称（可选）
 * @return 创建的探针指针
 * \endif
 */
DADataProbeMarker* DAFigureWidget::createHorizontalProbe(double yValue, const QString& name)
{
    DAChartWidget* chart = gca();
    if (!chart) {
        return nullptr;
    }

    QString probeName        = name.isEmpty() ? generateProbeName() : name;
    DADataProbeMarker* probe = new DADataProbeMarker(DADataProbeMarker::HorizontalProbe, probeName);
    probe->setYValue(yValue);
    probe->attach(chart);
    probe->captureData(true);
    chart->replot();

    return probe;
}

/**
 * \if ENGLISH
 * @brief Remove all data probes from the current chart
 * \endif
 *
 * \if CHINESE
 * @brief 删除当前图表中的所有数据探针
 * \endif
 */
void DAFigureWidget::removeAllProbes()
{
    const QList< DAChartWidget* > charts = getCharts();
    for (DAChartWidget* chart : charts) {
        const QwtPlotItemList& items = chart->itemList();
        QList< QwtPlotItem* > probesToRemove;
        for (QwtPlotItem* item : items) {
            if (item->rtti() == DADataProbeMarker::Rtti_DataProbeMarker) {
                probesToRemove.append(item);
            }
        }
        for (QwtPlotItem* item : probesToRemove) {
            item->detach();
            delete item;
        }
        if (!probesToRemove.isEmpty()) {
            chart->replot();
        }
    }
    d_ptr->mProbeNameCounter = 0;
}

/**
 * \if ENGLISH
 * @brief Get all data probes
 * @return List of all data probes
 * \endif
 *
 * \if CHINESE
 * @brief 获取所有数据探针
 * @return 所有数据探针列表
 * \endif
 */
QList< DADataProbeMarker* > DAFigureWidget::getProbes() const
{
    QList< DADataProbeMarker* > probes;
    const QList< DAChartWidget* > charts = getCharts();
    for (DAChartWidget* chart : charts) {
        const QwtPlotItemList& items = chart->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() == DADataProbeMarker::Rtti_DataProbeMarker) {
                probes.append(static_cast< DADataProbeMarker* >(item));
            }
        }
    }
    return probes;
}

/**
 * \if ENGLISH
 * @brief Get data probe by name
 * @param name The probe name to search for
 * @return Pointer to the probe, or nullptr if not found
 * \endif
 *
 * \if CHINESE
 * @brief 通过名称获取数据探针
 * @param name 要搜索的探针名称
 * @return 探针指针，如果未找到则返回nullptr
 * \endif
 */
DADataProbeMarker* DAFigureWidget::getProbeByName(const QString& name) const
{
    QList< DADataProbeMarker* > probes = getProbes();
    for (DADataProbeMarker* probe : probes) {
        if (probe->probeName() == name) {
            return probe;
        }
    }
    return nullptr;
}

/**
 * \if ENGLISH
 * @brief Rename a data probe
 * @param probe The probe to rename
 * @param newName The new name
 * @return true if successful, false if name already exists
 * \endif
 *
 * \if CHINESE
 * @brief 重命名数据探针
 * @param probe 要重命名的探针
 * @param newName 新名称
 * @return 成功返回true，名称已存在返回false
 * \endif
 */
bool DAFigureWidget::renameProbe(DADataProbeMarker* probe, const QString& newName)
{
    if (!probe) {
        return false;
    }
    if (probe->probeName() == newName) {
        return true;
    }
    if (isProbeNameExists(newName)) {
        return false;
    }
    probe->setProbeName(newName);
    if (QwtPlot* p = probe->plot()) {
        p->replot();
    }
    return true;
}

/**
 * \if ENGLISH
 * @brief Start vertical probe creation interaction mode
 * \endif
 *
 * \if CHINESE
 * @brief 开始垂直探针创建交互模式
 * \endif
 */
void DAFigureWidget::beginVerticalProbeEditor()
{
    beginChartEditor(VerticalDataProbe);
}

/**
 * \if ENGLISH
 * @brief Start horizontal probe creation interaction mode
 * \endif
 *
 * \if CHINESE
 * @brief 开始水平探针创建交互模式
 * \endif
 */
void DAFigureWidget::beginHorizontalProbeEditor()
{
    beginChartEditor(HorizontalDataProbe);
}

}
