#ifndef DATABLESELECTIONSERVICE_H
#define DATABLESELECTIONSERVICE_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include <QHash>
#include <QItemSelection>
#include <QList>
#include <QPair>
#include <QPoint>
#include <QString>
#include <QVector>

class QItemSelectionModel;
class QModelIndex;

namespace DA
{
class DATablePageContext;

/**
 * @brief 表格页选区/坐标查询服务（008 期1）
 *
 * 收口全部「选区 → 数据源绝对坐标」的换算口径：视图行号是滑动窗内逻辑行，
 * 凡是拿行号去读写数据的地方一律经 DATablePageContext::toActualRow 换算成
 * 数据源绝对行号。全部方法都是只读查询：不发信号、不弹窗、不写存储、不入撤销栈。
 * 生命周期：由宿主页面构造并持有；构造注入 DATablePageContext，不持有窗口。
 */
class DAGUI_API DATableSelectionService
{
public:
    explicit DATableSelectionService(const DATablePageContext& ctx);

public:
    // 表格是否有选中项（主窗格选择模型；冻结窗格共享同一个）
    bool haveSelection() const;
    // 选中的数据列（去重；ensureInDataframe=true 时限定在数据列范围内）
    QList< int > selectedDataframeColumns(bool ensureInDataframe = true) const;
    // 选中的数据行（数据源绝对行号，去重）
    QList< int > selectedDataframeRows(bool ensureInDataframe = true) const;
    // 完全选中的整列（逐列行区间合并判定，不物化 selectedIndexes）
    QList< int > fullySelectedDataframeColumns(bool ensureInDataframe = true) const;
    // 完全选中的整行（数据源绝对行号）
    QList< int > fullySelectedDataframeRows(bool ensureInDataframe = true) const;
    // 选区首个数据行（数据源绝对行号；无选区返回 -1）
    int oneSelectedDataframeRow(bool ensureInDataframe = true) const;
    // 选区首个数据列（无选区返回 -1）
    int oneSelectedDataframeColumn(bool ensureInDataframe = true) const;
    // 选中的单元格（QPoint(数据源绝对行号, 列号)）
    QList< QPoint > selectedDataframeCells(bool ensureInDataframe = true) const;
    // 选中列转 DAData 序列列表（单列返回该列 Series，多列逐列返回）
    QList< DAData > selectedSeries() const;
    // 实际数据列的身份表（(列名, occurrence)，数据列序升序，空名列跳过）
    QList< QPair< QString, int > > actualColumnIdentities() const;
    // 选择模型（只取主窗格；冻结窗格共享同一个 selectionModel）
    QItemSelectionModel* selectionModel() const;
    // 选区段数（0=无选区，>1=多段）
    int selectionSegmentCount() const;
    // 选区包围盒（闭区间；行号为数据源绝对行号；无选区返回 false）
    bool selectionBoundingBox(int* rowBegin, int* rowEnd, int* colBegin, int* colEnd) const;
    // 选中单元数（几何去重；按选区区间累加宽×高，不物化索引）
    int selectedCellCount() const;
    // 当前单元格（数据源绝对坐标；越出数据范围返回 false）
    bool currentActualCell(int* actualRow, int* actualCol) const;
    // 当前单元格模型索引（currentIndex 无效时取选区左上角；都无返回无效索引）
    QModelIndex currentOrSelectionTopLeftIndex() const;
    // 当前选区为单格时取其显示文本（Qt::DisplayRole）；否则空串
    QString currentSingleCellDisplayText() const;
    // 有序选区：按视图顺序返回选区涉及的数据列（隐藏列按视图状态跳过）
    QList< int > selectedDataColumnsInViewOrder(bool ensureInDataframe = true) const;
    // 按选中列复制/导出的列范围（无选中回退 fallbackColumn，仍无效返回空表）
    QList< int > selectedColumnsForDataAction(int fallbackColumn) const;
    // 选区按列分组的连续行区间：col -> 若干 [rowStart,rowEnd] 闭区间（绝对行号升序）
    QHash< int, QVector< QPair< int, int > > > selectedColumnRowRuns() const;
    // 选区是否与行级样式相交（稀疏键表判定，不展开选区；windowStart 为当前窗口起始行）
    bool selectionIntersectsRowStyles(const QItemSelection& sel, int windowStart) const;
    // 选区是否与单元格级样式相交（同上）
    bool selectionIntersectsCellStyles(const QItemSelection& sel, int windowStart) const;

private:
    const DATablePageContext& mCtx;  ///< 依赖注入的上下文（构造注入，非拥有）
};
}  // end of namespace DA
#endif  // DATABLESELECTIONSERVICE_H
