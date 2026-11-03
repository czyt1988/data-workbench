#ifndef DATABLEVALIDATIONCONTROLLER_H
#define DATABLEVALIDATIONCONTROLLER_H
#include "DAGuiAPI.h"
#include "DATableValidationRule.h"
#include <QList>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QVector>
#include <functional>

class QMessageBox;

namespace DA
{
class DACommandDataFrame_renameColumns;
class DADialogValidationReport;
class DATablePageContext;

/**
 * @brief 单条规则的校验结果（求值输出，报告对话框消费）
 *
 * 原 DADataOperateOfDataFrameWidget::ValidationRuleOutcome（public API），
 * Facade 以 using 别名保留旧名（008 期3）。
 */
struct DATableValidationOutcome
{
    DATableValidationRule rule;   ///< 规则定义
    int columnPosition { -1 };    ///< 解析出的列位置（-1 = 列不存在）
    bool columnMissing { false }; ///< 规则引用的列不存在（跳过求值）
    QVector< int > rows;          ///< 违规行（数据源绝对行号，升序）
};

/**
 * @brief 数据校验控制器（008 期3）
 *
 * 规则编辑对话框、求值（只读，红线2：不改数据、不入撤销栈）、报告窗、违规高亮、
 * 改名迁移。构造注入上下文（校验子存储/高亮层/数据/撤销栈/定位桥），不持窗口指针；
 * 求值经 DAPyScripts::validateDataFrame 一次下发（空值判定/比较全在 pandas，红线1）。
 * 生命周期：宿主页面构造并持有（QObject 父子持有报告窗）。
 */
class DAGUI_API DATableValidationController : public QObject
{
    Q_OBJECT
public:
    explicit DATableValidationController(const DATablePageContext& ctx, QObject* parent = nullptr);

public:
    // 规则显示文本（报告与列表共用）：列（第 n 个）· 类型 参数 — 名称
    QString validationRuleDisplayText(const DATableValidationRule& rule) const;
    // 打开规则编辑对话框；确定后按差异生成 setRules 命令入页撤销栈（D4）
    void showRulesDialog();
    // 执行校验求值（只读）：求值全部启用规则、违规装载 Source::Validation 高亮
    //（Error/Warning 两级）；elapsedMs 非空时输出耗时。唯一性缓存命中不重算（§4.9）
    QList< DATableValidationOutcome > evaluateRules(int* elapsedMs = nullptr);
    // 执行校验并弹出报告对话框（非模态，点击样例行定位）
    void runAndShowReport();
    // 清除违规高亮（只清 Source::Validation，不动查找命中——来源隔离）
    void clearHighlights();
    // 列改名确认：提示「有 N 条校验规则引用它」，返回是否继续（无引用直接 true）
    bool confirmRuleRename(const QList< QPair< QString, QString > >& renames) const;
    // 把规则引用迁移挂进改名命令的 directional 回调（与改名同一撤销单元，R20）
    void wireRuleRenameMigration(DACommandDataFrame_renameColumns* cmd,
                                 const QList< QPair< QString, QString > >& renames) const;

private:
    const DATablePageContext& mCtx;  ///< 依赖注入的上下文（构造注入，非拥有）
    QPointer< DADialogValidationReport > mValidationReport;  ///< 非模态校验报告窗（重跑前先关旧的）
};
}  // end of namespace DA
#endif  // DATABLEVALIDATIONCONTROLLER_H
