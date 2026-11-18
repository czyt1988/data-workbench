#include "DAFigureWidgetCommands.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "DAChartUtil.h"
// qwt
#include "qwt_figure_grid.h"
// qwt3d
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"
// std
#include <limits>
namespace DA
{
namespace
{
/**
 * @brief 记录控件的网格单元格
 *
 * 只有网格启用且控件确实是网格成员时才返回 valid=true 的快照；其余情况返回无效快照，
 * 命令按原来的自由布局路径执行（自由布局行为零改动）。
 * @param fig 绘图窗口
 * @param w 成员控件
 * @return 网格单元格快照
 */
DAFigureGridCellSnapshot captureGridCell(DAFigureWidget* fig, QWidget* w)
{
    DAFigureGridCellSnapshot snap;
    if (!fig || !w || !fig->isGridLayoutEnabled()) {
        return snap;
    }
    QwtFigureGrid* grid = fig->grid();
    if (!grid) {
        return snap;
    }
    snap.valid = grid->gridPosition(w, &snap.row, &snap.col, &snap.rowSpan, &snap.colSpan);
    return snap;
}

/**
 * @brief 按网格快照把控件放回 figure
 *
 * redo/undo 的网格分支：走 @ref DAFigureWidget::setGridCell（网格坐标登记 + 重排），
 * 而不是自由布局的 addChart/setWidgetNormPos，否则恢复出来的子图会与网格成员叠压。
 * @param fig 绘图窗口
 * @param w 成员控件
 * @param snap 网格单元格快照
 * @return 已按网格登记返回true；快照无效、网格已关闭或登记失败返回false（调用方回退自由布局）
 */
bool applyGridCellIfPossible(DAFigureWidget* fig, QWidget* w, const DAFigureGridCellSnapshot& snap)
{
    if (!fig || !w || !snap.valid || !fig->isGridLayoutEnabled()) {
        return false;
    }
    if (fig->setGridCell(w, snap.row, snap.col, snap.rowSpan, snap.colSpan)) {
        return true;
    }
    qWarning() << "DAFigureWidgetCommands: restoring grid cell" << snap.row << snap.col << "failed, fall back to free layout";
    return false;
}

/**
 * @brief 把已从 figure 移除的 2D 子图恢复为网格成员
 *
 * undo 会经 removeChart 移除 layout item（子图仍以 figure 为父），此时直接 setGridCell 会因
 * 「已在 figure 中」分支只更新登记而不补回 layout item，子图不显示。所以先按自由矩形补回成员，
 * 再按单元格登记（网格随后接管几何）。
 * @param fig 绘图窗口
 * @param chart 子图
 * @param snap 网格单元格快照
 * @param freeRect 自由布局归一化矩形（补回 layout item 时使用）
 * @return 已按网格登记返回true；快照无效、网格已关闭或登记失败返回false（调用方回退自由布局）
 */
bool restoreChartAsGridMember(DAFigureWidget* fig,
                              DAChartWidget* chart,
                              const DAFigureGridCellSnapshot& snap,
                              const QRectF& freeRect)
{
    if (!fig || !chart || !snap.valid || !fig->isGridLayoutEnabled()) {
        return false;
    }
    if (!fig->hasPlot(chart)) {
        fig->addChart(chart, freeRect);
    }
    if (fig->setGridCell(chart, snap.row, snap.col, snap.rowSpan, snap.colSpan)) {
        return true;
    }
    qWarning() << "DAFigureWidgetCommands: restoring chart grid cell" << snap.row << snap.col
               << "failed, fall back to free layout";
    return false;
}

/**
 * @brief 把已从 figure 移除的 3D 子图恢复为网格成员
 * @param fig 绘图窗口
 * @param chart3d 3D 子图
 * @param snap 网格单元格快照
 * @param freeRect 自由布局归一化矩形（补回成员时使用）
 * @return 已按网格登记返回true；快照无效、网格已关闭或登记失败返回false（调用方回退自由布局）
 */
bool restore3DChartAsGridMember(DAFigureWidget* fig,
                                DAChart3DWidget* chart3d,
                                const DAFigureGridCellSnapshot& snap,
                                const QRectF& freeRect)
{
    if (!fig || !chart3d || !snap.valid || !fig->isGridLayoutEnabled()) {
        return false;
    }
    if (!fig->has3DChart(chart3d)) {
        fig->add3DChart(chart3d, freeRect);
    }
    if (fig->setGridCell(chart3d, snap.row, snap.col, snap.rowSpan, snap.colSpan)) {
        return true;
    }
    qWarning() << "DAFigureWidgetCommands: restoring 3D chart grid cell" << snap.row << snap.col
               << "failed, fall back to free layout";
    return false;
}

/**
 * @brief 单元格是否已被占用
 * @param grid 网格模型
 * @param row 行
 * @param col 列
 * @return 已被某个成员覆盖返回true
 */
bool isGridCellOccupied(QwtFigureGrid* grid, int row, int col)
{
    if (!grid) {
        return false;
    }
    const QList< QWidget* > items = grid->items();
    for (QWidget* w : items) {
        int r = 0;
        int c = 0;
        int rs = 1;
        int cs = 1;
        if (!grid->gridPosition(w, &r, &c, &rs, &cs)) {
            continue;
        }
        if (row >= r && row < r + rs && col >= c && col < c + cs) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 为新建成员挑选单元格
 *
 * 优先取第一个空闲单元格；网格已被占满时退回 (0,0)——网格模型允许同格共存
 * （信号分析的热图与三维瀑布就是同一格按模式显隐切换）。
 * @param fig 绘图窗口（必须已启用网格）
 * @return 选中的单元格快照
 */
DAFigureGridCellSnapshot pickFreeGridCell(DAFigureWidget* fig)
{
    DAFigureGridCellSnapshot snap;
    QwtFigureGrid* grid = fig ? fig->grid() : nullptr;
    if (!grid || !grid->isEnabled()) {
        return snap;
    }
    for (int r = 0; r < grid->rowCount(); ++r) {
        for (int c = 0; c < grid->columnCount(); ++c) {
            if (!isGridCellOccupied(grid, r, c)) {
                snap.valid = true;
                snap.row   = r;
                snap.col   = c;
                return snap;
            }
        }
    }
    snap.valid = true;
    snap.row   = 0;
    snap.col   = 0;
    return snap;
}

/**
 * @brief 由归一化矩形反查目标单元格
 *
 * 供自由布局的 Resize 命令在网格成员上换算目标单元格：取距离矩形左上角最近的单元格中心，
 * 保留原有跨度。落在轨道间隙时同样能选到相邻单元格。
 * @param fig 绘图窗口（必须已启用网格）
 * @param normRect 目标归一化矩形
 * @param span 原跨度（行列跨度）
 * @return 目标单元格快照
 */
DAFigureGridCellSnapshot gridCellForNormRect(DAFigureWidget* fig, const QRectF& normRect, int rowSpan, int colSpan)
{
    DAFigureGridCellSnapshot snap;
    QwtFigureGrid* grid = fig ? fig->grid() : nullptr;
    if (!grid || !grid->isEnabled()) {
        return snap;
    }
    const int rows = grid->rowCount();
    const int cols = grid->columnCount();
    const QPointF corner = normRect.topLeft();
    qreal bestDistance    = std::numeric_limits< qreal >::max();
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const QRectF cell = grid->normRectFor(r, c);
            const QPointF center = cell.center();
            const qreal dx       = center.x() - corner.x();
            const qreal dy       = center.y() - corner.y();
            const qreal distance = dx * dx + dy * dy;
            if (distance < bestDistance) {
                bestDistance = distance;
                snap.row     = r;
                snap.col     = c;
            }
        }
    }
    snap.rowSpan = qBound(1, rowSpan, rows - snap.row);
    snap.colSpan = qBound(1, colSpan, cols - snap.col);
    snap.valid   = true;
    return snap;
}
}  // namespace

//----------------------------------------------------
// DAFigureWidgetCommand_base
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param par 父命令
 */
DAFigureWidgetCommandBase::DAFigureWidgetCommandBase(DAFigureWidget* fig, QUndoCommand* par)
    : QUndoCommand(par), figureWidget(fig)
{
}

/**
 * @brief 获取关联的绘图窗口
 * @return 绘图窗口指针
 */
DAFigureWidget* DAFigureWidgetCommandBase::figure()
{
    return figureWidget;
}

//----------------------------------------------------
// DAFigureWidgetCommandCreateChart
//----------------------------------------------------

/**
 * @brief 构造函数，通过百分比坐标创建图表
 * @param fig 关联的绘图窗口
 * @param xPresent x坐标百分比
 * @param yPresent y坐标百分比
 * @param wPresent 宽度百分比
 * @param hPresent 高度百分比
 * @param par 父命令
 */
DAFigureWidgetCommandCreateChart::DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                                                   qreal xPresent,
                                                                   qreal yPresent,
                                                                   qreal wPresent,
                                                                   qreal hPresent,
                                                                   QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart(nullptr)
    , mChartSize(xPresent, yPresent, wPresent, hPresent)
    , mNeedDelete(false)
{
    setText(QObject::tr("create chart"));  // cn:创建绘图
}

/**
 * @brief 构造函数，通过归一化矩形创建图表
 * @param fig 关联的绘图窗口
 * @param versatileSize 归一化矩形
 * @param par 父命令
 */
DAFigureWidgetCommandCreateChart::DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                                                   const QRectF& versatileSize,
                                                                   QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(nullptr), mChartSize(versatileSize), mNeedDelete(false)
{
    setText(QObject::tr("create chart"));  // cn:创建绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandCreateChart::~DAFigureWidgetCommandCreateChart()
{
    if (mNeedDelete) {
        if (mChart) {
            mChart->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：创建或重新添加图表
 *
 * 首次创建沿用 createChart 的完整语义（轴刻度初始化等），网格 figure 上再登记到空闲单元格；
 * undo 之后的 redo 按快照恢复原单元格，避免恢复成自由成员与网格成员叠压。
 */
void DAFigureWidgetCommandCreateChart::redo()
{
    mNeedDelete = false;
    if (!figure()) {
        return;
    }
    if (mChart) {
        // 这是执行undo后执行redo会进入这个分支
        if (!restoreChartAsGridMember(figure(), mChart, mGridCell, mChartSize)) {
            figure()->addChart(mChart, mChartSize);
        }
    } else {
        // 第一次执行redo会进入这里
        mChart = figure()->createChart(mChartSize);
        mChart->setAxisLabel(QwtAxis::XBottom, "x");
        mChart->setAxisLabel(QwtAxis::YLeft, "y");
        // 网格 figure：把新建子图登记进空闲单元格（不登记会作为自由成员压在其他成员之上）
        if (figure()->isGridLayoutEnabled()) {
            const DAFigureGridCellSnapshot cell = pickFreeGridCell(figure());
            if (applyGridCellIfPossible(figure(), mChart, cell)) {
                mGridCell = cell;
            }
        }
    }
}

/**
 * @brief 撤销操作：移除图表
 *
 * 移除前记录网格单元格：removeChart 会一并注销网格登记，redo 时按该单元格恢复。
 */
void DAFigureWidgetCommandCreateChart::undo()
{
    mNeedDelete = true;
    if (!figure() || !mChart) {
        return;
    }
    mGridCell = captureGridCell(figure(), mChart);
    figure()->removeChart(mChart);
}

/**
 * @brief 获取创建的图表窗口
 * @return 图表窗口指针
 */
DAChartWidget* DAFigureWidgetCommandCreateChart::getChartWidget()
{
    return mChart;
}

//===============================================================
// DAFigureWidgetCommandCreate3DChart
//===============================================================

/**
 * @brief 构造函数，通过百分比坐标创建3D图表
 * @param fig 关联的绘图窗口
 * @param xPresent x坐标百分比
 * @param yPresent y坐标百分比
 * @param wPresent 宽度百分比
 * @param hPresent 高度百分比
 * @param par 父命令
 */
DAFigureWidgetCommandCreate3DChart::DAFigureWidgetCommandCreate3DChart(DAFigureWidget* fig,
                                                                       qreal xPresent,
                                                                       qreal yPresent,
                                                                       qreal wPresent,
                                                                       qreal hPresent,
                                                                       QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(nullptr)
    , mChartSize(xPresent, yPresent, wPresent, hPresent)
    , mNeedDelete(false)
{
    setText(QObject::tr("create 3D chart"));  // cn:创建3D绘图
}

/**
 * @brief 构造函数，通过归一化矩形创建3D图表
 * @param fig 关联的绘图窗口
 * @param versatileSize 归一化矩形
 * @param par 父命令
 */
DAFigureWidgetCommandCreate3DChart::DAFigureWidgetCommandCreate3DChart(DAFigureWidget* fig,
                                                                       const QRectF& versatileSize,
                                                                       QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(nullptr)
    , mChartSize(versatileSize)
    , mNeedDelete(false)
{
    setText(QObject::tr("create 3D chart"));  // cn:创建3D绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandCreate3DChart::~DAFigureWidgetCommandCreate3DChart()
{
    if (mNeedDelete) {
        if (mChart3D) {
            mChart3D->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：创建或重新添加3D图表
 *
 * 网格 figure 上首次创建登记到空闲单元格；undo 之后的 redo 按快照恢复原单元格。
 */
void DAFigureWidgetCommandCreate3DChart::redo()
{
    mNeedDelete = false;
    if (!figure()) {
        return;
    }
    if (mChart3D) {
        // 这是执行undo后执行redo会进入这个分支
        if (!restore3DChartAsGridMember(figure(), mChart3D, mGridCell, mChartSize)) {
            figure()->add3DChart(mChart3D, mChartSize);
        }
    } else {
        // 第一次执行redo会进入这里
        mChart3D = figure()->create3DChart(mChartSize);
        if (mChart3D && figure()->isGridLayoutEnabled()) {
            const DAFigureGridCellSnapshot cell = pickFreeGridCell(figure());
            if (applyGridCellIfPossible(figure(), mChart3D, cell)) {
                mGridCell = cell;
            }
        }
    }
}

/**
 * @brief 撤销操作：移除3D图表
 *
 * 移除前记录网格单元格，redo 时按该单元格恢复。
 */
void DAFigureWidgetCommandCreate3DChart::undo()
{
    mNeedDelete = true;
    if (!figure() || !mChart3D) {
        return;
    }
    mGridCell = captureGridCell(figure(), mChart3D);
    figure()->remove3DChart(mChart3D);
}

/**
 * @brief 获取创建的3D图表窗口
 * @return 3D图表窗口指针
 */
DAChart3DWidget* DAFigureWidgetCommandCreate3DChart::getChart3DWidget()
{
    return mChart3D;
}

//===============================================================
// DAFigureWidgetCommandRemoveChart
//===============================================================

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart 要移除的图表
 * @param par 父命令
 */
DAFigureWidgetCommandRemoveChart::DAFigureWidgetCommandRemoveChart(DAFigureWidget* fig, DAChartWidget* chart, QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart)
{
    setText(QObject::tr("remove chart"));  // cn:移除绘图
                                           // 先要获取尺寸
    mChartNormRect = fig->axesNormRect(chart);
    // 网格成员：同时记录单元格，undo 时按网格恢复而不是退回自由布局叠压在其他成员上
    mGridCell = captureGridCell(fig, chart);
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandRemoveChart::~DAFigureWidgetCommandRemoveChart()
{
    if (mNeedDelete) {
        if (mChart) {
            mChart->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：移除图表
 */
void DAFigureWidgetCommandRemoveChart::redo()
{
    mNeedDelete = true;
    if (mChart) {
        figure()->removeChart(mChart);
    }
}

/**
 * @brief 撤销操作：重新添加图表
 *
 * 网格成员按记录的单元格恢复；自由成员恢复原有归一化矩形（行为不变）。
 */
void DAFigureWidgetCommandRemoveChart::undo()
{
    mNeedDelete = false;
    if (!figure() || !mChart) {
        return;
    }
    // redo 已移除 layout item，必须先补回成员再按单元格登记
    if (!restoreChartAsGridMember(figure(), mChart, mGridCell, mChartNormRect)) {
        figure()->addChart(mChart, mChartNormRect);
    }
}

//===============================================================
// DAFigureWidgetCommandRemove3DChart
//===============================================================

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart3d 要移除的3D图表
 * @param par 父命令
 */
DAFigureWidgetCommandRemove3DChart::DAFigureWidgetCommandRemove3DChart(DAFigureWidget* fig,
                                                                       DAChart3DWidget* chart3d,
                                                                       QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(chart3d)
{
    setText(QObject::tr("remove 3D chart"));  // cn:移除3D绘图
    // 使用 widgetNormRect 获取归一化位置（3D chart 不是 QwtPlot，不能用 axesNormRect）
    mChartNormRect = fig->widgetNormRect(chart3d);
    // 网格成员：同时记录单元格，undo 时按网格恢复
    mGridCell = captureGridCell(fig, chart3d);
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandRemove3DChart::~DAFigureWidgetCommandRemove3DChart()
{
    if (mNeedDelete) {
        if (mChart3D) {
            mChart3D->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：移除3D图表
 */
void DAFigureWidgetCommandRemove3DChart::redo()
{
    mNeedDelete = true;
    if (mChart3D) {
        figure()->remove3DChart(mChart3D);
    }
}

/**
 * @brief 撤销操作：重新添加3D图表
 *
 * 网格成员按记录的单元格恢复；自由成员恢复原有归一化矩形（行为不变）。
 */
void DAFigureWidgetCommandRemove3DChart::undo()
{
    mNeedDelete = false;
    if (!figure() || !mChart3D) {
        return;
    }
    // redo 已移除成员，必须先补回成员再按单元格登记
    if (!restore3DChartAsGridMember(figure(), mChart3D, mGridCell, mChartNormRect)) {
        figure()->add3DChart(mChart3D, mChartNormRect);
    }
}

//----------------------------------------------------
// DAFigureWidgetCommandResizeWidget
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param w 要调整尺寸的窗口
 * @param oldNormRect 原归一化矩形
 * @param newNormRect 新归一化矩形
 * @param par 父命令
 */
DAFigureWidgetCommandResizeWidget::DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig,
                                                                     QWidget* w,
                                                                     const QRectF& oldNormRect,
                                                                     const QRectF& newNormRect,
                                                                     QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mWidget(w), mOldNormRect(oldNormRect), mNewNormRect(newNormRect)
{
    setText(QObject::tr("set figure widget size"));  // cn:设置绘图中窗体的尺寸
    // 网格成员：记录调整前后的单元格（按左上角换算，保留原跨度），redo/undo 走 setGridCell
    mOldGridCell = captureGridCell(fig, w);
    if (mOldGridCell.valid) {
        mNewGridCell = gridCellForNormRect(fig, newNormRect, mOldGridCell.rowSpan, mOldGridCell.colSpan);
    }
}

/**
 * @brief 重做操作：设置窗口新尺寸
 *
 * 网格成员走网格单元格（与轨道比例一致），自由成员保持原有归一化矩形行为。
 */
void DAFigureWidgetCommandResizeWidget::redo()
{
    if (!mWidget) {
        return;
    }
    if (!applyGridCellIfPossible(figure(), mWidget, mNewGridCell)) {
        //= 给qwt_figure增加可以添加任意窗口的方法
        figure()->setWidgetNormPos(mWidget, mNewNormRect);
    }
}

/**
 * @brief 撤销操作：恢复窗口原尺寸
 *
 * 网格成员恢复原单元格，自由成员恢复原归一化矩形。
 */
void DAFigureWidgetCommandResizeWidget::undo()
{
    if (!mWidget) {
        return;
    }
    if (!applyGridCellIfPossible(figure(), mWidget, mOldGridCell)) {
        figure()->setWidgetNormPos(mWidget, mOldNormRect);
    }
}

//----------------------------------------------------
// DAFigureWidgetCommandResizeGrid
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param oldRows 调整前的行比例
 * @param oldCols 调整前的列比例
 * @param newRows 调整后的行比例
 * @param newCols 调整后的列比例
 * @param par 父命令
 */
DAFigureWidgetCommandResizeGrid::DAFigureWidgetCommandResizeGrid(DAFigureWidget* fig,
                                                                 const QVector< qreal >& oldRows,
                                                                 const QVector< qreal >& oldCols,
                                                                 const QVector< qreal >& newRows,
                                                                 const QVector< qreal >& newCols,
                                                                 QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mOldRows(oldRows)
    , mOldCols(oldCols)
    , mNewRows(newRows)
    , mNewCols(newCols)
{
    setText(QObject::tr("resize grid tracks"));  // cn:调整网格比例
}

/**
 * @brief 重做操作：应用新的行/列比例
 *
 * 第一次redo由拖动本身完成，跳过以免重复设置。
 */
void DAFigureWidgetCommandResizeGrid::redo()
{
    if (mSkipFirstRedo) {
        mSkipFirstRedo = false;
        return;
    }
    if (!figureWidget) {
        return;
    }
    figureWidget->setGridSizes(QList< qreal >(mNewRows.cbegin(), mNewRows.cend()),
                               QList< qreal >(mNewCols.cbegin(), mNewCols.cend()));
}

/**
 * @brief 撤销操作：恢复原来的行/列比例
 */
void DAFigureWidgetCommandResizeGrid::undo()
{
    if (!figureWidget) {
        return;
    }
    figureWidget->setGridSizes(QList< qreal >(mOldRows.cbegin(), mOldRows.cend()),
                               QList< qreal >(mOldCols.cbegin(), mOldCols.cend()));
}

//----------------------------------------------------
// DAFigureWidgetCommandAttachItem
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart 目标图表
 * @param item 要添加的图元
 * @param skipFirst 是否跳过第一次redo
 * @param par 父命令
 */
DAFigureWidgetCommandAttachItem::DAFigureWidgetCommandAttachItem(DAFigureWidget* fig,
                                                                 DAChartWidget* chart,
                                                                 QwtPlotItem* item,
                                                                 bool skipFirst,
                                                                 QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart), mItem(item), mSkipFirst(skipFirst), mNeedDelete(false)
{
    setText(QObject::tr("add item in chart"));  // cn:添加图元到绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandAttachItem::~DAFigureWidgetCommandAttachItem()
{
    if (mNeedDelete) {
        if (mItem) {
            delete mItem;
        }
    }
}

/**
 * @brief 重做操作：将图元附加到图表
 */
void DAFigureWidgetCommandAttachItem::redo()
{
    if (!mChart || !mItem) {
        return;
    }
    if (mSkipFirst) {
        mSkipFirst = false;
    } else {
        mItem->attach(mChart);
        mNeedDelete = false;
    }
}

/**
 * @brief 撤销操作：将图元从图表分离
 */
void DAFigureWidgetCommandAttachItem::undo()
{
    if (!mItem) {
        return;
    }
    mItem->detach();
    mNeedDelete = true;
}

//----------------------------------------------------
// DAFigureWidgetCommandAttach3DItem
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart3d 目标3D图表
 * @param item 要添加的3D图元
 * @param skipFirst 是否跳过第一次redo
 * @param par 父命令
 */
DAFigureWidgetCommandAttach3DItem::DAFigureWidgetCommandAttach3DItem(DAFigureWidget* fig,
                                                                     DAChart3DWidget* chart3d,
                                                                     Qwt3DPlotItem* item,
                                                                     bool skipFirst,
                                                                     QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(chart3d)
    , mItem(item)
    , mSkipFirst(skipFirst)
    , mNeedDelete(false)
{
    setText(QObject::tr("add 3D item in chart"));  // cn:添加3D图元到绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandAttach3DItem::~DAFigureWidgetCommandAttach3DItem()
{
    if (mNeedDelete) {
        if (mItem) {
            delete mItem;
        }
    }
}

/**
 * @brief 重做操作：将3D图元附加到3D图表
 */
void DAFigureWidgetCommandAttach3DItem::redo()
{
    if (!mChart3D || !mItem) {
        return;
    }
    if (mSkipFirst) {
        mSkipFirst = false;
    } else {
        mItem->attach(mChart3D);
        mNeedDelete = false;
        // 触发 GL 重绘（Qwt3DPlot 继承 QOpenGLWidget，update() 触发 paintGL）
        mChart3D->update();
    }
}

/**
 * @brief 撤销操作：将3D图元从3D图表分离
 */
void DAFigureWidgetCommandAttach3DItem::undo()
{
    if (!mItem) {
        return;
    }
    mItem->detach();
    mNeedDelete = true;
    if (mChart3D) {
        mChart3D->update();
    }
}

//----------------------------------------------------
// DAFigureWidgetCommandMoveItem
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param sourceChart 源图表
 * @param targetChart 目标图表
 * @param item 要移动的图元
 * @param par 父命令
 */
DAFigureWidgetCommandMoveItem::DAFigureWidgetCommandMoveItem(DAFigureWidget* fig,
                                                             DAChartWidget* sourceChart,
                                                             DAChartWidget* targetChart,
                                                             QwtPlotItem* item,
                                                             QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mSourceChart(sourceChart), mTargetChart(targetChart), mItem(item)
{
    setText(QObject::tr("move plot item to another chart"));  // cn:移动图元到另一个绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandMoveItem::~DAFigureWidgetCommandMoveItem()
{
    // item始终attach在某个plot上，无需在此管理生命周期
}

/**
 * @brief 重做操作：将图元移动到目标图表
 */
void DAFigureWidgetCommandMoveItem::redo()
{
    if (!mTargetChart || !mItem) {
        return;
    }
    mItem->attach(mTargetChart);
    if (mSourceChart) {
        DAChartUtil::replot(mSourceChart);
    }
    DAChartUtil::replot(mTargetChart);
}

/**
 * @brief 撤销操作：将图元移回源图表
 */
void DAFigureWidgetCommandMoveItem::undo()
{
    if (!mSourceChart || !mItem) {
        return;
    }
    mItem->attach(mSourceChart);
    if (mTargetChart) {
        DAChartUtil::replot(mTargetChart);
    }
    DAChartUtil::replot(mSourceChart);
}

//----------------------------------------------------
// DAFigureWidgetCommandMove3DItem
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param sourceChart3D 源3D图表
 * @param targetChart3D 目标3D图表
 * @param item 要移动的3D图元
 * @param par 父命令
 */
DAFigureWidgetCommandMove3DItem::DAFigureWidgetCommandMove3DItem(DAFigureWidget* fig,
                                                                  DAChart3DWidget* sourceChart3D,
                                                                  DAChart3DWidget* targetChart3D,
                                                                  Qwt3DPlotItem* item,
                                                                  QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mSourceChart3D(sourceChart3D)
    , mTargetChart3D(targetChart3D)
    , mItem(item)
{
    setText(QObject::tr("move 3D plot item to another chart"));  // cn:移动3D图元到另一个绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandMove3DItem::~DAFigureWidgetCommandMove3DItem()
{
    // item始终attach在某个plot上，无需在此管理生命周期
}

/**
 * @brief 重做操作：将3D图元移动到目标3D图表
 */
void DAFigureWidgetCommandMove3DItem::redo()
{
    if (!mTargetChart3D || !mItem) {
        return;
    }
    mItem->attach(mTargetChart3D);
    if (mSourceChart3D) {
        mSourceChart3D->update();
    }
    mTargetChart3D->update();
}

/**
 * @brief 撤销操作：将3D图元移回源3D图表
 */
void DAFigureWidgetCommandMove3DItem::undo()
{
    if (!mSourceChart3D || !mItem) {
        return;
    }
    mItem->attach(mSourceChart3D);
    if (mTargetChart3D) {
        mTargetChart3D->update();
    }
    mSourceChart3D->update();
}

//----------------------------------------------------
// DAFigureWidgetCommandDetachItem
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart 图元所在的图表
 * @param item 要删除的图元
 * @param par 父命令
 */
DAFigureWidgetCommandDetachItem::DAFigureWidgetCommandDetachItem(DAFigureWidget* fig,
                                                                 DAChartWidget* chart,
                                                                 QwtPlotItem* item,
                                                                 QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart), mItem(item), mNeedDelete(false)
{
    setText(QObject::tr("remove item from chart"));  // cn:删除绘图中的图元
}

/**
 * @brief 析构函数
 *
 * item 处于 detach 态（redo 后）时由本命令负责销毁
 */
DAFigureWidgetCommandDetachItem::~DAFigureWidgetCommandDetachItem()
{
    if (mNeedDelete && mItem) {
        delete mItem;
        mItem = nullptr;
    }
}

/**
 * @brief 重做操作：将图元从图表分离
 */
void DAFigureWidgetCommandDetachItem::redo()
{
    if (!mChart || !mItem) {
        return;
    }
    mItem->detach();
    mNeedDelete = true;
    if (mChart) {
        mChart->replot();
    }
}

/**
 * @brief 撤销操作：将图元重新附加到图表
 */
void DAFigureWidgetCommandDetachItem::undo()
{
    if (!mChart || !mItem) {
        return;
    }
    mItem->attach(mChart.data());
    mNeedDelete = false;
    if (mChart) {
        mChart->replot();
    }
}

//----------------------------------------------------
// DAFigureWidgetCommandMovePlotItemPosition
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart 图元所在的图表
 * @param item 要移动的图元
 * @param oldGeo 移动前几何快照
 * @param newGeo 移动后几何快照
 * @param par 父命令
 */
DAFigureWidgetCommandMovePlotItemPosition::DAFigureWidgetCommandMovePlotItemPosition(
    DAFigureWidget* fig,
    DAChartWidget* chart,
    QwtPlotItem* item,
    const DAChartElementHitTester::ItemGeometry& oldGeo,
    const DAChartElementHitTester::ItemGeometry& newGeo,
    QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart), mItem(item), mOldGeo(oldGeo), mNewGeo(newGeo)
{
    setText(QObject::tr("move plot item position"));  // cn:移动图元位置
}

/**
 * @brief 重做操作：恢复到移动后几何
 */
void DAFigureWidgetCommandMovePlotItemPosition::redo()
{
    if (!mChart || !mItem) {
        return;
    }
    DAChartElementHitTester::setItemGeometry(mItem, mNewGeo);
    mChart->replot();
}

/**
 * @brief 撤销操作：恢复到移动前几何
 */
void DAFigureWidgetCommandMovePlotItemPosition::undo()
{
    if (!mChart || !mItem) {
        return;
    }
    DAChartElementHitTester::setItemGeometry(mItem, mOldGeo);
    mChart->replot();
}

}  // namespace DA
