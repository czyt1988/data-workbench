#include "Table/Controllers/DATableValidationController.h"
#include "Table/Core/DATablePageContext.h"
// qt
#include <QDialog>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QWidget>
// DAGui
#include "Table/Views/DADataTableView.h"
#include "Table/Ops/DACommandsDataFrame.h"
#include "Table/Commands/DACommandsTableValidation.h"
#include "DACallBackInterface.h"
#include "Table/Views/DATableHighlightOverlay.h"
#include "Table/Metadata/DATableValidationStore.h"
#include "Table/Dialogs/DADialogValidationRules.h"
#include "Table/Dialogs/DADialogValidationReport.h"
// DAData
#include "DADataPyDataFrame.h"
// python 算子封装（校验求值）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

namespace DA
{
//===================================================
// DATableValidationController
//===================================================

DATableValidationController::DATableValidationController(const DATablePageContext& ctx, QObject* parent)
    : QObject(parent), mCtx(ctx)
{
}

/**
 * @brief 规则显示文本（报告与列表共用）：列（第 n 个）· 类型 参数 — 名称
 * @param rule 规则
 * @return 已翻译的显示文本
 */
QString DATableValidationController::validationRuleDisplayText(const DATableValidationRule& rule) const
{
    QString typeName;
    switch (rule.type) {
    case DATableValidationRule::Type::NumericRange:
        typeName = tr("Numeric range");  // cn:数值区间
        break;
    case DATableValidationRule::Type::DateRange:
        typeName = tr("Date range");  // cn:日期区间
        break;
    case DATableValidationRule::Type::EnumSet:
        typeName = tr("Allowed values (enum)");  // cn:枚举集合
        break;
    case DATableValidationRule::Type::NotEmpty:
        typeName = tr("Not empty");  // cn:非空
        break;
    case DATableValidationRule::Type::Unique:
        typeName = tr("Unique");  // cn:唯一
        break;
    case DATableValidationRule::Type::Regex:
        typeName = tr("Regex (full match)");  // cn:正则（整格匹配）
        break;
    }
    QString colText = rule.columnName;
    if (rule.columnOccurrence > 0) {
        colText = tr("%1 (occurrence %2)").arg(rule.columnName).arg(rule.columnOccurrence + 1);  // cn:%1（第 %2 个同名列）
    }
    QString text = QStringLiteral("%1 · %2").arg(colText, typeName);
    const QString params = rule.paramSummary();
    if (!params.isEmpty()) {
        text += QLatin1Char(' ') + params;
    }
    if (!rule.label.isEmpty()) {
        text += QStringLiteral(" — ") + rule.label;
    }
    return text;
}

/**
 * @brief 打开校验规则编辑对话框（005 T1.25）
 *
 * 一窗两栏编辑工作规则集；确定后与 store 现值对比，有差异才生成一条
 * DACommandValidationRule_setRules 入页撤销栈（旧新对象记录，D4；无差异不推空命令）。
 * 规则变化后既有违规高亮可能与新规则不一致 → 清 Validation 来源高亮（重跑由用户触发）。
 */
void DATableValidationController::showRulesDialog()
{
    DAPyDataFrame df = mCtx.df();
    DATableValidationStore* vaStore = mCtx.validationStore();
    if (df.isNone() || !vaStore) {
        return;
    }
    DADialogValidationRules dlg(df.columns(), vaStore->rules(), mCtx.hostWidget());
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    const QList< DATableValidationRule > oldRules = vaStore->rules();
    const QList< DATableValidationRule > newRules = dlg.resultRules();
    if (oldRules == newRules) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandValidationRule_setRules(vaStore, oldRules, newRules));
    }
    clearHighlights();
}

/**
 * @brief 执行校验求值（只读，红线2：不改数据、不入撤销栈）
 *
 * 全部启用规则一次下发 da_validate_dataframe（空值判定/比较全在 pandas，红线1）；
 * 唯一性规则命中 store 缓存则不重算（§4.9，打点 uniquePythonEvals 可测）。列名 +
 * 第 n 个同名列解析列位置（R20），列缺失的规则标记 columnMissing 跳过。结果装载
 * 瞬态高亮层 Source::Validation（Error→ViolationError / Warning→ViolationWarning，
 * 来源隔离：不动查找命中）。
 * @param elapsedMs 非空时输出求值耗时（毫秒）
 * @return 逐规则结果（含列缺失标记与违规行清单）
 */
QList< DATableValidationOutcome > DATableValidationController::evaluateRules(int* elapsedMs)
{
    QList< DATableValidationOutcome > outcomes;
    if (elapsedMs) {
        *elapsedMs = 0;
    }
    DAPyDataFrame df = mCtx.df();
    DATableValidationStore* vaStore = mCtx.validationStore();
    if (df.isNone() || !vaStore) {
        return outcomes;
    }
    const QList< int > enabledIdx = vaStore->enabledRuleIndexes();
    if (enabledIdx.isEmpty()) {
        return outcomes;
    }
    const QList< DATableValidationRule > all = vaStore->rules();
    const QList< QString > colNames          = df.columns();
    // 列名 + 第 n 个同名列 → 列位置（R20 重名消歧，与算子同一解析语义）
    auto resolvePos = [ &colNames ](const QString& name, int occurrence) -> int {
        int seen = 0;
        for (int i = 0; i < colNames.size(); ++i) {
            if (colNames[ i ] == name) {
                if (seen == occurrence) {
                    return i;
                }
                ++seen;
            }
        }
        return -1;
    };
    QList< DAPyScriptsDataFrame::ValidationRuleSpec > specs;
    QElapsedTimer timer;
    timer.start();
    for (int ruleIdx : std::as_const(enabledIdx)) {
        const DATableValidationRule r = all[ ruleIdx ];
        DATableValidationOutcome oc;
        oc.rule           = r;
        oc.columnPosition = resolvePos(r.columnName, r.columnOccurrence);
        oc.columnMissing  = oc.columnPosition < 0;
        const int outcomePos = outcomes.size();
        outcomes.append(oc);
        if (oc.columnMissing) {
            continue;
        }
        // 唯一性规则命中缓存 → 不重算（§4.9：O(n) 的 pandas 操作按列缓存重复集合）
        if (r.type == DATableValidationRule::Type::Unique && vaStore->uniqueCacheValid(r)) {
            outcomes[ outcomePos ].rows = vaStore->uniqueCacheRows(r);
            continue;
        }
        if (r.type == DATableValidationRule::Type::Unique) {
            vaStore->countUniquePythonEval();
        }
        DAPyScriptsDataFrame::ValidationRuleSpec sp;
        sp.index      = outcomePos;
        sp.column     = r.columnName;
        sp.occurrence = r.columnOccurrence;
        switch (r.type) {
        case DATableValidationRule::Type::NumericRange:
            sp.type       = QStringLiteral("numeric-range");
            sp.numericMin = r.numericMin;
            sp.numericMax = r.numericMax;
            sp.hasMin     = r.hasNumericMin;
            sp.hasMax     = r.hasNumericMax;
            break;
        case DATableValidationRule::Type::DateRange:
            sp.type    = QStringLiteral("date-range");
            sp.dateMin = r.dateMin.isValid() ? r.dateMin.toString(Qt::ISODateWithMs) : QString();
            sp.dateMax = r.dateMax.isValid() ? r.dateMax.toString(Qt::ISODateWithMs) : QString();
            break;
        case DATableValidationRule::Type::EnumSet:
            sp.type       = QStringLiteral("enum");
            sp.enumValues = r.enumValues;
            break;
        case DATableValidationRule::Type::NotEmpty:
            sp.type = QStringLiteral("not-empty");
            break;
        case DATableValidationRule::Type::Unique:
            sp.type = QStringLiteral("unique");
            break;
        case DATableValidationRule::Type::Regex:
            sp.type         = QStringLiteral("regex");
            sp.regexPattern = r.regexPattern;
            break;
        }
        specs.append(sp);
    }
    QList< DAPyScriptsDataFrame::ValidationRuleResult > results;
    QString err;
    if (!DAPyScripts::getDataFrame().validateDataFrame(df, specs, &results, &err)) {
        daWarning << tr("Validation failed: %1").arg(err);  // cn:数据校验失败：%1
        return QList< DATableValidationOutcome >();
    }
    for (const DAPyScriptsDataFrame::ValidationRuleResult& res : std::as_const(results)) {
        if (res.index >= 0 && res.index < outcomes.size()) {
            outcomes[ res.index ].rows = res.rows;
            // 唯一性结果写缓存（数据变更后由失效挂点清空）
            if (outcomes[ res.index ].rule.type == DATableValidationRule::Type::Unique) {
                vaStore->setUniqueCache(outcomes[ res.index ].rule, res.rows);
            }
        }
    }
    if (elapsedMs) {
        *elapsedMs = static_cast< int >(timer.elapsed());
    }
    // 违规装载瞬态高亮层（Source::Validation；来源隔离：不碰 Find 的集合，红线2）
    QVector< QPair< int, int > > errCells, warnCells;
    for (const DATableValidationOutcome& oc : std::as_const(outcomes)) {
        if (oc.columnMissing) {
            continue;
        }
        QVector< QPair< int, int > >& target =
            (oc.rule.level == DATableValidationRule::Level::Error) ? errCells : warnCells;
        target.reserve(target.size() + oc.rows.size());
        for (int row : oc.rows) {
            target.append(qMakePair(row, oc.columnPosition));
        }
    }
    if (DATableHighlightOverlay* overlay = mCtx.highlightOverlay()) {
        overlay->set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, errCells);
        overlay->set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, warnCells);
    }
    return outcomes;
}

/**
 * @brief 执行校验并弹出报告对话框（非模态，点击样例行定位）
 *
 * 无启用规则时提示并直接打开规则编辑对话框；报告样例行上限 50 条/规则（完整清单
 * 走高亮与导出）。定位走上页注入的定位桥（007 的定位单入口语义）。
 */
void DATableValidationController::runAndShowReport()
{
    DAPyDataFrame df = mCtx.df();
    DATableValidationStore* vaStore = mCtx.validationStore();
    if (df.isNone() || !vaStore) {
        return;
    }
    if (vaStore->enabledRuleIndexes().isEmpty()) {
        daInfo << tr("No enabled validation rules yet — add rules first");  // cn:暂无启用的校验规则——请先添加规则
        showRulesDialog();
        return;
    }
    int elapsedMs = 0;
    const QList< DATableValidationOutcome > outcomes = evaluateRules(&elapsedMs);
    if (outcomes.isEmpty()) {
        return;  // 求值失败（已提示）
    }
    QList< DADialogValidationReport::Entry > entries;
    entries.reserve(outcomes.size());
    const int c_sampleLimit = 50;
    for (const DATableValidationOutcome& oc : std::as_const(outcomes)) {
        DADialogValidationReport::Entry e;
        e.ruleText       = validationRuleDisplayText(oc.rule);
        e.isError        = oc.rule.level == DATableValidationRule::Level::Error;
        e.columnPosition = oc.columnPosition;
        e.columnMissing  = oc.columnMissing;
        e.rows           = oc.rows;
        const int sampleN = qMin(oc.rows.size(), c_sampleLimit);
        for (int i = 0; i < sampleN; ++i) {
            const int row = oc.rows[ i ];
            QString valTxt;
            if (!oc.columnMissing && oc.columnPosition >= 0) {
                valTxt = df.iat(static_cast< std::size_t >(row), static_cast< std::size_t >(oc.columnPosition)).toString();
            }
            e.rowTexts.append(tr("Row %1: %2").arg(row + 1).arg(valTxt));  // cn:行 %1：%2
        }
        entries.append(e);
    }
    // 已有报告窗口先关（避免多次校验叠加多个非模态窗）
    if (mValidationReport) {
        mValidationReport->close();
    }
    DADialogValidationReport* dlg = new DADialogValidationReport(mCtx.hostWidget());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    mValidationReport = dlg;
    dlg->setResults(entries, vaStore->enabledRuleIndexes().size(), elapsedMs);
    connect(dlg, &DADialogValidationReport::locateCellRequested, this, [ this ](int row, int col) {
        // 007：报告样例行定位走定位桥（隐藏列自动取消隐藏 + 冻结区处置；选中命中格）
        if (col >= 0 && mCtx.locateActualCell) {
            mCtx.locateActualCell(row, col, true);
        }
    });
    connect(dlg, &DADialogValidationReport::clearHighlightsRequested, this, &DATableValidationController::clearHighlights);
    dlg->show();
}

/**
 * @brief 清除违规高亮（只清 Source::Validation，查找命中不受影响——来源隔离，红线2）
 */
void DATableValidationController::clearHighlights()
{
    if (DATableHighlightOverlay* overlay = mCtx.highlightOverlay()) {
        overlay->clear(DATableHighlightOverlay::Source::Validation);
    }
}

/**
 * @brief 列改名确认：提示「有 N 条校验规则引用它」，返回是否继续（无引用直接 true）
 * @param renames 改名对（old→new，仅变化的）
 * @return 继续返回 true
 */
bool DATableValidationController::confirmRuleRename(const QList< QPair< QString, QString > >& renames) const
{
    DATableValidationStore* vaStore = mCtx.validationStore();
    if (!vaStore || vaStore->isEmpty() || renames.isEmpty()) {
        return true;
    }
    int total = 0;
    QStringList cols;
    for (const auto& p : std::as_const(renames)) {
        const int n = vaStore->rulesReferencing(p.first);
        if (n > 0) {
            total += n;
            cols.append(p.first);
        }
    }
    if (total == 0) {
        return true;
    }
    const auto answer = QMessageBox::question(
        mCtx.hostWidget(),
        tr("Rename Column"),  // cn:重命名列
        tr("%1 validation rule(s) reference the renamed column(s): %2. Their references will be updated automatically. Continue?")
            .arg(total)
            .arg(cols.join(QStringLiteral(", "))),  // cn:有 %1 条校验规则引用被改名的列：%2。其引用将自动批量更新，是否继续？
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    return answer == QMessageBox::Yes;
}

/**
 * @brief 把校验规则引用迁移挂进改名命令的 directional 回调（与改名同一撤销单元，R20）
 *
 * 链式包裹既有回调（计算列迁移先挂）：setDirectionalCallBack 是覆盖语义，
 * 直接再设会丢掉计算列迁移，故取旧回调包进新回调。
 * @param cmd 改名命令（exec 前接线）
 * @param renames 改名对（old→new）
 */
void DATableValidationController::wireRuleRenameMigration(DACommandDataFrame_renameColumns* cmd,
                                                         const QList< QPair< QString, QString > >& renames) const
{
    DATableValidationStore* vaStore = mCtx.validationStore();
    if (!cmd || !vaStore || renames.isEmpty()) {
        return;
    }
    DACallBackInterface::DirectionalCallBack prevDirectional = cmd->getDirectionalCallBack();
    cmd->setDirectionalCallBack([ prevDirectional, vaStore, renames ](bool isUndo) {
        if (prevDirectional) {
            prevDirectional(isUndo);
        }
        for (const auto& p : std::as_const(renames)) {
            if (isUndo) {
                vaStore->renameColumnReferences(p.second, p.first);
            } else {
                vaStore->renameColumnReferences(p.first, p.second);
            }
        }
    });
}
}  // end of namespace DA
