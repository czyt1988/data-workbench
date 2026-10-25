#include "DADialogTableColumnView.h"
#include "ui_DADialogTableColumnView.h"
// qt
#include <QSpinBox>
// DAWidgets
#include "DACheckListWidget.h"

namespace DA
{

namespace
{
// 列标识 (列名, occurrence) 存进 DAItemEntry::data，避免重名列混淆
QPair< QString, int > idOf(const DAItemEntry& e)
{
    const QVariantList l = e.data.toList();
    return qMakePair(l.value(0).toString(), l.value(1).toInt());
}

DAItemEntry entryOf(const QPair< QString, int >& id, const QString& text)
{
    QVariantList data;
    data << id.first << id.second;
    return DAItemEntry { text, data };
}
}  // namespace

/**
 * @brief 构造函数：只建壳不设状态（配合 setup 复用）
 * @param parent 父窗口
 */
DADialogTableColumnView::DADialogTableColumnView(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogTableColumnView)
{
    ui->setupUi(this);
    connect(ui->checkListColumns, &DACheckListWidget::checkedEntriesChanged, this, [ this ]() { updateFreezeRange(); });
}

DADialogTableColumnView::~DADialogTableColumnView()
{
    delete ui;
}

/**
 * @brief (re)初始化列表与冻结数
 *
 * 列表按全局列序排列，勾选 = 可见；重名列显示“列名 (2)”（occurrence+1，
 * 显示文本是数据不翻译）。冻结数起始值取生效冻结数（夹紧后的值），上限 =
 * 当前勾选数。
 * @param orderedColumns 全局列序（含隐藏列）
 * @param hiddenColumns 当前隐藏集合
 * @param effectiveFrozenColumnCount 生效冻结数
 */
void DADialogTableColumnView::setup(const QList< QPair< QString, int > >& orderedColumns,
                                    const QSet< QPair< QString, int > >& hiddenColumns,
                                    int effectiveFrozenColumnCount)
{
    QVector< DAItemEntry > entries;
    QVector< DAItemEntry > visible;
    entries.reserve(orderedColumns.size());
    for (const QPair< QString, int >& id : orderedColumns) {  // const：避免 COW 深拷贝
        const QString text = id.second == 0 ? id.first : QStringLiteral("%1 (%2)").arg(id.first).arg(id.second + 1);
        const DAItemEntry e = entryOf(id, text);
        entries.append(e);
        if (!hiddenColumns.contains(id)) {
            visible.append(e);
        }
    }
    ui->checkListColumns->setItems(entries);
    ui->checkListColumns->setCheckedEntries(visible);
    const int checked = ui->checkListColumns->checkedCount();
    ui->spinFreeze->setRange(0, checked);
    ui->spinFreeze->setValue(qBound(0, effectiveFrozenColumnCount, checked));
}

/**
 * @brief 确定后的新隐藏集合
 *
 * = 列全集 − 勾选集合（未勾选项按 data 里的 (名, occurrence) 收进 QSet）。
 * @return 新隐藏集合
 */
QSet< QPair< QString, int > > DADialogTableColumnView::hiddenColumns() const
{
    QSet< QPair< QString, int > > checked;
    const QVector< DAItemEntry > checkedEntries = ui->checkListColumns->checkedEntries();
    for (const DAItemEntry& e : checkedEntries) {
        checked.insert(idOf(e));
    }
    QSet< QPair< QString, int > > res;
    const QVector< DAItemEntry > all = ui->checkListColumns->items();
    for (const DAItemEntry& e : all) {
        const QPair< QString, int > id = idOf(e);
        if (!checked.contains(id)) {
            res.insert(id);
        }
    }
    return res;
}

/**
 * @brief 确定后的新冻结列数
 * @return SpinBox 当前值
 */
int DADialogTableColumnView::frozenColumnCount() const
{
    return ui->spinFreeze->value();
}

/**
 * @brief 勾选变化时收紧冻结数上限（= 勾选数）
 *
 * Qt 的 QSpinBox::setRange 会自动夹紧当前值到新范围。
 */
void DADialogTableColumnView::updateFreezeRange()
{
    ui->spinFreeze->setRange(0, ui->checkListColumns->checkedCount());
}
}  // end of namespace DA
