#include "DADialogGotoCell.h"
// qt
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace DA
{

/**
 * @brief 构造函数：只建 UI 外壳不设状态（配合 setup 复用）
 * @param parent 父窗口
 */
DADialogGotoCell::DADialogGotoCell(QWidget* parent) : QDialog(parent)
{
    setupUi();
}

DADialogGotoCell::~DADialogGotoCell()
{
}

/**
 * @brief 构建 UI（QFormLayout 三行 + 勾选 + 快捷按钮 + Ok/Cancel）
 */
void DADialogGotoCell::setupUi()
{
    setWindowTitle(tr("Go To"));  // cn:定位
    QVBoxLayout* root = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout();
    // 行号
    QHBoxLayout* rowLay = new QHBoxLayout();
    mSpinRow            = new QSpinBox(this);
    mSpinRow->setRange(1, 1);
    mLabelRowRange = new QLabel(this);
    rowLay->addWidget(mSpinRow);
    rowLay->addWidget(mLabelRowRange);
    rowLay->addStretch();
    form->addRow(tr("Row number:"), rowLay);  // cn:行号：
    // 索引标签（不设 QCompleter：索引可能上亿条）
    mEditIndexLabel = new QLineEdit(this);
    form->addRow(tr("Index label:"), mEditIndexLabel);  // cn:索引标签：
    // 列名
    mComboColumn = new QComboBox(this);
    mComboColumn->setEditable(true);
    QCompleter* completer = new QCompleter(mComboColumn);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    mComboColumn->setCompleter(completer);
    mComboColumn->setInsertPolicy(QComboBox::NoInsert);
    form->addRow(tr("Column:"), mComboColumn);  // cn:列：
    root->addLayout(form);
    // 勾选项（无隐藏列时隐藏）
    mChkKeepHidden = new QCheckBox(tr("Keep hidden columns hidden"), this);  // cn:保持隐藏列隐藏
    root->addWidget(mChkKeepHidden);
    // 快捷按钮行
    QHBoxLayout* quickLay = new QHBoxLayout();
    QPushButton* firstRow = new QPushButton(tr("First row"), this);        // cn:首行
    QPushButton* lastRow  = new QPushButton(tr("Last row"), this);        // cn:尾行
    QPushButton* firstCol = new QPushButton(tr("First column"), this);    // cn:首列
    QPushButton* lastCol  = new QPushButton(tr("Last column"), this);     // cn:尾列
    quickLay->addWidget(firstRow);
    quickLay->addWidget(lastRow);
    quickLay->addWidget(firstCol);
    quickLay->addWidget(lastCol);
    quickLay->addStretch();
    root->addLayout(quickLay);
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    root->addWidget(mButtonBox);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &DADialogGotoCell::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &DADialogGotoCell::reject);
    connect(firstRow, &QPushButton::clicked, this, [ this ]() { applyQuickRow(Request::RowKind::FirstRow); });
    connect(lastRow, &QPushButton::clicked, this, [ this ]() { applyQuickRow(Request::RowKind::LastRow); });
    connect(firstCol, &QPushButton::clicked, this, [ this ]() { applyQuickColumn(Request::ColumnKind::FirstColumn); });
    connect(lastCol, &QPushButton::clicked, this, [ this ]() { applyQuickColumn(Request::ColumnKind::LastColumn); });
}

/**
 * @brief (re)初始化对话框状态
 *
 * 第一步清 mPrepared（复用不回显上一次的快捷 kind），行列初值来自当前单元格。
 * @param totalRows 总行数（SpinBox 上限）
 * @param columnNames 全部列名（重名保留重复项）
 * @param currentRowPosition 当前行位置（1 基）
 * @param currentColumnName 当前列名
 * @param hasHiddenColumns 是否存在隐藏列（决定勾选项可见性）
 */
void DADialogGotoCell::setup(int totalRows, const QStringList& columnNames, int currentRowPosition,
                             const QString& currentColumnName, bool hasHiddenColumns)
{
    mPrepared = Request();  // 清上一次快捷 kind
    mInitialRowPosition = qBound(1, currentRowPosition, qMax(1, totalRows));
    mInitialColumnName  = currentColumnName;
    mSpinRow->setRange(1, qMax(1, totalRows));
    mSpinRow->setValue(mInitialRowPosition);
    mLabelRowRange->setText(tr("1 - %1").arg(totalRows));
    mEditIndexLabel->clear();
    mComboColumn->clear();
    mComboColumn->addItems(columnNames);
    if (!mInitialColumnName.isEmpty()) {
        mComboColumn->setCurrentText(mInitialColumnName);
    }
    mChkKeepHidden->setChecked(false);
    mChkKeepHidden->setVisible(hasHiddenColumns);
}

/**
 * @brief 用户请求
 *
 * 快捷按钮已置 kind 的直接沿用；行维度按"索引标签优先、行号改动次之"判定，
 * 列维度按"列名改动过"判定——没改 = 保持当前行/列。
 * @return 请求结构
 */
DADialogGotoCell::Request DADialogGotoCell::request() const
{
    Request r = mPrepared;  // 快捷按钮已置 kind 的直接沿用
    if (r.rowKind == Request::RowKind::None) {
        if (!mEditIndexLabel->text().trimmed().isEmpty()) {  // 索引标签优先（更精确）
            r.rowKind    = Request::RowKind::IndexLabel;
            r.indexLabel = mEditIndexLabel->text().trimmed();
        } else if (mSpinRow->value() != mInitialRowPosition) {  // 没改 = 不改行
            r.rowKind     = Request::RowKind::Position;
            r.rowPosition = mSpinRow->value();
        }
    }
    if (r.columnKind == Request::ColumnKind::None) {
        const QString col = mComboColumn->currentText().trimmed();
        if (!col.isEmpty() && col != mInitialColumnName) {  // 没改 = 保持当前列
            r.columnKind = Request::ColumnKind::Name;
            r.columnName = col;
        }
    }
    r.keepHidden = mChkKeepHidden->isChecked();
    return r;
}

/**
 * @brief 行快捷项：置 kind 后立即关框
 */
void DADialogGotoCell::applyQuickRow(Request::RowKind k)
{
    mPrepared.rowKind = k;
    accept();
}

/**
 * @brief 列快捷项：置 kind 后立即关框
 */
void DADialogGotoCell::applyQuickColumn(Request::ColumnKind k)
{
    mPrepared.columnKind = k;
    accept();
}
}  // end of namespace DA
