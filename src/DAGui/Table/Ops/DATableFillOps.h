#ifndef DATABLEFILLOPS_H
#define DATABLEFILLOPS_H
#include "Table/Ops/DATableDataOperation.h"
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

namespace DA
{
class DAPyDataFrame;
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;

/**
 * @brief 填充算子（008 期4：选区填充/序列填充）
 *
 * FillMode 经 Facade 别名保持旧 public API 兼容。选区填充按列独立
 * （连续行区间 run 内传递，Python 算子 da_fill_selection 求值）；
 * 序列填充经对话框收集参数（da_fill_series，预览与写入同一算子）。
 * 全部变化清单汇总成一条 paste 命令入栈（一次填充 = 一次撤销，D4）。
 * 后效走 Values commit。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableFillOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    /**
     * @brief 填充模式（Facade 以 using 别名保持旧 public API）
     */
    enum class FillMode
    {
        Down,      ///< 向下填充（每列选区首行值填满其余行，Ctrl+D）
        Up,        ///< 向上填充（每列选区末行值填满其余行）
        Forward,   ///< 选区内向下传递最后一个非空值（ffill）
        Backward   ///< 选区内向上传递下一个非空值（bfill）
    };

    DATableFillOps(const DATablePageContext& ctx,
                   const DATableSelectionService& sel,
                   DATableMutationCoordinator* mutation,
                   QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

public:
    // 选区填充（向下/向上/ffill/bfill，按列独立 run；一次填充 = 一次撤销）
    int fillSelection(FillMode mode);
    // 打开序列填充对话框（数值等差/日期步进）；接受后对选区每列各生成一个序列
    int showFillSeriesDialog();

private:
    // 用填充清单构造一条 paste 命令并入栈（值记录，D4；一次填充 = 一步撤销）
    int pushFillPasteCommand(const DAPyDataFrame& df,
                             const QList< int >& rows,
                             const QList< int >& cols,
                             const QList< QVariant >& values,
                             const QString& text);

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableMutationCoordinator* mMutation { nullptr };  ///< 编辑后效协调器（宿主持有，非拥有）
};
}  // end of namespace DA
#endif  // DATABLEFILLOPS_H
