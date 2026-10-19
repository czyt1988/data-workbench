#include "DADialogTableColumnView.h"
#include "ui_DADialogTableColumnView.h"
// qt
#include <QListWidget>
#include <QSpinBox>

namespace DA
{

/**
 * @brief 构造函数：只建壳不设状态（配合 setup 复用）
 * @param parent 父窗口
 */
DADialogTableColumnView::DADialogTableColumnView(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogTableColumnView)
{
    ui->setupUi(this);
    connect(ui->listColumns, &QListWidget::itemChanged, this, [ this ]() { updateFreezeRange(); });
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
    mOrderedColumns = orderedColumns;
    ui->listColumns->blockSignals(true);  // 重建期间不发 itemChanged
    ui->listColumns->clear();
    int checked = 0;
    for (const QPair< QString, int >& id : orderedColumns) {  // const：避免 COW 深拷贝
        QListWidgetItem* item = new QListWidgetItem(
            id.second == 0 ? id.first : QStringLiteral("%1 (%2)").arg(id.first).arg(id.second + 1), ui->listColumns);
        item->setData(Qt::UserRole, id.first);      // 写回按 (名, occurrence)
        item->setData(Qt::UserRole + 1, id.second);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        const bool visible = !hiddenColumns.contains(id);
        item->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
        if (visible) {
            ++checked;
        }
    }
    ui->listColumns->blockSignals(false);
    ui->spinFreeze->setRange(0, checked);
    ui->spinFreeze->setValue(qBound(0, effectiveFrozenColumnCount, checked));
}

/**
 * @brief 确定后的新隐藏集合
 *
 * = 列全集 − 勾选集合（未勾选项按 (UserRole, UserRole+1) 收进 QSet）。
 * @return 新隐藏集合
 */
QSet< QPair< QString, int > > DADialogTableColumnView::hiddenColumns() const
{
    QSet< QPair< QString, int > > res;
    const int n = ui->listColumns->count();
    for (int i = 0; i < n; ++i) {
        QListWidgetItem* item = ui->listColumns->item(i);
        if (item->checkState() == Qt::Unchecked) {
            res.insert(qMakePair(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toInt()));
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
    int checked = 0;
    const int n = ui->listColumns->count();
    for (int i = 0; i < n; ++i) {
        if (ui->listColumns->item(i)->checkState() == Qt::Checked) {
            ++checked;
        }
    }
    ui->spinFreeze->setRange(0, checked);
}
}  // end of namespace DA
