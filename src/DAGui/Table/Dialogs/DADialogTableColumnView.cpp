#include "Table/Dialogs/DADialogTableColumnView.h"
// qt
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace DA
{

namespace
{
// 表格列索引
constexpr int c_colName   = 0;
constexpr int c_colHidden = 1;
constexpr int c_colFrozen = 2;

// 行背景色（DESIGN.md 官方色板，柔和不侵略）：
// 隐藏=浅灰 #B2B2B2（禁用语义）；冻结=浅蓝 #BDCDE8（选中底色语义）；
// 双勾（既冻结又隐藏）= 蓝灰中间色（两色各取一半，不引入新色板外的色值——
// 由官方色板两色混合而来）。为让文字可读，透明度压到 30%（在白底上呈柔和淡彩）
QColor hiddenRowColor()
{
    QColor c(0xB2, 0xB2, 0xB2);
    c.setAlpha(77);  // 30%
    return c;
}
QColor frozenRowColor()
{
    QColor c(0xBD, 0xCD, 0xE8);
    c.setAlpha(100);  // ~39%
    return c;
}
QColor frozenHiddenRowColor()
{
    QColor c((0xB2 + 0xBD) / 2, (0xB2 + 0xCD) / 2, (0xB2 + 0xE8) / 2);
    c.setAlpha(100);
    return c;
}

// 重名列显示文本：第二次出现起显示 "name (2)"（occurrence+1，显示文本不翻译）
QString displayTextOf(const QPair< QString, int >& id)
{
    return id.second == 0 ? id.first : QStringLiteral("%1 (%2)").arg(id.first).arg(id.second + 1);
}
}  // namespace

/**
 * @brief 构造函数：搜索框 + 三列表格 + 两个快捷按钮 + 标准按钮盒
 * @param parent 父窗口
 */
DADialogTableColumnView::DADialogTableColumnView(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Column View"));  // cn:列视图
    setMinimumSize(QSize(420, 480));
    QVBoxLayout* root = new QVBoxLayout(this);

    // 顶部：搜索框（保留 006d 原有能力）
    mSearchEdit = new QLineEdit(this);
    mSearchEdit->setObjectName(QStringLiteral("searchEdit"));
    mSearchEdit->setPlaceholderText(tr("Search..."));  // cn:搜索...
    mSearchEdit->setClearButtonEnabled(true);
    root->addWidget(mSearchEdit);

    // 中部：三列表格
    mTable = new QTableWidget(this);
    mTable->setObjectName(QStringLiteral("columnTable"));
    mTable->setColumnCount(3);
    QStringList headers;
    headers << tr("Column")   // cn:列名
            << tr("Hidden")   // cn:隐藏
            << tr("Frozen");  // cn:冻结
    mTable->setHorizontalHeaderLabels(headers);
    mTable->verticalHeader()->setVisible(false);
    mTable->horizontalHeader()->setStretchLastSection(false);
    mTable->horizontalHeader()->setSectionResizeMode(c_colName, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(c_colHidden, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(c_colFrozen, QHeaderView::ResizeToContents);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    root->addWidget(mTable, 1);

    // 底部：两个快捷按钮（替代原全选/反选 + 冻结数 spinbox）
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(0, 0, 0, 0);
    mBtnShowAll = new QPushButton(tr("Show All"), this);  // cn:显示全部
    mBtnShowAll->setObjectName(QStringLiteral("btnShowAll"));
    mBtnShowAll->setToolTip(tr("Unhide all columns"));  // cn:取消所有列的隐藏
    mBtnUnfreeze = new QPushButton(tr("Unfreeze All"), this);  // cn:取消全部冻结
    mBtnUnfreeze->setObjectName(QStringLiteral("btnUnfreeze"));
    mBtnUnfreeze->setToolTip(tr("Remove column freezing"));  // cn:取消所有列的冻结
    btnRow->addWidget(mBtnShowAll);
    btnRow->addWidget(mBtnUnfreeze);
    btnRow->addStretch(1);
    root->addLayout(btnRow);

    // 标准按钮盒
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    // 信号接线
    connect(mSearchEdit, &QLineEdit::textChanged, this, &DADialogTableColumnView::applySearchFilter);
    connect(mBtnShowAll, &QPushButton::clicked, this, &DADialogTableColumnView::showAllColumns);
    connect(mBtnUnfreeze, &QPushButton::clicked, this, &DADialogTableColumnView::unfreezeAllColumns);
    connect(mTable, &QTableWidget::cellChanged, this, &DADialogTableColumnView::onCellChanged);
}

DADialogTableColumnView::~DADialogTableColumnView()
{
}

/**
 * @brief (re)初始化表格
 *
 * 行 = 全局列序（含隐藏列，隐藏行留在原位勾选隐藏）；冻结勾选 = 该列是否在
 * 冻结集合里（逐列独立）。重开对话框时重建（用户上次的未确定编辑不保留）。
 * @param orderedColumns 全局列序（含隐藏列）
 * @param hiddenColumns 当前隐藏集合
 * @param frozenColumns 当前冻结列集合
 */
void DADialogTableColumnView::setup(const QList< QPair< QString, int > >& orderedColumns,
                                    const QSet< QPair< QString, int > >& hiddenColumns,
                                    const QSet< QPair< QString, int > >& frozenColumns)
{
    rebuildRows(orderedColumns, hiddenColumns, frozenColumns);
}

/**
 * @brief 重建表格行
 */
void DADialogTableColumnView::rebuildRows(const QList< QPair< QString, int > >& orderedColumns,
                                          const QSet< QPair< QString, int > >& hiddenColumns,
                                          const QSet< QPair< QString, int > >& frozenColumns)
{
    mUpdating = true;
    mOrderedColumns = orderedColumns;
    mTable->setRowCount(orderedColumns.size());
    for (int r = 0; r < orderedColumns.size(); ++r) {
        const QPair< QString, int >& id = orderedColumns.at(r);
        // 列名（可编辑；data 存行号自校验用）
        QTableWidgetItem* nameItem = new QTableWidgetItem(displayTextOf(id));
        nameItem->setData(Qt::UserRole, r);
        nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
        mTable->setItem(r, c_colName, nameItem);
        // 隐藏勾选（CheckState）
        QTableWidgetItem* hiddenItem = new QTableWidgetItem();
        hiddenItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        hiddenItem->setCheckState(hiddenColumns.contains(id) ? Qt::Checked : Qt::Unchecked);
        hiddenItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(r, c_colHidden, hiddenItem);
        // 冻结勾选（逐列独立，不再强制前缀）
        QTableWidgetItem* frozenItem = new QTableWidgetItem();
        frozenItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        frozenItem->setCheckState(frozenColumns.contains(id) ? Qt::Checked : Qt::Unchecked);
        frozenItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(r, c_colFrozen, frozenItem);
    }
    mUpdating = false;
    refreshRowBackgrounds();
}

/**
 * @brief 确定后的新隐藏集合
 *
 * = 所有隐藏勾选的行对应的列身份（含被搜索过滤掉的行）。
 */
QSet< QPair< QString, int > > DADialogTableColumnView::hiddenColumns() const
{
    QSet< QPair< QString, int > > res;
    for (int r = 0; r < mTable->rowCount() && r < mOrderedColumns.size(); ++r) {
        if (QTableWidgetItem* it = mTable->item(r, c_colHidden); it && it->checkState() == Qt::Checked) {
            res.insert(mOrderedColumns.at(r));
        }
    }
    return res;
}

/**
 * @brief 确定后的冻结列集合
 * @return 所有冻结勾选行的列身份（顺序由调用方按全局列序合并）
 */
QSet< QPair< QString, int > > DADialogTableColumnView::frozenColumns() const
{
    QSet< QPair< QString, int > > res;
    for (int r = 0; r < mTable->rowCount() && r < mOrderedColumns.size(); ++r) {
        if (QTableWidgetItem* it = mTable->item(r, c_colFrozen); it && it->checkState() == Qt::Checked) {
            res.insert(mOrderedColumns.at(r));
        }
    }
    return res;
}

/**
 * @brief 确定后的改名对
 *
 * 列名单元格文本与初始显示文本不同的行视为改名：((旧名, occurrence) → 新名)。
 * 重名列显示 "name (2)" 的行若保持显示文本不变不算改名；用户把它改成
 * 纯文本 "x" 即改名为 "x"（无消歧位语义，页面侧做重名校验）。
 */
QList< QPair< QPair< QString, int >, QString > > DADialogTableColumnView::renamedColumns() const
{
    QList< QPair< QPair< QString, int >, QString > > res;
    for (int r = 0; r < mTable->rowCount() && r < mOrderedColumns.size(); ++r) {
        const QTableWidgetItem* it = mTable->item(r, c_colName);
        if (!it) {
            continue;
        }
        const QString newText = it->text().trimmed();
        if (newText != displayTextOf(mOrderedColumns.at(r)) && !newText.isEmpty()) {
            res.append(qMakePair(mOrderedColumns.at(r), newText));
        }
    }
    return res;
}

/**
 * @brief 单元格变化统一入口
 *
 * 列名列：编辑提交（校验在 validateColumnName，非法回滚）；
 * 隐藏/冻结列：勾选变化——逐列独立，刷新行背景色即可。
 * @param row 行号
 * @param column 列号
 */
void DADialogTableColumnView::onCellChanged(int row, int column)
{
    if (mUpdating) {
        return;
    }
    if (column == c_colName) {
        QTableWidgetItem* it = mTable->item(row, c_colName);
        if (!it) {
            return;
        }
        if (!validateColumnName(row, it->text().trimmed())) {
            mUpdating = true;
            it->setText(displayTextOf(mOrderedColumns.at(row)));
            mUpdating = false;
        }
        return;
    }
    if (column == c_colHidden || column == c_colFrozen) {
        refreshRowBackgrounds();
    }
}

/**
 * @brief 列名编辑校验
 *
 * 非空 + 不与其他行的新列名重复（含未变化的行）。非法返回 false（调用方回滚）。
 * @param row 编辑的行
 * @param newName 新列名
 */
bool DADialogTableColumnView::validateColumnName(int row, const QString& newName)
{
    if (newName.isEmpty()) {
        return false;
    }
    for (int r = 0; r < mTable->rowCount(); ++r) {
        if (r == row) {
            continue;
        }
        const QTableWidgetItem* it = mTable->item(r, c_colName);
        if (it && it->text().trimmed() == newName) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 刷新全部行背景色（DESIGN.md 色板的柔和透明变体）
 */
void DADialogTableColumnView::refreshRowBackgrounds()
{
    for (int r = 0; r < mTable->rowCount(); ++r) {
        const bool hidden = mTable->item(r, c_colHidden) && mTable->item(r, c_colHidden)->checkState() == Qt::Checked;
        const bool frozen = mTable->item(r, c_colFrozen) && mTable->item(r, c_colFrozen)->checkState() == Qt::Checked;
        QColor bg;
        if (hidden && frozen) {
            bg = frozenHiddenRowColor();
        } else if (hidden) {
            bg = hiddenRowColor();
        } else if (frozen) {
            bg = frozenRowColor();
        }
        for (int c = 0; c < 3; ++c) {
            if (QTableWidgetItem* it = mTable->item(r, c)) {
                it->setBackground(bg.isValid() ? QBrush(bg) : QBrush());
            }
        }
    }
}

/**
 * @brief 搜索过滤：按列名包含匹配（大小写不敏感），隐藏不匹配的行
 *
 * 被过滤行的勾选/改名状态保留（确定时仍计入结果）。
 * @param text 搜索文本
 */
void DADialogTableColumnView::applySearchFilter(const QString& text)
{
    const QString key = text.trimmed();
    for (int r = 0; r < mTable->rowCount(); ++r) {
        const QTableWidgetItem* it = mTable->item(r, c_colName);
        mTable->setRowHidden(r, !key.isEmpty() && (!it || !it->text().contains(key, Qt::CaseInsensitive)));
    }
}

/**
 * @brief 显示全部：清掉所有行的隐藏勾选（对应「取消所有隐藏」）
 */
void DADialogTableColumnView::showAllColumns()
{
    mUpdating = true;
    for (int r = 0; r < mTable->rowCount(); ++r) {
        if (QTableWidgetItem* it = mTable->item(r, c_colHidden); it) {
            it->setCheckState(Qt::Unchecked);
        }
    }
    mUpdating = false;
    refreshRowBackgrounds();
}

/**
 * @brief 取消全部冻结：清掉所有行的冻结勾选
 */
void DADialogTableColumnView::unfreezeAllColumns()
{
    mUpdating = true;
    for (int r = 0; r < mTable->rowCount(); ++r) {
        if (QTableWidgetItem* it = mTable->item(r, c_colFrozen); it) {
            it->setCheckState(Qt::Unchecked);
        }
    }
    mUpdating = false;
    refreshRowBackgrounds();
}
}  // namespace DA
