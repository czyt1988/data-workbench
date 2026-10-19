#include "DADialogMergeColumns.h"
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAWaitCursorScoped.h"
#include <QListWidget>
#include <QLineEdit>
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
#include <QToolButton>

namespace DA
{

/**
 * @brief 构造
 * @param df 数据（只读；预览在 head 副本上做）
 * @param sourceCols 源列位置（初始顺序 = 传入顺序，一般为选区升序）
 * @param parent 父窗口
 */
DADialogMergeColumns::DADialogMergeColumns(const DAPyDataFrame& df, const QList< int >& sourceCols, QWidget* parent)
    : QDialog(parent), mDf(df), mSourceCols(sourceCols)
{
    setupUI();
    setWindowTitle(tr("Merge Columns"));  // cn:合并列
    runPreview();
}

DADialogMergeColumns::~DADialogMergeColumns()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）：左=顺序列表+参数，右=状态+预览
 */
void DADialogMergeColumns::setupUI()
{
    setModal(true);
    const QList< QString > cols = mDf.columns();
    QWidget* leftPanel          = new QWidget(this);
    QVBoxLayout* leftLay        = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    QGroupBox* orderGroup       = new QGroupBox(tr("Merge order (top to bottom)"), leftPanel);  // cn:合并顺序（自上而下）
    QHBoxLayout* orderLay       = new QHBoxLayout(orderGroup);
    mListColumns                = new QListWidget(orderGroup);
    mListColumns->setObjectName(QStringLiteral("listMergeColumns"));
    for (int c : std::as_const(mSourceCols)) {
        if (c >= 0 && c < cols.size()) {
            QListWidgetItem* item = new QListWidgetItem(cols.at(c), mListColumns);
            item->setData(Qt::UserRole, c);
        }
    }
    if (mListColumns->count() > 0) {
        mListColumns->setCurrentRow(0);
    }
    QVBoxLayout* btnLay = new QVBoxLayout();
    QToolButton* btnUp   = new QToolButton(orderGroup);
    QToolButton* btnDown = new QToolButton(orderGroup);
    btnUp->setArrowType(Qt::UpArrow);
    btnDown->setArrowType(Qt::DownArrow);
    btnUp->setObjectName(QStringLiteral("buttonMoveUp"));
    btnDown->setObjectName(QStringLiteral("buttonMoveDown"));
    btnLay->addWidget(btnUp);
    btnLay->addWidget(btnDown);
    btnLay->addStretch();
    orderLay->addWidget(mListColumns, 1);
    orderLay->addLayout(btnLay);
    leftLay->addWidget(orderGroup, 1);
    QFormLayout* form = new QFormLayout();
    mEditSep          = new QLineEdit(leftPanel);
    mEditSep->setObjectName(QStringLiteral("lineEditSep"));
    mEditSep->setPlaceholderText(tr("can be empty"));  // cn:可为空
    form->addRow(tr("Separator:"), mEditSep);  // cn:分隔符：
    mCheckSkipNa = new QCheckBox(tr("Skip missing values (no extra separator)"), leftPanel);  // cn:跳过空值（不留多余分隔符）
    mCheckSkipNa->setChecked(true);
    form->addRow(mCheckSkipNa);
    mEditName = new QLineEdit(leftPanel);
    mEditName->setObjectName(QStringLiteral("lineEditMergeName"));
    // 默认结果列名：各源列名以 "_" 连接
    QStringList srcNames;
    for (int c : std::as_const(mSourceCols)) {
        if (c >= 0 && c < cols.size()) {
            srcNames.append(cols.at(c));
        }
    }
    mEditName->setText(srcNames.join(QLatin1Char('_')));
    form->addRow(tr("Result column name:"), mEditName);  // cn:结果列名：
    leftLay->addLayout(form);

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
    connect(btnUp, &QToolButton::clicked, this, &DADialogMergeColumns::onMoveUp);
    connect(btnDown, &QToolButton::clicked, this, &DADialogMergeColumns::onMoveDown);

    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(300);
    connect(mDebounceTimer, &QTimer::timeout, this, &DADialogMergeColumns::runPreview);
    connect(mEditSep, &QLineEdit::textChanged, this, &DADialogMergeColumns::onInputChanged);
    connect(mCheckSkipNa, &QCheckBox::toggled, this, &DADialogMergeColumns::onInputChanged);
    connect(mEditName, &QLineEdit::textEdited, this, [ this ]() {
        mNameEdited = true;
        onInputChanged();
    });

    resize(820, 460);
}

void DADialogMergeColumns::onMoveUp()
{
    const int row = mListColumns->currentRow();
    if (row <= 0) {
        return;
    }
    QListWidgetItem* item = mListColumns->takeItem(row);
    mListColumns->insertItem(row - 1, item);
    mListColumns->setCurrentRow(row - 1);
    runPreview();
}

void DADialogMergeColumns::onMoveDown()
{
    const int row = mListColumns->currentRow();
    if (row < 0 || row >= mListColumns->count() - 1) {
        return;
    }
    QListWidgetItem* item = mListColumns->takeItem(row);
    mListColumns->insertItem(row + 1, item);
    mListColumns->setCurrentRow(row + 1);
    runPreview();
}

void DADialogMergeColumns::onInputChanged()
{
    mDebounceTimer->start();
}

/**
 * @brief 当前顺序的源列位置列表（列表项顺序 = 拼接顺序）
 */
QList< int > DADialogMergeColumns::orderedCols() const
{
    QList< int > cols;
    cols.reserve(mListColumns->count());
    for (int i = 0; i < mListColumns->count(); ++i) {
        cols.append(mListColumns->item(i)->data(Qt::UserRole).toInt());
    }
    return cols;
}

void DADialogMergeColumns::setStatusError(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#CE6043;"));  // 橙红（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(false);
}

void DADialogMergeColumns::setStatusOk(const QString& msg)
{
    mLabelStatus->setStyleSheet(QStringLiteral("color:#669E8B;"));  // 绿（官方色板）
    mLabelStatus->setText(msg);
    mButtonOk->setEnabled(true);
}

/**
 * @brief 校验 + 头部预览（预览与执行同一算子：da_head_copy + da_merge_columns，红线5）
 */
void DADialogMergeColumns::runPreview()
{
    mTablePreview->clear();
    mTablePreview->setRowCount(0);
    mTablePreview->setColumnCount(0);
    if (mDf.isNone()) {
        setStatusError(tr("No data"));  // cn:无数据
        return;
    }
    const QList< int > cols = orderedCols();
    if (cols.size() < 2) {
        setStatusError(tr("Merge requires at least 2 columns"));  // cn:合并至少需要 2 列
        return;
    }
    const QString name = mEditName->text().trimmed();
    if (name.isEmpty()) {
        setStatusError(tr("Result column name must not be empty"));  // cn:结果列名不能为空
        return;
    }
    const QList< QString > existing = mDf.columns();
    if (existing.contains(name)) {
        setStatusError(tr("Column \"%1\" already exists, please use another result name").arg(name));  // cn:列“%1”已存在，请更换结果列名
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    DAWaitCursorScoped waitCursor;
    DAPyDataFrame head       = py.headCopy(mDf, c_previewRows);
    if (head.isNone()) {
        setStatusError(tr("Failed to build preview sample"));  // cn:预览样本构建失败
        return;
    }
    QString err;
    const int pos = py.mergeColumns(head, cols, mEditSep->text(), mCheckSkipNa->isChecked(), name, &err);
    if (pos < 0) {
        setStatusError(tr("Merge failed: %1").arg(err));  // cn:合并失败：%1
        return;
    }
    // 预览表：结果列 + 其余保留列（head 副本已被就地合并，值即所见）
    const auto shape    = head.shape();
    const int nrows     = static_cast< int >(shape.first);
    const QStringList headCols = head.columns();
    mTablePreview->setColumnCount(headCols.size());
    mTablePreview->setRowCount(nrows);
    mTablePreview->setHorizontalHeaderLabels(headCols);
    for (int c = 0; c < headCols.size(); ++c) {
        for (int r = 0; r < nrows; ++r) {
            mTablePreview->setItem(r, c, new QTableWidgetItem(head.iat(r, c).toString()));
        }
    }
    mTablePreview->resizeColumnsToContents();
    setStatusOk(tr("OK — merged into \"%1\" at column %2").arg(name).arg(pos + 1));  // cn:通过 — 合并为“%1”，位于第 %2 列
}

/**
 * @brief 接受后的合并参数（cols 顺序 = 列表顺序 = 拼接顺序）
 */
DACommandDataFrame_mergeColumns::MergeParams DADialogMergeColumns::params() const
{
    DACommandDataFrame_mergeColumns::MergeParams p;
    p.cols   = orderedCols();
    p.sep    = mEditSep->text();
    p.skipNa = mCheckSkipNa->isChecked();
    p.name   = mEditName->text().trimmed();
    return p;
}

}  // end of namespace DA
