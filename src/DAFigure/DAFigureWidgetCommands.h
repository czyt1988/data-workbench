#ifndef DAFIGUREWIDGETCOMMANDS_H
#define DAFIGUREWIDGETCOMMANDS_H
#include "DAFigureAPI.h"
#include <QUndoCommand>
#include <QRectF>
class QWidget;
class QwtPlotItem;
namespace DA
{
class DAChartWidget;
class DAFigureWidget;
/**
 * @brief DAFigureWidget命令的基本体
 */
class DAFIGURE_API DAFigureWidgetCommandBase : public QUndoCommand
{
public:
	DAFigureWidgetCommandBase(DAFigureWidget* fig, QUndoCommand* par = nullptr);
	DAFigureWidget* figure();

public:
	DAFigureWidget* figureWidget { nullptr };
	QList< DAChartWidget* > chartWidgetsList;
};

/**
 * @brief 创建绘图
 */
class DAFIGURE_API DAFigureWidgetCommandCreateChart : public DAFigureWidgetCommandBase
{
public:
	DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                     qreal xPresent,
                                     qreal yPresent,
                                     qreal wPresent,
                                     qreal hPresent,
                                     QUndoCommand* par = nullptr);
	DAFigureWidgetCommandCreateChart(DAFigureWidget* fig, const QRectF& versatileSize, QUndoCommand* par = nullptr);

	~DAFigureWidgetCommandCreateChart();

	void redo() override;

	void undo() override;

	DAChartWidget* getChartWidget();

public:
	DAChartWidget* mChart { nullptr };
	QRectF mChartSize;
	bool mNeedDelete { false };
};

/**
 * @brief 移除绘图
 */
class DAFIGURE_API DAFigureWidgetCommandRemoveChart : public DAFigureWidgetCommandBase
{
public:
	DAFigureWidgetCommandRemoveChart(DAFigureWidget* fig, DAChartWidget* chart, QUndoCommand* par = nullptr);
	~DAFigureWidgetCommandRemoveChart();

	void redo() override;
	void undo() override;

public:
	DAChartWidget* mChart { nullptr };
    QRectF mChartNormRect;
	bool mIsRelative { true };
	bool mNeedDelete { false };
};

/**
 * @brief 设置绘图中窗体的尺寸
 */
class DAFIGURE_API DAFigureWidgetCommandResizeWidget : public DAFigureWidgetCommandBase
{
public:
    DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig,
                                      QWidget* w,
                                      const QRectF& oldNormRect,
                                      const QRectF& newNormRect,
                                      QUndoCommand* par = nullptr);
	void redo() override;
	void undo() override;

public:
	QWidget* mWidget;
    QRectF mOldNormRect;
    QRectF mNewNormRect;
};

/**
 * @brief 添加Item
 */
class DAFIGURE_API DAFigureWidgetCommandAttachItem : public DAFigureWidgetCommandBase
{
public:
	/**
	 * @brief 添加Item
	 * @param fig figure
	 * @param chart 对应的DAChartWidget指针
	 * @param item 对应的QwtPlotItem
	 * @param skipFirst 第一次跳过item->attach(chart);操作，后续的redo不会再跳过
	 * @param par
	 */
	DAFigureWidgetCommandAttachItem(DAFigureWidget* fig,
                                    DAChartWidget* chart,
                                    QwtPlotItem* item,
                                    bool skipFirst    = true,
                                    QUndoCommand* par = nullptr);
	~DAFigureWidgetCommandAttachItem();
	void redo() override;
	void undo() override;

public:
	DAChartWidget* mChart;
	QwtPlotItem* mItem;
	bool mSkipFirst;
	bool mNeedDelete { false };
};
}
#endif  // DAFIGUREWIDGETCOMMANDS_H
