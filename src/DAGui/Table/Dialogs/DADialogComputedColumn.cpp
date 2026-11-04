#include "Table/Dialogs/DADialogComputedColumn.h"
#include "Common/CodeEditor/DAPythonCodeEdit.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAWaitCursorScoped.h"
// Qt
#include <QAction>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace DA
{
namespace
{
// 判断字符是否可组成标识符（插入列名时决定是否补空格）
bool isWordChar(const QChar& c)
{
    return c.isLetterOrNumber() || c == QLatin1Char('_') || c == QLatin1Char('`');
}
}  // namespace

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
 * 布局：顶部目标列名；中部左右分割（左 2/3 代码编辑区，右 1/3 预览）；
 * 底部确定/取消。列名不占独立列表，改由编辑区上方的「插入列名」菜单注入。
 */
void DADialogComputedColumn::setupUI()
{
    setModal(true);

    // —— 目标列名 ——
    QWidget* targetPanel    = new QWidget(this);
    QHBoxLayout* targetLay  = new QHBoxLayout(targetPanel);
    targetLay->setContentsMargins(0, 0, 0, 0);
    QLabel* targetLabel = new QLabel(tr("Target column:"), targetPanel);  // cn:目标列名：
    mEditTarget         = new QLineEdit(targetPanel);
    mEditTarget->setObjectName(QStringLiteral("lineEditComputedTarget"));
    mEditTarget->setPlaceholderText(tr("New column name"));  // cn:新列名
    targetLay->addWidget(targetLabel);
    targetLay->addWidget(mEditTarget, 1);

    // —— 左侧：代码编辑区 ——
    QWidget* editorPanel    = new QWidget(this);
    QVBoxLayout* editorLay  = new QVBoxLayout(editorPanel);
    editorLay->setContentsMargins(0, 0, 0, 0);
    editorLay->setSpacing(4);
    QHBoxLayout* editorHeader = new QHBoxLayout();
    QLabel* exprLabel         = new QLabel(tr("Python expression / code"), editorPanel);  // cn:Python 表达式 / 代码段
    mButtonInsertColumn       = new QToolButton(editorPanel);
    mButtonInsertColumn->setObjectName(QStringLiteral("toolButtonInsertComputedColumn"));
    mButtonInsertColumn->setText(tr("Insert Column"));  // cn:插入列名
    mButtonInsertColumn->setToolTip(tr("Insert a column name at the cursor"));  // cn:在光标处插入列名
    mButtonInsertColumn->setPopupMode(QToolButton::InstantPopup);
    mButtonInsertColumn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    QMenu* columnMenu = new QMenu(mButtonInsertColumn);
    const QList< QString > cols = mDf.columns();
    for (const QString& c : cols) {
        QAction* act = columnMenu->addAction(c);
        connect(act, &QAction::triggered, this, [ this, c ]() { insertColumnName(c); });
    }
    if (columnMenu->isEmpty()) {
        columnMenu->addAction(tr("(no columns)"))->setEnabled(false);  // cn:（无列）
    }
    mButtonInsertColumn->setMenu(columnMenu);
    editorHeader->addWidget(exprLabel);
    editorHeader->addStretch();
    editorHeader->addWidget(mButtonInsertColumn);
    editorLay->addLayout(editorHeader);

    mEditExpr = new DAPythonCodeEdit(editorPanel);
    mEditExpr->setObjectName(QStringLiteral("plainTextEditComputedExpr"));
    mEditExpr->setToolTip(
        tr("Write a Python expression or code block. The last expression is the result, or assign it to `result`. "
           "Column names can be inserted from the menu."));  // cn:写 Python 表达式或代码段；最后一行表达式为结果，或赋值给 result；列名可从菜单插入
    QList< DAPythonCompletionEntry > entries;
    entries.reserve(cols.size());
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    for (const QString& c : cols) {
        DAPythonCompletionEntry e;
        e.text       = c;
        e.insertText = py.needsBacktick(c) ? (QStringLiteral("`") + c + QStringLiteral("`")) : c;
        entries.append(e);
    }
    mEditExpr->setExtraEntries(entries);
    editorLay->addWidget(mEditExpr, 1);

    // —— 右侧：校验状态 + 头部预览 ——
    QWidget* previewPanel   = new QWidget(this);
    QVBoxLayout* previewLay = new QVBoxLayout(previewPanel);
    previewLay->setContentsMargins(0, 0, 0, 0);
    mLabelStatus            = new QLabel(previewPanel);
    mLabelStatus->setObjectName(QStringLiteral("labelComputedStatus"));
    mLabelStatus->setWordWrap(true);
    mLabelStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mLabelStatus->setMinimumHeight(40);
    previewLay->addWidget(mLabelStatus);
    QGroupBox* previewGroup = new QGroupBox(tr("Preview (first %1 rows)").arg(c_previewRows), previewPanel);  // cn:预览（前 %1 行）
    QVBoxLayout* previewBoxLay = new QVBoxLayout(previewGroup);
    previewBoxLay->setContentsMargins(4, 4, 4, 4);
    mTablePreview = new QTableWidget(previewGroup);
    mTablePreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTablePreview->verticalHeader()->setVisible(false);
    previewBoxLay->addWidget(mTablePreview);
    previewLay->addWidget(previewGroup, 1);

    // —— 左右分割：编辑 2/3，预览 1/3 ——
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("splitterComputed"));
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(editorPanel);
    splitter->addWidget(previewPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({ 660, 330 });

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mButtonOk                 = buttons->button(QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    mButtonOk->setText(tr("OK"));                                      // cn:确定
    mButtonOk->setEnabled(false);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->addWidget(targetPanel);
    mainLay->addWidget(splitter, 1);
    mainLay->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 去抖校验（300ms）
    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(300);
    connect(mDebounceTimer, &QTimer::timeout, this, &DADialogComputedColumn::runValidationAndPreview);
    connect(mEditTarget, &QLineEdit::textChanged, this, &DADialogComputedColumn::onInputChanged);
    connect(mEditExpr, &QPlainTextEdit::textChanged, this, &DADialogComputedColumn::onInputChanged);
    // Ctrl+Enter：校验通过时直接确定
    connect(mEditExpr, &DAPythonCodeEdit::submitRequested, this, [ this ]() {
        if (mButtonOk->isEnabled()) {
            accept();
        }
    });

    resize(1060, 640);
}

/**
 * @brief 编辑模式（列头右键「编辑公式」）：目标列名固定只读，代码段预填
 * @param def 既有定义
 */
void DADialogComputedColumn::setEditMode(const DAComputedColumn& def)
{
    mEditMode = true;
    setWindowTitle(tr("Edit Formula"));  // cn:编辑公式
    mEditTarget->setText(def.targetColumnName);
    mEditTarget->setReadOnly(true);
    mEditExpr->setPlainText(def.expr);
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
    return mEditExpr->toPlainText().trimmed();
}

QStringList DADialogComputedColumn::dependencies() const
{
    return mDependencies;
}

/**
 * @brief 在代码编辑器光标处插入列名（按需加反引号与空格）
 * @param name 列名
 */
void DADialogComputedColumn::insertColumnName(const QString& name)
{
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QString ref              = py.needsBacktick(name) ? (QStringLiteral("`") + name + QStringLiteral("`")) : name;
    // 光标两侧都是标识符字符时补空格，避免粘成新标识符
    const QString text = mEditExpr->toPlainText();
    const int pos      = mEditExpr->textCursor().position();
    const QChar prev   = (pos > 0) ? text.at(pos - 1) : QChar();
    const QChar next   = (pos < text.size()) ? text.at(pos) : QChar();
    if (!ref.isEmpty() && isWordChar(prev) && isWordChar(ref.at(0))) {
        ref.prepend(QLatin1Char(' '));
    }
    if (!ref.isEmpty() && isWordChar(next) && isWordChar(ref.at(ref.size() - 1))) {
        ref.append(QLatin1Char(' '));
    }
    mEditExpr->insertCodeAtCursor(ref);
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
 * 目标列名非空 → 创建模式目标不存在（禁止自覆盖 R7）→ 代码非空 →
 * 代码可解析（ast）→ 依赖不含目标列（禁止自覆盖）→ head 副本求值成功。
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
        setStatusError(tr("Please input the Python expression or code"));  // cn:请输入 Python 表达式或代码
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
