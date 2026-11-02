#ifndef DATABLECOLUMNOPS_H
#define DATABLECOLUMNOPS_H
#include "TableOps/DATableDataOperation.h"
#include <QList>
#include <QObject>
#include <QString>

class QDomDocument;

namespace DA
{
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;
class DATableViewStateController;

/**
 * @brief 列增删算子（008 期4：插列/删列）
 *
 * insertColumnAt：插列对话框（DADialogInsertNewColumn，范围/默认值两模式）；
 * removeColumns：删列（视图状态整店 XML 快照 undo 还原、样式/条件格式/批注
 * 位置键重映射、计算列依赖失效提示）。后效走 Structure commit（关浮窗 +
 * 宿主树刷新）；删列后视图状态重投影经视图状态控制器。
 * 生命周期：宿主页面构造并持有（QObject 父子持有插列对话框）。
 */
class DAGUI_API DATableColumnOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    DATableColumnOps(const DATablePageContext& ctx,
                     const DATableSelectionService& sel,
                     DATableMutationCoordinator* mutation,
                     DATableViewStateController* viewState,
                     QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

public:
    // 在 col 位置插入新列（插列对话框；非 dataframe 数据静默忽略）
    void insertColumnAt(int col);
    // 选中列右边/左边插入新列（选区解析目标列）
    void insertColumnRightBySelect();
    void insertColumnLeftBySelect();
    // 移除选中的列（选区解析列号）
    int removeSelectColumn();
    // 移除指定列（表头右键「删除列」直接入口；不依赖选区）
    int removeColumnAt(int col);
    // 按列号列表删除列（removeSelectColumn/removeColumnAt 共用命令管线）
    int removeColumns(const QList< int >& columns);

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableMutationCoordinator* mMutation { nullptr };          ///< 编辑后效协调器（宿主持有，非拥有）
    DATableViewStateController* mViewState { nullptr };        ///< 视图状态控制器（删列后重投影）
};
}  // end of namespace DA
#endif  // DATABLECOLUMNOPS_H
