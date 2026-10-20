#include "DADialogConditionalFormatQuickRule.h"
#include "DAConditionalFormatPresets.h"
#include "DAColorPickerButton.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QStringList>
#include <QBrush>

namespace DA
{

class DADialogConditionalFormatQuickRule::PrivateData
{
    DA_DECLARE_PUBLIC(DADialogConditionalFormatQuickRule)
public:
    PrivateData(DADialogConditionalFormatQuickRule* p);
    void init();

    QLabel* mLabelDesc { nullptr };        ///< 条件说明（随条件变化）
    QLineEdit* mEditValue1 { nullptr };    ///< 条件值 1
    QLineEdit* mEditValue2 { nullptr };    ///< 条件值 2（仅介于）
    QLabel* mLabelValue1 { nullptr };
    QLabel* mLabelValue2 { nullptr };
    QRadioButton* mRadioDuplicate { nullptr };  ///< 重复（重复/唯一条件用）
    QRadioButton* mRadioUnique { nullptr };     ///< 唯一
    QWidget* mRowDup { nullptr };               ///< 重复/唯一行容器
    QWidget* mRowValue { nullptr };             ///< 条件值行容器
    QComboBox* mComboStyle { nullptr };    ///< 预设样式下拉（末项=自定义颜色）
    QWidget* mRowCustom { nullptr };       ///< 自定义颜色行容器
    DAColorPickerButton* mBtnBg { nullptr };
    DAColorPickerButton* mBtnFg { nullptr };
};

DADialogConditionalFormatQuickRule::PrivateData::PrivateData(DADialogConditionalFormatQuickRule* p) : q_ptr(p)
{
}

void DADialogConditionalFormatQuickRule::PrivateData::init()
{
    DA_Q(DADialogConditionalFormatQuickRule, q);
    q->resize(360, 220);
    QVBoxLayout* root = new QVBoxLayout(q);

    mLabelDesc = new QLabel(q);
    mLabelDesc->setWordWrap(true);
    root->addWidget(mLabelDesc);

    // 条件值行（介于双值；重复/唯一隐藏）
    mRowValue    = new QWidget(q);
    QHBoxLayout* vl = new QHBoxLayout(mRowValue);
    vl->setContentsMargins(0, 0, 0, 0);
    mLabelValue1 = new QLabel(q->tr("Value:"), mRowValue);  // cn:值：
    vl->addWidget(mLabelValue1);
    mEditValue1 = new QLineEdit(mRowValue);
    vl->addWidget(mEditValue1, 1);
    mLabelValue2 = new QLabel(q->tr("And (upper bound):"), mRowValue);  // cn:且（上界）：
    vl->addWidget(mLabelValue2);
    mEditValue2 = new QLineEdit(mRowValue);
    vl->addWidget(mEditValue2, 1);
    root->addWidget(mRowValue);

    // 重复/唯一行（仅重复/唯一条件显示）
    mRowDup       = new QWidget(q);
    QHBoxLayout* dl = new QHBoxLayout(mRowDup);
    dl->setContentsMargins(0, 0, 0, 0);
    dl->addWidget(new QLabel(q->tr("Rule:"), mRowDup));  // cn:规则：
    mRadioDuplicate = new QRadioButton(q->tr("Duplicate values"), mRowDup);  // cn:重复值
    mRadioUnique    = new QRadioButton(q->tr("Unique values"), mRowDup);     // cn:唯一值
    mRadioDuplicate->setChecked(true);
    QButtonGroup* grp = new QButtonGroup(mRowDup);
    grp->addButton(mRadioDuplicate);
    grp->addButton(mRadioUnique);
    dl->addWidget(mRadioDuplicate);
    dl->addWidget(mRadioUnique);
    dl->addStretch(1);
    root->addWidget(mRowDup);

    // 格式样式行（预设 + 自定义）
    QFormLayout* form = new QFormLayout();
    mComboStyle       = new QComboBox(q);
    const QList< DACFHighlightStyle > styles = DAConditionalFormatPresets::highlightStyles();
    const QStringList names                  = q->highlightStyleNames();
    for (int i = 0; i < styles.size(); ++i) {
        mComboStyle->addItem(DAConditionalFormatPresets::highlightThumb(styles.at(i)),
                             names.value(i, QString::number(i)));
    }
    mComboStyle->addItem(QIcon(), q->tr("Custom Colors..."));  // cn:自定义颜色...
    form->addRow(q->tr("Format Style:"), mComboStyle);  // cn:格式样式：
    root->addLayout(form);

    // 自定义颜色行（仅"自定义颜色"显示）
    mRowCustom    = new QWidget(q);
    QHBoxLayout* cl = new QHBoxLayout(mRowCustom);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->addWidget(new QLabel(q->tr("Background:"), mRowCustom));  // cn:背景：
    mBtnBg = new DAColorPickerButton(mRowCustom);
    mBtnBg->setColor(QColor(0xCE, 0x60, 0x43));  // 官方橙红
    cl->addWidget(mBtnBg);
    cl->addWidget(new QLabel(q->tr("Foreground:"), mRowCustom));  // cn:前景：
    mBtnFg = new DAColorPickerButton(mRowCustom);
    mBtnFg->setColor(QColor(0xFF, 0xFF, 0xFF));  // 白
    cl->addWidget(mBtnFg);
    cl->addStretch(1);
    root->addWidget(mRowCustom);

    root->addStretch(1);
    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q);
    QObject::connect(box, &QDialogButtonBox::accepted, q, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, q, &QDialog::reject);
    root->addWidget(box);

    QObject::connect(mComboStyle, QOverload< int >::of(&QComboBox::currentIndexChanged), q, [ this ]() {
        DA_Q(DADialogConditionalFormatQuickRule, qq);
        qq->updateStyleUi();
    });
}

DADialogConditionalFormatQuickRule::DADialogConditionalFormatQuickRule(
    DATableConditionalFormatRule::Condition cond, QWidget* parent)
    : QDialog(parent), mCondition(cond), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->init();
    updateConditionUi();
    updateStyleUi();
}

DADialogConditionalFormatQuickRule::~DADialogConditionalFormatQuickRule()
{
}

/**
 * @brief 按条件刷新标题/值输入可用性/重复单选可见性
 */
void DADialogConditionalFormatQuickRule::updateConditionUi()
{
    DA_D(d);
    const bool dupUnique = (mCondition == DATableConditionalFormatRule::Condition::Duplicate
                            || mCondition == DATableConditionalFormatRule::Condition::Unique);
    d->mRowDup->setVisible(dupUnique);
    d->mRowValue->setVisible(!dupUnique);
    if (dupUnique) {
        d->mRadioDuplicate->setChecked(mCondition == DATableConditionalFormatRule::Condition::Duplicate);
        d->mRadioUnique->setChecked(mCondition == DATableConditionalFormatRule::Condition::Unique);
        setWindowTitle(tr("Duplicate/Unique Values"));  // cn:重复/唯一值
        d->mLabelDesc->setText(tr("Format cells whose values are duplicate or unique in the column."));  // cn:对列内重复或唯一的值设置格式。
        return;
    }
    const bool between = (mCondition == DATableConditionalFormatRule::Condition::Between);
    d->mEditValue2->setVisible(between);
    d->mLabelValue2->setVisible(between);
    switch (mCondition) {
    case DATableConditionalFormatRule::Condition::Greater:
        setWindowTitle(tr("Greater Than"));  // cn:大于
        d->mLabelDesc->setText(tr("Format cells that are greater than:"));  // cn:为大于以下值的单元格设置格式：
        break;
    case DATableConditionalFormatRule::Condition::Less:
        setWindowTitle(tr("Less Than"));  // cn:小于
        d->mLabelDesc->setText(tr("Format cells that are less than:"));  // cn:为小于以下值的单元格设置格式：
        break;
    case DATableConditionalFormatRule::Condition::Between:
        setWindowTitle(tr("Between"));  // cn:介于
        d->mLabelDesc->setText(tr("Format cells that are between:"));  // cn:为介于以下值的单元格设置格式：
        break;
    case DATableConditionalFormatRule::Condition::Equal:
        setWindowTitle(tr("Equal To"));  // cn:等于
        d->mLabelDesc->setText(tr("Format cells that are equal to:"));  // cn:为等于以下值的单元格设置格式：
        break;
    case DATableConditionalFormatRule::Condition::Contains:
        setWindowTitle(tr("Text Contains"));  // cn:文本包含
        d->mLabelDesc->setText(tr("Format cells that contain the text:"));  // cn:为包含以下文本的单元格设置格式：
        break;
    default:
        break;
    }
}

/**
 * @brief 样式下拉变化：自定义时显示颜色行
 */
void DADialogConditionalFormatQuickRule::updateStyleUi()
{
    DA_D(d);
    const QList< DACFHighlightStyle > styles = DAConditionalFormatPresets::highlightStyles();
    d->mRowCustom->setVisible(d->mComboStyle->currentIndex() >= styles.size());
}

/**
 * @brief 确定：校验条件值非空（需要值的条件）
 */
void DADialogConditionalFormatQuickRule::accept()
{
    DA_D(d);
    const bool dupUnique = (mCondition == DATableConditionalFormatRule::Condition::Duplicate
                            || mCondition == DATableConditionalFormatRule::Condition::Unique);
    if (!dupUnique) {
        const bool between = (mCondition == DATableConditionalFormatRule::Condition::Between);
        if (d->mEditValue1->text().trimmed().isEmpty()
            || (between && d->mEditValue2->text().trimmed().isEmpty())) {
            QMessageBox::information(this, windowTitle(),
                                     tr("Please enter the condition value first."));  // cn:请先输入条件值。
            return;
        }
    }
    QDialog::accept();
}

/**
 * @brief 预设样式显示名（与 DAConditionalFormatPresets::highlightStyles 同序）
 * @return 显示名列表
 */
QStringList DADialogConditionalFormatQuickRule::highlightStyleNames() const
{
    return { tr("Orange-red fill with white text"),      // cn:橙红底白字
             tr("Yellow fill with dark slate text"),     // cn:金黄底深蓝灰字
             tr("Green fill with white text"),           // cn:绿底白字
             tr("Light blue fill with dark slate text"), // cn:浅蓝底深蓝灰字
             tr("Orange-red text") };                    // cn:仅橙红文本
}

/**
 * @brief 确定后的规则（CellRule：条件 + 条件值 + 样式）
 * @return 规则
 */
DATableConditionalFormatRule DADialogConditionalFormatQuickRule::resultRule() const
{
    DA_DC(d);
    DATableConditionalFormatRule r;
    r.type = DATableConditionalFormatRule::Type::CellRule;
    const bool dupUnique = (mCondition == DATableConditionalFormatRule::Condition::Duplicate
                            || mCondition == DATableConditionalFormatRule::Condition::Unique);
    if (dupUnique) {
        r.condition = d->mRadioUnique->isChecked() ? DATableConditionalFormatRule::Condition::Unique
                                                   : DATableConditionalFormatRule::Condition::Duplicate;
    } else {
        r.condition = mCondition;
        const QString v1 = d->mEditValue1->text().trimmed();
        const QString v2 = d->mEditValue2->text().trimmed();
        r.value1         = v1.isEmpty() ? QVariant() : QVariant(v1);
        r.value2         = v2.isEmpty() ? QVariant() : QVariant(v2);
    }
    // 样式：预设或自定义颜色
    const QList< DACFHighlightStyle > styles = DAConditionalFormatPresets::highlightStyles();
    const int idx                            = d->mComboStyle->currentIndex();
    DATableCellStyle s;
    if (idx >= 0 && idx < styles.size()) {
        const DACFHighlightStyle& ps = styles.at(idx);
        if (ps.hasBackground()) {
            s.setBackground(QBrush(ps.background));
        }
        if (ps.hasForeground()) {
            s.setForeground(ps.foreground);
        }
    } else {
        if (d->mBtnBg->color().isValid()) {
            s.setBackground(QBrush(d->mBtnBg->color()));
        }
        if (d->mBtnFg->color().isValid()) {
            s.setForeground(d->mBtnFg->color());
        }
    }
    r.style = s;
    return r;
}

}  // end of namespace DA
