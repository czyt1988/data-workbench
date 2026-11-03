#include "Table/Controllers/DATableConditionalFormatController.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Core/DATableSelectionService.h"
// stl
#include <algorithm>
// qt
#include <QAction>
#include <QDialog>
#include <QIcon>
#include <QMenu>
#include <QWidget>
// DAGui
#include "Table/Metadata/DAConditionalFormatPresets.h"
#include "Table/Models/DADataTableModel.h"
#include "Table/Commands/DACommandsConditionalFormat.h"
#include "Table/Metadata/DATableConditionalFormatStore.h"
#include "Table/Dialogs/DADialogConditionalFormat.h"
#include "Table/Dialogs/DADialogConditionalFormatQuickRule.h"
// DAData
#include "DADataPyDataFrame.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

namespace DA
{
//===================================================
// DATableConditionalFormatController
//===================================================

DATableConditionalFormatController::DATableConditionalFormatController(const DATablePageContext& ctx,
                                                                       const DATableSelectionService& sel,
                                                                       QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel)
{
}

/**
 * @brief 打开条件格式规则编辑对话框（003 T1.5）
 *
 * 作用列 = 当前选区覆盖的列集合（无选区回退当前格所在列）；对话框编辑期间
 * 用 store 的「预览规则集」实时叠加（不落库），确定后按 resultRules 与 store
 * 现值的差异生成增/删/改/移子命令的父命令入页撤销栈（D4：一切写操作入栈；
 * 条件格式不写 DataFrame，红线3），取消还原。惰性/只读源上色阶与数据条
 * 在对话框内置灰并给出说明（§1.4-2），单元格规则正常。
 * @param initialType DATableConditionalFormatRule::Type 整数值（-1=不预设）
 */
void DATableConditionalFormatController::showDialog(int initialType)
{
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    if (!cfStore) {
        // 开发诊断：无元数据注册表（正常 APP 链路不会出现）
        qWarning() << "DATableConditionalFormatController::showDialog: metadata registry unavailable";
        return;
    }
    QList< int > cols = mSel.selectedDataframeColumns();
    if (cols.isEmpty()) {
        const int c = mSel.oneSelectedDataframeColumn();
        if (c >= 0) {
            cols.append(c);
        }
    }
    if (cols.isEmpty()) {
        daWarning << tr("Please select a column first");  // cn:请先选择列
        return;
    }
    // 列名：dataframe 用 columns()；惰性表等无 dataframe 时退回模型表头文本
    QStringList columnNames;
    DAPyDataFrame df = mCtx.df();
    if (!df.isNone()) {
        columnNames = df.columns();
    } else if (DADataTableModel* model = mCtx.model()) {
        const int cc = model->columnCount();
        for (int c = 0; c < cc; ++c) {
            columnNames.append(model->headerData(c, Qt::Horizontal).toString());
        }
    }
    DADialogConditionalFormat dlg(cfStore, mCtx.hostWidget());
    dlg.setup(columnNames, cols, mCtx.isColumnStatsSourceAvailable(), initialType);
    if (QDialog::Accepted == dlg.exec()) {
        QUndoCommand* cmd = buildCommands(dlg.resultRules());
        if (cmd && mCtx.pushCommand) {
            mCtx.pushCommand(cmd);
        }
    }
}

/**
 * @brief 清除选中列的条件格式规则（一次命令，可撤销）
 */
void DATableConditionalFormatController::clearFromSelection()
{
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    if (!cfStore) {
        return;
    }
    const QList< int > cols = mSel.selectedDataframeColumns();
    if (cols.isEmpty()) {
        daWarning << tr("Please select a column first");  // cn:请先选择列
        return;
    }
    QUndoCommand* parent = new QUndoCommand(tr("Clear Conditional Format Rules"));  // cn:清除条件格式规则
    bool any             = false;
    for (int col : cols) {
        if (cfStore->hasRules(col)) {
            new DACommandConditionalFormat_clearColumn(cfStore, col, parent);
            any = true;
        }
    }
    if (!any) {
        delete parent;
        daInfo << tr("The selected columns have no conditional format rules");  // cn:所选列没有条件格式规则
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(parent);
    }
}

/**
 * @brief 清除全表条件格式规则（一次命令，可撤销）
 */
void DATableConditionalFormatController::clearAll()
{
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    if (!cfStore) {
        return;
    }
    const QList< int > cols = cfStore->columnsWithRules();
    if (cols.isEmpty()) {
        daInfo << tr("The table has no conditional format rules");  // cn:本表没有条件格式规则
        return;
    }
    QUndoCommand* parent = new QUndoCommand(tr("Clear Conditional Format Rules"));  // cn:清除条件格式规则
    for (int c : cols) {
        new DACommandConditionalFormat_clearColumn(cfStore, c, parent);
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(parent);
    }
}

/**
 * @brief 条件格式入口作用列
 *
 * 选区覆盖列；无选区回退当前格所在列；空列表 = 无有效列（调用方提示）。
 * @return 作用列（升序不保证，按选区返回序）
 */
QList< int > DATableConditionalFormatController::scopeColumns() const
{
    QList< int > cols = mSel.selectedDataframeColumns();
    if (cols.isEmpty()) {
        const int c = mSel.oneSelectedDataframeColumn();
        if (c >= 0) {
            cols.append(c);
        }
    }
    return cols;
}

/**
 * @brief 给每个作用列追加一条规则（预设一键施加）
 *
 * 新规则集 = 落库规则 + 追加项，经 buildCommands 差异生成撤销命令入栈
 * （D4：一切写操作入栈；条件格式不写 DataFrame，红线3）。
 * @param rule 预设规则
 */
void DATableConditionalFormatController::applyRuleToScope(const DATableConditionalFormatRule& rule)
{
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    if (!cfStore) {
        return;
    }
    const QList< int > cols = scopeColumns();
    if (cols.isEmpty()) {
        daWarning << tr("Please select a column first");  // cn:请先选择列
        return;
    }
    QHash< int, QVector< DATableConditionalFormatRule > > newRules;
    for (int c : cols) {
        QVector< DATableConditionalFormatRule > v = cfStore->columnRules(c);
        v.append(rule);
        newRules.insert(c, v);
    }
    if (QUndoCommand* cmd = buildCommands(newRules)) {
        if (mCtx.pushCommand) {
            mCtx.pushCommand(cmd);
        }
    }
}

/**
 * @brief 突出显示单元格规则/重复值快速对话框：确定后施加到作用列
 * @param cond 条件（重复/唯一在对话框内可切换）
 */
void DATableConditionalFormatController::showQuickCellRuleDialog(DATableConditionalFormatRule::Condition cond)
{
    DADialogConditionalFormatQuickRule dlg(cond, mCtx.hostWidget());
    if (QDialog::Accepted == dlg.exec()) {
        applyRuleToScope(dlg.resultRule());
    }
}

/**
 * @brief 填充条件格式菜单（WPS 式预设入口）
 *
 * 结构（分组间分隔符）：数据条▸ / 色阶▸ ‖ 突出显示单元格规则▸ ‖ 清除规则▸ ‖ 自定义设置...。
 * 数据条/色阶子菜单为预设缩略图一键施加 + 「其他规则...」进自定义对话框；
 * 突出显示子菜单含大于/小于/介于/等于/文本包含（快速对话框）、发生日期▸（十项一键）、
 * 重复值...（快速对话框）与「其他规则...」；惰性/只读源上数据条与色阶子菜单置灰。
 * @param menu 目标菜单（先 clear 再填充；Ribbon 与右键子菜单共用）
 */
void DATableConditionalFormatController::populateMenu(QMenu* menu)
{
    if (!menu) {
        return;
    }
    using CfRule  = DATableConditionalFormatRule;
    using CfCond  = DATableConditionalFormatRule::Condition;
    using CfType  = DATableConditionalFormatRule::Type;
    const QIcon cfIcon(QStringLiteral(":/DAGui/icon/conditional-format.svg"));
    const bool scaleBarAvailable = mCtx.isColumnStatsSourceAvailable();
    menu->clear();

    // 预设色名（与 DAConditionalFormatPresets 数据条六色同序）
    const QStringList barColorNames { tr("Blue"),    // cn:蓝色
                                      tr("Green"),    // cn:绿色
                                      tr("Red"),      // cn:红色
                                      tr("Yellow"),   // cn:黄色
                                      tr("Cyan"),     // cn:青色
                                      tr("Purple") };  // cn:紫色

    // —— 数据条▸（渐变 6 + 实心 6 + 其他规则）——
    QMenu* barMenu = menu->addMenu(cfIcon, tr("Data Bars"));  // cn:数据条
    barMenu->setEnabled(scaleBarAvailable);
    {
        const QList< DACFDataBarPreset > presets = DAConditionalFormatPresets::dataBarPresets();
        QAction* head = barMenu->addAction(tr("Gradient Fill"));  // cn:渐变填充
        head->setEnabled(false);
        for (int i = 0; i < 6 && i < presets.size(); ++i) {
            const DACFDataBarPreset ps = presets.at(i);
            QAction* a = barMenu->addAction(DAConditionalFormatPresets::dataBarThumb(ps.color, ps.gradient),
                                            tr("%1 Gradient Fill").arg(barColorNames.value(i)));  // cn:%1渐变填充
            connect(a, &QAction::triggered, this, [ this, ps ]() {
                CfRule r;
                r.type       = CfType::DataBar;
                r.barColor   = ps.color;
                r.barGradient = true;
                applyRuleToScope(r);
            });
        }
        barMenu->addSeparator();
        QAction* head2 = barMenu->addAction(tr("Solid Fill"));  // cn:实心填充
        head2->setEnabled(false);
        for (int i = 6; i < presets.size(); ++i) {
            const DACFDataBarPreset ps = presets.at(i);
            QAction* a = barMenu->addAction(DAConditionalFormatPresets::dataBarThumb(ps.color, ps.gradient),
                                            tr("%1 Solid Fill").arg(barColorNames.value(i - 6)));  // cn:%1实心填充
            connect(a, &QAction::triggered, this, [ this, ps ]() {
                CfRule r;
                r.type     = CfType::DataBar;
                r.barColor = ps.color;
                applyRuleToScope(r);
            });
        }
        barMenu->addSeparator();
        QAction* other = barMenu->addAction(tr("Other Rules..."));  // cn:其他规则...
        connect(other, &QAction::triggered, this, [ this ]() {
            showDialog(static_cast< int >(CfType::DataBar));
        });
    }

    // —— 色阶▸（12 预设 + 其他规则）——
    QMenu* scaleMenu = menu->addMenu(cfIcon, tr("Color Scales"));  // cn:色阶
    scaleMenu->setEnabled(scaleBarAvailable);
    {
        const QList< DACFColorScalePreset > presets = DAConditionalFormatPresets::colorScalePresets();
        for (const DACFColorScalePreset& ps : presets) {
            QAction* a = scaleMenu->addAction(DAConditionalFormatPresets::colorScaleThumb(ps.colors),
                                              tr("%1-Color Scale").arg(ps.colors.size()));  // cn:%1色色阶
            connect(a, &QAction::triggered, this, [ this, ps ]() {
                CfRule r;
                r.type          = CfType::ColorScale;
                r.threeColorScale = (ps.colors.size() >= 3);
                if (ps.colors.size() >= 3) {
                    r.scaleMinColor = ps.colors.at(0);
                    r.scaleMidColor = ps.colors.at(1);
                    r.scaleMaxColor = ps.colors.at(2);
                } else if (ps.colors.size() == 2) {
                    r.scaleMinColor = ps.colors.at(0);
                    r.scaleMaxColor = ps.colors.at(1);
                }
                applyRuleToScope(r);
            });
        }
        scaleMenu->addSeparator();
        QAction* other = scaleMenu->addAction(tr("Other Rules..."));  // cn:其他规则...
        connect(other, &QAction::triggered, this, [ this ]() {
            showDialog(static_cast< int >(CfType::ColorScale));
        });
    }
    menu->addSeparator();

    // —— 突出显示单元格规则▸ ——
    QMenu* hlMenu = menu->addMenu(cfIcon, tr("Highlight Cells Rules"));  // cn:突出显示单元格规则
    {
        QAction* aGreater = hlMenu->addAction(tr("Greater Than..."));  // cn:大于...
        connect(aGreater, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Greater); });
        QAction* aLess = hlMenu->addAction(tr("Less Than..."));  // cn:小于...
        connect(aLess, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Less); });
        QAction* aBetween = hlMenu->addAction(tr("Between..."));  // cn:介于...
        connect(aBetween, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Between); });
        QAction* aEqual = hlMenu->addAction(tr("Equal To..."));  // cn:等于...
        connect(aEqual, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Equal); });
        QAction* aContains = hlMenu->addAction(tr("Text Contains..."));  // cn:文本包含...
        connect(aContains, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Contains); });
        // 发生日期▸：十项一键施加（默认橙红底白字样式）
        QMenu* dateMenu = hlMenu->addMenu(tr("A Date Occurring"));  // cn:发生日期
        const QList< QPair< CfRule::DateOccurrence, QString > > dateItems {
            { CfRule::DateOccurrence::Yesterday, tr("Yesterday") },                // cn:昨天
            { CfRule::DateOccurrence::Today, tr("Today") },                        // cn:今天
            { CfRule::DateOccurrence::Tomorrow, tr("Tomorrow") },                  // cn:明天
            { CfRule::DateOccurrence::Last7Days, tr("In the Last 7 Days") },       // cn:最近 7 天
            { CfRule::DateOccurrence::LastWeek, tr("Last Week") },                 // cn:上周
            { CfRule::DateOccurrence::ThisWeek, tr("This Week") },                 // cn:本周
            { CfRule::DateOccurrence::NextWeek, tr("Next Week") },                 // cn:下周
            { CfRule::DateOccurrence::LastMonth, tr("Last Month") },               // cn:上月
            { CfRule::DateOccurrence::ThisMonth, tr("This Month") },               // cn:本月
            { CfRule::DateOccurrence::NextMonth, tr("Next Month") },               // cn:下月
        };
        for (const auto& item : dateItems) {
            QAction* a = dateMenu->addAction(item.second);
            connect(a, &QAction::triggered, this, [ this, item ]() {
                CfRule r;
                r.type      = CfType::CellRule;
                r.condition = CfCond::DateOccurring;
                r.value1    = static_cast< int >(item.first);
                const DACFHighlightStyle def = DAConditionalFormatPresets::highlightStyles().value(0);
                if (def.hasBackground()) {
                    r.style.setBackground(QBrush(def.background));
                }
                if (def.hasForeground()) {
                    r.style.setForeground(def.foreground);
                }
                applyRuleToScope(r);
            });
        }
        QAction* aDup = hlMenu->addAction(tr("Duplicate Values..."));  // cn:重复值...
        connect(aDup, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Duplicate); });
        hlMenu->addSeparator();
        QAction* aOther = hlMenu->addAction(tr("Other Rules..."));  // cn:其他规则...
        connect(aOther, &QAction::triggered, this, [ this ]() {
            showDialog(static_cast< int >(CfType::CellRule));
        });
    }
    menu->addSeparator();

    // —— 清除规则▸ ——
    QMenu* clearMenu = menu->addMenu(tr("Clear Rules"));  // cn:清除规则
    {
        QAction* aSel = clearMenu->addAction(tr("Clear Rules from Selected Columns"));  // cn:清除所选列的规则
        connect(aSel, &QAction::triggered, this, &DATableConditionalFormatController::clearFromSelection);
        QAction* aAll = clearMenu->addAction(tr("Clear Rules from Entire Sheet"));  // cn:清除全表的规则
        connect(aAll, &QAction::triggered, this, &DATableConditionalFormatController::clearAll);
    }
    menu->addSeparator();

    // —— 自定义设置...（规则管理对话框）——
    QAction* aManage = menu->addAction(cfIcon, tr("Custom Rules..."));  // cn:自定义设置...
    connect(aManage, &QAction::triggered, this, [ this ]() { showDialog(); });
}

/**
 * @brief 规则集差异 → 撤销命令（父命令聚合，旧新对象记录范式 D4）
 *
 * 每列独立 diff：新集为空 → clearColumn；旧集为空 → 逐条 add；同元素多重集
 * 仅顺序不同 → move 序列；其余 → 位置式 diff（前缀 modify + 尾部 add/remove）。
 * undo/redo 结果对象与差异前/后完全一致（单测钉住）。
 * @param newRules 对话框结果（列 → 规则列表）
 * @return 父命令；无任何差异返回 nullptr（调用方不推空命令）
 */
QUndoCommand* DATableConditionalFormatController::buildCommands(
    const QHash< int, QVector< DATableConditionalFormatRule > >& newRules) const
{
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    QUndoCommand* parent = new QUndoCommand(tr("Change Conditional Format Rules"));  // cn:更改条件格式规则
    bool any             = false;
    QList< int > cols    = newRules.keys();
    std::sort(cols.begin(), cols.end());
    for (int col : std::as_const(cols)) {
        // 预览已在对话框 accept 时清除，columnRules 返回落库值
        const QVector< DATableConditionalFormatRule > oldRules = cfStore->columnRules(col);
        const QVector< DATableConditionalFormatRule > newCol   = newRules.value(col);
        if (oldRules == newCol) {
            continue;
        }
        any = true;
        if (newCol.isEmpty()) {
            new DACommandConditionalFormat_clearColumn(cfStore, col, parent);
            continue;
        }
        if (oldRules.isEmpty()) {
            for (int i = 0; i < newCol.size(); ++i) {
                new DACommandConditionalFormat_addRule(cfStore, col, newCol.at(i), i, parent);
            }
            continue;
        }
        // 纯重排检测（同元素多重集）：move 序列（保持"移动"语义，与对话框上移/下移对应）
        QVector< DATableConditionalFormatRule > rest = oldRules;
        bool permutation                             = true;
        for (const DATableConditionalFormatRule& r : newCol) {
            int found = -1;
            for (int i = 0; i < rest.size(); ++i) {
                if (rest.at(i) == r) {
                    found = i;
                    break;
                }
            }
            if (found < 0) {
                permutation = false;
                break;
            }
            rest.removeAt(found);
        }
        if (permutation && rest.isEmpty()) {
            QVector< DATableConditionalFormatRule > cur = oldRules;
            for (int i = 0; i < newCol.size(); ++i) {
                int j = i;
                while (j < cur.size() && !(cur.at(j) == newCol.at(i))) {
                    ++j;
                }
                if (j > i && j < cur.size()) {
                    new DACommandConditionalFormat_moveRule(cfStore, col, j, i, parent);
                    const DATableConditionalFormatRule r = cur.takeAt(j);
                    cur.insert(i, r);
                }
            }
            continue;
        }
        // 位置式 diff：公共前缀 modify + 尾部 add（新更长）/ remove（旧更长，从尾到头保持索引有效）
        const int n = qMin(oldRules.size(), newCol.size());
        for (int i = 0; i < n; ++i) {
            if (!(oldRules.at(i) == newCol.at(i))) {
                new DACommandConditionalFormat_modifyRule(cfStore, col, i, oldRules.at(i), newCol.at(i), parent);
            }
        }
        for (int i = n; i < newCol.size(); ++i) {
            new DACommandConditionalFormat_addRule(cfStore, col, newCol.at(i), i, parent);
        }
        for (int i = oldRules.size() - 1; i >= n; --i) {
            new DACommandConditionalFormat_removeRule(cfStore, col, i, parent);
        }
    }
    if (!any) {
        delete parent;
        return nullptr;
    }
    return parent;
}
}  // end of namespace DA
