#ifndef DAFIGUREWIDGETCOMMANDS_H
#define DAFIGUREWIDGETCOMMANDS_H
#include "DAFigureAPI.h"
#include <QUndoCommand>
#include <QPointer>
#include <QRectF>
#include "DAChartElementHitTester.h"
class QWidget;
class QwtPlotItem;
class Qwt3DPlotItem;
namespace DA
{
class DAChartWidget;
class DAChart3DWidget;
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
 * @brief 创建3D绘图
 */
class DAFIGURE_API DAFigureWidgetCommandCreate3DChart : public DAFigureWidgetCommandBase
{
public:
	DAFigureWidgetCommandCreate3DChart(DAFigureWidget* fig,
                                       qreal xPresent,
                                       qreal yPresent,
                                       qreal wPresent,
                                       qreal hPresent,
                                       QUndoCommand* par = nullptr);
	DAFigureWidgetCommandCreate3DChart(DAFigureWidget* fig, const QRectF& versatileSize, QUndoCommand* par = nullptr);
	~DAFigureWidgetCommandCreate3DChart();

	void redo() override;
	void undo() override;

	DAChart3DWidget* getChart3DWidget();

public:
	DAChart3DWidget* mChart3D { nullptr };
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
 * @brief 移除3D绘图
 */
class DAFIGURE_API DAFigureWidgetCommandRemove3DChart : public DAFigureWidgetCommandBase
{
public:
	DAFigureWidgetCommandRemove3DChart(DAFigureWidget* fig, DAChart3DWidget* chart, QUndoCommand* par = nullptr);
	~DAFigureWidgetCommandRemove3DChart();

	void redo() override;
	void undo() override;

public:
	DAChart3DWidget* mChart3D { nullptr };
	QRectF mChartNormRect;
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
	QPointer< QWidget > mWidget;
    QRectF mOldNormRect;
    QRectF mNewNormRect;
};

/**
 * @brief 添加Item
 */
class DAFIGURE_API DAFigureWidgetCommandAttachItem : public DAFigureWidgetCommandBase
{
public:
	// 添加Item
	DAFigureWidgetCommandAttachItem(DAFigureWidget* fig,
                                    DAChartWidget* chart,
                                    QwtPlotItem* item,
                                    bool skipFirst    = true,
                                    QUndoCommand* par = nullptr);
	~DAFigureWidgetCommandAttachItem();
	void redo() override;
	void undo() override;

public:
	QPointer< DAChartWidget > mChart;
	QwtPlotItem* mItem;
	bool mSkipFirst;
	bool mNeedDelete { false };
};

/**
 * @brief 添加3D Item
 */
class DAFIGURE_API DAFigureWidgetCommandAttach3DItem : public DAFigureWidgetCommandBase
{
public:
	// 添加3D Item
	DAFigureWidgetCommandAttach3DItem(DAFigureWidget* fig,
                                     DAChart3DWidget* chart3d,
                                     Qwt3DPlotItem* item,
                                     bool skipFirst    = true,
                                     QUndoCommand* par = nullptr);
	~DAFigureWidgetCommandAttach3DItem();
	void redo() override;
	void undo() override;

public:
	QPointer< DAChart3DWidget > mChart3D;
	Qwt3DPlotItem* mItem;
	bool mSkipFirst;
	bool mNeedDelete { false };
};

/**
 * @brief 把图元从一个绘图移动到另一个绘图
 *
 * redo: item->attach(targetChart)
 * undo: item->attach(sourceChart)
 * QwtPlotItem::attach会自动先detach旧的plot再attach新的plot，
 * 树模型通过QwtPlot::itemAttached信号自动更新。
 */
class DAFIGURE_API DAFigureWidgetCommandMoveItem : public DAFigureWidgetCommandBase
{
public:
	DAFigureWidgetCommandMoveItem(DAFigureWidget* fig,
                                 DAChartWidget* sourceChart,
                                 DAChartWidget* targetChart,
                                 QwtPlotItem* item,
                                 QUndoCommand* par = nullptr);
	~DAFigureWidgetCommandMoveItem();
	void redo() override;
	void undo() override;

public:
	QPointer< DAChartWidget > mSourceChart;
	QPointer< DAChartWidget > mTargetChart;
	QwtPlotItem* mItem;
};

/**
 * @brief 把3D图元从一个3D绘图移动到另一个3D绘图
 *
 * redo: item->attach(targetChart3D)
 * undo: item->attach(sourceChart3D)
 * Qwt3DPlotItem::attach会自动先detach旧的plot再attach新的plot
 */
class DAFIGURE_API DAFigureWidgetCommandMove3DItem : public DAFigureWidgetCommandBase
{
public:
    DAFigureWidgetCommandMove3DItem(DAFigureWidget* fig,
                                  DAChart3DWidget* sourceChart3D,
                                  DAChart3DWidget* targetChart3D,
                                  Qwt3DPlotItem* item,
                                  QUndoCommand* par = nullptr);
    ~DAFigureWidgetCommandMove3DItem();
    void redo() override;
    void undo() override;

public:
    QPointer< DAChart3DWidget > mSourceChart3D;
    QPointer< DAChart3DWidget > mTargetChart3D;
    Qwt3DPlotItem* mItem;
};

/**
 * @brief 把图元从绘图分离（删除）
 *
 * redo: item->detach()
 * undo: item->attach(chart)
 * 析构时若 item 仍处于 detach 态则 delete（与 AttachItem 的 mNeedDelete 模式对称）
 */
class DAFIGURE_API DAFigureWidgetCommandDetachItem : public DAFigureWidgetCommandBase
{
public:
    DAFigureWidgetCommandDetachItem(DAFigureWidget* fig,
                                    DAChartWidget* chart,
                                    QwtPlotItem* item,
                                    QUndoCommand* par = nullptr);
    ~DAFigureWidgetCommandDetachItem();
    void redo() override;
    void undo() override;

public:
    QPointer< DAChartWidget > mChart;
    QwtPlotItem* mItem;
    bool mNeedDelete { false };
};

/**
 * @brief 改变图元在绘图内的几何位置（拖动移动）
 *
 * 几何快照通过 DAChartElementHitTester::itemGeometry/setItemGeometry 记录与恢复，
 * 支持 marker 族（锚点）与箭头（起终点）
 */
class DAFIGURE_API DAFigureWidgetCommandMovePlotItemPosition : public DAFigureWidgetCommandBase
{
public:
    DAFigureWidgetCommandMovePlotItemPosition(DAFigureWidget* fig,
                                              DAChartWidget* chart,
                                              QwtPlotItem* item,
                                              const DAChartElementHitTester::ItemGeometry& oldGeo,
                                              const DAChartElementHitTester::ItemGeometry& newGeo,
                                              QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

public:
    QPointer< DAChartWidget > mChart;
    QwtPlotItem* mItem;
    DAChartElementHitTester::ItemGeometry mOldGeo;
    DAChartElementHitTester::ItemGeometry mNewGeo;
};
}
#endif  // DAFIGUREWIDGETCOMMANDS_H
