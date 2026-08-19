#include "DADataTableView.h"
#include "DALogCategory.h"
#include <QKeyEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QSet>
#include <QPair>
#include <climits>
namespace DA
{
DADataTableView::DADataTableView(QWidget* parent) : DACacheWindowTableView(parent)
{
}

DADataTableView::~DADataTableView()
{
}

DADataTableModel* DADataTableView::getDataModel() const
{
    return qobject_cast< DADataTableModel* >(model());
}

void DADataTableView::setData(const DAData& d)
{
    DADataTableModel* m = getDataModel();
    if (!m) {
        daWarning << tr("DADataTableView requires a model to be set first");  // cn:你需要先设置模型
        return;
    }
    m->setData(d);
}

DAData DADataTableView::getData() const
{
    DADataTableModel* m = getDataModel();
    if (m) {
        return m->getData();
    }
    return DAData();
}

/**
 * @brief 按键事件：Ctrl+C / Ctrl+Insert 复制选中单元格
 * @param event 按键事件
 */
void DADataTableView::keyPressEvent(QKeyEvent* event)
{
    if (event == QKeySequence::Copy) {
        if (copySelectionToClipboard()) {
            event->accept();
            return;
        }
    }
    QTableView::keyPressEvent(event);
}

/**
 * @brief 把选中单元格按包围矩形拼成 Tab/换行分隔文本写入剪贴板
 *
 * 逐格读取 model 的 Qt::DisplayRole（已随列显示格式格式化），故复制内容随显示格式变化。
 * 未选中的格留空，行内 Tab 分隔、行间换行，便于粘贴到 Excel。
 * @return 成功复制返回 true，无选中返回 false
 */
bool DADataTableView::copySelectionToClipboard()
{
    QItemSelectionModel* sm = selectionModel();
    if (!sm) {
        return false;
    }
    QModelIndexList idxs = sm->selectedIndexes();
    if (idxs.isEmpty()) {
        return false;
    }
    // 包围矩形
    int minRow = INT_MAX, maxRow = INT_MIN, minCol = INT_MAX, maxCol = INT_MIN;
    QSet< QPair< int, int > > sel;
    sel.reserve(idxs.size());
    for (const QModelIndex& i : std::as_const(idxs)) {
        minRow = qMin(minRow, i.row());
        maxRow = qMax(maxRow, i.row());
        minCol = qMin(minCol, i.column());
        maxCol = qMax(maxCol, i.column());
        sel.insert(qMakePair(i.row(), i.column()));
    }
    QString text;
    for (int r = minRow; r <= maxRow; ++r) {
        for (int c = minCol; c <= maxCol; ++c) {
            if (c > minCol) {
                text += QLatin1Char('\t');
            }
            if (sel.contains(qMakePair(r, c))) {
                QModelIndex idx = model()->index(r, c);
                if (idx.isValid()) {
                    text += idx.data(Qt::DisplayRole).toString();
                }
            }
        }
        text += QLatin1Char('\n');
    }
    QClipboard* cb = QGuiApplication::clipboard();
    if (cb) {
        cb->setText(text);
    }
    return true;
}
}
