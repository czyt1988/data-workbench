#include "DADialogConditionalFormat.h"
#include "ui_DADialogConditionalFormat.h"
#include "DATableConditionalFormatStore.h"
// qt
#include <QColorDialog>
#include <QFontDialog>
#include <QStandardItemModel>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QPainter>
#include <QPixmap>
#include <algorithm>

namespace DA
{

namespace
{
// 列表项数据角色：列索引 / 规则在该列内的序号
constexpr int c_roleColumn = Qt::UserRole;
constexpr int c_roleIndex  = Qt::UserRole + 1;

// 色块图标（48x24 实心色 + 中灰描边，色值取官方色板）
QIcon makeSwatchIcon(const QColor& color)
{
    QPixmap pm(48, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.fillRect(pm.rect().adjusted(1, 1, -1, -1), color.isValid() ? color : QColor(Qt::white));
    p.setPen(QColor(0x72, 0x72, 0x72));  // 中灰描边（官方色板）
    p.drawRect(pm.rect().adjusted(0, 0, -1, -1));
    p.end();
    return QIcon(pm);
}
}

/**
 * @brief 构造函数：构建 UI 外壳（列/类型下拉在 setup 填充）
 * @param store 条件格式子存储（非拥有，预览叠加通道，可空）
 * @param parent 父窗口
 */
DADialogConditionalFormat::DADialogConditionalFormat(DATableConditionalFormatStore* store, QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogConditionalFormat), mStore(store)
{
    ui->setupUi(this);
    // 规则类型下拉（枚举顺序 = 页序：0 CellRule / 1 ColorScale / 2 DataBar）
    ui->comboType->addItem(tr("Cell Rule"));      // cn:单元格规则
    ui->comboType->addItem(tr("Color Scale"));    // cn:色阶
    ui->comboType->addItem(tr("Data Bar"));       // cn:数据条
    // 条件下拉（枚举顺序与 DATableConditionalFormatRule::Condition 一致）
    ui->comboCondition->addItem(tr("greater than"));    // cn:大于
    ui->comboCondition->addItem(tr("less than"));       // cn:小于
    ui->comboCondition->addItem(tr("between"));         // cn:介于
    ui->comboCondition->addItem(tr("equal to"));        // cn:等于
    ui->comboCondition->addItem(tr("contains"));        // cn:包含
    ui->comboCondition->addItem(tr("is empty"));        // cn:为空
    ui->comboCondition->addItem(tr("is not empty"));    // cn:非空
    ui->labelDisabledHint->setVisible(false);

    connect(ui->listRules, &QListWidget::currentRowChanged, this, &DADialogConditionalFormat::onRuleListCurrentChanged);
    connect(ui->buttonAdd, &QPushButton::clicked, this, &DADialogConditionalFormat::onAddRule);
    connect(ui->buttonRemove, &QPushButton::clicked, this, &DADialogConditionalFormat::onRemoveRule);
    connect(ui->buttonMoveUp, &QPushButton::clicked, this, &DADialogConditionalFormat::onMoveUp);
    connect(ui->buttonMoveDown, &QPushButton::clicked, this, &DADialogConditionalFormat::onMoveDown);
    connect(ui->buttonClearColumn, &QPushButton::clicked, this, &DADialogConditionalFormat::onClearColumnRules);
    connect(ui->comboType, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->comboColumn, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogConditionalFormat::onColumnComboChanged);
    connect(ui->checkEnabled, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->comboCondition, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->editValue1, &QLineEdit::textEdited, this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->editValue2, &QLineEdit::textEdited, this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->checkUseBackground, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->checkUseForeground, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->checkUseFont, &QCheckBox::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    connect(ui->radioTwoColor, &QRadioButton::toggled, this, &DADialogConditionalFormat::onEditorChanged);
    // 颜色/字体按钮：QColorDialog::getColor（父窗口传对话框自身，避免弹到主窗口外）
    connect(ui->buttonBackground, &QToolButton::clicked, this, [ this ]() {
        QColor c = ui->checkUseBackground->isChecked() ? mSwatchBg : QColor(0xCE, 0x60, 0x43);
        if (pickColor(&c)) {
            mSwatchBg = c;
            ui->checkUseBackground->setChecked(true);
            onEditorChanged();
        }
    });
    connect(ui->buttonForeground, &QToolButton::clicked, this, [ this ]() {
        QColor c = ui->checkUseForeground->isChecked() ? mSwatchFg : QColor(0x5D, 0x6D, 0x7E);
        if (pickColor(&c)) {
            mSwatchFg = c;
            ui->checkUseForeground->setChecked(true);
            onEditorChanged();
        }
    });
    connect(ui->buttonFont, &QToolButton::clicked, this, [ this ]() {
        bool ok  = false;
        QFont f = QFontDialog::getFont(&ok, mSwatchFont, this, tr("Select Font"));  // cn:选择字体
        if (ok) {
            mSwatchFont = f;
            ui->checkUseFont->setChecked(true);
            onEditorChanged();
        }
    });
    connect(ui->buttonScaleMin, &QToolButton::clicked, this, [ this ]() {
        if (pickColor(&mScaleMinColor)) {
            onEditorChanged();
        }
    });
    connect(ui->buttonScaleMid, &QToolButton::clicked, this, [ this ]() {
        if (pickColor(&mScaleMidColor)) {
            onEditorChanged();
        }
    });
    connect(ui->buttonScaleMax, &QToolButton::clicked, this, [ this ]() {
        if (pickColor(&mScaleMaxColor)) {
            onEditorChanged();
        }
    });
    connect(ui->buttonBarColor, &QToolButton::clicked, this, [ this ]() {
        if (pickColor(&mBarColor)) {
            onEditorChanged();
        }
    });
}

DADialogConditionalFormat::~DADialogConditionalFormat()
{
    // 兜底：未经 accept/reject 直接销毁（如 deleteLater）时也要还原预览
    clearStorePreview();
    delete ui;
}

/**
 * @brief (re)初始化对话框状态
 * @param columnNames 全部列名（位置索引对齐）
 * @param scopeColumns 本次编辑的列集合（选区覆盖列）
 * @param scaleBarAvailable 色阶/数据条是否可用（惰性/只读源 false，§1.4-2）
 */
void DADialogConditionalFormat::setup(const QStringList& columnNames, const QList< int >& scopeColumns, bool scaleBarAvailable)
{
    mColumnNames       = columnNames;
    mScopeColumns      = scopeColumns;
    mScaleBarAvailable = scaleBarAvailable;
    std::sort(mScopeColumns.begin(), mScopeColumns.end());
    // 工作集初始化：从 store 拷贝 scope 列的现有规则（编辑不落库，确定才提交）
    mWorking.clear();
    for (int col : std::as_const(mScopeColumns)) {
        mWorking.insert(col, mStore ? mStore->columnRules(col) : QVector< DATableConditionalFormatRule >());
    }
    // 列下拉：仅 scope 列（入口作用于选区所在列，计划 WP3 步骤 3）
    ui->comboColumn->blockSignals(true);
    ui->comboColumn->clear();
    for (int col : std::as_const(mScopeColumns)) {
        const QString name = (col >= 0 && col < mColumnNames.size()) ? mColumnNames.at(col) : tr("Column %1").arg(col);
        ui->comboColumn->addItem(name, col);
    }
    ui->comboColumn->blockSignals(false);
    // 惰性/只读源：色阶与数据条置灰 + 说明文案（单元格规则正常提供）
    if (QStandardItemModel* m = qobject_cast< QStandardItemModel* >(ui->comboType->model())) {
        if (QStandardItem* item = m->item(1)) {
            item->setEnabled(scaleBarAvailable);
        }
        if (QStandardItem* item = m->item(2)) {
            item->setEnabled(scaleBarAvailable);
        }
    }
    ui->labelDisabledHint->setVisible(!scaleBarAvailable);
    ui->labelDisabledHint->setText(tr("Color scale and data bar need full-column statistics, "
                                      "which are unavailable for lazy-loaded or read-only data sources. "
                                      "Cell rules are still available."));  // cn:色阶与数据条需要全列统计，惰性加载或只读数据源不可用；单元格规则仍可正常使用
    rebuildRuleList();
    // 初始预览 = 现状（无视觉变化），后续编辑实时叠加
    refreshPreview();
}

/**
 * @brief 编辑结果（scope 列 → 规则列表；调用方 diff 生成撤销命令）
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
    QListWidgetItem* item = ui->listRules->currentItem();
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
 * @brief 重建左侧规则列表（按 scope 列分组、列内顺序即优先级；选中项跟随 mEditCol/mEditIndex）
 */
void DADialogConditionalFormat::rebuildRuleList()
{
    ui->listRules->blockSignals(true);
    ui->listRules->clear();
    int selectRow = -1;
    int row       = 0;
    for (int col : std::as_const(mScopeColumns)) {
        const QVector< DATableConditionalFormatRule >& rules = mWorking.value(col);
        for (int i = 0; i < rules.size(); ++i) {
            QListWidgetItem* item = new QListWidgetItem(ruleSummary(col, rules.at(i)), ui->listRules);
            item->setData(c_roleColumn, col);
            item->setData(c_roleIndex, i);
            if (col == mEditCol && i == mEditIndex) {
                selectRow = row;
            }
            ++row;
        }
    }
    if (selectRow >= 0) {
        ui->listRules->setCurrentRow(selectRow);
    } else if (ui->listRules->count() > 0) {
        ui->listRules->setCurrentRow(0);
    } else {
        mEditCol   = -1;
        mEditIndex = -1;
    }
    ui->listRules->blockSignals(false);
    // blockSignals 期间 currentRowChanged 不触发，这里主动装载选中项
    int col = -1, idx = -1;
    if (currentItemKey(&col, &idx)) {
        loadRuleToEditor(mWorking.value(col).at(idx), col, idx);
    }
    updateButtonStates();
}

/**
 * @brief 装载规则到右侧编辑器（守卫期间屏蔽控件信号）
 * @param rule 规则
 * @param column 所属列
 * @param index 列内序号
 */
void DADialogConditionalFormat::loadRuleToEditor(const DATableConditionalFormatRule& rule, int column, int index)
{
    mUpdatingUi = true;
    mEditCol    = column;
    mEditIndex  = index;
    ui->comboType->setCurrentIndex(static_cast< int >(rule.type));
    ui->stackEditor->setCurrentIndex(static_cast< int >(rule.type));
    const int colComboIdx = ui->comboColumn->findData(column);
    if (colComboIdx >= 0) {
        ui->comboColumn->setCurrentIndex(colComboIdx);
    }
    ui->checkEnabled->setChecked(rule.enabled);
    ui->comboCondition->setCurrentIndex(static_cast< int >(rule.condition));
    ui->editValue1->setText(rule.value1.isValid() ? rule.value1.toString() : QString());
    ui->editValue2->setText(rule.value2.isValid() ? rule.value2.toString() : QString());
    ui->checkUseBackground->setChecked(rule.style.backgroundValid());
    ui->checkUseForeground->setChecked(rule.style.foregroundValid());
    ui->checkUseFont->setChecked(rule.style.fontValid());
    if (rule.style.backgroundValid()) {
        mSwatchBg = rule.style.background().color();
    }
    if (rule.style.foregroundValid()) {
        mSwatchFg = rule.style.foreground();
    }
    if (rule.style.fontValid()) {
        mSwatchFont = rule.style.font();
    }
    ui->radioThreeColor->setChecked(rule.threeColorScale);
    ui->radioTwoColor->setChecked(!rule.threeColorScale);
    mScaleMinColor = rule.scaleMinColor;
    mScaleMidColor = rule.scaleMidColor;
    mScaleMaxColor = rule.scaleMaxColor;
    mBarColor      = rule.barColor;
    updateValueEditsEnabled();
    updateSwatchButtons();
    mUpdatingUi = false;
}

/**
 * @brief 把编辑器当前值写回工作集选中规则
 * @return 写回成功返回 true（无有效选中返回 false）
 */
bool DADialogConditionalFormat::commitEditorToWorking()
{
    int col = -1, idx = -1;
    if (mEditCol < 0 || mEditIndex < 0) {
        if (!currentItemKey(&col, &idx)) {
            return false;
        }
    } else {
        col = mEditCol;
        idx = mEditIndex;
    }
    auto it = mWorking.find(col);
    if (it == mWorking.end() || idx < 0 || idx >= it.value().size()) {
        return false;
    }
    DATableConditionalFormatRule& r = it.value()[ idx ];
    r.type                          = static_cast< DATableConditionalFormatRule::Type >(ui->comboType->currentIndex());
    r.enabled                       = ui->checkEnabled->isChecked();
    r.condition                     = static_cast< DATableConditionalFormatRule::Condition >(ui->comboCondition->currentIndex());
    const QString v1                = ui->editValue1->text();
    const QString v2                = ui->editValue2->text();
    r.value1                        = v1.isEmpty() ? QVariant() : QVariant(v1);
    r.value2                        = v2.isEmpty() ? QVariant() : QVariant(v2);
    DATableCellStyle s;
    if (ui->checkUseBackground->isChecked()) {
        s.setBackground(QBrush(mSwatchBg));
    }
    if (ui->checkUseForeground->isChecked()) {
        s.setForeground(mSwatchFg);
    }
    if (ui->checkUseFont->isChecked()) {
        s.setFont(mSwatchFont);
    }
    r.style           = s;
    r.threeColorScale = ui->radioThreeColor->isChecked();
    r.scaleMinColor   = mScaleMinColor;
    r.scaleMidColor   = mScaleMidColor;
    r.scaleMaxColor   = mScaleMaxColor;
    r.barColor        = mBarColor;
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
 * @brief 按条件操作符刷新参数输入可用性（Between 双值 / IsEmpty 系无值）
 */
void DADialogConditionalFormat::updateValueEditsEnabled()
{
    const DATableConditionalFormatRule::Condition c =
        static_cast< DATableConditionalFormatRule::Condition >(ui->comboCondition->currentIndex());
    const bool noValue = (c == DATableConditionalFormatRule::Condition::IsEmpty
                          || c == DATableConditionalFormatRule::Condition::IsNotEmpty);
    const bool between = (c == DATableConditionalFormatRule::Condition::Between);
    ui->editValue1->setEnabled(!noValue);
    ui->labelValue1->setEnabled(!noValue);
    ui->editValue2->setEnabled(between);
    ui->labelValue2->setEnabled(between);
}

/**
 * @brief 色块按钮图标与状态刷新
 */
void DADialogConditionalFormat::updateSwatchButtons()
{
    ui->buttonBackground->setIcon(makeSwatchIcon(mSwatchBg));
    ui->buttonForeground->setIcon(makeSwatchIcon(mSwatchFg));
    ui->buttonScaleMin->setIcon(makeSwatchIcon(mScaleMinColor));
    ui->buttonScaleMid->setIcon(makeSwatchIcon(mScaleMidColor));
    ui->buttonScaleMax->setIcon(makeSwatchIcon(mScaleMaxColor));
    ui->buttonBarColor->setIcon(makeSwatchIcon(mBarColor));
    const bool three = ui->radioThreeColor->isChecked();
    ui->buttonScaleMid->setEnabled(three);
    ui->labelScaleMid->setEnabled(three);
    ui->buttonFont->setText(mSwatchFont.family().isEmpty() ? tr("Font...")  // cn:字体...
                                                           : QStringLiteral("%1 %2").arg(mSwatchFont.family()).arg(mSwatchFont.pointSize()));
}

/**
 * @brief 列表/按钮可用性刷新
 */
void DADialogConditionalFormat::updateButtonStates()
{
    int col = -1, idx = -1;
    const bool has = currentItemKey(&col, &idx);
    ui->buttonRemove->setEnabled(has);
    ui->buttonMoveUp->setEnabled(has && idx > 0);
    ui->buttonMoveDown->setEnabled(has);  // 末尾判断需要列内数量，交给点击时兜底
    ui->buttonClearColumn->setEnabled(has || ui->comboColumn->count() > 0);
    ui->stackEditor->setEnabled(has);
    ui->checkEnabled->setEnabled(has);
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
        }
        break;
    }
    case DATableConditionalFormatRule::Type::ColorScale:
        desc = rule.threeColorScale ? tr("3-color scale")  // cn:三色色阶
                                    : tr("2-color scale");  // cn:双色色阶
        break;
    case DATableConditionalFormatRule::Type::DataBar:
        desc = tr("data bar");  // cn:数据条
        break;
    }
    const QString prefix = rule.enabled ? QString() : tr("[Off] ");  // cn:[停用]
    return QStringLiteral("[%1] %2%3").arg(colName, prefix, desc);
}

/**
 * @brief 颜色选择（QColorDialog::getColor，父窗口传对话框自身）
 * @param color 输入当前色/输出选中色
 * @return 选中有效色返回 true
 */
bool DADialogConditionalFormat::pickColor(QColor* color)
{
    const QColor c = QColorDialog::getColor(*color, this, tr("Select Color"));  // cn:选择颜色
    if (!c.isValid()) {
        return false;
    }
    *color = c;
    return true;
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
 * @brief 列下拉变化：把选中规则改派到新列（追加到目标列末尾，优先级最低）
 * @param comboIndex 下拉新位置
 */
void DADialogConditionalFormat::onColumnComboChanged(int comboIndex)
{
    if (mUpdatingUi || comboIndex < 0) {
        return;
    }
    int col = -1, idx = -1;
    if (!currentItemKey(&col, &idx)) {
        return;
    }
    const int newCol = ui->comboColumn->itemData(comboIndex).toInt();
    if (newCol == col || !mWorking.contains(newCol)) {
        return;
    }
    commitEditorToWorking();  // 编辑器最新值先写回，再随规则迁移
    const DATableConditionalFormatRule r = mWorking[ col ].takeAt(idx);
    mWorking[ newCol ].append(r);
    mEditCol   = newCol;
    mEditIndex = mWorking[ newCol ].size() - 1;
    rebuildRuleList();
    refreshPreview();
}

/**
 * @brief 新增规则（目标列 = 列下拉当前列；默认单元格规则-大于）
 */
void DADialogConditionalFormat::onAddRule()
{
    if (mScopeColumns.isEmpty()) {
        return;
    }
    int col = ui->comboColumn->currentData().toInt();
    if (!mScopeColumns.contains(col)) {
        col = mScopeColumns.first();
    }
    DATableConditionalFormatRule r;  // 默认 CellRule/Greater，样式未设（用户勾选后取色）
    mWorking[ col ].append(r);
    mEditCol   = col;
    mEditIndex = mWorking[ col ].size() - 1;
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
        col = ui->comboColumn->currentData().toInt();
    }
    if (!mWorking.contains(col)) {
        return;
    }
    mWorking[ col ].clear();
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
    int col = -1, idx = -1;
    const bool hadSelection = currentItemKey(&col, &idx);
    if (hadSelection) {
        mEditCol   = col;
        mEditIndex = idx;
    }
    if (!commitEditorToWorking()) {
        // 无有效选中：只同步页切换与控件状态
        ui->stackEditor->setCurrentIndex(ui->comboType->currentIndex());
        updateValueEditsEnabled();
        updateSwatchButtons();
        return;
    }
    // 类型切换 → 页切换；色阶/数据条在不可用源上防御性回退（正常已被置灰拦截）
    const int typeIdx = ui->comboType->currentIndex();
    ui->stackEditor->setCurrentIndex(typeIdx);
    if (!mScaleBarAvailable && typeIdx != 0) {
        mUpdatingUi = true;
        ui->comboType->setCurrentIndex(0);
        ui->stackEditor->setCurrentIndex(0);
        mUpdatingUi                              = false;
        mWorking[ mEditCol ][ mEditIndex ].type = DATableConditionalFormatRule::Type::CellRule;
    }
    updateValueEditsEnabled();
    updateSwatchButtons();
    // 刷新列表摘要（不重建，保持选中与滚动位置）
    if (QListWidgetItem* item = ui->listRules->currentItem()) {
        item->setText(ruleSummary(mEditCol, mWorking.value(mEditCol).at(mEditIndex)));
    }
    refreshPreview();
}

}  // end of namespace DA
