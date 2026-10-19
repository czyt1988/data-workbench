#include "DADialogSplitColumn.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAWaitCursorScoped.h"
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QButtonGroup>

namespace DA
{

/**
 * @brief 构造
 * @param df 数据（只读；预览在 head 副本上做，不触碰原表）
 * @param sourceCol 源列位置
 * @param parent 父窗口
 */
DADialogSplitColumn::DADialogSplitColumn(const DAPyDataFrame& df, int sourceCol, QWidget* parent)
    : QDialog(parent), mDf(df), mSourceCol(sourceCol)
{
    setupUI();
    setWindowTitle(tr("Split Column"));  // cn:分列
    runPreview();
}

DADialogSplitColumn::~DADialogSplitColumn()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）
 */
void DADialogSplitColumn::setupUI()
{
    setModal(true);
    QWidget* leftPanel  = new QWidget(this);
    QFormLayout* form   = new QFormLayout(leftPanel);
    form->setContentsMargins(0, 0, 0, 0);
    mLabelSource        = new QLabel(leftPanel);
    const QList< QString > cols = mDf.columns();
    if (mSourceCol >= 0 && mSourceCol < cols.size()) {
        mLabelSource->setText(tr("Source column: %1").arg(cols.at(mSourceCol)));  // cn:源列：%1
    }
    form->addRow(mLabelSource);
    // 分隔模式（三选一，各带参数输入）
    QGroupBox* modeGroup = new QGroupBox(tr("Split Mode"), leftPanel);  // cn:分隔模式
    QVBoxLayout* modeLay = new QVBoxLayout(modeGroup);
    mRadioChar           = new QRadioButton(tr("Delimiter (single char or string)"), modeGroup);  // cn:分隔符（单字符/字符串）
    mRadioChar->setChecked(true);
    mEditDelimiter       = new QLineEdit(QStringLiteral(","), modeGroup);
    mEditDelimiter->setObjectName(QStringLiteral("lineEditDelimiter"));
    mRadioRegex = new QRadioButton(tr("Regular expression (capture groups)"), modeGroup);  // cn:正则（捕获组）
    mEditPattern = new QLineEdit(modeGroup);
    mEditPattern->setObjectName(QStringLiteral("lineEditPattern"));
    mEditPattern->setPlaceholderText(QStringLiteral(R"(e.g. (\d{4})-(\d{2})-(\d{2}))"));
    mEditPattern->setEnabled(false);
    mRadioWidth = new QRadioButton(tr("Fixed widths"), modeGroup);  // cn:固定宽度
    mEditWidths = new QLineEdit(modeGroup);
    mEditWidths->setObjectName(QStringLiteral("lineEditWidths"));
    mEditWidths->setPlaceholderText(tr("comma separated, e.g. 3,5,8"));  // cn:逗号分隔，如 3,5,8
    mEditWidths->setEnabled(false);
    modeLay->addWidget(mRadioChar);
    modeLay->addWidget(mEditDelimiter);
    modeLay->addWidget(mRadioRegex);
    modeLay->addWidget(mEditPattern);
    modeLay->addWidget(mRadioWidth);
    modeLay->addWidget(mEditWidths);
    form->addRow(modeGroup);
    // 上限与溢出策略
    mSpinMaxColumns = new QSpinBox(leftPanel);
    mSpinMaxColumns->setRange(0, 1000);
    mSpinMaxColumns->setValue(0);
    mSpinMaxColumns->setSpecialValueText(tr("Unlimited"));  // cn:不限
    form->addRow(tr("Max columns:"), mSpinMaxColumns);      // cn:最大列数：
    mComboOverflow = new QComboBox(leftPanel);
    mComboOverflow->addItem(tr("Discard extra parts"));           // cn:丢弃超出部分
    mComboOverflow->addItem(tr("Keep remainder in last column")); // cn:剩余并入最后一列（保留原文）
    mComboOverflow->addItem(tr("Report error"));                  // cn:报错
    form->addRow(tr("Overflow policy:"), mComboOverflow);         // cn:溢出策略：
    mCheckInferNumeric = new QCheckBox(tr("Try to parse result columns as numeric"), leftPanel);  // cn:结果列尝试数值解析
    mCheckInferNumeric->setChecked(true);
    form->addRow(mCheckInferNumeric);
    mEditNames = new QLineEdit(leftPanel);
    mEditNames->setObjectName(QStringLiteral("lineEditNames"));
    mEditNames->setPlaceholderText(tr("comma separated result column names"));  // cn:逗号分隔的结果列名
    form->addRow(tr("Result names:"), mEditNames);  // cn:结果列名：

    QWidget* rightPanel   = new QWidget(this);
    QVBoxLayout* rightLay = new QVBoxLayout(rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    mLabelStatus = new QLabel(rightPanel);
    mLabelStatus->setWordWrap(true);
    rightLay->addWidget(mLabelStatus);
    QGroupBox* previewGroup = new QGroupBox(tr("Preview (first %1 rows)").arg(c_previewRows), rightPanel);  // cn:预览（前 %1 行）
    QVBoxLayout* previewLay = new QVBoxLayout(previewGroup);
    previewLay->setContentsMargins(4, 4, 4, 4);
    mTablePreview = new QTableWidget(previewGroup);
    mTablePreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTablePreview->verticalHeader()->setVisible(false);
    previewLay->addWidget(mTablePreview);
    rightLay->addWidget(previewGroup, 1);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtonOk                 = buttons->button(QDialogButtonBox::Ok);
    mButtonOk->setText(tr("OK"));                                      // cn:确定
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    mButtonOk->setEnabled(false);
    QHBoxLayout* bodyLay = new QHBoxLayout();
    bodyLay->addWidget(leftPanel, 2);
    bodyLay->addWidget(rightPanel, 3);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->addLayout(bodyLay, 1);
    mainLay->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 模式切换与输入联动（300ms 去抖预览）
    // 用 QButtonGroup::buttonClicked(QAbstractButton*)（Qt5/Qt6 都有；idClicked 是 5.15+，兼容考虑不用）
    QButtonGroup* modeGroupBtns = new QButtonGroup(this);
    modeGroupBtns->addButton(mRadioChar);
    modeGroupBtns->addButton(mRadioRegex);
    modeGroupBtns->addButton(mRadioWidth);
    connect(modeGroupBtns, QOverload< QAbstractButton* >::of(&QButtonGroup::buttonClicked), this, [ this ](QAbstractButton*) {
        onModeChanged();
    });
    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(300);
    connect(mDebounceTimer, &QTimer::timeout, this, &DADialogSplitColumn::runPreview);
    for (QLineEdit* e : { mEditDelimiter, mEditPattern, mEditWidths }) {
        connect(e, &QLineEdit::textChanged, this, &DADialogSplitColumn::onInputChanged);
    }
    connect(mSpinMaxColumns, QOverload< int >::of(&QSpinBox::valueChanged), this, &DADialogSplitColumn::onInputChanged);
    connect(mComboOverflow, &QComboBox::currentIndexChanged, this, &DADialogSplitColumn::onInputChanged);
    connect(mCheckInferNumeric, &QCheckBox::toggled, this, &DADialogSplitColumn::onInputChanged);
    connect(mEditNames, &QLineEdit::textEdited, this, &DADialogSplitColumn::onNamesEdited);

    resize(820, 460);
}

void DADialogSplitColumn::onModeChanged()
{
    mEditDelimiter->setEnabled(mRadioChar->isChecked());
    mEditPattern->setEnabled(mRadioRegex->isChecked());
    mEditWidths->setEnabled(mRadioWidth->isChecked());
    onInputChanged();
}

void DADialogSplitColumn::onInputChanged()
{
    mDebounceTimer->start();
}

void DADialogSplitColumn::onNamesEdited()
{
    mNamesEdited = true;
    mDebounceTimer->start();
}

void DADialogSplitColumn::setStatusError(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#CE6043;"));  // 橙红（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(false);
}

void DADialogSplitColumn::setStatusOk(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#669E8B;"));  // 绿（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(true);
}

/**
 * @brief 收集当前模式参数（不含结果列名），无效时输出错误
 */
bool DADialogSplitColumn::collectModeParams(DACommandDataFrame_splitColumn::SplitParams& p, QString* err) const
{
    p.col         = mSourceCol;
    p.maxColumns  = mSpinMaxColumns->value();
    p.inferNumeric = mCheckInferNumeric->isChecked();
    switch (mComboOverflow->currentIndex()) {
    case 1:
        p.overflow = QStringLiteral("keep");
        break;
    case 2:
        p.overflow = QStringLiteral("error");
        break;
    default:
        p.overflow = QStringLiteral("discard");
        break;
    }
    if (mRadioChar->isChecked()) {
        p.mode      = QStringLiteral("char");
        p.delimiter = mEditDelimiter->text();
        if (p.delimiter.isEmpty()) {
            if (err) {
                *err = tr("Delimiter must not be empty");  // cn:分隔符不能为空
            }
            return false;
        }
    } else if (mRadioRegex->isChecked()) {
        p.mode    = QStringLiteral("regex");
        p.pattern = mEditPattern->text().trimmed();
        if (p.pattern.isEmpty()) {
            if (err) {
                *err = tr("Regular expression must not be empty");  // cn:正则不能为空
            }
            return false;
        }
    } else {
        p.mode = QStringLiteral("width");
        const QStringList parts = mEditWidths->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString& s : parts) {
            bool ok   = false;
            const int w = s.trimmed().toInt(&ok);
            if (!ok || w <= 0) {
                if (err) {
                    *err = tr("Widths must be comma separated positive integers");  // cn:宽度必须是逗号分隔的正整数
                }
                return false;
            }
            p.widths.append(w);
        }
        if (p.widths.isEmpty()) {
            if (err) {
                *err = tr("Widths must not be empty");  // cn:宽度列表不能为空
            }
            return false;
        }
    }
    return true;
}

/**
 * @brief 校验 + 头部预览（预览与执行同一算子：da_head_copy + da_split_column，红线5）
 */
void DADialogSplitColumn::runPreview()
{
    mPreviewColumnCount = 0;
    mTablePreview->clear();
    mTablePreview->setRowCount(0);
    mTablePreview->setColumnCount(0);
    if (mDf.isNone() || mSourceCol < 0) {
        setStatusError(tr("No data"));  // cn:无数据
        return;
    }
    DACommandDataFrame_splitColumn::SplitParams p;
    QString err;
    if (!collectModeParams(p, &err)) {
        setStatusError(err);
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    DAWaitCursorScoped waitCursor;
    DAPyDataFrame head       = py.headCopy(mDf, c_previewRows);
    if (head.isNone()) {
        setStatusError(tr("Failed to build preview sample"));  // cn:预览样本构建失败
        return;
    }
    // 第一遍：自动命名求自然结果（列数与值）
    QStringList names = py.splitColumn(head, p.col, p.mode, p.delimiter, p.pattern, p.widths, p.maxColumns, p.overflow, p.inferNumeric, QStringList(), &err);
    if (names.isEmpty()) {
        setStatusError(tr("Split failed: %1").arg(err));  // cn:分列失败：%1
        return;
    }
    mPreviewColumnCount = names.size();
    // 自动命名与现有列冲突时要求用户改名（重名会破坏按名撤销的定位）
    const QList< QString > existing = mDf.columns();
    if (!mNamesEdited) {
        for (const QString& n : std::as_const(names)) {
            if (existing.contains(n)) {
                setStatusError(tr("Auto name \"%1\" conflicts with an existing column, please rename the result columns")
                                   .arg(n));  // cn:自动列名“%1”与现有列重名，请修改结果列名
                return;
            }
        }
    }
    // 结果列名校验：用户改过 → 用用户列名（个数一致、非空、互不重复、不与现有列重名）
    QStringList finalNames = names;
    if (mNamesEdited) {
        finalNames.clear();
        const QStringList parts = mEditNames->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString& s : parts) {
            finalNames.append(s.trimmed());
        }
        if (finalNames.size() != mPreviewColumnCount) {
            setStatusError(tr("Result name count (%1) does not match the split column count (%2)")
                               .arg(finalNames.size())
                               .arg(mPreviewColumnCount));  // cn:结果列名个数（%1）与分列数（%2）不一致
            return;
        }
        for (int i = 0; i < finalNames.size(); ++i) {
            const QString& n = finalNames.at(i);
            if (n.isEmpty()) {
                setStatusError(tr("Result column name must not be empty"));  // cn:结果列名不能为空
                return;
            }
            if (finalNames.indexOf(n) != i) {
                setStatusError(tr("Duplicate result column name \"%1\"").arg(n));  // cn:结果列名“%1”重复
                return;
            }
            if (existing.contains(n)) {
                setStatusError(tr("Result column name \"%1\" already exists in the table").arg(n));  // cn:结果列名“%1”与现有列重名
                return;
            }
        }
        p.newNames = finalNames;
    } else {
        // 自动命名回显（不触发 textEdited）
        QSignalBlocker blocker(mEditNames);
        mEditNames->setText(names.join(QLatin1Char(',')));
        p.newNames = names;
    }
    // 预览表：源列 + 结果列（head 副本已被第一遍就地拆分，值即所见）
    const auto shape = head.shape();
    const int nrows  = static_cast< int >(shape.first);
    QStringList previewHeader;
    previewHeader.append(mDf.columns().value(mSourceCol));
    previewHeader.append(finalNames);
    mTablePreview->clear();
    mTablePreview->setColumnCount(previewHeader.size());
    mTablePreview->setRowCount(nrows);
    mTablePreview->setHorizontalHeaderLabels(previewHeader);
    const QList< QString > headCols = head.columns();
    for (int c = 0; c < previewHeader.size(); ++c) {
        const int pos = headCols.indexOf(previewHeader.at(c));
        for (int r = 0; r < nrows; ++r) {
            if (pos < 0) {
                continue;
            }
            mTablePreview->setItem(r, c, new QTableWidgetItem(head.iat(r, pos).toString()));
        }
    }
    mTablePreview->resizeColumnsToContents();
    setStatusOk(tr("OK — split into %1 column(s)").arg(mPreviewColumnCount));  // cn:通过 — 拆分为 %1 列
}

/**
 * @brief 接受后的分列参数（结果列名以输入框为准；未编辑过时即自动命名）
 */
DACommandDataFrame_splitColumn::SplitParams DADialogSplitColumn::params() const
{
    DACommandDataFrame_splitColumn::SplitParams p;
    QString err;
    collectModeParams(p, &err);  // 已校验（OK 可用才会被接受）
    const QStringList parts = mEditNames->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString& s : parts) {
        p.newNames.append(s.trimmed());
    }
    return p;
}

}  // end of namespace DA
