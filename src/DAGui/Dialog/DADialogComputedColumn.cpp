#include "DADialogComputedColumn.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAWaitCursorScoped.h"
#include <QLineEdit>
#include <QListWidget>
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

namespace DA
{

/**
 * @brief 构造（创建模式）
 * @param df 数据（只读；预览在 head 副本上做，不触碰原表）
 * @param parent 父窗口
 */
DADialogComputedColumn::DADialogComputedColumn(const DAPyDataFrame& df, QWidget* parent) : QDialog(parent), mDf(df)
{
    setupUI();
    setWindowTitle(tr("New Computed Column"));  // cn:新建计算列
    runValidationAndPreview();
}

DADialogComputedColumn::~DADialogComputedColumn()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）
 *
 * 左侧：目标列名 + 表达式 + 列名点选列表；右侧：状态标签 + 头部预览表。
 */
void DADialogComputedColumn::setupUI()
{
    setModal(true);
    QWidget* leftPanel = new QWidget(this);
    QFormLayout* form  = new QFormLayout(leftPanel);
    form->setContentsMargins(0, 0, 0, 0);
    mEditTarget = new QLineEdit(leftPanel);
    mEditTarget->setObjectName(QStringLiteral("lineEditComputedTarget"));
    mEditTarget->setPlaceholderText(tr("New column name"));  // cn:新列名
    form->addRow(tr("Target Column:"), mEditTarget);  // cn:目标列名：
    mEditExpr = new QLineEdit(leftPanel);
    mEditExpr->setObjectName(QStringLiteral("lineEditComputedExpr"));
    mEditExpr->setPlaceholderText(tr("e.g. A + B (column names can be inserted from the list below)"));  // cn:如 A + B（列名可从下方列表点选插入）
    form->addRow(tr("Expression:"), mEditExpr);  // cn:表达式：
    QGroupBox* colGroup = new QGroupBox(tr("Columns (click to insert)"), leftPanel);  // cn:列名（点选插入）
    QVBoxLayout* colLayout = new QVBoxLayout(colGroup);
    colLayout->setContentsMargins(4, 4, 4, 4);
    mListColumns = new QListWidget(colGroup);
    colLayout->addWidget(mListColumns);
    form->addRow(colGroup);

    QWidget* rightPanel  = new QWidget(this);
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

    QHBoxLayout* bodyLay = new QHBoxLayout();
    bodyLay->addWidget(leftPanel, 2);
    bodyLay->addWidget(rightPanel, 3);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtonOk                 = buttons->button(QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    mButtonOk->setText(tr("OK"));                                      // cn:确定
    mButtonOk->setEnabled(false);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->addLayout(bodyLay, 1);
    mainLay->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 列名列表填充（点选插入用 itemClicked：同一行可重复点击插入）
    const QList< QString > cols = mDf.columns();
    for (const QString& c : cols) {
        mListColumns->addItem(c);
    }
    connect(mListColumns, &QListWidget::itemClicked, this, [ this ](QListWidgetItem* item) {
        if (item) {
            onColumnItemClicked(mListColumns->row(item));
        }
    });

    // 去抖校验（300ms）
    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(300);
    connect(mDebounceTimer, &QTimer::timeout, this, &DADialogComputedColumn::runValidationAndPreview);
    connect(mEditTarget, &QLineEdit::textChanged, this, &DADialogComputedColumn::onInputChanged);
    connect(mEditExpr, &QLineEdit::textChanged, this, &DADialogComputedColumn::onInputChanged);

    resize(760, 420);
}

/**
 * @brief 编辑模式（列头右键「编辑公式」）：目标列名固定只读，表达式预填
 * @param def 既有定义
 */
void DADialogComputedColumn::setEditMode(const DAComputedColumn& def)
{
    mEditMode = true;
    setWindowTitle(tr("Edit Formula"));  // cn:编辑公式
    mEditTarget->setText(def.targetColumnName);
    mEditTarget->setReadOnly(true);
    mEditExpr->setText(def.expr);
    runValidationAndPreview();
}

bool DADialogComputedColumn::isEditMode() const
{
    return mEditMode;
}

QString DADialogComputedColumn::targetColumnName() const
{
    return mEditTarget->text().trimmed();
}

QString DADialogComputedColumn::expressionBody() const
{
    return mEditExpr->text().trimmed();
}

QStringList DADialogComputedColumn::dependencies() const
{
    return mDependencies;
}

/**
 * @brief 列名点选插入：在表达式光标处插入列名（含空格/特殊字符自动加反引号，R20）
 * @param row 列表行
 */
void DADialogComputedColumn::onColumnItemClicked(int row)
{
    if (row < 0 || !mListColumns->item(row)) {
        return;
    }
    const QString name = mListColumns->item(row)->text();
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QString ref              = py.needsBacktick(name) ? (QLatin1Char('`') + name + QLatin1Char('`')) : name;
    mEditExpr->insert(ref);
}

void DADialogComputedColumn::onInputChanged()
{
    mDebounceTimer->start();
}

void DADialogComputedColumn::setStatusError(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#CE6043;"));  // 橙红（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(false);
}

void DADialogComputedColumn::setStatusOk(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#669E8B;"));  // 绿（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(true);
}

/**
 * @brief 校验 + 头部预览（预览与执行同一算子：da_head_copy + da_eval_column）
 *
 * 校验链（任一失败 → 红字提示 + OK 置灰，不写任何数据）：
 * 目标列名非空 → 创建模式目标不存在（禁止自覆盖 R7）→ 表达式非空 →
 * 表达式可解析（ast）→ 依赖不含目标列（禁止自覆盖）→ head 副本求值成功。
 */
void DADialogComputedColumn::runValidationAndPreview()
{
    mDependencies.clear();
    mTablePreview->clear();
    mTablePreview->setRowCount(0);
    mTablePreview->setColumnCount(0);
    if (mDf.isNone()) {
        setStatusError(tr("No data"));  // cn:无数据
        return;
    }
    const QString target = targetColumnName();
    const QString body   = expressionBody();
    if (target.isEmpty()) {
        setStatusError(tr("Please specify the target column name"));  // cn:请指定目标列名
        return;
    }
    const QList< QString > cols = mDf.columns();
    QStringList colNames;
    colNames.reserve(cols.size());
    for (const QString& c : cols) {
        colNames.append(c);
    }
    if (!mEditMode && colNames.contains(target)) {
        setStatusError(tr("Column \"%1\" already exists. A computed column must write to a new column (self-overwrite is not allowed).")
                           .arg(target));  // cn:列“%1”已存在。计算列必须写入新列（禁止自覆盖）。
        return;
    }
    if (body.isEmpty()) {
        setStatusError(tr("Please input the expression"));  // cn:请输入表达式
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    // 依赖提取（ast 解析；语法错误在此暴露）
    bool dependsOk  = false;
    QString parseErr;
    const QStringList deps = py.exprDepends(mDf, body, &dependsOk, &parseErr);
    if (!dependsOk) {
        setStatusError(tr("Expression syntax error: %1").arg(parseErr));  // cn:表达式语法错误：%1
        return;
    }
    if (deps.contains(target)) {
        setStatusError(tr("The expression must not reference the target column \"%1\" (self-overwrite is not allowed).")
                           .arg(target));  // cn:表达式不得引用目标列“%1”（禁止自覆盖）。
        return;
    }
    mDependencies = deps;
    // 头部预览：head 副本上调用与执行完全相同的算子
    DAWaitCursorScoped waitCursor;
    DAPyDataFrame headDf = py.headCopy(mDf, c_previewRows);
    if (headDf.isNone()) {
        setStatusError(tr("Failed to build preview sample"));  // cn:预览样本构建失败
        mDependencies.clear();
        return;
    }
    const DAPyScriptsDataFrame::EvalColumnResult r = py.evalColumn(headDf, target, body);
    if (!r.ok) {
        setStatusError(tr("Expression evaluation failed: %1").arg(r.errorMessage));  // cn:表达式求值失败：%1
        mDependencies.clear();
        return;
    }
    // 预览表：依赖列 + 目标列
    QStringList previewCols = deps;
    if (!previewCols.contains(target)) {
        previewCols.append(target);
    }
    if (previewCols.isEmpty()) {
        previewCols.append(target);
    }
    const QList< QString > headCols = headDf.columns();
    const auto shape                = headDf.shape();
    const int nrows                 = static_cast< int >(shape.first);
    mTablePreview->clear();
    mTablePreview->setColumnCount(previewCols.size());
    mTablePreview->setRowCount(nrows);
    mTablePreview->setHorizontalHeaderLabels(previewCols);
    for (int c = 0; c < previewCols.size(); ++c) {
        const int pos = headCols.indexOf(previewCols.at(c));
        for (int rr = 0; rr < nrows; ++rr) {
            if (pos < 0) {
                continue;
            }
            QTableWidgetItem* item = new QTableWidgetItem(headDf.iat(rr, pos).toString());
            mTablePreview->setItem(rr, c, item);
        }
    }
    mTablePreview->resizeColumnsToContents();
    setStatusOk(tr("OK — %1 dependency column(s): %2").arg(deps.size()).arg(deps.join(QStringLiteral(", "))));  // cn:通过 — 依赖 %1 列：%2
}

}  // end of namespace DA
