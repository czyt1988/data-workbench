#ifndef DAFIGUREWIDGET_H
#define DAFIGUREWIDGET_H
#include "DAFigureAPI.h"
#include <QScrollArea>
#include <QPainter>
#include "DAChartWidget.h"
#include "DAChartFactory.h"
#include "DAColorTheme.h"
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
class QwtFigure;
class QwtPlotSeriesDataPickerGroup;
namespace DA
{
class DAChartAxisRangeBinder;
class DAFigureWidgetOverlay;
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
	// 开启子窗口编辑模式
	void setSubChartEditorEnable(bool enable = true);
	DAFigureWidgetOverlay* getSubChartEditor() const;
	// 判断是否在进行子窗口编辑
	bool isEnableSubChartEditor() const;
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
	void addItem_(DAChartWidget* chart, QwtPlotItem* item);
	// 支持redo/undo的addCurve，等同于gca()->addCurve
	QwtPlotCurve* addCurve_(const QVector< QPointF >& xyDatas);
	QwtPlotCurve* addScatter_(const QVector< QPointF >& xyDatas);
	// 添加柱状图
	QwtPlotBarChart* addBar_(const QVector< QPointF >& xyDatas);
	QwtPlotIntervalCurve* addErrorBar_(const QVector< double >& values,
	                                   const QVector< double >& mins,
	                                   const QVector< double >& maxs);

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

protected:
	void keyPressEvent(QKeyEvent* e);
	void showEvent(QShowEvent* e);
private slots:
	// 窗口的位置发生改变槽
	void onWidgetGeometryChanged(QWidget* w, const QRectF& oldNormGeo, const QRectF& newNormGeo);
	// DAFigureOverlayChartEditor的激活窗口变化
	void onOverlayActiveWidgetChanged(QWidget* oldActive, QWidget* newActive);
	void onAxesAdded(QwtPlot* newAxes);
	void onAxesRemoved(QwtPlot* removedAxes);
	void onCurrentAxesChanged(QwtPlot* plot);
	void onChartPropertyChanged(DA::DAChartWidget* chart, DAChartWidget::ChartPropertyChangeFlags flag);

private:
	void init();
	// 建立DataPickerGroup
	void setupDataPickerGroup();
};

DAFIGURE_API QDataStream& operator<<(QDataStream& out, const DAFigureWidget* p);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, DAFigureWidget* p);
}  // end DA namespace
#endif  // SAFIGUREWINDOW_H
