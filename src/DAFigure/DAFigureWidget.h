#ifndef DAFIGUREWIDGET_H
#define DAFIGUREWIDGET_H
#include "DAFigureAPI.h"
#include <QScrollArea>
#include <QPainter>
#include "DAChartWidget.h"
#include "DAChartFactory.h"
#include "DAColorTheme.h"
#include "DAFigureChartEditorWidgetOverlay.h"
#include "DAChartElementHitTester.h"
#include "DAFigureElementSelection.h"
// qt
class QPaintEvent;
class QFocusEvent;
class QUndoCommand;
class QUndoStack;
class QKeyEvent;
class QShowEvent;
// qwt
class QwtPlot;
class QwtPlotCurve;
class QwtPlotItem;
class Qwt3DPlot;
class Qwt3DPlotItem;
class QwtFigure;
class QwtPlotSeriesDataPickerGroup;
namespace DA
{
class DAChartAxisRangeBinder;
class DAFigureWidgetOverlay;
class DADataProbeMarker;
class DAChart3DWidget;
/**
 * @brief 绘图窗口
 *
 * - 绘图窗口默认会构建一个QwtFigure
 * - 内部携带一个回退栈，可以接收命令
 * - 提供可redo/undo的快捷接口
 * - QwtFigure如果超出显示范围会显示滚动条滚动显示
 */
class DAFIGURE_API DAFigureWidget : public QScrollArea
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFigureWidget)
public:
    /**
     * @brief 内置的绘图编辑器类型
     */
    enum ChartEditorType
    {
        SubChartEditor = 0,      ///< 子图编辑器
        RectSelectEditor,        ///< 矩形选择编辑器
        EllipseSelectEditor,     ///< 椭圆选择编辑器
        PolygonSelectEditor,     ///< 多边形选择编辑器
        HLineMarker,             ///< 水平线标记
        VLineMarker,             ///< 垂直线标记
        CrossMarker,             ///< 交叉标记
        ArrowMarker,             ///< 箭头标记
        VerticalDataProbe,       ///< 垂直数据探针
        HorizontalDataProbe,     ///< 水平数据探针
        TextMarker,              ///< 文本标注
        PointerSelector,         ///< 指针选择工具（选择/拖动/删除绘图元素）
        BuilinEditorCount,       ///< 内置编辑器数量
        UserDefineEditor = 1000  ///< 用户自定义编辑器
    };
    Q_ENUM(ChartEditorType)

    /**
     * @brief 绘图编辑器状态
     */
    enum ChartEditorStatus
    {
        EndEdit   = 0,    ///< 结束编辑
        BeginEdit = 1,    ///< 开始编辑
        UnknowEditStatus  ///< 未知编辑状态
    };
    Q_ENUM(ChartEditorStatus)
public:
    explicit DAFigureWidget(QWidget* parent = nullptr);
    ~DAFigureWidget();
    // 获取绘图窗口
    QwtFigure* figure() const;
    // id
    QString getFigureId() const;
    void setFigureId(const QString& id);
    // 获取DAChartFactory
    DAChartFactory* getChartFactory() const;
    // 设置ChartFactory
    void setupChartFactory(DAChartFactory* fac);
    // 添加一个2D chart
    DAChartWidget* createChart();
    DAChartWidget* createChart(const QRectF& versatileSize);
    DAChartWidget* createChart(float xVersatile, float yVersatile, float wVersatile, float hVersatile);
    DAChartWidget* createChart_();
    DAChartWidget* createChart_(const QRectF& versatileSize);
    // 移除chart，但不会delete
    void removeChart(DAChartWidget* chart);
    void removeChart_(DAChartWidget* chart);
    //

    // ========== 3D chart 管理 ==========

    // 添加一个3D chart
    DAChart3DWidget* create3DChart();
    DAChart3DWidget* create3DChart(const QRectF& versatileSize);
    DAChart3DWidget* create3DChart(float xVersatile, float yVersatile, float wVersatile, float hVersatile);
    DAChart3DWidget* create3DChart_();
    DAChart3DWidget* create3DChart_(const QRectF& versatileSize);
    // 移除3D chart，但不会delete
    void remove3DChart(DAChart3DWidget* chart);
    void remove3DChart_(DAChart3DWidget* chart);
    // 添加一个已有的3D chart
    void add3DChart(DAChart3DWidget* chart, qreal xVersatile, qreal yVersatile, qreal wVersatile, qreal hVersatile);
    void add3DChart(DAChart3DWidget* chart, const QRectF& versatileSize);
    // 获取所有的3D图表
    QList< DAChart3DWidget* > get3DCharts() const;
    // 获取当前的3D绘图指针
    DAChart3DWidget* getCurrent3DChart() const;
    // 设置当前的3D绘图
    void setCurrent3DChart(DAChart3DWidget* chart);
    // 获取当前的3D chart，如果没有则创建一个新3D chart，此函数不返回nullptr
    DAChart3DWidget* current3DChart();
    // 返回当前光标下的3D图
    DAChart3DWidget* getUnderCursor3DChart() const;
    // 是否存在这个3D绘图
    bool has3DChart(DAChart3DWidget* chart) const;
    // 获取3D图表的数量
    int get3DChartCount() const;

    // 添加一个已有的chart
    void addChart(DAChartWidget* chart, qreal xVersatile, qreal yVersatile, qreal wVersatile, qreal hVersatile);
    void addChart(DAChartWidget* chart, const QRectF& versatileSize);
    // 获取所有的图表(注意次获取没有顺序)
    QList< DAChartWidget* > getCharts() const;

    // 获取当前的2d绘图指针
    DAChartWidget* getCurrentChart() const;
    DAChartWidget* gca() const;
    // 设置当前的2dplot
    void setCurrentChart(QwtPlot* p);
    // 获取当前的chart，如果没有current chart，或figure不存在chart，则创建一个新chart，此函数不返回nullptr
    DAChartWidget* currentChart();
    // 返回当前光标下的widget
    QWidget* getUnderCursorWidget() const;
    // 返回在当前光标下的2D图
    DAChartWidget* getUnderCursorChart() const;
    // 清空所有图 会连续发送chartRemoved信号，此函数会销毁chart对象
    void clear();
    // 设置画布背景色 - 支持redo-undo
    void setBackgroundColor(const QBrush& brush);
    void setBackgroundColor(const QColor& clr);
    const QBrush& getBackgroundColor() const;
    // 通过item查找对应的SAChart2D，如果没有返回nullptr
    DAChartWidget* findChartFromItem(QwtPlotItem* item) const;

    // 获取图表的数量
    int getChartCount() const;
    // 获取默认的绘图颜色
    virtual QColor getDefaultColor() const;
    // 是否存在这个绘图
    bool hasPlot(QwtPlot* chart, bool includeParasite = true) const;
    // 绑定坐标轴范围,让follower的坐标轴范围跟随source
    bool bindAxisRange(QwtPlot* source, QwtPlot* follower, QwtAxisId axisid);
    bool bindAxisRange(QwtPlot* source, QwtAxisId sourceAxisid, QwtPlot* follower, QwtAxisId followerAxisid);
    bool unbindAxisRange(QwtPlot* source, QwtPlot* follower, QwtAxisId axisid);
    bool unbindAxisRange(QwtPlot* source, QwtAxisId sourceAxisid, QwtPlot* follower, QwtAxisId followerAxisid);
    // 获取坐标轴绑定信息
    QList< DAChartAxisRangeBinder* > getBindAxisRangeInfos() const;
    // 设置联动数据拾取，联动数据拾取会让当前所有绘图的QwtPlotSeriesDataPicker建立分组，进行联动
    void setDataPickerGroupEnabled(bool on);
    bool isDataPickerGroupEnabled() const;
    QwtPlotSeriesDataPickerGroup* getDataPickerGroup() const;
    // 通过id查找qwtplot
    QwtPlot* findPlotById(const QString& id, bool findParasite = true) const;
    // 设置颜色主题
    void setColorTheme(const DAColorTheme& th);
    DAColorTheme getColorTheme() const;
    const DAColorTheme& colorTheme() const;
    DAColorTheme& colorTheme();
    // 把当前绘图复制到剪切板
    void copyToClipboard();
    // 开始进行矩形选框交互
    void beginChartEditor(ChartEditorType type);
    // 结束chart editor
    void endChartEditor();
    // 当前是否有编辑器激活
    bool isChartEditorActive() const;

public:
    // ========== 数据探针相关接口 ==========

    // Create a vertical data probe at specified x value
    DADataProbeMarker* createVerticalProbe(double xValue, const QString& name = QString());
    // Create a horizontal data probe at specified y value
    DADataProbeMarker* createHorizontalProbe(double yValue, const QString& name = QString());
    // Remove all data probes from the current chart
    void removeAllProbes();
    // Get all data probes
    QList< DADataProbeMarker* > getProbes() const;
    // Get data probe by name
    DADataProbeMarker* getProbeByName(const QString& name) const;
    // Rename a data probe
    bool renameProbe(DADataProbeMarker* probe, const QString& newName);
    // Start vertical probe creation interaction mode
    void beginVerticalProbeEditor();
    // Start horizontal probe creation interaction mode
    void beginHorizontalProbeEditor();

public:
    // figure的接口转接
    //  Get the normalized rectangle for a axes/获取绘图的归一化矩形
    QRectF axesNormRect(QwtPlot* plot) const;
    // Get the normalized rectangle for a child widget/获取子窗口的的归一化矩形
    QRectF widgetNormRect(QWidget* w) const;
    // Add a widget with normalized coordinates/使用归一化坐标添加widget
    void addWidget(QWidget* widget, qreal left, qreal top, qreal width, qreal height);
    void addWidget(QWidget* widget,
                   int rowCnt,
                   int colCnt,
                   int row,
                   int col,
                   int rowSpan  = 1,
                   int colSpan  = 1,
                   qreal wspace = 0.0,
                   qreal hspace = 0.0);
    // 改变已经添加的窗口的位置占比,如果窗口还没添加，此函数无效
    void setWidgetNormPos(QWidget* widget, const QRectF& rect);
    // 获取在此坐标下的绘图，如果此坐标下没有，则返回nullptr，存在寄生轴情况只返回宿主轴
    QwtPlot* plotUnderPos(const QPoint& pos) const;
    void setFaceBrush(const QBrush& brush);
    QBrush getFaceBrush() const;

public:
    // 绘图相关

    // redo/undo的additem
    bool addItem_(QwtPlotItem* item);
    void addItem_(DAChartWidget* chart, QwtPlotItem* item, bool skipfirstRedo = false);
    // 支持redo/undo的removeitem（detach）
    void removeItem_(DAChartWidget* chart, QwtPlotItem* item);
    // 支持redo/undo的图元位置移动
    void moveItemPosition_(DAChartWidget* chart,
                           QwtPlotItem* item,
                           const DAChartElementHitTester::ItemGeometry& oldGeo,
                           const DAChartElementHitTester::ItemGeometry& newGeo);
    // 支持redo/undo的addCurve，等同于gca()->addCurve
    QwtPlotCurve* addCurve_(const QVector< QPointF >& xyDatas);
    QwtPlotCurve* addScatter_(const QVector< QPointF >& xyDatas);
    // 添加柱状图
    QwtPlotBarChart* addBar_(const QVector< QPointF >& xyDatas);
    QwtPlotIntervalCurve*
    addErrorBar_(const QVector< double >& values, const QVector< double >& mins, const QVector< double >& maxs);

    // ========== 3D item undoable 接口 ==========

    // 支持redo/undo的添加3D item
    void add3DItem_(DAChart3DWidget* chart3d, Qwt3DPlotItem* item, bool skipfirstRedo = false);

public:
    // 推送一个命令
    void push(QUndoCommand* cmd);
    // 获取Undo Stack
    QUndoStack* getUndoStack();

Q_SIGNALS:
    /**
     * @brief 添加了chart
     * @param chart指针
     */
    void chartAdded(DA::DAChartWidget* c);

    /**
     * @brief 绘图移除信号
     * @param c
     */
    void chartRemoved(DA::DAChartWidget* c);

    // 当前选中的发生改变
    /**
     * @brief 当前的绘图发生了变更
     *
     * 当前窗口是figure的默认窗口，任何对figure的动作会作用于当前绘图
     * @param w
     */
    void currentChartChanged(DA::DAChartWidget* c);

    /**
     * @brief 添加了3D chart
     * @param chart3d指针
     */
    void chart3DAdded(DA::DAChart3DWidget* c);

    /**
     * @brief 3D绘图移除信号
     * @param c
     */
    void chart3DRemoved(DA::DAChart3DWidget* c);

    /**
     * @brief 当前的3D绘图发生了变更
     * @param c
     */
    void current3DChartChanged(DA::DAChart3DWidget* c);

    /**
     * @brief chartEditorStatusChanged
     */
    void chartEditorStatusChanged(DA::DAFigureWidget::ChartEditorStatus status);

    /**
     * @brief 绘图元素被选中的信号（指针工具/树形控件等来源）
     *
     * 由DAAppController消费，联动右侧属性设置面板
     */
    void figureElementClicked(const DA::DAFigureElementSelection& sel);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;
    void showEvent(QShowEvent* e) override;
private Q_SLOTS:
    // 窗口的位置发生改变槽
    void onWidgetGeometryChanged(QWidget* w, const QRectF& oldNormGeo, const QRectF& newNormGeo);
    // DAFigureOverlayChartEditor的激活窗口变化
    void onOverlayActiveWidgetChanged(QWidget* oldActive, QWidget* newActive);
    void onAxesAdded(QwtPlot* newAxes);
    void onAxesRemoved(QwtPlot* removedAxes);
    void onCurrentAxesChanged(QwtPlot* plot);
    void onChartPropertyChanged(DA::DAChartWidget* chart, DAChartWidget::ChartPropertyChangeFlags flag);
    void onFigureChartEditorFinished(bool isCancel);

private:
    void init();
    // 建立DataPickerGroup
    void setupDataPickerGroup();
    // 编辑器开始
    void emitChartEditorBeginEdit();
    void emitChartEditorFinishEdit();
    // Probe name generation (internal use)
    QString generateProbeName();
    bool isProbeNameExists(const QString& name) const;
    QString probeNameFromCounter(int counter) const;
};

DAFIGURE_API QDataStream& operator<<(QDataStream& out, const DAFigureWidget* p);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, DAFigureWidget* p);
}  // end DA namespace
#endif  // SAFIGUREWINDOW_H
