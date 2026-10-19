#include "DADialogValidationRules.h"
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QDateTime>

namespace DA
{

namespace
{
// 参数页索引（与 mComboType 顺序一致）
enum ParamPage
{
    PageNumericRange = 0,
    PageDateRange,
    PageEnum,
    PageNoParam,  // NotEmpty / Unique
    PageRegex
};
}

/**
 * @brief 构造
 * @param columnNames 全部列名（位置索引对齐，含重名）
 * @param rules 初始规则集
 * @param parent 父窗口
 */
DADialogValidationRules::DADialogValidationRules(const QStringList& columnNames,
                                                 const QList< DATableValidationRule >& rules,
                                                 QWidget* parent)
    : QDialog(parent), mColumnNames(columnNames), mWorking(rules)
{
    setupUI();
    rebuildRuleList();
    if (!mWorking.isEmpty()) {
        mListRules->setCurrentRow(0);
    }
    updateButtonStates();
}

DADialogValidationRules::~DADialogValidationRules()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）：左列表 + 右编辑面板
 */
void DADialogValidationRules::setupUI()
{
    setWindowTitle(tr("Validation Rules"));  // cn:数据校验规则
    QVBoxLayout* main = new QVBoxLayout(this);
    QHBoxLayout* body = new QHBoxLayout();
    // —— 左栏：规则列表 + 增删移按钮 ——
    QVBoxLayout* left = new QVBoxLayout();
    mListRules = new QListWidget(this);
    mListRules->setObjectName(QStringLiteral("validationRuleList"));
    left->addWidget(mListRules, 1);
    QHBoxLayout* leftBtns = new QHBoxLayout();
    mBtnAdd    = new QPushButton(tr("Add"), this);     // cn:添加
    mBtnRemove = new QPushButton(tr("Remove"), this);  // cn:删除
    mBtnUp     = new QPushButton(tr("Up"), this);      // cn:上移
    mBtnDown   = new QPushButton(tr("Down"), this);    // cn:下移
    mBtnAdd->setObjectName(QStringLiteral("btnAddRule"));
    mBtnRemove->setObjectName(QStringLiteral("btnRemoveRule"));
    leftBtns->addWidget(mBtnAdd);
    leftBtns->addWidget(mBtnRemove);
    leftBtns->addWidget(mBtnUp);
    leftBtns->addWidget(mBtnDown);
    left->addLayout(leftBtns);
    body->addLayout(left, 1);
    // —— 右栏：编辑器 ——
    QFormLayout* form = new QFormLayout();
    mComboColumn = new QComboBox(this);
    mComboColumn->setObjectName(QStringLiteral("comboValidationColumn"));
    // 列下拉：位置对齐全部列，重名列以「(第 n 个)」消歧显示（R20）
    QHash< QString, int > seen;
    for (int i = 0; i < mColumnNames.size(); ++i) {
        const QString name = mColumnNames[ i ];
        const int occ      = seen.value(name, 0);
        seen[ name ]       = occ + 1;
        QString text       = name;
        if (occ > 0 || mColumnNames.count(name) > 1) {
            text = tr("%1 (occurrence %2)").arg(name).arg(occ + 1);  // cn:%1（第 %2 个同名列）
        }
        mComboColumn->addItem(text, QVariant(i));
    }
    mComboType = new QComboBox(this);
    mComboType->setObjectName(QStringLiteral("comboValidationType"));
    mComboType->addItem(tr("Numeric range"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Type::NumericRange)));  // cn:数值区间
    mComboType->addItem(tr("Date range"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Type::DateRange)));        // cn:日期区间
    mComboType->addItem(tr("Allowed values (enum)"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Type::EnumSet)));  // cn:枚举集合
    mComboType->addItem(tr("Not empty"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Type::NotEmpty)));          // cn:非空
    mComboType->addItem(tr("Unique"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Type::Unique)));               // cn:唯一
    mComboType->addItem(tr("Regex (full match)"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Type::Regex)));    // cn:正则（整格匹配）
    mComboLevel = new QComboBox(this);
    mComboLevel->setObjectName(QStringLiteral("comboValidationLevel"));
    mComboLevel->addItem(tr("Error"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Level::Error)));    // cn:错误
    mComboLevel->addItem(tr("Warning"), QVariant::fromValue(static_cast< int >(DATableValidationRule::Level::Warning)));  // cn:警告
    mCheckEnabled = new QCheckBox(tr("Enabled"), this);  // cn:启用
    mCheckEnabled->setChecked(true);
    mEditLabel = new QLineEdit(this);
    mEditLabel->setObjectName(QStringLiteral("editValidationLabel"));
    mEditLabel->setPlaceholderText(tr("Display name in report (optional)"));  // cn:报告显示名（可选）
    form->addRow(tr("Column"), mComboColumn);    // cn:列
    form->addRow(tr("Type"), mComboType);        // cn:类型
    form->addRow(tr("Level"), mComboLevel);      // cn:级别
    form->addRow(QString(), mCheckEnabled);
    form->addRow(tr("Name"), mEditLabel);        // cn:名称
    // 参数堆叠页
    mParamStack = new QStackedWidget(this);
    // 数值区间
    QWidget* numPage  = new QWidget(mParamStack);
    QFormLayout* numF = new QFormLayout(numPage);
    mEditNumMin       = new QLineEdit(numPage);
    mEditNumMax       = new QLineEdit(numPage);
    mEditNumMin->setObjectName(QStringLiteral("editNumericMin"));
    mEditNumMax->setObjectName(QStringLiteral("editNumericMax"));
    mEditNumMin->setPlaceholderText(tr("Empty = no lower bound"));  // cn:留空 = 不限下界
    mEditNumMax->setPlaceholderText(tr("Empty = no upper bound"));  // cn:留空 = 不限上界
    numF->addRow(tr("Minimum"), mEditNumMin);  // cn:下界
    numF->addRow(tr("Maximum"), mEditNumMax);  // cn:上界
    mParamStack->insertWidget(PageNumericRange, numPage);
    // 日期区间
    QWidget* datePage  = new QWidget(mParamStack);
    QFormLayout* dateF = new QFormLayout(datePage);
    mEditDateMin       = new QLineEdit(datePage);
    mEditDateMax       = new QLineEdit(datePage);
    mEditDateMin->setObjectName(QStringLiteral("editDateMin"));
    mEditDateMax->setObjectName(QStringLiteral("editDateMax"));
    mEditDateMin->setPlaceholderText(QStringLiteral("yyyy-MM-dd[ HH:mm:ss]"));
    mEditDateMax->setPlaceholderText(QStringLiteral("yyyy-MM-dd[ HH:mm:ss]"));
    dateF->addRow(tr("From"), mEditDateMin);  // cn:起始
    dateF->addRow(tr("To"), mEditDateMax);    // cn:截止
    mParamStack->insertWidget(PageDateRange, datePage);
    // 枚举
    QWidget* enumPage  = new QWidget(mParamStack);
    QVBoxLayout* enumL = new QVBoxLayout(enumPage);
    mEditEnumValues    = new QPlainTextEdit(enumPage);
    mEditEnumValues->setObjectName(QStringLiteral("editEnumValues"));
    mEditEnumValues->setPlaceholderText(tr("One allowed value per line"));  // cn:每行一个允许值
    mCheckEnumAllowOther = new QCheckBox(tr("Allow other values when editing (dropdown)"), enumPage);  // cn:编辑时允许其他值（下拉）
    mCheckEnumAllowOther->setObjectName(QStringLiteral("checkEnumAllowOther"));
    mCheckEnumAllowOther->setChecked(true);
    enumL->addWidget(mEditEnumValues);
    enumL->addWidget(mCheckEnumAllowOther);
    mParamStack->insertWidget(PageEnum, enumPage);
    // 无参数页（非空/唯一）
    QWidget* noPage   = new QWidget(mParamStack);
    QLabel* noLabel   = new QLabel(tr("No parameters"), noPage);  // cn:无参数
    QVBoxLayout* noL  = new QVBoxLayout(noPage);
    noL->addWidget(noLabel);
    noL->addStretch();
    mParamStack->insertWidget(PageNoParam, noPage);
    // 正则
    QWidget* regexPage  = new QWidget(mParamStack);
    QFormLayout* regexF = new QFormLayout(regexPage);
    mEditRegex          = new QLineEdit(regexPage);
    mEditRegex->setObjectName(QStringLiteral("editRegex"));
    mEditRegex->setPlaceholderText(tr("Full-match pattern, e.g. [A-Z]{3}\\d+"));  // cn:整格匹配正则，如 [A-Z]{3}\d+
    regexF->addRow(tr("Pattern"), mEditRegex);  // cn:正则
    mParamStack->insertWidget(PageRegex, regexPage);
    form->addRow(tr("Parameters"), mParamStack);  // cn:参数
    // 错误提示
    mLabelError = new QLabel(this);
    mLabelError->setObjectName(QStringLiteral("validationErrorLabel"));
    mLabelError->setStyleSheet(QStringLiteral("color:#CE6043;"));  // 官方橙红（错误语义，DESIGN.md §2）
    mLabelError->setWordWrap(true);
    mLabelError->hide();
    form->addRow(QString(), mLabelError);
    body->addLayout(form, 1);
    main->addLayout(body, 1);
    // 按钮
    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    box->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("buttonOk"));
    main->addWidget(box);
    connect(box, &QDialogButtonBox::accepted, this, &DADialogValidationRules::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // 编辑器信号
    connect(mListRules, &QListWidget::currentRowChanged, this, &DADialogValidationRules::onRuleListCurrentChanged);
    connect(mBtnAdd, &QPushButton::clicked, this, &DADialogValidationRules::onAddRule);
    connect(mBtnRemove, &QPushButton::clicked, this, &DADialogValidationRules::onRemoveRule);
    connect(mBtnUp, &QPushButton::clicked, this, &DADialogValidationRules::onMoveUp);
    connect(mBtnDown, &QPushButton::clicked, this, &DADialogValidationRules::onMoveDown);
    connect(mComboType, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogValidationRules::onTypeChanged);
    auto editorChanged = [ this ]() {
        onEditorChanged();
    };
    connect(mComboColumn, QOverload< int >::of(&QComboBox::currentIndexChanged), this, editorChanged);
    connect(mComboLevel, QOverload< int >::of(&QComboBox::currentIndexChanged), this, editorChanged);
    connect(mCheckEnabled, &QCheckBox::toggled, this, editorChanged);
    connect(mEditLabel, &QLineEdit::textChanged, this, editorChanged);
    connect(mEditNumMin, &QLineEdit::textChanged, this, editorChanged);
    connect(mEditNumMax, &QLineEdit::textChanged, this, editorChanged);
    connect(mEditDateMin, &QLineEdit::textChanged, this, editorChanged);
    connect(mEditDateMax, &QLineEdit::textChanged, this, editorChanged);
    connect(mEditEnumValues, &QPlainTextEdit::textChanged, this, editorChanged);
    connect(mCheckEnumAllowOther, &QCheckBox::toggled, this, editorChanged);
    connect(mEditRegex, &QLineEdit::textChanged, this, editorChanged);
    resize(720, 480);
}

/**
 * @brief 重建左侧规则列表（保持选中跟随 mEditIndex）
 */
void DADialogValidationRules::rebuildRuleList()
{
    const bool wasUpdating = mUpdatingUi;
    mUpdatingUi            = true;
    mListRules->clear();
    for (const DATableValidationRule& r : std::as_const(mWorking)) {
        mListRules->addItem(ruleSummary(r));
    }
    if (mEditIndex >= 0 && mEditIndex < mListRules->count()) {
        mListRules->setCurrentRow(mEditIndex);
    }
    mUpdatingUi = wasUpdating;
}

/**
 * @brief 规则摘要（列表项显示）：[级别] 列（第n个） 类型 参数
 */
QString DADialogValidationRules::ruleSummary(const DATableValidationRule& rule) const
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
    const QString levelName = rule.level == DATableValidationRule::Level::Warning ? tr("Warning")  // cn:警告
                                                                                   : tr("Error");  // cn:错误
    QString colText         = rule.columnName;
    if (rule.columnOccurrence > 0) {
        colText = tr("%1 (occurrence %2)").arg(rule.columnName).arg(rule.columnOccurrence + 1);  // cn:%1（第 %2 个同名列）
    }
    QString summary = QStringLiteral("[%1] %2 · %3").arg(levelName, colText, typeName);
    const QString params = rule.paramSummary();
    if (!params.isEmpty()) {
        summary += QLatin1Char(' ') + params;
    }
    if (!rule.label.isEmpty()) {
        summary += QStringLiteral(" — ") + rule.label;
    }
    if (!rule.enabled) {
        summary += tr(" (disabled)");  // cn:（已停用）
    }
    return summary;
}

/**
 * @brief 列表选中变化：先提交旧选中项编辑器，再装载新规则
 */
void DADialogValidationRules::onRuleListCurrentChanged(int row)
{
    if (mUpdatingUi) {
        return;
    }
    // 提交上一个选中项（参数非法不阻止切换，保留工作集原值）
    if (mEditIndex >= 0 && mEditIndex < mWorking.size()) {
        commitEditorToWorking(nullptr);
    }
    mEditIndex = (row >= 0 && row < mWorking.size()) ? row : -1;
    if (mEditIndex >= 0) {
        loadRuleToEditor(mWorking[ mEditIndex ]);
    }
    updateButtonStates();
}

void DADialogValidationRules::onAddRule()
{
    // 提交当前编辑
    if (mEditIndex >= 0 && mEditIndex < mWorking.size()) {
        commitEditorToWorking(nullptr);
    }
    DATableValidationRule r;
    r.type = DATableValidationRule::Type::NumericRange;
    // 默认列 = 下拉当前列
    QString name;
    int occ = 0;
    currentColumnChoice(&name, &occ);
    r.columnName       = name;
    r.columnOccurrence = occ;
    mWorking.append(r);
    mEditIndex = mWorking.size() - 1;
    rebuildRuleList();
    mListRules->setCurrentRow(mEditIndex);
    loadRuleToEditor(mWorking[ mEditIndex ]);
    updateButtonStates();
}

void DADialogValidationRules::onRemoveRule()
{
    if (mEditIndex < 0 || mEditIndex >= mWorking.size()) {
        return;
    }
    mWorking.removeAt(mEditIndex);
    mEditIndex = qMin(mEditIndex, mWorking.size() - 1);
    rebuildRuleList();
    if (mEditIndex >= 0) {
        mListRules->setCurrentRow(mEditIndex);
        loadRuleToEditor(mWorking[ mEditIndex ]);
    }
    updateButtonStates();
}

void DADialogValidationRules::onMoveUp()
{
    if (mEditIndex <= 0 || mEditIndex >= mWorking.size()) {
        return;
    }
    mWorking.swapItemsAt(mEditIndex, mEditIndex - 1);
    --mEditIndex;
    rebuildRuleList();
    mListRules->setCurrentRow(mEditIndex);
}

void DADialogValidationRules::onMoveDown()
{
    if (mEditIndex < 0 || mEditIndex >= mWorking.size() - 1) {
        return;
    }
    mWorking.swapItemsAt(mEditIndex, mEditIndex + 1);
    ++mEditIndex;
    rebuildRuleList();
    mListRules->setCurrentRow(mEditIndex);
}

/**
 * @brief 类型切换 → 参数页切换 + 提交
 */
void DADialogValidationRules::onTypeChanged(int comboIndex)
{
    Q_UNUSED(comboIndex);
    updateParamPages();
    onEditorChanged();
}

/**
 * @brief 编辑器任意变化 → 提交回工作集并刷新列表摘要（参数非法只标错，不丢已输入内容）
 */
void DADialogValidationRules::onEditorChanged()
{
    if (mUpdatingUi || mEditIndex < 0 || mEditIndex >= mWorking.size()) {
        return;
    }
    QString err;
    if (commitEditorToWorking(&err)) {
        mLabelError->hide();
        const int keepRow = mListRules->currentRow();
        mUpdatingUi       = true;
        if (QListWidgetItem* item = mListRules->item(keepRow)) {
            item->setText(ruleSummary(mWorking[ mEditIndex ]));
        }
        mUpdatingUi = false;
    } else {
        mLabelError->setText(err);
        mLabelError->show();
    }
}

/**
 * @brief 装载规则到右侧编辑器（置守卫屏蔽信号）
 */
void DADialogValidationRules::loadRuleToEditor(const DATableValidationRule& rule)
{
    mUpdatingUi = true;
    // 列下拉定位到 (列名, occurrence)
    int colComboIndex = 0;
    QHash< QString, int > seen;
    for (int i = 0; i < mColumnNames.size(); ++i) {
        const QString name = mColumnNames[ i ];
        const int occ      = seen.value(name, 0);
        seen[ name ]       = occ + 1;
        if (name == rule.columnName && occ == rule.columnOccurrence) {
            colComboIndex = i;
            break;
        }
    }
    mComboColumn->setCurrentIndex(colComboIndex);
    // 类型
    for (int i = 0; i < mComboType->count(); ++i) {
        if (mComboType->itemData(i).toInt() == static_cast< int >(rule.type)) {
            mComboType->setCurrentIndex(i);
            break;
        }
    }
    for (int i = 0; i < mComboLevel->count(); ++i) {
        if (mComboLevel->itemData(i).toInt() == static_cast< int >(rule.level)) {
            mComboLevel->setCurrentIndex(i);
            break;
        }
    }
    mCheckEnabled->setChecked(rule.enabled);
    mEditLabel->setText(rule.label);
    mEditNumMin->setText(rule.hasNumericMin ? QString::number(rule.numericMin) : QString());
    mEditNumMax->setText(rule.hasNumericMax ? QString::number(rule.numericMax) : QString());
    mEditDateMin->setText(rule.dateMin.isValid() ? rule.dateMin.toString(QStringLiteral("yyyy-MM-dd")) : QString());
    mEditDateMax->setText(rule.dateMax.isValid() ? rule.dateMax.toString(QStringLiteral("yyyy-MM-dd")) : QString());
    mEditEnumValues->setPlainText(rule.enumValues.join(QLatin1Char('\n')));
    mCheckEnumAllowOther->setChecked(rule.enumAllowOther);
    mEditRegex->setText(rule.regexPattern);
    mUpdatingUi = false;
    updateParamPages();
    mLabelError->hide();
}

/**
 * @brief 编辑器当前值写回工作集选中项
 * @param err 非空时输出参数错误描述
 * @return 参数合法且已写回返回 true；非法时不写回返回 false
 */
bool DADialogValidationRules::commitEditorToWorking(QString* err)
{
    if (mEditIndex < 0 || mEditIndex >= mWorking.size()) {
        return false;
    }
    DATableValidationRule r = mWorking[ mEditIndex ];
    QString name;
    int occ = 0;
    currentColumnChoice(&name, &occ);
    r.columnName       = name;
    r.columnOccurrence = occ;
    r.type             = static_cast< DATableValidationRule::Type >(mComboType->currentData().toInt());
    r.level            = static_cast< DATableValidationRule::Level >(mComboLevel->currentData().toInt());
    r.enabled          = mCheckEnabled->isChecked();
    r.label            = mEditLabel->text().trimmed();
    // 参数
    const QString minTxt = mEditNumMin->text().trimmed();
    const QString maxTxt = mEditNumMax->text().trimmed();
    r.hasNumericMin      = !minTxt.isEmpty();
    r.hasNumericMax      = !maxTxt.isEmpty();
    bool ok              = false;
    if (r.hasNumericMin) {
        r.numericMin = minTxt.toDouble(&ok);
        if (!ok) {
            if (err) {
                *err = tr("Minimum is not a valid number");  // cn:下界不是有效数值
            }
            return false;
        }
    }
    if (r.hasNumericMax) {
        r.numericMax = maxTxt.toDouble(&ok);
        if (!ok) {
            if (err) {
                *err = tr("Maximum is not a valid number");  // cn:上界不是有效数值
            }
            return false;
        }
    }
    const QString dminTxt = mEditDateMin->text().trimmed();
    const QString dmaxTxt = mEditDateMax->text().trimmed();
    r.dateMin             = dminTxt.isEmpty() ? QDateTime() : QDateTime::fromString(dminTxt, QStringLiteral("yyyy-MM-dd"));
    if (r.dateMin.isNull() && !dminTxt.isEmpty()) {
        r.dateMin = QDateTime::fromString(dminTxt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    if (!dminTxt.isEmpty() && !r.dateMin.isValid()) {
        if (err) {
            *err = tr("From-date is invalid (use yyyy-MM-dd)");  // cn:起始日期无效（用 yyyy-MM-dd）
        }
        return false;
    }
    r.dateMax = dmaxTxt.isEmpty() ? QDateTime() : QDateTime::fromString(dmaxTxt, QStringLiteral("yyyy-MM-dd"));
    if (r.dateMax.isNull() && !dmaxTxt.isEmpty()) {
        r.dateMax = QDateTime::fromString(dmaxTxt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    if (!dmaxTxt.isEmpty() && !r.dateMax.isValid()) {
        if (err) {
            *err = tr("To-date is invalid (use yyyy-MM-dd)");  // cn:截止日期无效（用 yyyy-MM-dd）
        }
        return false;
    }
    QStringList enumVals;
    const QStringList lines = mEditEnumValues->toPlainText().split(QLatin1Char('\n'));
    for (const QString& l : lines) {
        const QString t = l.trimmed();
        if (!t.isEmpty()) {
            enumVals.append(t);
        }
    }
    r.enumValues     = enumVals;
    r.enumAllowOther = mCheckEnumAllowOther->isChecked();
    r.regexPattern   = mEditRegex->text();
    // 完整性
    if (!r.isValid()) {
        if (err) {
            *err = tr("Rule parameters are incomplete (both bounds empty / no allowed values / empty pattern)");  // cn:规则参数不完整（上下界全空/无允许值/正则为空）
        }
        return false;
    }
    mWorking[ mEditIndex ] = r;
    return true;
}

/**
 * @brief 类型 → 参数页切换
 */
void DADialogValidationRules::updateParamPages()
{
    const auto type = static_cast< DATableValidationRule::Type >(mComboType->currentData().toInt());
    switch (type) {
    case DATableValidationRule::Type::NumericRange:
        mParamStack->setCurrentIndex(PageNumericRange);
        break;
    case DATableValidationRule::Type::DateRange:
        mParamStack->setCurrentIndex(PageDateRange);
        break;
    case DATableValidationRule::Type::EnumSet:
        mParamStack->setCurrentIndex(PageEnum);
        break;
    case DATableValidationRule::Type::Regex:
        mParamStack->setCurrentIndex(PageRegex);
        break;
    case DATableValidationRule::Type::NotEmpty:
    case DATableValidationRule::Type::Unique:
    default:
        mParamStack->setCurrentIndex(PageNoParam);
        break;
    }
}

void DADialogValidationRules::updateButtonStates()
{
    const bool hasSel = mEditIndex >= 0 && mEditIndex < mWorking.size();
    mBtnRemove->setEnabled(hasSel);
    mBtnUp->setEnabled(hasSel && mEditIndex > 0);
    mBtnDown->setEnabled(hasSel && mEditIndex < mWorking.size() - 1);
    const bool editorOn = hasSel;
    mComboColumn->setEnabled(editorOn);
    mComboType->setEnabled(editorOn);
    mComboLevel->setEnabled(editorOn);
    mCheckEnabled->setEnabled(editorOn);
    mEditLabel->setEnabled(editorOn);
    mParamStack->setEnabled(editorOn);
}

/**
 * @brief 列下拉当前选中 → (列名, occurrence)
 */
void DADialogValidationRules::currentColumnChoice(QString* name, int* occurrence) const
{
    const int pos = mComboColumn->currentData().toInt();
    if (pos < 0 || pos >= mColumnNames.size()) {
        if (name) {
            *name = QString();
        }
        if (occurrence) {
            *occurrence = 0;
        }
        return;
    }
    const QString colName = mColumnNames[ pos ];
    int occ               = 0;
    for (int i = 0; i < pos; ++i) {
        if (mColumnNames[ i ] == colName) {
            ++occ;
        }
    }
    if (name) {
        *name = colName;
    }
    if (occurrence) {
        *occurrence = occ;
    }
}

/**
 * @brief 确定：提交当前编辑器（参数非法阻止关闭并标错），结果交调用方 diff 后入栈
 */
void DADialogValidationRules::accept()
{
    if (mEditIndex >= 0 && mEditIndex < mWorking.size()) {
        QString err;
        if (!commitEditorToWorking(&err)) {
            mLabelError->setText(err.isEmpty() ? tr("Invalid rule parameters")  // cn:规则参数无效
                                               : err);
            mLabelError->show();
            return;
        }
    }
    QDialog::accept();
}

QList< DATableValidationRule > DADialogValidationRules::resultRules() const
{
    return mWorking;
}

}  // end of namespace DA
