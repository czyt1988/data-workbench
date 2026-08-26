// DAAgentPermissionSettingsWidget.cpp
// permission-layer P1 骨架 + P2 补全：权限模式默认值 / 审批超时 / manual 拦截开关 /
// 判官配置 / 路径规则表 / 代码危险模式清单 / 工具分级覆盖。
// 读写经 DAAgentInterface::getPermissionConfig / setPermissionConfig（contains 守卫，
// 未携带的 key 不受影响）；保存后由接口侧触发 reconfigure 下发 Python 子进程。
#include "DAAgentPermissionSettingsWidget.h"
#include "DALogCategory.h"
#include <QJsonArray>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QIcon>
#include <QScrollArea>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QStyledItemDelegate>
#include <QRegularExpression>
#include <algorithm>
#include <functional>

namespace {

// Qt5/Qt6 双兼容 helper：取值并兜底默认值（QJsonObject::value(key,default) Qt5 不存在）
int jsonInt(const QJsonObject& o, const char* key, int def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toInt() : def;
}

bool jsonBool(const QJsonObject& o, const char* key, bool def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isBool() ? v.toBool() : def;
}

QString jsonString(const QJsonObject& o, const char* key)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isString() ? v.toString() : QString();
}

/// 下拉列委托：单元格编辑时提供固定选项的 QComboBox
class DAComboItemDelegate : public QStyledItemDelegate
{
public:
    DAComboItemDelegate(const QStringList& options, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), mOptions(options)
    {
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        QComboBox* cb = new QComboBox(parent);
        cb->addItems(mOptions);
        return cb;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        QComboBox* cb = qobject_cast< QComboBox* >(editor);
        if (cb) {
            cb->setCurrentText(index.data(Qt::DisplayRole).toString());
        }
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        QComboBox* cb = qobject_cast< QComboBox* >(editor);
        if (cb) {
            model->setData(index, cb->currentText(), Qt::EditRole);
        }
    }

private:
    QStringList mOptions;
};

/// 多行文本 → 模式清单（逐行去空白、跳过空行）
QStringList linesToPatterns(const QString& text)
{
    QStringList out;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]")));
    for (const QString& line : lines) {
        const QString t = line.trimmed();
        if (!t.isEmpty()) {
            out.append(t);
        }
    }
    return out;
}

}  // namespace

namespace DA
{

/** @brief 构造 Agent 权限设置页控件 */
DAAgentPermissionSettingsWidget::DAAgentPermissionSettingsWidget(QWidget* parent)
    : DAAbstractSettingPage(parent)
{
    setupUI();
}

/** @brief 注入 Agent 接口并加载已有配置 */
void DAAgentPermissionSettingsWidget::setAgentInterface(DAAgentInterface* p)
{
    mAgentInterface = p;
    if (p) {
        QSignalBlocker blocker(this);
        loadConfig();
    }
}

/** @brief 构建设置页界面（内容较长，整体置于滚动区内） */
void DAAgentPermissionSettingsWidget::setupUI()
{
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    QWidget* content = new QWidget(scroll);
    QVBoxLayout* mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // ---- 默认权限模式 ----
    QGroupBox* modeBox = new QGroupBox(tr("Default Permission Mode"), content);  //cn:默认权限模式
    QFormLayout* modeForm = new QFormLayout(modeBox);
    mModeCombo = new QComboBox(modeBox);
    // 展示顺序：auto → manual → yolo（未配置时默认）；data 存协议值
    mModeCombo->addItem(tr("Auto (rules + judge)"), QStringLiteral("auto"));       //cn:自动（规则+判官）
    mModeCombo->addItem(tr("Manual (ask every write/code)"), QStringLiteral("manual"));  //cn:手动（写入/代码每次询问）
    mModeCombo->addItem(tr("Full Auto (yolo)"), QStringLiteral("yolo"));           //cn:全自动（yolo）
    modeForm->addRow(tr("Mode:"), mModeCombo);  //cn:模式：
    mainLayout->addWidget(modeBox);

    // ---- 审批与拦截 ----
    QGroupBox* askBox = new QGroupBox(tr("Approval"), content);  //cn:审批
    QFormLayout* askForm = new QFormLayout(askBox);
    mApprovalTimeoutSpin = new QSpinBox(askBox);
    mApprovalTimeoutSpin->setRange(10, 86400);
    mApprovalTimeoutSpin->setSuffix(QStringLiteral(" s"));
    mApprovalTimeoutSpin->setToolTip(tr("Max wait for user approval on file writes / code execution"));  //cn:文件写入/代码执行等待用户批准的最长时间
    askForm->addRow(tr("Approval timeout:"), mApprovalTimeoutSpin);  //cn:审批超时：
    mManualBlockInapp = new QCheckBox(tr("In manual mode, also ask before in-app chart edits"), askBox);  //cn:手动模式下，应用内图表修改也需询问
    askForm->addRow(QString(), mManualBlockInapp);
    mainLayout->addWidget(askBox);

    // ---- 判官配置 ----
    QGroupBox* judgeBox = new QGroupBox(tr("Code Judge (optional)"), content);  //cn:代码判官（可选）
    QFormLayout* judgeForm = new QFormLayout(judgeBox);
    mJudgeModelEdit = new QLineEdit(judgeBox);
    mJudgeModelEdit->setPlaceholderText(tr("Leave empty to disable the judge"));  //cn:留空则不启用判官
    judgeForm->addRow(tr("Judge model:"), mJudgeModelEdit);  //cn:判官模型：
    mJudgeTimeoutSpin = new QSpinBox(judgeBox);
    mJudgeTimeoutSpin->setRange(1, 600);
    mJudgeTimeoutSpin->setSuffix(QStringLiteral(" s"));
    judgeForm->addRow(tr("Judge timeout:"), mJudgeTimeoutSpin);  //cn:判官超时：
    mJudgeHint = new QLabel(judgeBox);
    mJudgeHint->setWordWrap(true);
    mJudgeHint->setStyleSheet(QStringLiteral("color: #727272;"));
    // 判官预期管理提示（母文档 §6.4 [v2.1]，对应 D1 兜底）
    mJudgeHint->setText(tr("When the judge model is not configured, Auto mode still asks for "
                           "approval on every code execution (even if no dangerous pattern "
                           "matches). Configuring a judge enables automatic allow/deny for "
                           "gray-area code, and code matching no pattern is then allowed "
                           "silently."));  //cn:未配置判官模型时，自动模式对代码执行仍会逐次询问（即使未命中任何危险模式）；配置判官后可对灰区代码自动放行/拒绝，未命中模式的代码将静默放行。
    judgeForm->addRow(QString(), mJudgeHint);
    mainLayout->addWidget(judgeBox);

    // ---- 路径规则表 ----
    QGroupBox* rulesBox = new QGroupBox(tr("File Path Rules"), content);  //cn:文件路径规则
    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesBox);
    QLabel* rulesHint = new QLabel(rulesBox);
    rulesHint->setWordWrap(true);
    rulesHint->setStyleSheet(QStringLiteral("color: #727272;"));
    rulesHint->setText(tr("Evaluated in order, first match wins; file writes matching no rule "
                          "ask for approval. Variables: ${workspace}, ${project}, ${data}, "
                          "${exe}, ${home}. Global deny rows (tool = *, action = deny) are "
                          "hard safety rules and cannot be edited or removed."));  //cn:按顺序求值、首条命中生效；未命中规则的写入将征求确认。可用变量：${workspace}、${project}、${data}、${exe}、${home}。全局拒绝行（工具 = *、动作 = deny）为硬性安全规则，不可编辑或删除。
    rulesLayout->addWidget(rulesHint);
    mRulesTable = new QTableWidget(0, 3, rulesBox);
    mRulesTable->setHorizontalHeaderLabels({
        tr("Tool"),          //cn:工具
        tr("Scope (glob)"),  //cn:范围（glob）
        tr("Action")         //cn:动作
    });
    mRulesTable->horizontalHeader()->setStretchLastSection(true);
    mRulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    mRulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mRulesTable->setMinimumHeight(140);
    mRulesTable->setItemDelegateForColumn(2, new DAComboItemDelegate(
        {QStringLiteral("allow"), QStringLiteral("deny"), QStringLiteral("ask")}, mRulesTable));
    rulesLayout->addWidget(mRulesTable);
    QHBoxLayout* rulesBtnLayout = new QHBoxLayout();
    mRuleAddBtn = new QPushButton(tr("Add Rule"), rulesBox);  //cn:新增规则
    mRuleRemoveBtn = new QPushButton(tr("Remove Selected"), rulesBox);  //cn:删除选中
    rulesBtnLayout->addWidget(mRuleAddBtn);
    rulesBtnLayout->addWidget(mRuleRemoveBtn);
    rulesBtnLayout->addStretch(1);
    rulesLayout->addLayout(rulesBtnLayout);
    mainLayout->addWidget(rulesBox);

    // ---- 代码危险模式清单 ----
    QGroupBox* patternsBox = new QGroupBox(tr("Code Danger Patterns (Auto mode)"), content);  //cn:代码危险模式（自动模式）
    QVBoxLayout* patternsLayout = new QVBoxLayout(patternsBox);
    QLabel* patternsHint = new QLabel(patternsBox);
    patternsHint->setWordWrap(true);
    patternsHint->setStyleSheet(QStringLiteral("color: #727272;"));
    // 咨询性防线明示（母文档 §9.2 [v2.1]：静态模式可被混淆绕过，manual 是严格场景的逃生门）
    patternsHint->setText(tr("One regular expression per line; invalid expressions are skipped. "
                             "Deny patterns reject code outright; Escalate patterns are referred "
                             "to the judge model. Note: this judging layer is an advisory "
                             "defense, not a security boundary — static patterns can be bypassed "
                             "by obfuscation (e.g. indirect attribute access, importing a "
                             "malicious module from a clean entry script, rewriting the script "
                             "after judging). For strict scenarios use Manual mode."));  //cn:每行一条正则表达式；非法表达式将被跳过。拒绝（deny）模式直接拒绝代码；升级（escalate）模式交由判官模型裁决。注意：判定层是咨询性防线而非安全边界——静态模式可被混淆绕过（如间接属性访问、经干净入口脚本导入恶意模块、判定后改写脚本等）。严格场景请使用手动（manual）模式。
    patternsLayout->addWidget(patternsHint);
    patternsLayout->addWidget(new QLabel(tr("Deny patterns (matched => reject):"), patternsBox));  //cn:拒绝模式（命中即拒绝）：
    mDenyPatternsEdit = new QPlainTextEdit(patternsBox);
    mDenyPatternsEdit->setPlaceholderText(tr("One regex per line"));  //cn:每行一条正则
    mDenyPatternsEdit->setMaximumHeight(120);
    patternsLayout->addWidget(mDenyPatternsEdit);
    patternsLayout->addWidget(new QLabel(tr("Escalate patterns (matched => consult judge):"), patternsBox));  //cn:升级模式（命中交判官裁决）：
    mEscalatePatternsEdit = new QPlainTextEdit(patternsBox);
    mEscalatePatternsEdit->setPlaceholderText(tr("One regex per line"));  //cn:每行一条正则
    mEscalatePatternsEdit->setMaximumHeight(120);
    patternsLayout->addWidget(mEscalatePatternsEdit);
    mainLayout->addWidget(patternsBox);

    // ---- 工具分级覆盖 ----
    QGroupBox* tierBox = new QGroupBox(tr("Tool Tier Overrides"), content);  //cn:工具分级覆盖
    QVBoxLayout* tierLayout = new QVBoxLayout(tierBox);
    QLabel* tierHint = new QLabel(tierBox);
    tierHint->setWordWrap(true);
    tierHint->setStyleSheet(QStringLiteral("color: #727272;"));
    tierHint->setText(tr("Explicitly assign a risk tier to a tool (typically plugin tools). "
                          "Unlisted unknown plugin tools default to the \"unknown\" tier, which "
                          "asks for approval in Auto/Manual modes; e.g. map a known-reversible "
                          "chart plugin tool to inapp_mutate to let it pass silently."));  //cn:为工具显式指定风险分级（通常用于插件工具）。未列出的未知插件工具默认归入 "unknown" 分级，在自动/手动模式下会征求确认；例如可把已知可逆的图表类插件工具归入 inapp_mutate 使其静默放行。
    tierLayout->addWidget(tierHint);
    mTierTable = new QTableWidget(0, 2, tierBox);
    mTierTable->setHorizontalHeaderLabels({
        tr("Tool"),   //cn:工具
        tr("Tier")    //cn:分级
    });
    mTierTable->horizontalHeader()->setStretchLastSection(true);
    mTierTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTierTable->setMinimumHeight(100);
    mTierTable->setItemDelegateForColumn(1, new DAComboItemDelegate(
        {QStringLiteral("read"), QStringLiteral("inapp_mutate"), QStringLiteral("file_write"),
         QStringLiteral("code_exec"), QStringLiteral("unknown")}, mTierTable));
    tierLayout->addWidget(mTierTable);
    QHBoxLayout* tierBtnLayout = new QHBoxLayout();
    mTierAddBtn = new QPushButton(tr("Add Override"), tierBox);  //cn:新增覆盖
    mTierRemoveBtn = new QPushButton(tr("Remove Selected"), tierBox);  //cn:删除选中
    tierBtnLayout->addWidget(mTierAddBtn);
    tierBtnLayout->addWidget(mTierRemoveBtn);
    tierBtnLayout->addStretch(1);
    tierLayout->addLayout(tierBtnLayout);
    mainLayout->addWidget(tierBox);

    mainLayout->addStretch(1);
    scroll->setWidget(content);

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    // 任意控件变更 → 标记设置页为 dirty（顶层据此启用"应用"）
    auto mark = [this]() { emit settingChanged(); };
    connect(mModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, mark);
    connect(mApprovalTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mManualBlockInapp, &QCheckBox::toggled, this, mark);
    connect(mJudgeModelEdit, &QLineEdit::textChanged, this, mark);
    connect(mJudgeTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mRulesTable, &QTableWidget::cellChanged, this, mark);
    // 单元格内容变化后按新内容同步该行的锁定状态（如把普通行改成全局 deny）
    connect(mRulesTable, &QTableWidget::cellChanged, this, [this](int row, int /*col*/) {
        if (row >= 0 && row < mRulesTable->rowCount()) {
            applyRowLock(row);
        }
    });
    connect(mDenyPatternsEdit, &QPlainTextEdit::textChanged, this, mark);
    connect(mEscalatePatternsEdit, &QPlainTextEdit::textChanged, this, mark);
    connect(mTierTable, &QTableWidget::cellChanged, this, mark);

    connect(mRuleAddBtn, &QPushButton::clicked, this, [this]() {
        appendRuleRow(QStringLiteral("write_file"), QStringLiteral("${workspace}/**"), QStringLiteral("allow"));
        emit settingChanged();
    });
    connect(mRuleRemoveBtn, &QPushButton::clicked, this, [this]() {
        // 锁定行（全局硬 deny）不可删除；从下往上移除避免行号漂移
        QList< int > rows;
        const QList< QTableWidgetItem* > selected = mRulesTable->selectedItems();
        for (QTableWidgetItem* item : selected) {
            const int row = item->row();
            if (!rows.contains(row) && !isLockedRule(mRulesTable->item(row, 0) ? mRulesTable->item(row, 0)->text() : QString(),
                                                     mRulesTable->item(row, 2) ? mRulesTable->item(row, 2)->text() : QString())) {
                rows.append(row);
            }
        }
        std::sort(rows.begin(), rows.end(), std::greater< int >());
        if (rows.isEmpty()) {
            return;
        }
        for (int row : rows) {
            mRulesTable->removeRow(row);
        }
        emit settingChanged();
    });
    connect(mTierAddBtn, &QPushButton::clicked, this, [this]() {
        appendTierRow(QString(), QStringLiteral("inapp_mutate"));
        emit settingChanged();
    });
    connect(mTierRemoveBtn, &QPushButton::clicked, this, [this]() {
        QList< int > rows;
        const QList< QTableWidgetItem* > selected = mTierTable->selectedItems();
        for (QTableWidgetItem* item : selected) {
            const int row = item->row();
            if (!rows.contains(row)) {
                rows.append(row);
            }
        }
        std::sort(rows.begin(), rows.end(), std::greater< int >());
        if (rows.isEmpty()) {
            return;
        }
        for (int row : rows) {
            mTierTable->removeRow(row);
        }
        emit settingChanged();
    });
}

/** @brief 追加一条路径规则行（编程式填充，屏蔽表格信号避免误标 dirty） */
void DAAgentPermissionSettingsWidget::appendRuleRow(const QString& tool, const QString& scope, const QString& action)
{
    const int row = mRulesTable->rowCount();
    {
        QSignalBlocker blocker(mRulesTable);
        mRulesTable->insertRow(row);
        QTableWidgetItem* toolItem = new QTableWidgetItem(tool);
        QTableWidgetItem* scopeItem = new QTableWidgetItem(scope);
        QTableWidgetItem* actionItem = new QTableWidgetItem(action);
        mRulesTable->setItem(row, 0, toolItem);
        mRulesTable->setItem(row, 1, scopeItem);
        mRulesTable->setItem(row, 2, actionItem);
    }
    applyRowLock(row);
}

/** @brief 按当前单元格内容应用/解除行锁定（全局硬 deny 行不可编辑、不可删除） */
void DAAgentPermissionSettingsWidget::applyRowLock(int row)
{
    const QString tool = mRulesTable->item(row, 0) ? mRulesTable->item(row, 0)->text() : QString();
    const QString action = mRulesTable->item(row, 2) ? mRulesTable->item(row, 2)->text() : QString();
    const bool locked = isLockedRule(tool, action);
    for (int col = 0; col < mRulesTable->columnCount(); ++col) {
        QTableWidgetItem* item = mRulesTable->item(row, col);
        if (!item) {
            continue;
        }
        Qt::ItemFlags flags = item->flags();
        if (locked) {
            flags &= ~Qt::ItemIsEditable;
            item->setToolTip(tr("Hard safety rule: cannot be edited or removed"));  //cn:硬性安全规则：不可编辑或删除
        } else {
            flags |= Qt::ItemIsEditable;
            item->setToolTip(QString());
        }
        item->setFlags(flags);
    }
}

/** @brief 追加一条分级覆盖行 */
void DAAgentPermissionSettingsWidget::appendTierRow(const QString& tool, const QString& tier)
{
    const int row = mTierTable->rowCount();
    QSignalBlocker blocker(mTierTable);
    mTierTable->insertRow(row);
    mTierTable->setItem(row, 0, new QTableWidgetItem(tool));
    mTierTable->setItem(row, 1, new QTableWidgetItem(tier));
}

/** @brief 全局拒绝行判定：tool=="*" 且 action=="deny"（母文档 §7.1 界面锁定） */
bool DAAgentPermissionSettingsWidget::isLockedRule(const QString& tool, const QString& action)
{
    return tool.trimmed() == QStringLiteral("*")
           && action.trimmed().compare(QStringLiteral("deny"), Qt::CaseInsensitive) == 0;
}

/** @brief 从接口加载权限配置到界面 */
void DAAgentPermissionSettingsWidget::loadConfig()
{
    if (!mAgentInterface) {
        daDebug << "[DAAgentPermissionSettings] loadConfig skipped: no agent interface injected";
        return;
    }
    const QJsonObject c = mAgentInterface->getPermissionConfig();
    const QString mode = jsonString(c, "mode");
    int idx = mModeCombo->findData(mode);
    if (idx < 0) {
        idx = mModeCombo->findData(QStringLiteral("yolo"));  // 未知/缺失回退 yolo（默认全自动）
    }
    mModeCombo->setCurrentIndex(qMax(0, idx));
    mApprovalTimeoutSpin->setValue(jsonInt(c, "tool_approval_timeout_sec", 600));
    mManualBlockInapp->setChecked(jsonBool(c, "manual_block_inapp_tools", false));
    mJudgeModelEdit->setText(jsonString(c, "judge_model"));
    mJudgeTimeoutSpin->setValue(jsonInt(c, "judge_timeout_sec", 30));

    // 路径规则表（load 侧已由引擎强制回填硬 deny 种子，此处仅渲染）
    {
        QSignalBlocker blocker(mRulesTable);
        mRulesTable->setRowCount(0);
    }
    const QJsonArray rulesArr = c.value(QStringLiteral("rules")).toArray();
    for (const QJsonValue& v : rulesArr) {
        if (!v.isObject()) {
            continue;
        }
        const QJsonObject r = v.toObject();
        appendRuleRow(r.value(QStringLiteral("tool")).toString(),
                      r.value(QStringLiteral("scope")).toString(),
                      r.value(QStringLiteral("action")).toString());
    }

    // 危险模式清单（每行一条正则）
    const QJsonObject patterns = c.value(QStringLiteral("code_patterns")).toObject();
    QStringList denyLines;
    for (const QJsonValue& v : patterns.value(QStringLiteral("deny")).toArray()) {
        denyLines.append(v.toString());
    }
    QStringList escalateLines;
    for (const QJsonValue& v : patterns.value(QStringLiteral("escalate")).toArray()) {
        escalateLines.append(v.toString());
    }
    {
        QSignalBlocker blockerDeny(mDenyPatternsEdit);
        QSignalBlocker blockerEscalate(mEscalatePatternsEdit);
        mDenyPatternsEdit->setPlainText(denyLines.join(QLatin1Char('\n')));
        mEscalatePatternsEdit->setPlainText(escalateLines.join(QLatin1Char('\n')));
    }

    // 分级覆盖表
    {
        QSignalBlocker blocker(mTierTable);
        mTierTable->setRowCount(0);
    }
    const QJsonObject overrides = c.value(QStringLiteral("tier_overrides")).toObject();
    for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
        appendTierRow(it.key(), it.value().toString());
    }
}

/** @brief 将界面配置写回接口（contains 守卫，未携带的 key 不受影响） */
void DAAgentPermissionSettingsWidget::saveConfig()
{
    if (!mAgentInterface) {
        daDebug << "[DAAgentPermissionSettings] saveConfig skipped: no agent interface injected";
        return;
    }
    QJsonObject c;
    c[QStringLiteral("mode")]                      = mModeCombo->currentData().toString();
    c[QStringLiteral("tool_approval_timeout_sec")] = mApprovalTimeoutSpin->value();
    c[QStringLiteral("manual_block_inapp_tools")]  = mManualBlockInapp->isChecked();
    c[QStringLiteral("judge_model")]               = mJudgeModelEdit->text().trimmed();
    c[QStringLiteral("judge_timeout_sec")]         = mJudgeTimeoutSpin->value();

    // 路径规则表 → rules 数组（引擎侧保存时仍会强制回填硬 deny 种子）
    QJsonArray rulesArr;
    for (int row = 0; row < mRulesTable->rowCount(); ++row) {
        const QString tool = mRulesTable->item(row, 0) ? mRulesTable->item(row, 0)->text().trimmed() : QString();
        const QString scope = mRulesTable->item(row, 1) ? mRulesTable->item(row, 1)->text().trimmed() : QString();
        const QString action = mRulesTable->item(row, 2) ? mRulesTable->item(row, 2)->text().trimmed().toLower() : QString();
        if (tool.isEmpty() && scope.isEmpty()) {
            continue;  // 跳过空行
        }
        QJsonObject r;
        r[QStringLiteral("tool")]   = tool;
        r[QStringLiteral("scope")]  = scope;
        r[QStringLiteral("action")] = action;
        rulesArr.append(r);
    }
    c[QStringLiteral("rules")] = rulesArr;

    // 危险模式清单 → code_patterns
    QJsonObject patterns;
    QJsonArray denyArr;
    for (const QString& p : linesToPatterns(mDenyPatternsEdit->toPlainText())) {
        denyArr.append(p);
    }
    QJsonArray escalateArr;
    for (const QString& p : linesToPatterns(mEscalatePatternsEdit->toPlainText())) {
        escalateArr.append(p);
    }
    patterns[QStringLiteral("deny")]     = denyArr;
    patterns[QStringLiteral("escalate")] = escalateArr;
    c[QStringLiteral("code_patterns")] = patterns;

    // 分级覆盖表 → tier_overrides（空工具名的行跳过）
    QJsonObject overrides;
    for (int row = 0; row < mTierTable->rowCount(); ++row) {
        const QString tool = mTierTable->item(row, 0) ? mTierTable->item(row, 0)->text().trimmed() : QString();
        const QString tier = mTierTable->item(row, 1) ? mTierTable->item(row, 1)->text().trimmed() : QString();
        if (tool.isEmpty() || tier.isEmpty()) {
            continue;
        }
        overrides[tool] = tier;
    }
    c[QStringLiteral("tier_overrides")] = overrides;

    mAgentInterface->setPermissionConfig(c);
}

/** @brief 应用设置页的配置变更 */
void DAAgentPermissionSettingsWidget::apply()
{
    daDebug << "[DAAgentPermissionSettings] apply entered";
    saveConfig();
    emit settingApplyed();
    daDebug << "[DAAgentPermissionSettings] apply done";
}

/** @brief 获取设置页图标（复用 agent 设置图标） */
QIcon DAAgentPermissionSettingsWidget::getSettingPageIcon() const
{
    return QIcon(":/DAGui/icon/setting-agent.svg");
}

} // namespace DA
