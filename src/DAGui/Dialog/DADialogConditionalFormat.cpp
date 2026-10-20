#include "DADialogConditionalFormat.h"
#include "DATableConditionalFormatStore.h"
#include "DAItemListEditWidget.h"
#include "DAColorPickerButton.h"
#include "DAColorScalePreviewWidget.h"
// qt
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QStackedWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QStandardItemModel>
#include <QFontDialog>
#include <QMessageBox>
#include <algorithm>

namespace DA
{

namespace
{
// 列表项数据角色：列索引 / 规则在该列内的序号
constexpr int c_roleColumn = Qt::UserRole;
constexpr int c_roleIndex  = Qt::UserRole + 1;
}  // namespace

/**
 * @brief 构造函数：代码构建渐进式 UI（列/类型内容在 setup 填充）
 * @param store 条件格式子存储（非拥有，预览叠加通道，可空）
 * @param parent 父窗口
 */
DADialogConditionalFormat::DADialogConditionalFormat(DATableConditionalFormatStore* store, QWidget* parent)
    : QDialog(parent), mStore(store)
{
    buildUi();
    connectUi();
}

DADialogConditionalFormat::~DADialogConditionalFormat()
{
    // 兜底：未经 accept/reject 直接销毁（如 deleteLater）时也要还原预览
    clearStorePreview();
}

/**
 * @brief 代码构建渐进式布局（自上而下：类型 → 列 → 条件与格式 → 现有规则）
 */
void DADialogConditionalFormat::buildUi()
{
    setWindowTitle(tr("Conditional Formatting Rules"));  // cn:条件格式规则
    resize(560, 720);
    QVBoxLayout* root = new QVBoxLayout(this);

    // ① 规则类型
    QGroupBox* gType   = new QGroupBox(tr("Rule Type"), this);  // cn:规则类型
    QHBoxLayout* typeL = new QHBoxLayout(gType);
    mComboType         = new QComboBox(gType);
    mComboType->setObjectName(QStringLiteral("comboType"));  // 单测经 objectName 取控件
    mComboType->addItem(tr("Cell Rule"));    // cn:单元格规则
    mComboType->addItem(tr("Color Scale"));  // cn:色阶
    mComboType->addItem(tr("Data Bar"));     // cn:数据条
    typeL->addWidget(mComboType, 1);
    root->addWidget(gType);

    // ② 应用列（通用列选择控件：行内删除/菜单添加/搜索多选/移除所有）
    QGroupBox* gCols   = new QGroupBox(tr("Applies To Columns"), this);  // cn:应用列
    QVBoxLayout* colsL = new QVBoxLayout(gCols);
    mColumnWidget      = new DAItemListEditWidget(gCols);
    mColumnWidget->setMaximumHeight(150);
    colsL->addWidget(mColumnWidget);
    root->addWidget(gCols);

    // ③ 条件与格式（按类型切页）
    QGroupBox* gCond   = new QGroupBox(tr("Condition and Format"), this);  // cn:条件与格式
    QVBoxLayout* condL = new QVBoxLayout(gCond);
    mStackEditor       = new QStackedWidget(gCond);
    // —— 单元格规则页 ——
    QWidget* pageCell     = new QWidget(mStackEditor);
    QFormLayout* cellForm = new QFormLayout(pageCell);
    mComboCondition       = new QComboBox(pageCell);
    mComboCondition->addItem(tr("greater than"));     // cn:大于
    mComboCondition->addItem(tr("less than"));        // cn:小于
    mComboCondition->addItem(tr("between"));          // cn:介于
    mComboCondition->addItem(tr("equal to"));         // cn:等于
    mComboCondition->addItem(tr("contains"));         // cn:包含
    mComboCondition->addItem(tr("is empty"));         // cn:为空
    mComboCondition->addItem(tr("is not empty"));     // cn:非空
    mComboCondition->addItem(tr("a date occurring"));  // cn:发生日期
    mComboCondition->addItem(tr("duplicate values"));  // cn:重复值
    mComboCondition->addItem(tr("unique values"));     // cn:唯一值
    cellForm->addRow(tr("Condition:"), mComboCondition);  // cn:条件：
    mRowDateOcc  = new QWidget(pageCell);
    mComboDateOcc = new QComboBox(mRowDateOcc);
    for (int i = 0; i < 10; ++i) {
        mComboDateOcc->addItem(dateOccurrenceName(i));
    }
    QHBoxLayout* dateL = new QHBoxLayout(mRowDateOcc);
    dateL->setContentsMargins(0, 0, 0, 0);
    dateL->addWidget(new QLabel(tr("Date:"), mRowDateOcc), 0);  // cn:日期：
    dateL->addWidget(mComboDateOcc, 1);
    cellForm->addRow(mRowDateOcc);
    mRowValue1   = new QWidget(pageCell);
    mLabelValue1 = new QLabel(tr("Value:"), mRowValue1);  // cn:值：
    mEditValue1  = new QLineEdit(mRowValue1);
    QHBoxLayout* v1L = new QHBoxLayout(mRowValue1);
    v1L->setContentsMargins(0, 0, 0, 0);
    v1L->addWidget(mLabelValue1, 0);
    v1L->addWidget(mEditValue1, 1);
    cellForm->addRow(mRowValue1);
    mRowValue2   = new QWidget(pageCell);
    mLabelValue2 = new QLabel(tr("And (upper bound):"), mRowValue2);  // cn:且（上界）：
    mEditValue2  = new QLineEdit(mRowValue2);
    QHBoxLayout* v2L = new QHBoxLayout(mRowValue2);
    v2L->setContentsMargins(0, 0, 0, 0);
    v2L->addWidget(mLabelValue2, 0);
    v2L->addWidget(mEditValue2, 1);
    cellForm->addRow(mRowValue2);
    QWidget* styleRow      = new QWidget(pageCell);
    QHBoxLayout* styleL    = new QHBoxLayout(styleRow);
    styleL->setContentsMargins(0, 0, 0, 0);
    mCheckUseBackground    = new QCheckBox(tr("Background"), styleRow);  // cn:背景
    mBtnBackground         = new DAColorPickerButton(styleRow);
    mCheckUseForeground    = new QCheckBox(tr("Foreground"), styleRow);  // cn:前景
    mBtnForeground         = new DAColorPickerButton(styleRow);
    mCheckUseFont          = new QCheckBox(tr("Font"), styleRow);  // cn:字体
    mBtnFont               = new QPushButton(tr("Font..."), styleRow);  // cn:字体...
    styleL->addWidget(mCheckUseBackground);
    styleL->addWidget(mBtnBackground);
    styleL->addWidget(mCheckUseForeground);
    styleL->addWidget(mBtnForeground);
    styleL->addWidget(mCheckUseFont);
    styleL->addWidget(mBtnFont);
    styleL->addStretch(1);
    cellForm->addRow(tr("Style:"), styleRow);  // cn:样式：
    mStackEditor->addWidget(pageCell);
    // —— 色阶页 ——
    QWidget* pageScale     = new QWidget(mStackEditor);
    QVBoxLayout* scaleL    = new QVBoxLayout(pageScale);
    QWidget* radioRow      = new QWidget(pageScale);
    QHBoxLayout* radioL    = new QHBoxLayout(radioRow);
    radioL->setContentsMargins(0, 0, 0, 0);
    mRadioTwoColor         = new QRadioButton(tr("2-Color Scale"), radioRow);  // cn:双色色阶
    mRadioThreeColor       = new QRadioButton(tr("3-Color Scale"), radioRow);  // cn:三色色阶
    mRadioTwoColor->setChecked(true);
    radioL->addWidget(mRadioTwoColor);
    radioL->addWidget(mRadioThreeColor);
    radioL->addStretch(1);
    scaleL->addWidget(radioRow);
    QWidget* colorRow    = new QWidget(pageScale);
    QHBoxLayout* colorL  = new QHBoxLayout(colorRow);
    colorL->setContentsMargins(0, 0, 0, 0);
    mBtnScaleMin         = new DAColorPickerButton(colorRow);
    mBtnScaleMid         = new DAColorPickerButton(colorRow);
    mBtnScaleMax         = new DAColorPickerButton(colorRow);
    colorL->addWidget(new QLabel(tr("Low:"), colorRow), 0);  // cn:低：
    colorL->addWidget(mBtnScaleMin, 1);
    colorL->addWidget(new QLabel(tr("Middle:"), colorRow), 0);  // cn:中：
    colorL->addWidget(mBtnScaleMid, 1);
    colorL->addWidget(new QLabel(tr("High:"), colorRow), 0);  // cn:高：
    colorL->addWidget(mBtnScaleMax, 1);
    scaleL->addWidget(colorRow);
    mScalePreview = new DAColorScalePreviewWidget(pageScale);
    scaleL->addWidget(mScalePreview);
    scaleL->addStretch(1);
    mStackEditor->addWidget(pageScale);
    // —— 数据条页 ——
    QWidget* pageBar     = new QWidget(mStackEditor);
    QHBoxLayout* barL    = new QHBoxLayout(pageBar);
    mBtnBarColor         = new DAColorPickerButton(pageBar);
    mCheckBarGradient    = new QCheckBox(tr("Gradient Fill"), pageBar);  // cn:渐变填充
    barL->addWidget(new QLabel(tr("Bar Color:"), pageBar), 0);  // cn:条颜色：
    barL->addWidget(mBtnBarColor, 1);
    barL->addWidget(mCheckBarGradient);
    barL->addStretch(1);
    mStackEditor->addWidget(pageBar);
    condL->addWidget(mStackEditor);
    mLabelDisabledHint = new QLabel(gCond);
    mLabelDisabledHint->setObjectName(QStringLiteral("labelDisabledHint"));  // 单测经 objectName 取控件
    mLabelDisabledHint->setWordWrap(true);
    mLabelDisabledHint->setVisible(false);
    condL->addWidget(mLabelDisabledHint);
    root->addWidget(gCond);

    // ④ 现有规则（优先级顺序）
    QGroupBox* gRules = new QGroupBox(
        tr("Existing Rules (top to bottom = evaluation order, later rules override earlier ones)"), this);  // cn:现有规则（自上而下=求值顺序，靠后规则覆盖靠前）
    QVBoxLayout* rulesL = new QVBoxLayout(gRules);
    mCheckEnabled       = new QCheckBox(tr("Enabled"), gRules);  // cn:启用
    mCheckEnabled->setChecked(true);
    rulesL->addWidget(mCheckEnabled);
    mListRules = new QListWidget(gRules);
    mListRules->setMinimumHeight(120);
    rulesL->addWidget(mListRules, 1);
    QWidget* btnRow   = new QWidget(gRules);
    QHBoxLayout* btnL = new QHBoxLayout(btnRow);
    btnL->setContentsMargins(0, 0, 0, 0);
    mBtnAdd           = new QPushButton(tr("Add Rule"), btnRow);  // cn:添加规则
    mBtnAdd->setObjectName(QStringLiteral("buttonAdd"));  // 单测经 objectName 取控件
    mBtnRemove        = new QPushButton(tr("Remove"), btnRow);    // cn:删除
    mBtnMoveUp        = new QPushButton(tr("Move Up"), btnRow);   // cn:上移
    mBtnMoveDown      = new QPushButton(tr("Move Down"), btnRow);  // cn:下移
    mBtnClearColumn   = new QPushButton(tr("Clear Rules of Selected Column"), btnRow);  // cn:清除所选列规则
    btnL->addWidget(mBtnAdd);
    btnL->addWidget(mBtnRemove);
    btnL->addWidget(mBtnMoveUp);
    btnL->addWidget(mBtnMoveDown);
    btnL->addWidget(mBtnClearColumn);
    btnL->addStretch(1);
    rulesL->addWidget(btnRow);
    root->addWidget(gRules, 1);

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(mButtonBox);
}

/**
 * @brief 信号接线
 */
void DADialogConditionalFormat::connectUi()
{
    connect(mListRules, &QListWidget::currentRowChanged, this, &DADialogConditionalFormat::onRuleListCurrentChanged);
    connect(mBtnAdd, &QPushButton::clicked, this, &DADialogConditionalFormat::onAddRule);
    connect(mBtnRemove, &QPushButton::clicked, this, &DADialogConditionalFormat::onRemoveRule);
    connect(mBtnMoveUp, &QPushButton::clicked, this, &DADialogConditionalFormat::onMoveUp);
    connect(mBtnMoveDown, &QPushButton::clicked, this, &DADialogConditionalFormat::onMoveDown);
    connect(mBtnClearColumn, &QPushButton::clicked, this, &DADialogConditionalFormat::onClearColumnRules);
    connect(mComboType, QOverload< int >::of(&QComboBox::currentIndexChanged), this,
            &DADialogConditionalFormat::onEditorChanged);
    connect(mColumnWidget, &DAItemListEditWidget::selectedItemsChanged, this,
            &DADialogConditionalFormat::onColumnsChanged);
    connect(mCheckEnabled, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mComboCondition, QOverload< int >::of(&QComboBox::currentIndexChanged), this,
            &DADialogConditionalFormat::onEditorChanged);
    connect(mComboDateOcc, QOverload< int >::of(&QComboBox::currentIndexChanged), this,
            &DADialogConditionalFormat::onEditorChanged);
    connect(mEditValue1, &QLineEdit::textEdited, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mEditValue2, &QLineEdit::textEdited, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mCheckUseBackground, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mCheckUseForeground, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mCheckUseFont, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mRadioTwoColor, &QRadioButton::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mCheckBarGradient, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    // 颜色按钮（DAColorPickerButton 自带调色菜单）→ 编辑器变更
    connect(mBtnBackground, &DAColorPickerButton::colorChanged, this, [ this ]() {
        mCheckUseBackground->setChecked(true);
        onEditorChanged();
    });
    connect(mBtnForeground, &DAColorPickerButton::colorChanged, this, [ this ]() {
        mCheckUseForeground->setChecked(true);
        onEditorChanged();
    });
    connect(mBtnScaleMin, &DAColorPickerButton::colorChanged, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mBtnScaleMid, &DAColorPickerButton::colorChanged, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mBtnScaleMax, &DAColorPickerButton::colorChanged, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mBtnBarColor, &DAColorPickerButton::colorChanged, this, &DADialogConditionalFormat::onEditorChanged);
    connect(mBtnFont, &QPushButton::clicked, this, [ this ]() {
        bool ok = false;
        QFont f = QFontDialog::getFont(&ok, mSwatchFont, this, tr("Select Font"));  // cn:选择字体
        if (ok) {
            mSwatchFont = f;
            mCheckUseFont->setChecked(true);
            mBtnFont->setText(QStringLiteral("%1 %2").arg(mSwatchFont.family()).arg(mSwatchFont.pointSize()));
            onEditorChanged();
        }
    });
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/**
 * @brief (re)初始化对话框状态
 * @param columnNames 全部列名（位置索引对齐）
 * @param scopeColumns 本次编辑的列集合（选区覆盖列，预填入列选择控件）
 * @param scaleBarAvailable 色阶/数据条是否可用（惰性/只读源 false，§1.4-2）
 * @param initialType 预设规则类型（DATableConditionalFormatRule::Type 整数值，-1=不预设）
 */
void DADialogConditionalFormat::setup(const QStringList& columnNames, const QList< int >& scopeColumns,
                                      bool scaleBarAvailable, int initialType)
{
    mColumnNames       = columnNames;
    mScaleBarAvailable = scaleBarAvailable;
    QList< int > scope = scopeColumns;
    std::sort(scope.begin(), scope.end());
    // 工作集初始化：store 已有规则的列 + scope 列（编辑不落库，确定才提交）
    mWorking.clear();
    QList< int > workingCols = mStore ? mStore->columnsWithRules() : QList< int >();
    for (int c : std::as_const(scope)) {
        if (!workingCols.contains(c)) {
            workingCols.append(c);
        }
    }
    std::sort(workingCols.begin(), workingCols.end());
    for (int col : std::as_const(workingCols)) {
        mWorking.insert(col, mStore ? mStore->columnRules(col) : QVector< DATableConditionalFormatRule >());
    }
    // 列选择控件：候选=全部列，预选=scope 列
    QVector< DAItemEntry > avail;
    for (int i = 0; i < mColumnNames.size(); ++i) {
        avail.append(DAItemEntry { mColumnNames.at(i), i });
    }
    mColumnWidget->setAvailableItems(avail);
    QVector< DAItemEntry > sel;
    for (int col : std::as_const(scope)) {
        const QString name = (col >= 0 && col < mColumnNames.size()) ? mColumnNames.at(col) : tr("Column %1").arg(col);
        sel.append(DAItemEntry { name, col });
    }
    mColumnWidget->setSelectedItems(sel);
    // 惰性/只读源：色阶与数据条置灰 + 说明文案（单元格规则正常提供）
    if (QStandardItemModel* m = qobject_cast< QStandardItemModel* >(mComboType->model())) {
        if (QStandardItem* item = m->item(1)) {
            item->setEnabled(scaleBarAvailable);
        }
        if (QStandardItem* item = m->item(2)) {
            item->setEnabled(scaleBarAvailable);
        }
    }
    mLabelDisabledHint->setVisible(!scaleBarAvailable);
    mLabelDisabledHint->setText(tr("Color scale and data bar need full-column statistics, "
                                   "which are unavailable for lazy-loaded or read-only data sources. "
                                   "Cell rules are still available."));  // cn:色阶与数据条需要全列统计，惰性加载或只读数据源不可用；单元格规则仍可正常使用
    // 预设类型（菜单「其他规则」入口）
    if (initialType >= 0 && initialType <= 2 && (initialType == 0 || scaleBarAvailable)) {
        mUpdatingUi = true;
        mComboType->setCurrentIndex(initialType);
        mStackEditor->setCurrentIndex(initialType);
        mUpdatingUi = false;
    }
    mEditCol   = -1;
    mEditIndex = -1;
    rebuildRuleList();
    updateValueEditsEnabled();
    updateScalePreview();
    // 初始预览 = 现状（无视觉变化），后续编辑实时叠加
    refreshPreview();
}

/**
 * @brief 编辑结果（列 → 规则列表；调用方 diff 生成撤销命令）
 * @return 工作规则集
 */
QHash< int, QVector< DATableConditionalFormatRule > > DADialogConditionalFormat::resultRules() const
{
    return mWorking;
}

/**
 * @brief 确定：提交编辑器 → 清预览 → 交调用方按差异入栈命令
 */
void DADialogConditionalFormat::accept()
{
    commitEditorToWorking();
    clearStorePreview();
    QDialog::accept();
}

/**
 * @brief 取消：不提交，清预览还原落库规则显示
 */
void DADialogConditionalFormat::reject()
{
    clearStorePreview();
    QDialog::reject();
}

/**
 * @brief 当前选中项对应的 (列, 规则索引)
 * @param column 输出列索引
 * @param index 输出规则索引
 * @return 有有效选中返回 true
 */
bool DADialogConditionalFormat::currentItemKey(int* column, int* index) const
{
    QListWidgetItem* item = mListRules->currentItem();
    if (!item) {
        return false;
    }
    const int col = item->data(c_roleColumn).toInt();
    const int idx = item->data(c_roleIndex).toInt();
    auto it       = mWorking.constFind(col);
    if (it == mWorking.constEnd() || idx < 0 || idx >= it.value().size()) {
        return false;
    }
    if (column) {
        *column = col;
    }
    if (index) {
        *index = idx;
    }
    return true;
}

/**
 * @brief 列选择控件当前选中的列索引列表
 * @return 列索引（控件列表序）
 */
QList< int > DADialogConditionalFormat::selectedColumns() const
{
    QList< int > cols;
    const QVector< DAItemEntry > sel = mColumnWidget->selectedItems();
    for (const DAItemEntry& e : sel) {
        cols.append(e.data.toInt());
    }
    return cols;
}

/**
 * @brief 重建规则列表（全工作集按列升序；选中项跟随 mEditCol/mEditIndex，无匹配不选中）
 */
void DADialogConditionalFormat::rebuildRuleList()
{
    mListRules->blockSignals(true);
    mListRules->clear();
    int selectRow = -1;
    int row       = 0;
    QList< int > cols = mWorking.keys();
    std::sort(cols.begin(), cols.end());
    for (int col : std::as_const(cols)) {
        const QVector< DATableConditionalFormatRule >& rules = mWorking.value(col);
        for (int i = 0; i < rules.size(); ++i) {
            QListWidgetItem* item = new QListWidgetItem(ruleSummary(col, rules.at(i)), mListRules);
            item->setData(c_roleColumn, col);
            item->setData(c_roleIndex, i);
            if (col == mEditCol && i == mEditIndex) {
                selectRow = row;
            }
            ++row;
        }
    }
    if (selectRow >= 0) {
        mListRules->setCurrentRow(selectRow);
    } else {
        mListRules->setCurrentRow(-1);
        mEditCol   = -1;
        mEditIndex = -1;
    }
    mListRules->blockSignals(false);
    // blockSignals 期间 currentRowChanged 不触发，这里主动装载选中项
    int col = -1, idx = -1;
    if (currentItemKey(&col, &idx)) {
        loadRuleToEditor(mWorking.value(col).at(idx), col, idx);
    }
    updateButtonStates();
}

/**
 * @brief 装载规则到编辑器（守卫期间屏蔽控件信号）
 * @param rule 规则
 * @param column 所属列
 * @param index 列内序号
 */
void DADialogConditionalFormat::loadRuleToEditor(const DATableConditionalFormatRule& rule, int column, int index)
{
    mUpdatingUi = true;
    mEditCol    = column;
    mEditIndex  = index;
    mComboType->setCurrentIndex(static_cast< int >(rule.type));
    mStackEditor->setCurrentIndex(static_cast< int >(rule.type));
    // 列选择控件 = 该规则所属列
    const QString name =
        (column >= 0 && column < mColumnNames.size()) ? mColumnNames.at(column) : tr("Column %1").arg(column);
    mColumnWidget->setSelectedItems({ DAItemEntry { name, column } });
    mCheckEnabled->setChecked(rule.enabled);
    mComboCondition->setCurrentIndex(static_cast< int >(rule.condition));
    if (rule.condition == DATableConditionalFormatRule::Condition::DateOccurring) {
        mComboDateOcc->setCurrentIndex(qBound(0, rule.value1.toInt(), 9));
    }
    mEditValue1->setText(rule.value1.isValid() ? rule.value1.toString() : QString());
    mEditValue2->setText(rule.value2.isValid() ? rule.value2.toString() : QString());
    mCheckUseBackground->setChecked(rule.style.backgroundValid());
    mCheckUseForeground->setChecked(rule.style.foregroundValid());
    mCheckUseFont->setChecked(rule.style.fontValid());
    if (rule.style.backgroundValid()) {
        mBtnBackground->setColor(rule.style.background().color());
    }
    if (rule.style.foregroundValid()) {
        mBtnForeground->setColor(rule.style.foreground());
    }
    if (rule.style.fontValid()) {
        mSwatchFont = rule.style.font();
        mBtnFont->setText(QStringLiteral("%1 %2").arg(mSwatchFont.family()).arg(mSwatchFont.pointSize()));
    }
    mRadioThreeColor->setChecked(rule.threeColorScale);
    mRadioTwoColor->setChecked(!rule.threeColorScale);
    mBtnScaleMin->setColor(rule.scaleMinColor);
    mBtnScaleMid->setColor(rule.scaleMidColor);
    mBtnScaleMax->setColor(rule.scaleMaxColor);
    mBtnBarColor->setColor(rule.barColor);
    mCheckBarGradient->setChecked(rule.barGradient);
    updateValueEditsEnabled();
    updateScalePreview();
    mUpdatingUi = false;
}

/**
 * @brief 编辑器当前值填入规则（类型/条件/值/样式/色阶/数据条）
 * @param rule 输出规则
 */
void DADialogConditionalFormat::fillRuleFromEditor(DATableConditionalFormatRule& rule) const
{
    rule.type    = static_cast< DATableConditionalFormatRule::Type >(mComboType->currentIndex());
    rule.enabled = mCheckEnabled->isChecked();
    rule.condition =
        static_cast< DATableConditionalFormatRule::Condition >(mComboCondition->currentIndex());
    switch (rule.condition) {
    case DATableConditionalFormatRule::Condition::DateOccurring:
        rule.value1 = mComboDateOcc->currentIndex();
        rule.value2 = QVariant();
        break;
    case DATableConditionalFormatRule::Condition::Duplicate:
    case DATableConditionalFormatRule::Condition::Unique:
    case DATableConditionalFormatRule::Condition::IsEmpty:
    case DATableConditionalFormatRule::Condition::IsNotEmpty:
        rule.value1 = QVariant();
        rule.value2 = QVariant();
        break;
    default: {
        const QString v1 = mEditValue1->text();
        const QString v2 = mEditValue2->text();
        rule.value1      = v1.isEmpty() ? QVariant() : QVariant(v1);
        rule.value2      = v2.isEmpty() ? QVariant() : QVariant(v2);
        break;
    }
    }
    DATableCellStyle s;
    if (mCheckUseBackground->isChecked()) {
        s.setBackground(QBrush(mBtnBackground->color()));
    }
    if (mCheckUseForeground->isChecked()) {
        s.setForeground(mBtnForeground->color());
    }
    if (mCheckUseFont->isChecked()) {
        s.setFont(mSwatchFont);
    }
    rule.style           = s;
    rule.threeColorScale = mRadioThreeColor->isChecked();
    rule.scaleMinColor   = mBtnScaleMin->color();
    rule.scaleMidColor   = mBtnScaleMid->color();
    rule.scaleMaxColor   = mBtnScaleMax->color();
    rule.barColor        = mBtnBarColor->color();
    rule.barGradient     = mCheckBarGradient->isChecked();
}

/**
 * @brief 把编辑器当前值写回工作集选中规则
 * @return 写回成功返回 true（无有效选中返回 false）
 */
bool DADialogConditionalFormat::commitEditorToWorking()
{
    int col = -1, idx = -1;
    if (!currentItemKey(&col, &idx)) {
        return false;
    }
    auto it = mWorking.find(col);
    if (it == mWorking.end() || idx < 0 || idx >= it.value().size()) {
        return false;
    }
    fillRuleFromEditor(it.value()[ idx ]);
    return true;
}

/**
 * @brief 工作集 → store 预览叠加（实时预览，不落库不入撤销栈）
 */
void DADialogConditionalFormat::refreshPreview()
{
    if (mStore) {
        mStore->setPreviewRules(mWorking);
    }
}

/**
 * @brief 清除 store 预览（确定/取消/析构兜底，还原落库规则显示）
 */
void DADialogConditionalFormat::clearStorePreview()
{
    if (mStore) {
        mStore->clearPreview();
    }
}

/**
 * @brief 按条件操作符刷新参数输入可用性（行容器显隐）
 */
void DADialogConditionalFormat::updateValueEditsEnabled()
{
    const DATableConditionalFormatRule::Condition c =
        static_cast< DATableConditionalFormatRule::Condition >(mComboCondition->currentIndex());
    const bool noValue = (c == DATableConditionalFormatRule::Condition::IsEmpty
                          || c == DATableConditionalFormatRule::Condition::IsNotEmpty
                          || c == DATableConditionalFormatRule::Condition::Duplicate
                          || c == DATableConditionalFormatRule::Condition::Unique);
    const bool between  = (c == DATableConditionalFormatRule::Condition::Between);
    const bool dateOcc  = (c == DATableConditionalFormatRule::Condition::DateOccurring);
    mRowDateOcc->setVisible(dateOcc);
    mRowValue1->setVisible(!noValue && !dateOcc);
    mRowValue2->setVisible(between);
}

/**
 * @brief 色阶预览条与中点色可用性刷新
 */
void DADialogConditionalFormat::updateScalePreview()
{
    const bool three = mRadioThreeColor->isChecked();
    mBtnScaleMid->setEnabled(three);
    QList< QColor > colors { mBtnScaleMin->color(), mBtnScaleMax->color() };
    if (three) {
        colors.insert(1, mBtnScaleMid->color());
    }
    mScalePreview->setColors(colors);
}

/**
 * @brief 列表/按钮可用性刷新
 */
void DADialogConditionalFormat::updateButtonStates()
{
    int col = -1, idx = -1;
    const bool has = currentItemKey(&col, &idx);
    mBtnRemove->setEnabled(has);
    mBtnMoveUp->setEnabled(has && idx > 0);
    mBtnMoveDown->setEnabled(has);  // 末尾判断需要列内数量，交给点击时兜底
    mBtnClearColumn->setEnabled(has || !selectedColumns().isEmpty());
    mCheckEnabled->setEnabled(has);
}

/**
 * @brief 规则摘要文本（列表项显示："[列名] 摘要"，停用加前缀）
 * @param column 列索引
 * @param rule 规则
 * @return 摘要
 */
QString DADialogConditionalFormat::ruleSummary(int column, const DATableConditionalFormatRule& rule) const
{
    const QString colName = (column >= 0 && column < mColumnNames.size())
                                ? mColumnNames.at(column)
                                : tr("Column %1").arg(column);
    QString desc;
    switch (rule.type) {
    case DATableConditionalFormatRule::Type::CellRule: {
        const QString v1 = rule.value1.isValid() ? rule.value1.toString() : QString();
        const QString v2 = rule.value2.isValid() ? rule.value2.toString() : QString();
        switch (rule.condition) {
        case DATableConditionalFormatRule::Condition::Greater:
            desc = tr("greater than %1").arg(v1);
            break;
        case DATableConditionalFormatRule::Condition::Less:
            desc = tr("less than %1").arg(v1);
            break;
        case DATableConditionalFormatRule::Condition::Between:
            desc = tr("between %1 and %2").arg(v1, v2);
            break;
        case DATableConditionalFormatRule::Condition::Equal:
            desc = tr("equal to %1").arg(v1);
            break;
        case DATableConditionalFormatRule::Condition::Contains:
            desc = tr("contains \"%1\"").arg(v1);
            break;
        case DATableConditionalFormatRule::Condition::IsEmpty:
            desc = tr("is empty");
            break;
        case DATableConditionalFormatRule::Condition::IsNotEmpty:
            desc = tr("is not empty");
            break;
        case DATableConditionalFormatRule::Condition::DateOccurring:
            desc = tr("date occurring: %1").arg(dateOccurrenceName(qBound(0, rule.value1.toInt(), 9)));
            break;
        case DATableConditionalFormatRule::Condition::Duplicate:
            desc = tr("duplicate values");
            break;
        case DATableConditionalFormatRule::Condition::Unique:
            desc = tr("unique values");
            break;
        }
        break;
    }
    case DATableConditionalFormatRule::Type::ColorScale:
        desc = rule.threeColorScale ? tr("3-color scale")   // cn:三色色阶
                                    : tr("2-color scale");  // cn:双色色阶
        break;
    case DATableConditionalFormatRule::Type::DataBar:
        desc = rule.barGradient ? tr("gradient data bar")  // cn:渐变数据条
                                : tr("data bar");          // cn:数据条
        break;
    }
    const QString prefix = rule.enabled ? QString() : tr("[Off] ");  // cn:[停用]
    return QStringLiteral("[%1] %2%3").arg(colName, prefix, desc);
}

/**
 * @brief 发生日期类别显示名（与 DateOccurrence 枚举同序）
 * @param occ 类别整数值
 * @return 显示名
 */
QString DADialogConditionalFormat::dateOccurrenceName(int occ) const
{
    switch (static_cast< DATableConditionalFormatRule::DateOccurrence >(occ)) {
    case DATableConditionalFormatRule::DateOccurrence::Yesterday:
        return tr("Yesterday");  // cn:昨天
    case DATableConditionalFormatRule::DateOccurrence::Today:
        return tr("Today");  // cn:今天
    case DATableConditionalFormatRule::DateOccurrence::Tomorrow:
        return tr("Tomorrow");  // cn:明天
    case DATableConditionalFormatRule::DateOccurrence::Last7Days:
        return tr("In the Last 7 Days");  // cn:最近 7 天
    case DATableConditionalFormatRule::DateOccurrence::LastWeek:
        return tr("Last Week");  // cn:上周
    case DATableConditionalFormatRule::DateOccurrence::ThisWeek:
        return tr("This Week");  // cn:本周
    case DATableConditionalFormatRule::DateOccurrence::NextWeek:
        return tr("Next Week");  // cn:下周
    case DATableConditionalFormatRule::DateOccurrence::LastMonth:
        return tr("Last Month");  // cn:上月
    case DATableConditionalFormatRule::DateOccurrence::ThisMonth:
        return tr("This Month");  // cn:本月
    case DATableConditionalFormatRule::DateOccurrence::NextMonth:
        return tr("Next Month");  // cn:下月
    }
    return tr("Today");  // cn:今天
}

/**
 * @brief 列表选择变化：装载新选中规则到编辑器
 * @param row 新行号
 */
void DADialogConditionalFormat::onRuleListCurrentChanged(int row)
{
    Q_UNUSED(row);
    if (mUpdatingUi) {
        return;
    }
    int col = -1, idx = -1;
    if (currentItemKey(&col, &idx)) {
        loadRuleToEditor(mWorking.value(col).at(idx), col, idx);
    } else {
        mEditCol   = -1;
        mEditIndex = -1;
    }
    updateButtonStates();
}

/**
 * @brief 列选择控件变化：绑定态把规则迁移/复制到新的列集合；自由定义态无操作
 */
void DADialogConditionalFormat::onColumnsChanged()
{
    if (mUpdatingUi) {
        return;
    }
    if (mEditCol < 0 || mEditIndex < 0) {
        return;  // 自由定义态：列集合随「添加规则」生效
    }
    commitEditorToWorking();
    const int oldCol = mEditCol;
    const int oldIdx = mEditIndex;
    auto it          = mWorking.find(oldCol);
    if (it == mWorking.end() || oldIdx < 0 || oldIdx >= it.value().size()) {
        return;
    }
    const DATableConditionalFormatRule r = it.value().at(oldIdx);
    it.value().removeAt(oldIdx);
    if (it.value().isEmpty()) {
        mWorking.erase(it);
    }
    mEditCol   = -1;
    mEditIndex = -1;
    const QList< int > cols = selectedColumns();
    for (int c : cols) {
        mWorking[ c ].append(r);
        if (mEditCol < 0) {
            mEditCol   = c;
            mEditIndex = mWorking[ c ].size() - 1;
        }
    }
    rebuildRuleList();
    refreshPreview();
}

/**
 * @brief 添加规则：把当前编辑器定义施加到所选各列（追加末尾），并绑定首列新规则
 */
void DADialogConditionalFormat::onAddRule()
{
    const QList< int > cols = selectedColumns();
    if (cols.isEmpty()) {
        QMessageBox::information(this, windowTitle(),
                                 tr("Please select at least one column first."));  // cn:请先选择至少一列。
        return;
    }
    DATableConditionalFormatRule r;
    fillRuleFromEditor(r);
    if (!mScaleBarAvailable && r.type != DATableConditionalFormatRule::Type::CellRule) {
        return;  // 惰性/只读源防御（正常已被置灰拦截）
    }
    for (int c : cols) {
        mWorking[ c ].append(r);
    }
    mEditCol   = cols.first();
    mEditIndex = mWorking.value(cols.first()).size() - 1;
    rebuildRuleList();
    refreshPreview();
}

/**
 * @brief 删除选中规则
 */
void DADialogConditionalFormat::onRemoveRule()
{
    int col = -1, idx = -1;
    if (!currentItemKey(&col, &idx)) {
        return;
    }
    mWorking[ col ].removeAt(idx);
    if (mWorking.value(col).isEmpty()) {
        mWorking.remove(col);
    }
    // 选中跟随：优先同列同位（下一条顶上），否则同列上一条
    const int remain = mWorking.value(col).size();
    mEditCol         = col;
    mEditIndex       = remain > 0 ? qMin(idx, remain - 1) : -1;
    if (mEditIndex < 0) {
        mEditCol = -1;
    }
    rebuildRuleList();
    refreshPreview();
}

/**
 * @brief 上移选中规则（列内优先级提升）
 */
void DADialogConditionalFormat::onMoveUp()
{
    int col = -1, idx = -1;
    if (!currentItemKey(&col, &idx) || idx <= 0) {
        return;
    }
    mWorking[ col ].swapItemsAt(idx, idx - 1);
    mEditCol   = col;
    mEditIndex = idx - 1;
    rebuildRuleList();
    refreshPreview();
}

/**
 * @brief 下移选中规则（列内优先级降低）
 */
void DADialogConditionalFormat::onMoveDown()
{
    int col = -1, idx = -1;
    if (!currentItemKey(&col, &idx)) {
        return;
    }
    if (idx >= mWorking.value(col).size() - 1) {
        return;
    }
    mWorking[ col ].swapItemsAt(idx, idx + 1);
    mEditCol   = col;
    mEditIndex = idx + 1;
    rebuildRuleList();
    refreshPreview();
}

/**
 * @brief 清除所选列的全部规则（工作集内清除，确定后经 clearColumn 命令入栈可撤销）
 */
void DADialogConditionalFormat::onClearColumnRules()
{
    int col = -1, idx = -1;
    if (!currentItemKey(&col, &idx)) {
        const QList< int > cols = selectedColumns();
        if (cols.isEmpty()) {
            return;
        }
        col = cols.first();
    }
    if (!mWorking.contains(col)) {
        return;
    }
    mWorking.remove(col);
    mEditCol   = -1;
    mEditIndex = -1;
    rebuildRuleList();
    refreshPreview();
}

/**
 * @brief 编辑器变更统一入口：写回工作集 → 刷新列表摘要与预览
 */
void DADialogConditionalFormat::onEditorChanged()
{
    if (mUpdatingUi) {
        return;
    }
    // 类型切换 → 页切换；色阶/数据条在不可用源上防御性回退（正常已被置灰拦截）
    const int typeIdx = mComboType->currentIndex();
    mStackEditor->setCurrentIndex(typeIdx);
    if (!mScaleBarAvailable && typeIdx != 0) {
        mUpdatingUi = true;
        mComboType->setCurrentIndex(0);
        mStackEditor->setCurrentIndex(0);
        mUpdatingUi = false;
    }
    updateValueEditsEnabled();
    updateScalePreview();
    int col = -1, idx = -1;
    if (!currentItemKey(&col, &idx)) {
        return;  // 自由定义态：不写工作集、不动预览
    }
    mEditCol   = col;
    mEditIndex = idx;
    if (!commitEditorToWorking()) {
        return;
    }
    // 刷新列表摘要（不重建，保持选中与滚动位置）
    if (QListWidgetItem* item = mListRules->currentItem()) {
        item->setText(ruleSummary(mEditCol, mWorking.value(mEditCol).at(mEditIndex)));
    }
    refreshPreview();
}

}  // end of namespace DA
