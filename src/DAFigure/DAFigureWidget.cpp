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
#include <QShortcut>
#include <QFileInfo>
#include <QSaveFile>
#include <QImage>
#include <QPixmap>
#include <QPdfWriter>
#include <QPageSize>
#include <QSvgGenerator>
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
#include "DAChartTextMarkerEditor.h"
#include "DADataProbeMarker.h"
#include "DAFigurePointerSelectorOverlay.h"
#include "DALogCategory.h"
#include "DASlowLog.h"
// qwt
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
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
#if defined(DA_ENABLE_SLOW_LOG)
/// 单调时钟纳秒时间戳：用于近似测量拖动中每个 move 触发的整图布局耗时
qint64 gridSlowLogStampNs()
{
    return std::chrono::duration_cast< std::chrono::nanoseconds >(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
#endif
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
    bool mDataProbeModeActive { false };              ///< 数据探针会话激活标志（持久会话，非overlay）
    bool mSuppressInteractionNotify { false };        ///< 探针模式主动关闭子图交互时抑制onChartPropertyChanged的互斥判定
    QShortcut* mDataProbeEscapeShortcut { nullptr };  ///< 探针模式的 Esc 退出快捷键（会话期间存在）
    QList< DAChartWidget* > m_2dCharts;                ///< 2D chart 列表（由onAxesAdded/onAxesRemoved维护）
    QList< DAChart3DWidget* > m_3dCharts;            ///< 3D chart 列表
    QPointer< DAChart3DWidget > mCurrent3DChart;    ///< 当前选中的 3D chart
    DAFigureCurrentSubChart mCurrentSubChartKind { DAFigureCurrentSubChart::None };  ///< 最近激活的子图类型
    // ===== 保留式网格布局状态 =====
    bool mGridLayoutEnabled { false };        ///< 是否已启用网格布局
    bool mSuppressGridUndo { false };         ///< 程序化设置比例期间抑制撤销记录
    bool mGridChangeActive { false };         ///< 是否处于一次交互式比例调整会话中
    bool mGridEditorBlockedWarned { false };  ///< 网格模式禁用子图编辑器的告警是否已发过
    QVector< qreal > mGridCommittedRows;      ///< 最近一次提交的行比例（拖动基线）
    QVector< qreal > mGridCommittedCols;      ///< 最近一次提交的列比例（拖动基线）
    QVector< qreal > mGridOldRows;            ///< 一次比例调整开始前的行比例
    QVector< qreal > mGridOldCols;            ///< 一次比例调整开始前的列比例
#if defined(DA_ENABLE_SLOW_LOG)
    // 拖动期整图布局采样：见 onGridSizesChanged 的说明
    int mGridDragLayoutPasses { 0 };       ///< 本次拖动中触发的整图布局次数（每次 sizesChanged 一次）
    qint64 mGridDragLayoutNs { 0 };        ///< 本次拖动累计布局耗时（近似，纳秒）
    qint64 mGridDragMaxLayoutNs { 0 };     ///< 本次拖动单次布局最大耗时（近似，纳秒）
    qint64 mGridDragLastStampNs { 0 };     ///< 上一次回调返回时的时间戳，0 表示无采样
#endif
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

#if defined(DA_ENABLE_SLOW_LOG)
    // 清空拖动期布局采样（新会话开始，或收尾报告后）
    void resetGridDragSlowLogSample()
    {
        mGridDragLayoutPasses = 0;
        mGridDragLayoutNs     = 0;
        mGridDragMaxLayoutNs  = 0;
        mGridDragLastStampNs  = 0;
    }

    /// 汇总一次拖动中被事件驱动的整图布局耗时；超过标签阈值才输出一条慢日志
    void reportGridDragSlowLog()
    {
        if (mGridDragLayoutPasses <= 0) {
            resetGridDragSlowLogSample();
            return;
        }
        // 末次布局发生在最后一次 move 之后，用「末帧时间戳→现在」补上
        if (mGridDragLastStampNs > 0) {
            const qint64 layoutNs = gridSlowLogStampNs() - mGridDragLastStampNs;
            if (layoutNs > 0) {
                mGridDragLayoutNs += layoutNs;
                mGridDragMaxLayoutNs = qMax(mGridDragMaxLayoutNs, layoutNs);
            }
        }
        const int passes      = mGridDragLayoutPasses;
        const qint64 totalNs  = mGridDragLayoutNs;
        const qint64 maxNs    = mGridDragMaxLayoutNs;
        resetGridDragSlowLogSample();

        if (totalNs < DASlowLog::thresholdMs("dafigure.grid.drag") * 1000000LL) {
            return;
        }
        const QString detail = QStringLiteral("passes=%1 total_ms=%2 max_ms=%3")
                                   .arg(passes)
                                   .arg(totalNs / 1000000.0, 0, 'f', 2)
                                   .arg(maxNs / 1000000.0, 0, 'f', 2);
        DASlowLog::emitScopeRecord("dafigure.grid.drag",
                                   totalNs,
                                   totalNs,
                                   0,
                                   __FILE__,
                                   __LINE__,
                                   "onGridSizesChangeFinished",
                                   detail,
                                   "main");
    }
#endif

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
                if (!isCancel) {
                    QwtPlotItem* item = editor->takeItem();
                    if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(plot)) {
                        fig->addItem_(chart, item, true);
                    } else {
                        daCritical << tr("Unexpected plotting operation: a chart that does not belong to the DAChartWidget "
                                      "type was added to the figure");  //cn:意外的绘图操作：不属于 DAChartWidget 类型的图表被添加到了 figure 中
                        item->detach();
                        delete item;
                    }
                }
                // EndEdit状态统一在onFigureChartEditorFinished中发射，保证所有编辑器类型（含指针选择器）ribbon按钮复位
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
    void beginTextMarkerEditor();
    void beginPointerSelectorEditor();
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
 * @brief 开始文本标注编辑器
 *
 * 文本添加模式在覆盖层上显示文本编辑光标（覆盖层是编辑期间的鼠标事件接收者），
 * 编辑结束覆盖层销毁，光标随之恢复
 */
void DAFigureWidget::PrivateData::beginTextMarkerEditor()
{
    mChartEditor = beginSelectEditor< DAChartTextMarkerEditor >();
    if (mChartEditor) {
        mChartEditor->setCursor(Qt::IBeamCursor);
    }
}

/**
 * @brief 开始指针选择器
 *
 * 指针工具是持久会话（不因单次交互结束），选择/拖动/删除绘图元素。
 * 选中结果经 figureElementClicked 信号联动属性面板；
 * 删除与移动经信号回到 DAFigureWidget 走 undo 栈
 */
void DAFigureWidget::PrivateData::beginPointerSelectorEditor()
{
    DAFigureWidget* fig = q_ptr;
    DAFigurePointerSelectorOverlay* overlay = new DAFigurePointerSelectorOverlay(fig->figure(), fig);
    mChartEditor                            = overlay;
    overlay->setEnabled(true);
    overlay->show();
    overlay->raise();
    fig->emitChartEditorBeginEdit();
    // 元素选中 → 联动属性面板
    DAFigureWidget::connect(
        overlay, &DAFigurePointerSelectorOverlay::elementSelected, fig, &DAFigureWidget::figureElementClicked);
    // 删除请求 → 走undo栈
    DAFigureWidget::connect(overlay, &DAFigurePointerSelectorOverlay::requestRemoveItem, fig, [ fig ](QwtPlotItem* item) {
        if (DAChartWidget* chart = fig->findChartFromItem(item)) {
            fig->removeItem_(chart, item);
        }
    });
    // 位置移动请求 → 走undo栈
    DAFigureWidget::connect(overlay,
                            &DAFigurePointerSelectorOverlay::requestMoveItemPosition,
                            fig,
                            [ fig ](QwtPlotItem* item,
                                    const DAChartElementHitTester::ItemGeometry& oldGeo,
                                    const DAChartElementHitTester::ItemGeometry& newGeo) {
                                if (DAChartWidget* chart = fig->findChartFromItem(item)) {
                                    fig->moveItemPosition_(chart, item, oldGeo, newGeo);
                                }
                            });
    // Esc退出 → 结束编辑会话（走现有endChartEditor销毁流程）
    DAFigureWidget::connect(
        overlay, &DAFigurePointerSelectorOverlay::finished, fig, &DAFigureWidget::onFigureChartEditorFinished);
    DAFigureWidget::connect(
        overlay, &DAFigureWidgetOverlay::activeWidgetChanged, fig, &DAFigureWidget::onOverlayActiveWidgetChanged);
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
    // 析构期间子控件由 ~QWidget 的 deleteChildren 释放，3D 子图的 destroyed 信号仍会触发
    // add3DChart 注册的清理 lambda——彼时 d_ptr（PIMPL 成员）已释放、内层 QwtFigure 也可能
    // 已删除，lambda 再访问两者就是 UAF（析构期崩溃）。在 PIMPL 仍存活时显式断开。
    const QList< DAChart3DWidget* > charts3d = d_ptr->m_3dCharts;
    for (DAChart3DWidget* c : charts3d) {
        if (c) {
            disconnect(c, &QObject::destroyed, this, nullptr);
        }
    }
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
    // 网格拖动：sizesChanged 在拖动中反复触发（只记录基线），松手时一次性入撤销栈
    connect(figure, &QwtFigure::gridSizesChanged, this, &DAFigureWidget::onGridSizesChanged);
    connect(figure, &QwtFigure::gridSizesChangeFinished, this, &DAFigureWidget::onGridSizesChangeFinished);
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
    push(new DAFigureWidgetCommandRemoveChart(this, chart));
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
    push(cmd);
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
    // 3D 子图的持久 id：加入 figure 时按 c3d-<n> 生成（存档区分多个 3D 子图、供 3D 图元数据引用）。
    // 与 QwtPlot::plotId 无关；undo/redo 重新加入时 id 已存在，保持不变。
    if (chart3d && chart3d->getChart3DId().isEmpty()) {
        int seq = 1;
        QString candidate;
        bool duplicated = true;
        while (duplicated) {
            candidate  = QStringLiteral("c3d-%1").arg(seq++);
            duplicated = false;
            const QList< DAChart3DWidget* >& existing = d_ptr->m_3dCharts;
            for (DAChart3DWidget* other : existing) {
                if (other && other != chart3d && other->getChart3DId() == candidate) {
                    duplicated = true;
                    break;
                }
            }
        }
        chart3d->setChart3DId(candidate);
    }
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
            // 当前 3D 子图销毁：最近激活类型回退到仍存活的 2D 当前子图
            d_ptr->mCurrentSubChartKind =
                getCurrentChart() ? DAFigureCurrentSubChart::Chart2D : DAFigureCurrentSubChart::None;
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
    // 从figure中移除：qwt负责注销网格登记、移除layout item、隐藏并从父窗口分离
    if (QwtFigure* fig = figure()) {
        fig->removeWidget(chart3d);
    }
    // 从跟踪列表中移除
    d_ptr->m_3dCharts.removeAll(chart3d);
    // 清理 current 指针
    if (d_ptr->mCurrent3DChart == chart3d) {
        d_ptr->mCurrent3DChart = nullptr;
        // 当前 3D 子图移除：最近激活类型回退到仍存活的 2D 当前子图
        d_ptr->mCurrentSubChartKind =
            getCurrentChart() ? DAFigureCurrentSubChart::Chart2D : DAFigureCurrentSubChart::None;
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
    push(new DAFigureWidgetCommandRemove3DChart(this, chart3d));
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
    push(cmd);
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
    // 最近激活子图类型跟踪：3D 激活；显式清空时回退到仍存活的 2D 当前子图
    if (chart3d) {
        d->mCurrentSubChartKind = DAFigureCurrentSubChart::Chart3D;
    } else {
        d->mCurrentSubChartKind = getCurrentChart() ? DAFigureCurrentSubChart::Chart2D : DAFigureCurrentSubChart::None;
    }
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
 * @brief 最近激活的子图类型
 *
 * 跟踪口径：2D 子图激活（setCurrentChart / qwt currentAxesChanged）记 Chart2D；
 * 3D 子图激活（setCurrent3DChart，含 3D 控件点击的事件过滤器）记 Chart3D；
 * 任一侧当前指针被清空时回退到另一侧仍存活的当前子图，两者皆无则为 None。
 * 读取时按当前指针存活性校验，保证返回的类型一定有可操作的子图。
 * @return None/Chart2D/Chart3D
 */
DAFigureCurrentSubChart DAFigureWidget::currentSubChartKind() const
{
    DA_DC(d);
    switch (d->mCurrentSubChartKind) {
    case DAFigureCurrentSubChart::Chart2D:
        return getCurrentChart() ? DAFigureCurrentSubChart::Chart2D : DAFigureCurrentSubChart::None;
    case DAFigureCurrentSubChart::Chart3D:
        return getCurrent3DChart() ? DAFigureCurrentSubChart::Chart3D : DAFigureCurrentSubChart::None;
    default:
        return DAFigureCurrentSubChart::None;
    }
}

//===================================================
// 保留式网格布局
//===================================================

/**
 * @brief 尺寸约束是否塌陷
 *
 * maximum<=0 会让子图宽度/高度被压成 0；minimum>maximum 是互相矛盾的约束（Qt 取 min 优先），
 * 两种情况网格子图都无法铺满，需要在启用网格前复位。
 * @param minimum 最小尺寸
 * @param maximum 最大尺寸
 * @return 塌陷返回true
 */
static bool isSizeConstraintCollapsed(int minimum, int maximum)
{
    return maximum <= 0 || minimum > maximum;
}

/**
 * @brief 启用保留式网格布局
 *
 * 网格启用后子图的位置与大小由行列轨道比例决定，可拖动分隔条调整比例；
 * 网格模式下子图编辑器（自由缩放）被禁用。
 * @param rows 行数，必须大于0
 * @param cols 列数，必须大于0
 * @return 行列数非法或figure不存在时返回false
 * @note 只在 figure 尺寸约束确实塌陷（maximum<=0 或 minimum>maximum）时复位该轴的 min/max，
 *       用户显式设置的有效约束保持不变；复位会发射 @ref figureSizeConstraintsChanged 让设置面板刷新。
 * @note 行列数不变时幂等（保留当前轨道比例与成员登记）；改变行列数会经qwt重建
 *       等分轨道并清空已有网格成员登记，旧成员退回自由布局，且此前的比例撤销命令
 *       因行列数不匹配而变成空操作
 */
bool DAFigureWidget::enableGridLayout(int rows, int cols)
{
    DA_D(d);
    QwtFigure* fig = figure();
    if (!fig) {
        return false;
    }
    // 先校验参数，避免非法调用改坏已启用的状态
    if (rows <= 0 || cols <= 0) {
        qWarning() << "DAFigureWidget::enableGridLayout: row and column count must be positive, got" << rows << cols;
        return false;
    }
    // 幂等：行列数不变时保留当前轨道比例与成员登记
    if (d->mGridLayoutEnabled && fig->grid() && fig->grid()->rowCount() == rows
        && fig->grid()->columnCount() == cols) {
        return true;
    }
    // 网格启用会activate布局：若保留DAFigureWidgetSettingPanel写入的塌陷值(max=0 / min>max)，
    // 网格子图无法铺满。只在塌陷时复位，用户显式设置的有效约束不能被覆盖。
    bool constraintsReset = false;
    if (isSizeConstraintCollapsed(fig->minimumWidth(), fig->maximumWidth())) {
        fig->setMinimumWidth(0);
        fig->setMaximumWidth(QWIDGETSIZE_MAX);
        constraintsReset = true;
    }
    if (isSizeConstraintCollapsed(fig->minimumHeight(), fig->maximumHeight())) {
        fig->setMinimumHeight(0);
        fig->setMaximumHeight(QWIDGETSIZE_MAX);
        constraintsReset = true;
    }
    const bool wasGridLayoutEnabled = d->mGridLayoutEnabled;
    d->mGridLayoutEnabled = false;
    d->mGridChangeActive = false;
    if (constraintsReset) {
        // 面板可能正显示塌陷值，复位后必须让它重新读取
        Q_EMIT figureSizeConstraintsChanged();
    }
    if (!fig->enableGrid(rows, cols)) {
        if (wasGridLayoutEnabled != d->mGridLayoutEnabled) {
            Q_EMIT gridLayoutEnabledChanged(d->mGridLayoutEnabled);
        }
        return false;
    }
    // 网格接管分隔带拖拽
    fig->setGridEditable(true);
    // 网格模式由轨道决定几何，退出自由缩放编辑器
    if (d->mChartEditor) {
        endChartEditor();
    }
    d->mGridLayoutEnabled = true;
    if (QwtFigureGrid* g = fig->grid()) {
        d->mGridCommittedRows = g->rowSizes();
        d->mGridCommittedCols = g->columnSizes();
    }
    if (!wasGridLayoutEnabled) {
        Q_EMIT gridLayoutEnabledChanged(true);
    }
    return true;
}

/**
 * @brief 是否已启用网格布局
 * @return 网格已启用返回true
 */
bool DAFigureWidget::isGridLayoutEnabled() const
{
    DA_DC(d);
    const QwtFigure* fig = figure();
    return d->mGridLayoutEnabled && fig && fig->isGridEnabled();
}

/**
 * @brief 获取底层保留式网格模型
 * @return 未启用网格时返回nullptr
 */
QwtFigureGrid* DAFigureWidget::grid() const
{
    QwtFigure* fig = figure();
    return fig ? fig->grid() : nullptr;
}

/**
 * @brief 把网格成员按当前轨道比例重新排版
 *
 * 仅改变网格登记（例如移动单元格）时layout不会自动重排，需要显式重算几何。
 * 不调用QLayout::activate()：未显示的figure会按最小尺寸被改尺寸。
 */
void DAFigureWidget::updateGridGeometry()
{
    QwtFigure* fig = figure();
    if (!fig) {
        return;
    }
    if (QLayout* lay = fig->layout()) {
        lay->invalidate();
        lay->setGeometry(fig->rect());
    }
}

/**
 * @brief 把子图/子控件登记到网格单元格
 *
 * 支持的成员：
 * - 已在figure中的2D/3D子图或普通控件：只更新网格登记并重新排版
 * - 尚未加入figure的QwtPlot：按网格坐标直接加入（发射axesAdded/currentAxesChanged）
 * - 尚未加入figure的DAChart3DWidget：复用add3DChart完成3D登记，再改为网格成员
 * - 其它控件：按网格坐标加入figure
 * @param chartOrWidget 子图或子控件指针
 * @param row 起始行（0基）
 * @param col 起始列（0基）
 * @param rowSpan 行跨度
 * @param colSpan 列跨度
 * @return 登记成功返回true；网格未启用、控件为空或位置越界返回false（失败时不改变已有登记）
 */
bool DAFigureWidget::setGridCell(QWidget* chartOrWidget, int row, int col, int rowSpan, int colSpan)
{
    DA_D(d);
    if (!chartOrWidget) {
        qWarning() << "DAFigureWidget::setGridCell: null widget";
        return false;
    }
    QwtFigure* fig = figure();
    QwtFigureGrid* g = fig ? fig->grid() : nullptr;
    if (!g || !g->isEnabled()) {
        qWarning() << "DAFigureWidget::setGridCell: the grid layout is not enabled";
        return false;
    }
    // 先校验目标单元格：非法时保持原有网格登记不变（否则先注销再失败会丢掉原登记）
    if (row < 0 || col < 0 || rowSpan <= 0 || colSpan <= 0 || row + rowSpan > g->rowCount()
        || col + colSpan > g->columnCount()) {
        qWarning() << "DAFigureWidget::setGridCell: cell" << row << col << rowSpan << colSpan << "does not fit the grid"
                   << g->rowCount() << "x" << g->columnCount();
        return false;
    }
    const bool inFigure = (chartOrWidget->parentWidget() == fig);
    // 已在网格中则先注销，允许移动单元格
    g->removeItem(chartOrWidget);
    if (inFigure) {
        // 已在figure中：只更新网格登记，不重建layout item
        const bool ok = g->addItem(chartOrWidget, row, col, rowSpan, colSpan);
        if (ok) {
            // qwt只在addWidget/removeWidget时重扫分隔条监视的成员控件，移动单元格后需强制刷新
            if (fig->isGridEditable()) {
                fig->setGridEditable(true);
            }
            updateGridGeometry();
        }
        return ok;
    }
    if (QwtPlot* plot = qobject_cast< QwtPlot* >(chartOrWidget)) {
        fig->addAxes(plot, row, col, rowSpan, colSpan);
        const bool ok = g->gridPosition(plot, nullptr, nullptr, nullptr, nullptr);
        if (ok) {
            if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(plot)) {
                d->syncColorCycleToChart(chart);
                // 与addChart保持一致：属性变化联动（先断开避免重复连接）
                disconnect(chart, &DAChartWidget::chartPropertiesChanged, this, nullptr);
                connect(chart, &DAChartWidget::chartPropertiesChanged, this, &DAFigureWidget::onChartPropertyChanged);
            }
            // QwtFigureLayout会跳过不可见成员，必须与addChart一致显式显示
            plot->show();
        }
        return ok;
    }
    if (DAChart3DWidget* chart3d = qobject_cast< DAChart3DWidget* >(chartOrWidget)) {
        // 复用add3DChart完成3D激活/事件过滤/信号等登记，再改为网格成员；
        // 单元格已在上方校验过，add3DChart之后不会出现网格登记失败
        add3DChart(chart3d, g->normRectFor(row, col, rowSpan, colSpan));
        const bool ok = g->addItem(chart3d, row, col, rowSpan, colSpan);
        if (ok) {
            updateGridGeometry();
        }
        return ok;
    }
    // 普通控件（非QwtPlot/非3D图）：按网格坐标加入figure
    // addGridItem只会往layout追加item，不会触发布局，且QwtFigureLayout会跳过不可见成员，
    // 因此需要与addChart/add3DChart一致显式显示后再重排
    const bool ok = fig->addGridItem(chartOrWidget, row, col, rowSpan, colSpan);
    if (ok) {
        chartOrWidget->show();
        updateGridGeometry();
    }
    return ok;
}

/**
 * @brief 当前网格行比例
 * @return 归一化行比例；未启用网格返回空列表
 */
QList< qreal > DAFigureWidget::gridRowSizes() const
{
    QwtFigureGrid* g = grid();
    if (!g) {
        return QList< qreal >();
    }
    const QVector< qreal > sizes = g->rowSizes();
    return QList< qreal >(sizes.cbegin(), sizes.cend());
}

/**
 * @brief 当前网格列比例
 * @return 归一化列比例；未启用网格返回空列表
 */
QList< qreal > DAFigureWidget::gridColumnSizes() const
{
    QwtFigureGrid* g = grid();
    if (!g) {
        return QList< qreal >();
    }
    const QVector< qreal > sizes = g->columnSizes();
    return QList< qreal >(sizes.cbegin(), sizes.cend());
}

/**
 * @brief 设置网格行/列比例
 *
 * 供XML恢复等程序化场景使用：内部重算几何，但不压入撤销栈
 * （交互式拖动由onGridSizesChangeFinished统一入栈一次）。
 * @param rows 行比例，个数必须等于网格行数，且全为有限正数
 * @param cols 列比例，个数必须等于网格列数，且全为有限正数
 * @return 网格未启用、个数不符或数值非法返回false
 */
bool DAFigureWidget::setGridSizes(const QList< qreal >& rows, const QList< qreal >& cols)
{
    DA_D(d);
    QwtFigureGrid* g = grid();
    if (!g || !g->isEnabled()) {
        qWarning() << "DAFigureWidget::setGridSizes: the grid layout is not enabled";
        return false;
    }
    if (rows.size() != g->rowCount() || cols.size() != g->columnCount()) {
        qWarning() << "DAFigureWidget::setGridSizes: expected" << g->rowCount() << "rows and" << g->columnCount()
                   << "columns, got" << rows.size() << cols.size();
        return false;
    }
    const QVector< qreal > rowSizes(rows.cbegin(), rows.cend());
    const QVector< qreal > colSizes(cols.cbegin(), cols.cend());
    for (qreal v : rowSizes) {
        if (!qIsFinite(v) || v <= 0.0) {
            qWarning() << "DAFigureWidget::setGridSizes: row ratios must be finite and positive";
            return false;
        }
    }
    for (qreal v : colSizes) {
        if (!qIsFinite(v) || v <= 0.0) {
            qWarning() << "DAFigureWidget::setGridSizes: column ratios must be finite and positive";
            return false;
        }
    }
    d->mSuppressGridUndo = true;
    g->setRowSizes(rowSizes);
    g->setColumnSizes(colSizes);
    d->mSuppressGridUndo = false;
    // 程序化设置后同步撤销基线，避免下一次交互拖动把旧比例当成基线
    d->mGridCommittedRows = g->rowSizes();
    d->mGridCommittedCols = g->columnSizes();
    d->mGridChangeActive = false;
    return true;
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

namespace
{
/**
 * @brief 通过 QSaveFile 写文件：先写同目录临时文件，commit 时替换目标
 *
 * 直接写目标文件有两个问题：写入失败会留下半成品；只判断 QFileInfo::exists() 又会在
 * 「覆盖保存但本次写入失败」时误报成功（旧文件仍然存在）。QSaveFile::commit() 只在
 * 本次写入成功时才替换目标文件，失败或取消时保留原有同名文件（C-13）。
 * @param filePath 目标文件路径
 * @param writeTo 写入回调（参数是已打开的写入设备，返回是否写入成功）
 * @param context 诊断用的调用位置
 * @return 目标文件被成功写入返回 true
 */
bool exportViaSaveFile(const QString& filePath, const std::function< bool(QIODevice&) >& writeTo, const char* context)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << context << "cannot open the target file for writing:" << filePath << file.errorString();
        return false;
    }
    if (!writeTo(file)) {
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        qWarning() << context << "cannot commit the exported file:" << filePath << file.errorString();
        return false;
    }
    return true;
}
}  // namespace

/**
 * @brief 把指定2D子图导出到文件
 *
 * - Png：用QWidget::grab取得子图位图后保存
 * - Svg：用QSvgGenerator把子图渲染为矢量图
 * - Pdf：用QPdfWriter把子图渲染为PDF（页面尺寸按96dpi换算成点，避免裁切）
 * 三种格式都经 QSaveFile 写入：失败不破坏已存在的同名文件，成功判定基于本次写入结果
 * 而不是「目标文件是否存在」（C-13）。
 * @param chart 要导出的2D子图
 * @param filePath 目标文件路径
 * @param format 导出格式
 * @return 成功返回true；chart为空、路径为空、尺寸为0或保存失败返回false
 */
bool DAFigureWidget::exportChartToFile(DAChartWidget* chart, const QString& filePath, DAFigureExportFormat format)
{
    if (nullptr == chart || filePath.isEmpty()) {
        return false;
    }
    const QSize chartSize = chart->size();
    if (chartSize.isEmpty()) {
        qWarning() << "DAFigureWidget::exportChartToFile: the chart size is empty, file path is" << filePath;
        return false;
    }
    switch (format) {
    case DAFigureExportFormat::Png: {
        // 说明：HiDPI 屏幕上 grab() 的像素尺寸 = 逻辑尺寸 × devicePixelRatio，与 SVG/PDF 的
        // 逻辑尺寸不一致；量化导出按设备像素保留清晰度（需要固定尺寸时可在导出前调整子图尺寸）
        const QPixmap pixmap = chart->grab();
        if (pixmap.isNull()) {
            return false;
        }
        return exportViaSaveFile(
            filePath,
            [ &pixmap ](QIODevice& device) { return pixmap.save(&device, "PNG"); },
            "DAFigureWidget::exportChartToFile");
    }
    case DAFigureExportFormat::Svg: {
        return exportViaSaveFile(
            filePath,
            [ chart, chartSize ](QIODevice& device) {
                QSvgGenerator generator;
                generator.setOutputDevice(&device);
                generator.setSize(chartSize);
                generator.setViewBox(QRect(QPoint(0, 0), chartSize));
                generator.setResolution(96);
                QPainter painter;
                if (!painter.begin(&generator)) {
                    return false;
                }
                chart->render(&painter);
                // QSvgGenerator 的内容在 painter.end() 时才写完
                return painter.end();
            },
            "DAFigureWidget::exportChartToFile");
    }
    case DAFigureExportFormat::Pdf: {
        return exportViaSaveFile(
            filePath,
            [ chart, chartSize ](QIODevice& device) {
                QPdfWriter writer(&device);
                writer.setResolution(96);
                // 96dpi的像素尺寸换算为点（1pt = 1/72 inch），页面与外框一致，避免渲染被裁切
                const QSizeF pageSizeInPoint(chartSize.width() * 72.0 / 96.0, chartSize.height() * 72.0 / 96.0);
                writer.setPageSize(QPageSize(pageSizeInPoint, QPageSize::Point));
                writer.setPageMargins(QMarginsF(0, 0, 0, 0));
                QPainter painter;
                if (!painter.begin(&writer)) {
                    return false;
                }
                chart->render(&painter);
                return painter.end();
            },
            "DAFigureWidget::exportChartToFile");
    }
    }
    return false;
}

/**
 * @brief 把指定3D子图导出到PNG文件
 *
 * 3D 子图固定走位图：qwt 虽然有 `Qwt3DPlot::saveVector`/`save`（gl2ps），但在当前 3D 渲染器
 * （VBO+GLSL、CPU 矩阵、无固定管线矩阵栈）下产物不可用——实测 SVG 页面盒错位裁掉右侧、文本/图例
 * 全丢，PDF 基本空白（详见 tmp/signal-analysis-chart-ops/03-3d-items-and-vector-export.md 附录）。
 * 因此这里 grabFramebuffer 取帧缓冲后按 PNG 保存，内容显式指定格式、不依赖文件后缀
 * （调用方负责让路径以 .png 结尾，见 @ref applyExportFilterSuffix 与导出动作的过滤器收窄）。
 * @param plot3d 要导出的3D子图
 * @param filePath 目标文件路径
 * @return 成功返回true；plot3d 为空、路径为空、控件尺寸为0、帧缓冲为空或保存失败返回false
 */
bool DAFigureWidget::export3DChartToFile(Qwt3DPlot* plot3d, const QString& filePath)
{
    if (nullptr == plot3d || filePath.isEmpty()) {
        return false;
    }
    const QSize size = plot3d->size();
    if (size.isEmpty()) {
        qWarning() << "DAFigureWidget::export3DChartToFile: the 3D sub-chart size is empty, file path is" << filePath;
        return false;
    }
    // 未初始化 GL（控件从未显示）时 grabFramebuffer 返回空图，按失败处理
    const QImage frame = plot3d->grabFramebuffer();
    if (frame.isNull()) {
        qWarning() << "DAFigureWidget::export3DChartToFile: the grabbed frame buffer is null, file path is" << filePath;
        return false;
    }
    const QPixmap pixmap = QPixmap::fromImage(frame);
    if (pixmap.isNull()) {
        return false;
    }
    return pixmap.save(filePath, "PNG");
}

/**
 * @brief 把指定2D子图复制到剪贴板
 * @param chart 要复制的2D子图，为空时使用当前子图
 * @return 成功返回true；没有可用子图或抓取失败返回false
 */
bool DAFigureWidget::copyChartToClipboard(DAChartWidget* chart)
{
    if (nullptr == chart) {
        chart = getCurrentChart();
    }
    if (nullptr == chart) {
        return false;
    }
    const QPixmap pixmap = chart->grab();
    if (pixmap.isNull()) {
        return false;
    }
    QApplication::clipboard()->setPixmap(pixmap);
    return true;
}

/**
 * @brief 根据文件后缀推断导出格式
 * @param filePath 文件路径
 * @return .svg返回Svg，.pdf返回Pdf，其它（含无后缀）按Png处理
 */
DAFigureExportFormat DAFigureWidget::exportFormatFromFilePath(const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QLatin1String("svg")) {
        return DAFigureExportFormat::Svg;
    }
    if (suffix == QLatin1String("pdf")) {
        return DAFigureExportFormat::Pdf;
    }
    return DAFigureExportFormat::Png;
}

/**
 * @brief 按文件对话框所选过滤器修正导出路径后缀
 *
 * 行为定义：
 * 1. 从 selectedFilter 里解析已知扩展名（png/svg/pdf，大小写不敏感，形如 @c *.svg）；
 * 2. 恰好解析出一个已知扩展名时以它为目标：路径当前后缀（小写）与目标一致则原样返回；
 *    当前后缀是已知图像后缀（png/svg/pdf 之一）则替换之；否则直接追加目标后缀；
 * 3. 过滤器解析不出唯一已知扩展名（无或多个）时：路径已有已知后缀则原样返回，
 *    否则追加 @c .png（保留旧默认行为）。
 *
 * 背景：默认文件名 chart.png 时，Windows 原生保存对话框在用户切换到 SVG/PDF 过滤器后
 * 仍保留 .png 后缀，若只按后缀推断格式会「选 SVG 却存成 PNG」；本助手以所选过滤器为权威。
 * @param filePath 用户在对话框里输入的路径
 * @param selectedFilter 对话框返回的所选过滤器
 * @return 修正后的路径
 */
QString DAFigureWidget::applyExportFilterSuffix(const QString& filePath, const QString& selectedFilter)
{
    if (filePath.isEmpty()) {
        return filePath;
    }
    // 1. 解析过滤器里的已知扩展名（大小写不敏感）
    QString targetSuffix;
    bool ambiguous = false;
    const QString lowerFilter = selectedFilter.toLower();
    const QStringList knownExts = { QStringLiteral("png"), QStringLiteral("svg"), QStringLiteral("pdf") };
    for (const QString& ext : knownExts) {
        if (lowerFilter.contains(QStringLiteral("*.") + ext)) {
            if (!targetSuffix.isEmpty()) {
                // 过滤器含多个已知扩展名，无法以它为唯一权威
                ambiguous = true;
                break;
            }
            targetSuffix = ext;
        }
    }
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const bool knownSuffix = (suffix == QLatin1String("png") || suffix == QLatin1String("svg")
                              || suffix == QLatin1String("pdf"));
    if (!ambiguous && !targetSuffix.isEmpty()) {
        // 2. 过滤器给出唯一已知扩展名：以它为目标修正后缀
        if (suffix == targetSuffix) {
            return filePath;
        }
        if (knownSuffix) {
            // 替换已知图像后缀（保留大小写无关的路径长度换算）
            return filePath.left(filePath.size() - suffix.size()) + targetSuffix;
        }
        return filePath + QLatin1Char('.') + targetSuffix;
    }
    // 3. 过滤器解析不出唯一已知扩展名：保留旧默认行为
    if (knownSuffix) {
        return filePath;
    }
    return filePath + QStringLiteral(".png");
}

/**
 * @brief 导出动作的文件对话框过滤器
 * @return 文件对话框过滤器字符串（PNG/SVG/PDF）
 */
QString DAFigureWidget::exportFileFilter()
{
    return tr("PNG Image (*.png);;SVG Image (*.svg);;PDF Document (*.pdf)");  // cn:PNG 图片 (*.png);;SVG 图片 (*.svg);;PDF 文档 (*.pdf)
}

/**
 * @brief 当前是否有编辑器激活
 * @return
 */
bool DAFigureWidget::isChartEditorActive() const
{
    DA_DC(d);
    return (d->mChartEditor != nullptr) || d->mDataProbeModeActive;
}

/**
 * @brief 开启图表编辑器
 * @param type 编辑器类型
 */
void DAFigureWidget::beginChartEditor(ChartEditorType type)
{
    DA_D(d);
    // 网格模式下子图几何由轨道比例决定，禁止自由缩放（Resize Chart）编辑器
    if (type == SubChartEditor && isGridLayoutEnabled()) {
        if (!d->mGridEditorBlockedWarned) {
            d->mGridEditorBlockedWarned = true;
            qWarning() << "DAFigureWidget::beginChartEditor: the sub chart editor is disabled while the grid layout is "
                          "enabled";
        }
        return;
    }
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
    case DataProbeEditor:
        beginDataProbeEditor();
        break;
    case TextMarker:
        d->beginTextMarkerEditor();
        break;
    case PointerSelector:
        d->beginPointerSelectorEditor();
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
    if (d->mDataProbeModeActive) {
        exitDataProbeMode();
        return;
    }
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
 * @brief 支持redo/undo的删除item（detach）
 * @param chart item所在的绘图
 * @param item 要删除的item
 */
void DAFigureWidget::removeItem_(DAChartWidget* chart, QwtPlotItem* item)
{
    if (!chart || !item) {
        return;
    }
    push(new DAFigureWidgetCommandDetachItem(this, chart, item));
}

/**
 * @brief 支持redo/undo的图元位置移动
 * @param chart item所在的绘图
 * @param item 要移动的item
 * @param oldGeo 移动前几何
 * @param newGeo 移动后几何
 */
void DAFigureWidget::moveItemPosition_(DAChartWidget* chart,
                                       QwtPlotItem* item,
                                       const DAChartElementHitTester::ItemGeometry& oldGeo,
                                       const DAChartElementHitTester::ItemGeometry& newGeo)
{
    if (!chart || !item || !oldGeo.valid || !newGeo.valid) {
        return;
    }
    push(new DAFigureWidgetCommandMovePlotItemPosition(this, chart, item, oldGeo, newGeo));
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
 * @brief 推入一个命令并激活本figure的undo栈
 *
 * push即激活，保证命令进入的栈就是QUndoGroup的active栈，
 * 全局undo/redo action状态立即反映本figure的操作
 * @param cmd 待推入的命令
 */
void DAFigureWidget::push(QUndoCommand* cmd)
{
    if (!cmd) {
        return;
    }
    d_ptr->mUndoStack.push(cmd);
    if (!d_ptr->mUndoStack.isActive()) {
        d_ptr->mUndoStack.setActive();
    }
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
    if (e->key() == Qt::Key_Escape) {
        if (isChartEditorActive()) {
            endChartEditor();
            e->accept();
            return;
        }
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
        // 探针模式激活期间新加入的子图同样应用探针交互（关左键交互、入联动拾取组）
        if (d_ptr->mDataProbeModeActive) {
            applyDataProbeModeToChart(c);
        }
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
    // 最近激活子图类型跟踪：2D 激活；当前 2D 被清空时回退到仍存活的当前 3D 子图
    if (chartWidget) {
        d_ptr->mCurrentSubChartKind = DAFigureCurrentSubChart::Chart2D;
    } else {
        d_ptr->mCurrentSubChartKind =
            d_ptr->mCurrent3DChart ? DAFigureCurrentSubChart::Chart3D : DAFigureCurrentSubChart::None;
    }
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
    if (flag.testFlag(DAChartWidget::AxisVisibilityChanged)) {
        // X 轴可见性变化 -> 全 figure 探针徽章按规则重算（动态跟随，常驻逻辑）
        refreshAllProbeLabels();
    }
    if (!d_ptr->mSuppressInteractionNotify && d_ptr->mDataProbeModeActive
        && (flag.testFlag(DAChartWidget::ZoomStateChanged) || flag.testFlag(DAChartWidget::PanStateChanged)
            || flag.testFlag(DAChartWidget::CrosshairStateChanged))) {
        // 用户开启了缩放/平移/十字线等左键交互 -> 双向互斥，自动退出探针模式
        exitDataProbeMode();
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
 * @brief 网格轨道比例变化（拖动过程中反复触发）
 *
 * 不在此压栈：只在一次调整会话的第一帧记下上一次提交的比例作为撤销基线，
 * 由onGridSizesChangeFinished在结束时统一压入一条命令。
 *
 * 慢日志（仅 DA_ENABLE_SLOW_LOG 配置）：本槽由 QwtFigure 转发
 * QwtFigureGrid::sizesChanged 触发，早于 qwt 的同步布局通道执行，因此无法在这里
 * 计时布局本身；改为在回调返回时记时间戳，下一次回调开始时取差值，近似等于上一次
 * move 触发的整图布局耗时（含少量事件派发开销）。松手时由
 * onGridSizesChangeFinished 汇总整次拖动的布局次数与总耗时。
 */
void DAFigureWidget::onGridSizesChanged()
{
    DA_D(d);
    if (d->mSuppressGridUndo || !d->mGridLayoutEnabled) {
        return;
    }
    DA_SLOW_LOG_SCOPE("dafigure.grid.sizesChanged");
    if (d->mGridChangeActive) {
        // 一次会话中的后续帧：不必重设撤销基线，但要采样布局耗时（见下方）
    } else {
        d->mGridChangeActive = true;
        d->mGridOldRows = d->mGridCommittedRows;
        d->mGridOldCols = d->mGridCommittedCols;
#if defined(DA_ENABLE_SLOW_LOG)
        // 新会话：清掉上一次（可能因 Esc/失焦被取消而没有收尾）的采样
        d->resetGridDragSlowLogSample();
#endif
    }
#if defined(DA_ENABLE_SLOW_LOG)
    const qint64 nowNs = gridSlowLogStampNs();
    if (d->mGridDragLastStampNs > 0) {
        const qint64 layoutNs = nowNs - d->mGridDragLastStampNs;
        d->mGridDragLayoutNs += layoutNs;
        d->mGridDragMaxLayoutNs = qMax(d->mGridDragMaxLayoutNs, layoutNs);
    }
    ++d->mGridDragLayoutPasses;
    // 时间戳在回调末尾取：之后 qwt 才执行本次 sizesChanged 的整图布局
    d->mGridDragLastStampNs = gridSlowLogStampNs();
#endif
}

/**
 * @brief 网格轨道比例调整结束
 *
 * 一次拖动只产生一条撤销命令；程序化设置（setGridSizes）不会走到这里。
 */
void DAFigureWidget::onGridSizesChangeFinished()
{
    DA_D(d);
    if (d->mSuppressGridUndo || !d->mGridLayoutEnabled) {
        return;
    }
    QwtFigureGrid* g = grid();
    if (!g) {
        return;
    }
#if defined(DA_ENABLE_SLOW_LOG)
    // 汇总本次拖动的整图布局统计（最后一次布局发生在松手之前的 move 里）
    d->reportGridDragSlowLog();
#endif
    const QVector< qreal > newRows = g->rowSizes();
    const QVector< qreal > newCols = g->columnSizes();
    if (!d->mGridChangeActive) {
        // 没有观察到实际变化，只同步基线
        d->mGridCommittedRows = newRows;
        d->mGridCommittedCols = newCols;
        return;
    }
    d->mGridChangeActive = false;
    const QVector< qreal > oldRows = d->mGridOldRows;
    const QVector< qreal > oldCols = d->mGridOldCols;
    d->mGridCommittedRows = newRows;
    d->mGridCommittedCols = newCols;
    if (oldRows == newRows && oldCols == newCols) {
        return;
    }
    // 比例已由拖动应用，命令的首个redo跳过，避免重复设置
    push(new DAFigureWidgetCommandResizeGrid(this, oldRows, oldCols, newRows, newCols));
}

/**
 * @brief 图表编辑器完成的槽函数
 * @param isCancel 是否取消
 */
void DAFigureWidget::onFigureChartEditorFinished(bool isCancel)
{
    Q_UNUSED(isCancel);
    endChartEditor();
    // 指针选择器等非创建型编辑器的 finished 信号不经 EditorFactory 的 lambda，
    // 这里统一发射结束状态，保证ribbon按钮复位
    emitChartEditorFinishEdit();
}

/**
 * @brief 序列化DAFigureWidget到数据流
 *
 * @note **生产保存/加载走 XML 路径**（DAXmlHelper::makeElement/loadElement + DAChartItemArchive）。
 *       本文件的 QDataStream 运算符是历史兼容路径（老工程字节流、Chart3DIntegrationTest），
 *       保持可用但不新增字段；新增持久化字段请加到 XML 侧。
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
    // 恢复完成后的兜底：探针徽章按当前各子图 X 轴可见性重算（恢复时序未知，统一刷新一次）
    p->refreshAllProbeLabels();
    return in;
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
 * @brief Start the data probe editing session (persistent mode)
 *
 * Unlike one-shot item editors, the data probe session stays active so the
 * user can click repeatedly to create probe groups. While active:
 * - zoom/pan/crosshair (left-button interactions) are turned off for all charts
 * - y-value picking and the cross-chart picker group are turned on, giving
 *   synchronized cursor readouts on every subplot
 * - each canvas click creates a same-named vertical probe on every 2D subplot
 *   at the clicked x value (single undo command)
 * The session ends via endChartEditor() (button toggle, Esc, or switching to
 * another editor tool). Interaction states are not restored afterwards.
 * \endif
 *
 * \if CHINESE
 * @brief 开始数据探针编辑会话（持久模式）
 *
 * 与一次性图元编辑器不同，数据探针会话持续保持激活，可连续点击创建探针组。
 * 会话期间：
 * - 关闭所有子图的缩放/平移/十字线（左键交互）
 * - 开启所有子图的 Y 值拾取与跨子图联动拾取组，鼠标移动时所有子图联动显示数值
 * - 画布点击时在所有 2D 子图上以点击处 x 值创建同名垂直探针（单条 undo 命令）
 * 会话经 endChartEditor() 结束（按钮再点、Esc 或切换其它编辑工具），
 * 结束后交互状态保持关闭不恢复。
 * \endif
 */
void DAFigureWidget::beginDataProbeEditor()
{
    enterDataProbeMode();
}

/**
 * @brief 进入数据探针模式的内部处理
 */
void DAFigureWidget::enterDataProbeMode()
{
    DA_D(d);
    if (d->mDataProbeModeActive) {
        return;
    }
    d->mDataProbeModeActive = true;
    // 探针依赖联动拾取组
    setDataPickerGroupEnabled(true);
    // 对所有已有 2D 子图应用探针模式交互（关左键交互、开 Y 拾取）
    const QList< DAChartWidget* > charts = getCharts();
    for (DAChartWidget* chart : charts) {
        applyDataProbeModeToChart(chart);
    }
    // 联动拾取组的点击 -> 创建探针组
    if (QwtPlotSeriesDataPickerGroup* group = getDataPickerGroup()) {
        connect(group, &QwtPlotSeriesDataPickerGroup::clicked, this, &DAFigureWidget::onDataProbePickerGroupClicked);
    }
    // Esc 退出：持久会话无 overlay 事件过滤器，用窗口级快捷键保证 Esc 在主窗口任意位置可退出
    d->mDataProbeEscapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(d->mDataProbeEscapeShortcut, &QShortcut::activated, this, [ this ]() { endChartEditor(); });
    emitChartEditorBeginEdit();
}

/**
 * @brief 退出数据探针模式的内部处理
 *
 * 按既定交互决策：退出后缩放/平移/十字线保持关闭，不做恢复
 */
void DAFigureWidget::exitDataProbeMode()
{
    DA_D(d);
    if (!d->mDataProbeModeActive) {
        return;
    }
    d->mDataProbeModeActive = false;
    if (d->mDataProbeEscapeShortcut) {
        d->mDataProbeEscapeShortcut->deleteLater();
        d->mDataProbeEscapeShortcut = nullptr;
    }
    if (QwtPlotSeriesDataPickerGroup* group = getDataPickerGroup()) {
        disconnect(group, &QwtPlotSeriesDataPickerGroup::clicked, this, &DAFigureWidget::onDataProbePickerGroupClicked);
    }
    emitChartEditorFinishEdit();
}

/**
 * @brief 探针模式激活期间对子图应用交互设置
 *
 * 关闭左键交互（缩放/平移/十字线），开启 Y 值拾取；
 * 关闭动作经 mSuppressInteractionNotify 抑制互斥判定，避免触发"用户开启交互->退出探针"的反向逻辑
 */
void DAFigureWidget::applyDataProbeModeToChart(DAChartWidget* chart)
{
    if (!chart) {
        return;
    }
    DA_D(d);
    d->mSuppressInteractionNotify = true;
    chart->enableZoom(false);
    chart->enablePan(false);
    chart->enableCrosshair(false);
    d->mSuppressInteractionNotify = false;
    chart->enableYValuePicking(true);
    if (QwtPlotSeriesDataPickerGroup* group = getDataPickerGroup()) {
        if (QwtPlotSeriesDataPicker* picker = chart->getDataPicker()) {
            group->addPicker(picker);
        }
    }
}

/**
 * @brief 探针会话中联动拾取组收到点击的槽函数
 *
 * 以拾取组第一个 picker 的特征点 x 值为探针位置（与插件行为一致），
 * 在所有 2D 子图上创建同名垂直探针（字母徽章），批量附加进一条 undo 命令
 */
void DAFigureWidget::onDataProbePickerGroupClicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos)
{
    Q_UNUSED(pos);
    DA_D(d);
    if (!d->mDataProbeModeActive || !picker) {
        return;
    }
    QwtPlotSeriesDataPickerGroup* group = getDataPickerGroup();
    if (!group) {
        return;
    }
    // 取点击位置的数据坐标 x（优先特征点，回退画布坐标反算）
    double xValue = 0.0;
    bool hasX = false;
    const QList< QwtPlotSeriesDataPicker* > pickers = group->pickers();
    for (QwtPlotSeriesDataPicker* p : pickers) {
        const QList< QwtPlotSeriesDataPicker::FeaturePoint > fps = p->featurePoints();
        if (!fps.isEmpty()) {
            xValue = fps.first().feature.x();
            hasX   = true;
            break;
        }
    }
    if (!hasX && picker->plot()) {
        // 无特征点（如子图无曲线）时用 invTransform 从画布坐标推 x
        xValue = picker->plot()->invTransform(QwtAxis::XBottom, pos.x());
        hasX   = true;
    }
    if (!hasX) {
        return;
    }

    const QString probeName = generateProbeName();
    QwtText nameText(probeName);
    // 批量创建走单一 undo 复合命令：所有子图的探针一次撤销
    QUndoCommand* batchCmd = new QUndoCommand(tr("add data probes"));  // cn:添加数据探针
    DADataProbeMarker* firstProbe = nullptr;
    const QList< DAChartWidget* > charts = getCharts();
    for (DAChartWidget* chart : charts) {
        DADataProbeMarker* probe = new DADataProbeMarker(DADataProbeMarker::VerticalProbe, nameText);
        probe->setXValue(xValue);
        // 徽章规则：xbottom 可见->底部；仅 xtop 可见->顶部；两轴均不可见->隐藏徽章
        bool labelVisible = chart->isAxisVisible(QwtAxis::XBottom);
        bool labelAtTop   = !labelVisible && chart->isAxisVisible(QwtAxis::XTop);
        probe->setLabelPosition(labelAtTop ? DADataProbeMarker::LabelAtTop : DADataProbeMarker::LabelAtBottom);
        probe->setLabelVisible(labelVisible || labelAtTop);
        probe->setLabelStyle(DADataProbeMarker::RoundedRectBadge);
        probe->attach(chart);
        probe->captureData(true);
        if (!firstProbe) {
            firstProbe = probe;
        }
        // skipFirst=true：item 已经 attach，命令仅入栈不再重复 attach
        new DAFigureWidgetCommandAttachItem(this, chart, probe, true, batchCmd);
        chart->replot();
    }
    if (nullptr == firstProbe) {
        delete batchCmd;
        return;
    }
    push(batchCmd);
    Q_EMIT dataProbeCreated(firstProbe, xValue);
}

/**
 * \if ENGLISH
 * @brief Refresh label position/visibility of all probes according to x-axis visibility
 * \endif
 *
 * \if CHINESE
 * @brief 按当前各子图 X 轴可见性重算所有探针的徽章位置/可见性
 * \endif
 */
void DAFigureWidget::refreshAllProbeLabels()
{
    const QList< DAChartWidget* > charts = getCharts();
    bool changed = false;
    for (DAChartWidget* chart : charts) {
        const bool xBottomVisible = chart->isAxisVisible(QwtAxis::XBottom);
        const bool xTopVisible    = chart->isAxisVisible(QwtAxis::XTop);
        const QwtPlotItemList& items = chart->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() != DADataProbeMarker::Rtti_DataProbeMarker) {
                continue;
            }
            DADataProbeMarker* probe = static_cast< DADataProbeMarker* >(item);
            if (probe->probeType() != DADataProbeMarker::VerticalProbe) {
                continue;
            }
            const bool newVisible = xBottomVisible || xTopVisible;
            const bool newAtTop   = !xBottomVisible && xTopVisible;
            if (probe->isLabelVisible() != newVisible
                || (newVisible && (probe->labelPosition() == DADataProbeMarker::LabelAtTop) != newAtTop)) {
                probe->setLabelPosition(newAtTop ? DADataProbeMarker::LabelAtTop : DADataProbeMarker::LabelAtBottom);
                probe->setLabelVisible(newVisible);
                changed = true;
            }
        }
        if (changed) {
            chart->replot();
        }
    }
}

}  // end DA namespace