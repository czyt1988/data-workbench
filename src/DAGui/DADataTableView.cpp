#include "DADataTableView.h"
#include "DALogCategory.h"
#include "DATableDataSource.h"
#include <QKeyEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QContextMenuEvent>
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
 * @brief 按键事件：Ctrl+C 复制；Ctrl+F 查找；Ctrl+H 替换；Esc 关查找条
 *
 * 表格快捷键的唯一挂点（红线5：Ribbon action 不设 shortcut，避免双触发）。
 * 查找/替换发表意信号交页面唤出浮动条；Esc 关查找条后仍交基类（保留退出编辑器等既有语义）。
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
    if (event->matches(QKeySequence::Find)) {
        Q_EMIT findRequested();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Replace)) {
        Q_EMIT replaceRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        // 焦点在表格时 Esc 关查找条（命中高亮由页面据 findBarCloseRequested 清理）；
        // 不 accept，Esc 继续交基类处理既有语义
        Q_EMIT findBarCloseRequested();
    }
    QTableView::keyPressEvent(event);
}

/**
 * @brief 数据源是否可编辑
 *
 * 判定路径同 DADataTableModel::actualFlags 的只读源分支：tableSource 非空且
 * isTableEditable() 为假（如数据库惰性表）→ 只读。无模型/无数据按只读处理。
 * @return 可编辑返回 true
 */
bool DADataTableView::isDataSourceEditable() const
{
    DADataTableModel* m = getDataModel();
    if (!m) {
        return false;
    }
    DAData d = m->getData();
    if (d.isNull()) {
        return false;
    }
    const DATableDataSource* ts = d.tableSource();
    return !(ts && !ts->isTableEditable());
}

/**
 * @brief 构建表体右键菜单
 *
 * v1 只放剪切 / 复制 / 粘贴三项（剪贴板组）。启用规则：无有效选区 → 全部置灰；
 * 数据源只读（isTableEditable() 为假）→ 粘贴置灰。三项均发表意信号交 APP 接线到
 * 既有槽逻辑（剪切/复制/粘贴路由），视图内不新增执行逻辑。
 *
 * 后续计划的分组按序追加在本菜单底部（组间用分隔符）：
 * - 002：查找 / 替换
 * - 003：条件格式
 * - 004：分列 / 合并 / 转置
 * - 005：填充 / 批注
 * @param at 右键命中的模型索引（v1 菜单项与命中位置无关，启用规则按选区/数据源）
 * @return 菜单（parent = this，调用方负责弹出与 deleteLater）
 */
QMenu* DADataTableView::buildContextMenu(const QModelIndex& at)
{
    Q_UNUSED(at);
    QMenu* menu           = new QMenu(this);
    const bool hasSel     = selectionModel() && selectionModel()->hasSelection();
    const bool editable   = isDataSourceEditable();
    QAction* actCut       = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/cut.svg")), tr("Cut"));     // cn:剪切
    QAction* actCopy      = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/copy.svg")), tr("Copy"));   // cn:复制
    QAction* actPaste     = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/paste.svg")), tr("Paste")); // cn:粘贴
    actCut->setEnabled(hasSel);
    actCopy->setEnabled(hasSel);
    actPaste->setEnabled(hasSel && editable);
    connect(actCut, &QAction::triggered, this, &DADataTableView::contextMenuCutRequested);
    connect(actCopy, &QAction::triggered, this, &DADataTableView::contextMenuCopyRequested);
    connect(actPaste, &QAction::triggered, this, &DADataTableView::contextMenuPasteRequested);
    // —— 002：查找 / 替换（与快捷键 Ctrl+F/Ctrl+H 共用表意信号，页面唤出浮动条）——
    menu->addSeparator();
    QAction* actFind    = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/find.svg")), tr("Find..."));     // cn:查找...
    QAction* actReplace = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/replace.svg")), tr("Replace..."));  // cn:替换...
    actReplace->setEnabled(editable);  // 只读源查找可用、替换置灰
    connect(actFind, &QAction::triggered, this, &DADataTableView::findRequested);
    connect(actReplace, &QAction::triggered, this, &DADataTableView::replaceRequested);
    // —— 后续计划的分组插入点在菜单底部（见函数注释），插入时组间加分隔符 ——
    return menu;
}

/**
 * @brief 表体右键菜单事件
 * @param event 上下文菜单事件
 */
void DADataTableView::contextMenuEvent(QContextMenuEvent* event)
{
    const QModelIndex at = indexAt(event->pos());
    QMenu* menu          = buildContextMenu(at);
    if (!menu) {
        return;
    }
    // 弹出后菜单独立生命周期，关闭即销毁
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    menu->popup(event->globalPos());
}

/**
 * @brief 把选中单元格按包围矩形拼成 Tab/换行分隔文本写入剪贴板
 *
 * 逐格读取 model 的 Qt::DisplayRole（已随列显示格式格式化），故复制内容随显示格式变化。
 * 未选中的格留空，行内 Tab 分隔、行间换行，便于粘贴到 Excel。
 * 供 keyPressEvent(Ctrl+C) 与主页剪贴板路由 actionCopy 表格分支共用。
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
