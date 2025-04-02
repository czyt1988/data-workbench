#ifndef DAFIGUREWIDGET_H
#define DAFIGUREWIDGET_H
#include "DAFigureAPI.h"
#include <QWidget>
#include <QPainter>
#include "DAChartWidget.h"
#include "DAFigureContainer.h"
#include "DAChartFactory.h"
#include "DAColorTheme.h"
class QPaintEvent;
class QFocusEvent;
class QUndoCommand;
class QUndoStack;
class QwtPlotCurve;
class QwtPlotItem;
namespace DA
{

class DAFigureWidgetOverlayChartEditor;
/**
 * @brief 绘图窗口
 */
class DAFIGURE_API DAFigureWidget : public DAFigureContainer
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAFigureWidget)
public:
	explicit DAFigureWidget(QWidget* parent = 0);
	~DAFigureWidget();
	// 获取DAChartFactory
	DAChartFactory* getChartFactory() const;
	// 设置ChartFactory
	void setupChartFactory(DAChartFactory* fac);
	// 添加一个2D chart
	DAChartWidget* createChart();
	DAChartWidget* createChart(float xPresent, float yPresent, float wPresent, float hPresent);
	DAChartWidget* createChart_();
	DAChartWidget* createChart_(float xPresent, float yPresent, float wPresent, float hPresent);
	// 移除chart，但不会delete
	void removeChart(DAChartWidget* chart);
	// 添加一个已有的chart
	void addChart(DAChartWidget* chart, float xPresent, float yPresent, float wPresent, float hPresent);
	// 获取所有的图表(注意次获取没有顺序)
	QList< DAChartWidget* > getCharts() const;
	// 获取所有的图表安装显示顺序,这个顺序是在窗口显示最顶层的排位最前
	QList< DAChartWidget* > getChartsOrdered() const;
	// 获取当前的2d绘图指针
	DAChartWidget* getCurrentChart() const;
	DAChartWidget* gca() const;
	// 设置当前的2dplot
	bool setCurrentChart(DAChartWidget* p);
	// 获取当前的chart，如果没有current chart，或figure不存在chart，则创建一个新chart，此函数不返回nullptr
	DAChartWidget* currentChart();
	// 返回当前光标下的widget
	QWidget* getUnderCursorWidget() const;
	// 返回在当前光标下的2D图
	DAChartWidget* getUnderCursorChart() const;
	// 清空所有图 会连续发送chartRemoved信号，此函数会销毁chart对象
	void clearAllCharts();
	// 设置画布背景色 - 支持redo-undo
	void setBackgroundColor(const QBrush& brush);
	void setBackgroundColor(const QColor& clr);
	const QBrush& getBackgroundColor() const;
	// 通过item查找对应的SAChart2D，如果没有返回nullptr
	DAChartWidget* findChartFromItem(QwtPlotItem* item) const;
	// 开启子窗口编辑模式
	void enableSubChartEditor(bool enable = true);
	DAFigureWidgetOverlayChartEditor* getSubChartEditor() const;
	// 判断是否在进行子窗口编辑
	bool isEnableSubChartEditor() const;
	// 获取图表的数量
	int getChartCount() const;
	// 获取默认的绘图颜色
	virtual QColor getDefaultColor() const;
	// 设置颜色主题
	void setFigureColorTheme(const DAColorTheme& th);
	DAColorTheme getFigureColorTheme() const;
	const DAColorTheme& figureColorTheme() const;
	DAColorTheme& figureColorTheme();

public:
	// 绘图相关

	// redo/undo的additem
	bool addItem_(QwtPlotItem* item);
	void addItem_(DAChartWidget* chart, QwtPlotItem* item);
	// 支持redo/undo的addCurve，等同于gca()->addCurve
	QwtPlotCurve* addCurve_(const QVector< QPointF >& xyDatas);
	QwtPlotCurve* addScatter_(const QVector< QPointF >& xyDatas);
	QwtPlotBarChart* addBar_(const QVector< QPointF >& xyDatas, const QColor& color);
	QwtPlotIntervalCurve* addErrorBar_(const QVector< QwtIntervalSample >& xyDatas);

public:
	// 推送一个命令
	void push(QUndoCommand* cmd);
	// 获取Undo Stack
	QUndoStack* getUndoStack();

protected:
	virtual void paintEvent(QPaintEvent* e) override;
signals:
	/**
	 * @brief 添加了chart
	 * @param chart指针
	 */
	void chartAdded(DA::DAChartWidget* c);

	/**
	 * @brief 绘图即将移除
	 * @param plot 即将移除的绘图，此时指针还有效
	 */
	void chartWillRemove(DA::DAChartWidget* c);

	// 当前选中的发生改变
	/**
	 * @brief 当前的绘图发生了变更
	 *
	 * 当前窗口是figure的默认窗口，任何对figure的动作会作用于当前绘图
	 * @param w
	 */
	void currentChartChanged(DA::DAChartWidget* c);
private slots:
	// 窗口的位置发生改变槽
	void onWidgetGeometryChanged(QWidget* w, const QRect& oldGeometry, const QRect& newGeometry);
	// DAFigureOverlayChartEditor的激活窗口变化
	void onOverlayActiveWidgetChanged(QWidget* oldActive, QWidget* newActive);
};

DAFIGURE_API QDataStream& operator<<(QDataStream& out, const DAFigureWidget* p);
DAFIGURE_API QDataStream& operator>>(QDataStream& in, DAFigureWidget* p);
}  // end DA namespace
#endif  // SAFIGUREWINDOW_H
