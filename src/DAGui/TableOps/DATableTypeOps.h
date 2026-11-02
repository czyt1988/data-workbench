#ifndef DATABLETYPEOPS_H
#define DATABLETYPEOPS_H
#include "TableOps/DATableDataOperation.h"
#include "TableCore/DATableMutationCoordinator.h"
#include <QList>
#include <QObject>
#include <QString>

namespace pybind11
{
class dict;
}

namespace DA
{
class DADialogDataframeColumnCastToDatetime;
class DADialogDataframeColumnCastToNumeric;
class DAPyDType;
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;

/**
 * @brief 列类型转换算子（008 期4：astype/数值/日期/转索引）
 *
 * changeSelectColumnType：astype 直接转换；castSelectToNum/castSelectToDatetime：
 * 转换对话框（懒建复用）；changeSelectColumnToIndex：列转索引（FullReset 后效）。
 * 类型变化经 selectTypeChanged 信号上报（页面转发给自己的同名信号，APP 同步 ribbon）。
 * 后效：astype/cast 走 Values commit（整列通知 + 受影响列统计失效 + 计算列标脏）；
 * setIndex 走 FullReset commit。
 * 生命周期：宿主页面构造并持有（QObject 父子持有两个转换对话框）。
 */
class DAGUI_API DATableTypeOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    DATableTypeOps(const DATablePageContext& ctx,
                   const DATableSelectionService& sel,
                   DATableMutationCoordinator* mutation,
                   QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

Q_SIGNALS:
    // 类型已变化（成功）/类型已不可用（失败空列表）：页面转发给自己的 selectTypeChanged
    void selectTypeChanged(const QList< int >& columns, const DA::DAPyDType& dtype);

public:
    // 设置选择列的数据类型（astype）；成功返回 true
    bool changeSelectColumnType(const DA::DAPyDType& dt);
    // 把选择的列转换为数值（对话框参数）
    void castSelectToNum();
    // 把选择的列转换为日期（对话框参数）
    void castSelectToDatetime();
    // 把选择的列转换为索引（结构性整表操作）；成功返回 true
    bool changeSelectColumnToIndex();

private:
    // 整列值变更的公共后效构造（Values commit + 整列通知形状）
    DATableMutation columnValuesMutation(const QList< int >& cols) const;

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableMutationCoordinator* mMutation { nullptr };  ///< 编辑后效协调器（宿主持有，非拥有）
    DADialogDataframeColumnCastToNumeric* mDialogCastNumArgs { nullptr };    ///< 数值转换对话框（懒建复用）
    DADialogDataframeColumnCastToDatetime* mDialogCastDatetimeArgs { nullptr };  ///< 日期转换对话框（懒建复用）
};
}  // end of namespace DA
#endif  // DATABLETYPEOPS_H
