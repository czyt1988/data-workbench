#ifndef DAFIGUREWIDGETCOMMANDS_H
#define DAFIGUREWIDGETCOMMANDS_H
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
class DAFigureWidgetCommand_base : public QUndoCommand
{
public:
    DAFigureWidgetCommand_base(DAFigureWidget* fig, QUndoCommand* par = nullptr);
    DAFigureWidget* figure();

public:
    DAFigureWidget* figureWidget { nullptr };
    QList< DAChartWidget* > chartWidgetsList;
};

/**
 * @brief 创建绘图
 */
class DAFigureWidgetCommandCreateChart : public DAFigureWidgetCommand_base
{
public:
    DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                     float xPresent,
                                     float yPresent,
                                     float wPresent,
                                     float hPresent,
                                     QUndoCommand* par = nullptr);

    ~DAFigureWidgetCommandCreateChart();

    void redo() override;

    void undo() override;

public:
    DAChartWidget* mChart { nullptr };
    float mXPresent;
    float mYPresent;
    float mWPresent;
    float mHPresent;
    bool mNeedDelete;
};

/**
 * @brief 设置绘图中窗体的尺寸
 */
class DAFigureWidgetCommandResizeWidget : public DAFigureWidgetCommand_base
{
public:
    DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig,
                                      QWidget* w,
                                      const QRectF& oldPresent,
                                      const QRectF& newPresent,
                                      QUndoCommand* par = nullptr);
    void redo() override;
    void undo() override;

public:
    QWidget* mWidget;
    QRectF mOldPresent;
    QRectF mNewPresent;
};

/**
 * @brief 添加Item
 */
class DAFigureWidgetCommandAttachItem : public DAFigureWidgetCommand_base
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
