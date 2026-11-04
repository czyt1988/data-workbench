#include "Table/Dialogs/DADialogManageColumns.h"
// qt
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QModelIndex>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <functional>

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

/**
 * @brief 支持内部拖动排序的表格
 *
 * 只做一件事：把「拖到自己身上」的落点换算成行移动回调（from → to），
 * 行数据的真源在对话框的 mRows，表格只是渲染，所以不走模型的 dropMimeData。
 * 落点换算按拖放指示器：AboveItem = 插到目标行之前，Below/OnItem = 之后；
 * 落到空白区 = 移到最后。搜索过滤（setRowHidden）不影响该换算。
 */
class DAColumnManageTable : public QTableWidget
{
public:
    using MoveHandler = std::function< void(int from, int to) >;
    explicit DAColumnManageTable(QWidget* parent = nullptr) : QTableWidget(parent)
    {
    }
    // 设置行移动回调
    void setMoveHandler(MoveHandler handler)
    {
        mMoveHandler = std::move(handler);
    }

protected:
    void dropEvent(QDropEvent* e) override
    {
        if (!mMoveHandler || e->source() != this) {
            QTableWidget::dropEvent(e);
            return;
        }
        const int from = currentRow();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const QPoint pos = e->pos();
#else
        const QPoint pos = e->position().toPoint();
#endif
        const QModelIndex idx = indexAt(pos);
        int to                = rowCount() - 1;
        if (idx.isValid()) {
            to = (dropIndicatorPosition() == QAbstractItemView::AboveItem) ? idx.row() : idx.row() + 1;
        }
        if (to > from) {
            --to;  // 按住的行先被取走，落点在它之后时目标行号前移一位
        }
        if (to < 0) {
            to = 0;
        }
        if (to >= rowCount()) {
            to = rowCount() - 1;
        }
        if (from >= 0 && to >= 0 && to != from) {
            mMoveHandler(from, to);
        }
        e->acceptProposedAction();
    }

private:
    MoveHandler mMoveHandler;
};
}  // namespace

/**
 * @brief 构造函数：搜索框 + 三列表格 + 四个快捷按钮 + 标准按钮盒
 * @param parent 父窗口
 */
DADialogManageColumns::DADialogManageColumns(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Manage Columns"));  // cn:列管理
    setMinimumSize(QSize(480, 520));
    QVBoxLayout* root = new QVBoxLayout(this);

    // 顶部：搜索框
    mSearchEdit = new QLineEdit(this);
    mSearchEdit->setObjectName(QStringLiteral("searchEdit"));
    mSearchEdit->setPlaceholderText(tr("Search..."));  // cn:搜索...
    mSearchEdit->setClearButtonEnabled(true);
    root->addWidget(mSearchEdit);

    // 操作提示：行序即列序（拖动或上移/下移）
    QLabel* hint = new QLabel(tr("Rows are shown in column order. Drag rows or use Move Up/Down to reorder columns."));  // cn:行序即列序，拖动行或使用上移/下移调整列顺序
    hint->setWordWrap(true);
    root->addWidget(hint);

    // 中部：三列表格
    mTable = new DAColumnManageTable(this);
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
    // 行拖动排序：内部移动（落点由 dropEvent 换算成行移动，不交给模型处理）
    mTable->setDragEnabled(true);
    mTable->setAcceptDrops(true);
    mTable->viewport()->setAcceptDrops(true);
    mTable->setDropIndicatorShown(true);
    mTable->setDragDropMode(QAbstractItemView::InternalMove);
    root->addWidget(mTable, 1);

    // 底部：上移/下移 + 两个快捷按钮
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(0, 0, 0, 0);
    mBtnMoveUp = new QPushButton(tr("Move Up"), this);  // cn:上移
    mBtnMoveUp->setObjectName(QStringLiteral("btnMoveUp"));
    mBtnMoveUp->setToolTip(tr("Move the selected column forward"));  // cn:把选中列向前移动
    mBtnMoveDown = new QPushButton(tr("Move Down"), this);  // cn:下移
    mBtnMoveDown->setObjectName(QStringLiteral("btnMoveDown"));
    mBtnMoveDown->setToolTip(tr("Move the selected column backward"));  // cn:把选中列向后移动
    mBtnShowAll = new QPushButton(tr("Show All"), this);  // cn:显示全部
    mBtnShowAll->setObjectName(QStringLiteral("btnShowAll"));
    mBtnShowAll->setToolTip(tr("Unhide all columns"));  // cn:取消所有列的隐藏
    mBtnUnfreeze = new QPushButton(tr("Unfreeze All"), this);  // cn:取消全部冻结
    mBtnUnfreeze->setObjectName(QStringLiteral("btnUnfreeze"));
    mBtnUnfreeze->setToolTip(tr("Remove column freezing"));  // cn:取消所有列的冻结
    btnRow->addWidget(mBtnMoveUp);
    btnRow->addWidget(mBtnMoveDown);
    btnRow->addSpacing(12);
    btnRow->addWidget(mBtnShowAll);
    btnRow->addWidget(mBtnUnfreeze);
    btnRow->addStretch(1);
    root->addLayout(btnRow);

    // 校验提示（内联状态，不弹模态框；文案色取 DESIGN.md 错误红 #CE6043）
    mStatusLabel = new QLabel(this);
    mStatusLabel->setObjectName(QStringLiteral("statusLabel"));
    mStatusLabel->setWordWrap(true);
    mStatusLabel->setStyleSheet(QStringLiteral("color: #CE6043;"));
    mStatusLabel->hide();
    root->addWidget(mStatusLabel);

    // 标准按钮盒
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    // 信号接线
    connect(mSearchEdit, &QLineEdit::textChanged, this, &DADialogManageColumns::applySearchFilter);
    connect(mBtnMoveUp, &QPushButton::clicked, this, [ this ]() { moveSelectedRow(-1); });
    connect(mBtnMoveDown, &QPushButton::clicked, this, [ this ]() { moveSelectedRow(1); });
    connect(mBtnShowAll, &QPushButton::clicked, this, &DADialogManageColumns::showAllColumns);
    connect(mBtnUnfreeze, &QPushButton::clicked, this, &DADialogManageColumns::unfreezeAllColumns);
    connect(mTable, &QTableWidget::cellChanged, this, &DADialogManageColumns::onCellChanged);
    connect(mTable, &QTableWidget::itemSelectionChanged, this, &DADialogManageColumns::updateMoveButtons);
    // 拖动落点由表格子类换算成行移动
    static_cast< DAColumnManageTable* >(mTable)->setMoveHandler([ this ](int from, int to) { moveRow(from, to); });
}

/**
 * @brief 析构函数
 */
DADialogManageColumns::~DADialogManageColumns()
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
void DADialogManageColumns::setup(const QList< QPair< QString, int > >& orderedColumns,
                                  const QSet< QPair< QString, int > >& hiddenColumns,
                                  const QSet< QPair< QString, int > >& frozenColumns)
{
    rebuildRows(orderedColumns, hiddenColumns, frozenColumns);
}

/**
 * @brief 重建行数据（列身份 + 当前编辑态）
 */
void DADialogManageColumns::rebuildRows(const QList< QPair< QString, int > >& orderedColumns,
                                        const QSet< QPair< QString, int > >& hiddenColumns,
                                        const QSet< QPair< QString, int > >& frozenColumns)
{
    mRows.clear();
    mRows.reserve(orderedColumns.size());
    for (const QPair< QString, int >& ident : orderedColumns) {
        Row row;
        row.ident  = ident;
        row.text   = displayTextOf(ident);
        row.hidden = hiddenColumns.contains(ident);
        row.frozen = frozenColumns.contains(ident);
        mRows.append(row);
    }
    mSearchEdit->clear();
    setStatusText(QString());
    refreshTable();
    updateMoveButtons();
}

/**
 * @brief 按 mRows 重建表格项
 *
 * 表格只是 mRows 的渲染层：行号 = mRows 下标，移动/改名后整体重建。
 */
void DADialogManageColumns::refreshTable()
{
    mUpdating = true;
    mTable->setRowCount(static_cast< int >(mRows.size()));
    for (int r = 0; r < mRows.size(); ++r) {
        const Row& row = mRows.at(r);
        // 列名（可编辑、可拖动；data 存行号自校验用）
        QTableWidgetItem* nameItem = new QTableWidgetItem(row.text);
        nameItem->setData(Qt::UserRole, r);
        nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
        mTable->setItem(r, c_colName, nameItem);
        // 隐藏勾选（CheckState）
        QTableWidgetItem* hiddenItem = new QTableWidgetItem();
        hiddenItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable
                             | Qt::ItemIsDragEnabled);
        hiddenItem->setCheckState(row.hidden ? Qt::Checked : Qt::Unchecked);
        hiddenItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(r, c_colHidden, hiddenItem);
        // 冻结勾选（逐列独立，不再强制前缀）
        QTableWidgetItem* frozenItem = new QTableWidgetItem();
        frozenItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable
                             | Qt::ItemIsDragEnabled);
        frozenItem->setCheckState(row.frozen ? Qt::Checked : Qt::Unchecked);
        frozenItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(r, c_colFrozen, frozenItem);
    }
    mUpdating = false;
    applySearchFilter(mSearchEdit->text());
    refreshRowBackgrounds();
}

/**
 * @brief 确定后的新隐藏集合
 *
 * = 所有隐藏勾选的行对应的列身份（含被搜索过滤掉的行）。
 */
QSet< QPair< QString, int > > DADialogManageColumns::hiddenColumns() const
{
    QSet< QPair< QString, int > > res;
    for (const Row& row : mRows) {
        if (row.hidden) {
            res.insert(row.ident);
        }
    }
    return res;
}

/**
 * @brief 确定后的冻结列集合
 * @return 所有冻结勾选行的列身份（顺序由调用方按全局列序合并）
 */
QSet< QPair< QString, int > > DADialogManageColumns::frozenColumns() const
{
    QSet< QPair< QString, int > > res;
    for (const Row& row : mRows) {
        if (row.frozen) {
            res.insert(row.ident);
        }
    }
    return res;
}

/**
 * @brief 确定后的改名对
 *
 * 列名文本与初始显示文本不同的行视为改名：((旧名, occurrence) → 新名)。
 * 重名列显示 "name (2)" 的行若保持显示文本不变不算改名；用户把它改成
 * 纯文本 "x" 即改名为 "x"（无消歧位语义，页面侧做重名校验）。
 */
QList< QPair< QPair< QString, int >, QString > > DADialogManageColumns::renamedColumns() const
{
    QList< QPair< QPair< QString, int >, QString > > res;
    for (const Row& row : mRows) {
        const QString newText = row.text.trimmed();
        if (!newText.isEmpty() && newText != displayTextOf(row.ident)) {
            res.append(qMakePair(row.ident, newText));
        }
    }
    return res;
}

/**
 * @brief 确定后的列顺序
 * @return 列身份有序表（行序 = 新全局列序）
 */
QList< QPair< QString, int > > DADialogManageColumns::columnOrder() const
{
    QList< QPair< QString, int > > res;
    res.reserve(mRows.size());
    for (const Row& row : mRows) {
        res.append(row.ident);
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
void DADialogManageColumns::onCellChanged(int row, int column)
{
    if (mUpdating || row < 0 || row >= mRows.size()) {
        return;
    }
    if (column == c_colName) {
        QTableWidgetItem* it = mTable->item(row, c_colName);
        if (!it) {
            return;
        }
        const QString text = it->text().trimmed();
        if (!validateColumnName(row, text)) {
            // 非法：回滚到本行上次有效文本（提示由 validateColumnName 内联给出）
            mUpdating = true;
            it->setText(mRows.at(row).text);
            mUpdating = false;
            return;
        }
        setStatusText(QString());
        mRows[ row ].text = text;
        return;
    }
    if (column == c_colHidden) {
        const QTableWidgetItem* it = mTable->item(row, c_colHidden);
        mRows[ row ].hidden       = it && it->checkState() == Qt::Checked;
        refreshRowBackgrounds();
        return;
    }
    if (column == c_colFrozen) {
        const QTableWidgetItem* it = mTable->item(row, c_colFrozen);
        mRows[ row ].frozen       = it && it->checkState() == Qt::Checked;
        refreshRowBackgrounds();
    }
}

/**
 * @brief 列名编辑校验
 *
 * 非空 + 不与其他行的列名重复（含未变化的行）。非法返回 false（调用方回滚）。
 * @param row 编辑的行
 * @param newName 新列名
 */
bool DADialogManageColumns::validateColumnName(int row, const QString& newName)
{
    if (newName.isEmpty()) {
        setStatusText(tr("Column name cannot be empty"));  // cn:列名不能为空
        return false;
    }
    for (int r = 0; r < mRows.size(); ++r) {
        if (r == row) {
            continue;
        }
        if (mRows.at(r).text.trimmed() == newName) {
            setStatusText(tr("Column name \"%1\" already exists, please use another name").arg(newName));  // cn:列名"%1"已存在，请使用其他名称
            return false;
        }
    }
    return true;
}

/**
 * @brief 设置/清除底部内联提示
 * @param text 提示文本（空 = 隐藏）
 */
void DADialogManageColumns::setStatusText(const QString& text)
{
    mStatusLabel->setText(text);
    mStatusLabel->setVisible(!text.isEmpty());
}

/**
 * @brief 上下移动选中行
 *
 * 只在可见行之间移动（搜索过滤掉的行跳过）：上移 = 与上方最近的可见行交换，
 * 下移 = 与下方最近的可见行交换。
 * @param delta -1 上移，+1 下移
 */
void DADialogManageColumns::moveSelectedRow(int delta)
{
    const int row = selectionRow();
    if (row < 0) {
        return;
    }
    int target = -1;
    for (int r = row + delta; r >= 0 && r < mRows.size(); r += delta) {
        if (!mTable->isRowHidden(r)) {
            target = r;
            break;
        }
    }
    if (target < 0) {
        return;
    }
    moveRow(row, target);
}

/**
 * @brief 把 from 行移动到 to 位置
 *
 * mRows 先动，再整体重建表格（保持勾选/改名文本），最后选中新位置。
 * @param from 源行
 * @param to 目标行
 */
void DADialogManageColumns::moveRow(int from, int to)
{
    if (from < 0 || from >= mRows.size() || to < 0 || to >= mRows.size() || from == to) {
        return;
    }
    mRows.move(from, to);
    refreshTable();
    mTable->selectRow(to);
    mTable->setCurrentCell(to, c_colName);
    updateMoveButtons();
}

/**
 * @brief 当前选中行
 * @return 行号；无选中返回 -1
 */
int DADialogManageColumns::selectionRow() const
{
    return mTable->currentRow();
}

/**
 * @brief 刷新上移/下移按钮可用态
 *
 * 无选中、已在首/末可见行时对应按钮禁用（拖动不受此限制）。
 */
void DADialogManageColumns::updateMoveButtons()
{
    const int row = selectionRow();
    bool canUp    = false;
    bool canDown  = false;
    if (row >= 0) {
        for (int r = row - 1; r >= 0; --r) {
            if (!mTable->isRowHidden(r)) {
                canUp = true;
                break;
            }
        }
        for (int r = row + 1; r < mRows.size(); ++r) {
            if (!mTable->isRowHidden(r)) {
                canDown = true;
                break;
            }
        }
    }
    if (mBtnMoveUp) {
        mBtnMoveUp->setEnabled(canUp);
    }
    if (mBtnMoveDown) {
        mBtnMoveDown->setEnabled(canDown);
    }
}

/**
 * @brief 刷新全部行背景色（DESIGN.md 色板的柔和透明变体）
 */
void DADialogManageColumns::refreshRowBackgrounds()
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
void DADialogManageColumns::applySearchFilter(const QString& text)
{
    const QString key = text.trimmed();
    for (int r = 0; r < mTable->rowCount(); ++r) {
        const QTableWidgetItem* it = mTable->item(r, c_colName);
        mTable->setRowHidden(r, !key.isEmpty() && (!it || !it->text().contains(key, Qt::CaseInsensitive)));
    }
    updateMoveButtons();
}

/**
 * @brief 显示全部：清掉所有行的隐藏勾选（对应「取消所有隐藏」）
 */
void DADialogManageColumns::showAllColumns()
{
    mUpdating = true;
    for (int r = 0; r < mTable->rowCount() && r < mRows.size(); ++r) {
        if (QTableWidgetItem* it = mTable->item(r, c_colHidden); it) {
            it->setCheckState(Qt::Unchecked);
        }
        mRows[ r ].hidden = false;
    }
    mUpdating = false;
    refreshRowBackgrounds();
}

/**
 * @brief 取消全部冻结：清掉所有行的冻结勾选
 */
void DADialogManageColumns::unfreezeAllColumns()
{
    mUpdating = true;
    for (int r = 0; r < mTable->rowCount() && r < mRows.size(); ++r) {
        if (QTableWidgetItem* it = mTable->item(r, c_colFrozen); it) {
            it->setCheckState(Qt::Unchecked);
        }
        mRows[ r ].frozen = false;
    }
    mUpdating = false;
    refreshRowBackgrounds();
}
}  // namespace DA
