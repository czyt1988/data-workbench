#include "Table/Dialogs/DATableSortDialog.h"
#include "Table/Controllers/DATableSortController.h"
// stl
#include <utility>
// qt
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace DA
{
namespace
{
// 行内控件统一宽度（表头行与数据行共用，保证上下对齐）
constexpr int c_rankWidth   = 18;  ///< 优先级序号列宽
constexpr int c_basisWidth  = 96;  ///< 「排序依据」列宽（当前只有一种依据，只读文字）
constexpr int c_orderWidth  = 104; ///< 次序下拉宽
constexpr int c_buttonWidth = 26;  ///< 行内小按钮宽
constexpr int c_buttonCount = 4;   ///< 行内按钮数（上移/下移/复制/删除）
constexpr int c_minWidth    = 620; ///< 对话框最小宽度（列名较长时由布局自行撑开）
}  // namespace

/**
 * @brief 构造
 * @param columnNames 当前表列名（下标即列号）
 * @param initialSpec 预置排序规格（列号 -1 = 索引，顺序即优先级；空则预置一行）
 * @param parent 父窗口
 */
DATableSortDialog::DATableSortDialog(const QStringList& columnNames,
                                     const QVector< QPair< int, Qt::SortOrder > >& initialSpec,
                                     QWidget* parent)
    : QDialog(parent), mColumnNames(columnNames)
{
    setupUI();
    setWindowTitle(tr("Sort"));  // cn:排序
    if (initialSpec.isEmpty()) {
        appendRow(0, Qt::AscendingOrder);
    } else {
        for (const auto& key : initialSpec) {
            appendRow(key.first, key.second);
        }
    }
    refreshRowState();
    // 收起多余空白（条件行数变化时 SetMinimumSize 约束会让对话框自动长高）
    adjustSize();
    if (width() < c_minWidth) {
        resize(c_minWidth, height());
    }
}

DATableSortDialog::~DATableSortDialog()
{
}

/**
 * @brief 界面搭建（代码构建，无 .ui）
 *
 * 结构照 Excel/WPS 的自定义排序窗口：顶部「添加条件」，中部「列 / 排序依据 / 次序」
 * 表头 + 条件行列表，底部提示与确定/取消。
 */
void DATableSortDialog::setupUI()
{
    setModal(true);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->setSizeConstraint(QLayout::SetMinimumSize);

    // 顶部工具条：添加条件（其余操作做成行内按钮，避免「先选行再点按钮」的两步操作）
    QHBoxLayout* toolLay = new QHBoxLayout();
    mButtonAdd          = new QToolButton(this);
    mButtonAdd->setText(tr("Add Condition"));  // cn:添加条件
    mButtonAdd->setIcon(QIcon(QStringLiteral(":/DAGui/icon/right-add.svg")));
    mButtonAdd->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mButtonAdd->setObjectName(QStringLiteral("toolButtonSortAddCondition"));
    toolLay->addWidget(mButtonAdd);
    toolLay->addStretch(1);
    mainLay->addLayout(toolLay);

    // 表头行（与条件行同宽布局，保证列对齐）
    QWidget* headerRow    = new QWidget(this);
    QHBoxLayout* headerLay = new QHBoxLayout(headerRow);
    headerLay->setContentsMargins(4, 0, 4, 0);
    headerLay->setSpacing(6);
    QLabel* headerRank = new QLabel(QString(), headerRow);
    headerRank->setFixedWidth(c_rankWidth);
    headerLay->addWidget(headerRank);
    QLabel* headerColumn = new QLabel(tr("Column"), headerRow);  // cn:列
    headerLay->addWidget(headerColumn, 1);
    QLabel* headerBasis = new QLabel(tr("Sort On"), headerRow);  // cn:排序依据
    headerBasis->setFixedWidth(c_basisWidth);
    headerLay->addWidget(headerBasis);
    QLabel* headerOrder = new QLabel(tr("Order"), headerRow);  // cn:次序
    headerOrder->setFixedWidth(c_orderWidth);
    headerLay->addWidget(headerOrder);
    for (int i = 0; i < c_buttonCount; ++i) {
        QLabel* pad = new QLabel(QString(), headerRow);
        pad->setFixedWidth(c_buttonWidth);
        headerLay->addWidget(pad);
    }
    mainLay->addWidget(headerRow);

    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLay->addWidget(line);

    // 条件行容器
    QWidget* rowsHost  = new QWidget(this);
    mRowsLayout        = new QVBoxLayout(rowsHost);
    mRowsLayout->setContentsMargins(0, 0, 0, 0);
    mRowsLayout->setSpacing(2);
    mainLay->addWidget(rowsHost);

    // 底部提示：排序不重编号，用「清除排序」还原
    mHintLabel = new QLabel(
        tr("Tip: sorting does not renumber rows. Use \"Clear Sort\" to restore the original row order."),
        this);  // cn:提示：排序不会重新编号行，可用「清除排序」按索引恢复原始行顺序。
    mHintLabel->setWordWrap(true);
    QFont hintFont = mHintLabel->font();
    hintFont.setPointSizeF(hintFont.pointSizeF() * 0.92);
    mHintLabel->setFont(hintFont);
    mainLay->addWidget(mHintLabel);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("OK"));                                      // cn:确定
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));                              // cn:取消
    mainLay->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(mButtonAdd, &QToolButton::clicked, this, &DATableSortDialog::addCondition);

    setMinimumWidth(c_minWidth);
}

/**
 * @brief 按给定列号/次序追加一行
 * @param column 列号（-1 = 索引）；越界按 0 处理
 * @param order 次序
 */
void DATableSortDialog::appendRow(int column, Qt::SortOrder order)
{
    RowWidgets row;
    row.container          = new QWidget(this);
    QHBoxLayout* rowLay    = new QHBoxLayout(row.container);
    rowLay->setContentsMargins(4, 0, 4, 0);
    rowLay->setSpacing(6);

    row.rank = new QLabel(row.container);
    row.rank->setFixedWidth(c_rankWidth);
    rowLay->addWidget(row.rank);

    row.column = new QComboBox(row.container);
    row.column->setObjectName(QStringLiteral("comboBoxSortColumn"));
    row.column->addItem(tr("Index (Row Number)"), DATableSortController::c_indexSortColumn);  // cn:索引（行号）
    for (int i = 0; i < mColumnNames.size(); ++i) {
        row.column->addItem(mColumnNames.at(i), i);
    }
    rowLay->addWidget(row.column, 1);

    // 排序依据：当前引擎只有「数值/单元格值」一种依据，用只读文字表达（不放假下拉）
    QLabel* basis = new QLabel(tr("Values"), row.container);  // cn:数值
    basis->setFixedWidth(c_basisWidth);
    basis->setToolTip(tr("Sorting always compares cell values (dates/numbers/text by their natural order)"));
    // cn:排序始终比较单元格的值（日期/数值/文本按其自然顺序）
    rowLay->addWidget(basis);

    row.order = new QComboBox(row.container);
    row.order->setObjectName(QStringLiteral("comboBoxSortOrder"));
    row.order->addItem(tr("Ascending"), static_cast< int >(Qt::AscendingOrder));   // cn:升序
    row.order->addItem(tr("Descending"), static_cast< int >(Qt::DescendingOrder));  // cn:降序
    row.order->setFixedWidth(c_orderWidth);
    rowLay->addWidget(row.order);

    auto makeButton = [ this, &rowLay, &row ](const QString& icon, const QString& tip) -> QToolButton* {
        QToolButton* btn = new QToolButton(row.container);
        btn->setAutoRaise(true);
        btn->setFixedWidth(c_buttonWidth);
        if (icon.isEmpty()) {
            btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        } else {
            btn->setIcon(QIcon(icon));
        }
        btn->setToolTip(tip);
        rowLay->addWidget(btn);
        return btn;
    };
    row.up     = makeButton(QString(), tr("Move up"));      // cn:上移
    row.up->setText(QStringLiteral("\u2191"));              // ↑
    row.down   = makeButton(QString(), tr("Move down"));    // cn:下移
    row.down->setText(QStringLiteral("\u2193"));            // ↓
    row.copy   = makeButton(QStringLiteral(":/DAGui/icon/copy.svg"), tr("Duplicate condition"));  // cn:复制条件
    row.remove = makeButton(QStringLiteral(":/DAGui/icon/delete.svg"), tr("Delete condition"));  // cn:删除条件

    // 列选择默认值（含索引伪列的 itemData=-1）
    const int wanted = (column == DATableSortController::c_indexSortColumn || column < 0)
                           ? DATableSortController::c_indexSortColumn
                           : qBound(0, column, qMax(0, mColumnNames.size() - 1));
    const int comboIdx = row.column->findData(wanted);
    row.column->setCurrentIndex(comboIdx >= 0 ? comboIdx : 0);
    row.order->setCurrentIndex(row.order->findData(static_cast< int >(order)));

    mRows.append(row);
    mRowsLayout->addWidget(row.container);

    // 行内按钮用捕获的行副本定位（控件指针稳定，行内容会随后续移动变化）
    connect(row.up, &QToolButton::clicked, this, [ this, row ]() { moveRow(row, -1); });
    connect(row.down, &QToolButton::clicked, this, [ this, row ]() { moveRow(row, 1); });
    connect(row.copy, &QToolButton::clicked, this, [ this, row ]() { duplicateRow(row); });
    connect(row.remove, &QToolButton::clicked, this, [ this, row ]() { removeRow(row); });
}

/**
 * @brief 该行当前选择的列号（-1 = 索引）
 */
int DATableSortDialog::rowColumn(const RowWidgets& row)
{
    if (!row.column) {
        return DATableSortController::c_indexSortColumn;
    }
    const QVariant data = row.column->currentData();
    return data.isValid() ? data.toInt() : DATableSortController::c_indexSortColumn;
}

/**
 * @brief 该行当前选择的次序
 */
Qt::SortOrder DATableSortDialog::rowOrder(const RowWidgets& row)
{
    if (!row.order) {
        return Qt::AscendingOrder;
    }
    const int v = row.order->currentData().toInt();
    return (v == static_cast< int >(Qt::DescendingOrder)) ? Qt::DescendingOrder : Qt::AscendingOrder;
}

/**
 * @brief 行下标
 * @return 不在列表中返回 -1
 */
int DATableSortDialog::rowIndexOf(const RowWidgets& row) const
{
    for (int i = 0; i < mRows.size(); ++i) {
        if (mRows.at(i).container == row.container) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 追加一行：列取第一个未被占用的列（都被占用时取第一列），次序升序
 */
void DATableSortDialog::addCondition()
{
    QList< int > used;
    for (const RowWidgets& r : std::as_const(mRows)) {
        used.append(rowColumn(r));
    }
    int column = 0;
    for (int i = 0; i < mColumnNames.size(); ++i) {
        if (!used.contains(i)) {
            column = i;
            break;
        }
    }
    appendRow(column, Qt::AscendingOrder);
    refreshRowState();
    // 新行的列下拉获得焦点，方便直接改列
    if (!mRows.isEmpty()) {
        mRows.last().column->setFocus();
    }
}

/**
 * @brief 删除一行（至少保留一行）
 * @param row 目标行
 */
void DATableSortDialog::removeRow(const RowWidgets& row)
{
    if (mRows.size() <= 1) {
        return;  // 至少保留一个条件（清空排序请用「清除排序」）
    }
    const int idx = rowIndexOf(row);
    if (idx < 0) {
        return;
    }
    mRows.removeAt(idx);
    if (row.container) {
        mRowsLayout->removeWidget(row.container);
        row.container->deleteLater();
    }
    refreshRowState();
}

/**
 * @brief 行整体上移/下移
 * @param row 目标行
 * @param delta -1 上移 / +1 下移
 */
void DATableSortDialog::moveRow(const RowWidgets& row, int delta)
{
    const int idx  = rowIndexOf(row);
    const int dest = idx + delta;
    if (idx < 0 || dest < 0 || dest >= mRows.size()) {
        return;
    }
    mRows.move(idx, dest);
    // 按 mRows 顺序重排布局里的控件（先摘下再插回，QWidget 顺序即优先级顺序）
    for (int i = 0; i < mRows.size(); ++i) {
        mRowsLayout->removeWidget(mRows.at(i).container);
    }
    for (int i = 0; i < mRows.size(); ++i) {
        mRowsLayout->addWidget(mRows.at(i).container);
    }
    refreshRowState();
}

/**
 * @brief 复制条件：在本行下方插入同样的一行
 * @param row 目标行
 */
void DATableSortDialog::duplicateRow(const RowWidgets& row)
{
    const int idx = rowIndexOf(row);
    if (idx < 0) {
        return;
    }
    appendRow(rowColumn(row), rowOrder(row));
    // appendRow 放在末尾：移到目标位置
    const int last = mRows.size() - 1;
    mRows.move(last, idx + 1);
    for (int i = 0; i < mRows.size(); ++i) {
        mRowsLayout->removeWidget(mRows.at(i).container);
    }
    for (int i = 0; i < mRows.size(); ++i) {
        mRowsLayout->addWidget(mRows.at(i).container);
    }
    refreshRowState();
}

/**
 * @brief 重建序号、刷新行按钮与确定按钮的可用状态
 */
void DATableSortDialog::refreshRowState()
{
    const int count = mRows.size();
    for (int i = 0; i < count; ++i) {
        RowWidgets& r = mRows[ i ];
        if (r.rank) {
            r.rank->setText(QStringLiteral("%1.").arg(i + 1));
        }
        if (r.up) {
            r.up->setEnabled(i > 0);
        }
        if (r.down) {
            r.down->setEnabled(i < count - 1);
        }
        if (r.remove) {
            r.remove->setEnabled(count > 1);
        }
    }
    if (QDialogButtonBox* box = findChild< QDialogButtonBox* >()) {
        if (QPushButton* okButton = box->button(QDialogButtonBox::Ok)) {
            okButton->setEnabled(count > 0);
        }
    }
}

/**
 * @brief 确定后的排序规格
 *
 * 同列（含索引）重复只保留首次出现（与 DATableSortController::applySpec 的清洗
 * 规则一致，保证「所见即所得」——结果以最靠前的条件为准）。
 * @return 列号（-1 = 索引）+ 方向列表，顺序即优先级
 */
QVector< QPair< int, Qt::SortOrder > > DATableSortDialog::sortSpec() const
{
    QVector< QPair< int, Qt::SortOrder > > spec;
    spec.reserve(mRows.size());
    for (const RowWidgets& r : mRows) {
        const QPair< int, Qt::SortOrder > key(rowColumn(r), rowOrder(r));
        bool duplicated = false;
        for (const auto& existing : std::as_const(spec)) {
            if (existing.first == key.first) {
                duplicated = true;
                break;
            }
        }
        if (!duplicated) {
            spec.append(key);
        }
    }
    return spec;
}
}  // end of namespace DA
