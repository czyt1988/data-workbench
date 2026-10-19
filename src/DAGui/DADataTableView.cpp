#include "DADataTableView.h"
#include "DALogCategory.h"
#include "DATableDataSource.h"
#include "DATableNoteStore.h"
#include <QKeyEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QContextMenuEvent>
#include <QSet>
#include <QHash>
#include <QPair>
#include <algorithm>
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
 * @brief 按键事件：Ctrl+C 复制；Ctrl+F 查找；Ctrl+H 替换；Ctrl+D 向下填充；Esc 关查找条
 *
 * 表格快捷键的唯一挂点（红线5：Ribbon action 不设 shortcut，避免双触发）。
 * 查找/替换发表意信号交页面唤出浮动条；Esc 关查找条后仍交基类（保留退出编辑器等既有语义）。
 * 未处理的键一律转发基类 DACacheWindowTableView（恢复整表语义键 Ctrl+G/Ctrl+Home/Ctrl+End/
 * PageUp/PageDown/Home/End——QTableView 自己的 Home/End 只作用于模型窗口内，滑动窗下等于"窗口首尾"）。
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
    // Ctrl+D 向下填充（005 T1.24；QKeySequence 无 FillDown 标准键，手工判定组合键）
    if (event->key() == Qt::Key_D && event->modifiers() == Qt::ControlModifier) {
        Q_EMIT fillDownRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        // 焦点在表格时 Esc 关查找条（命中高亮由页面据 findBarCloseRequested 清理）；
        // 不 accept，Esc 继续交基类处理既有语义
        Q_EMIT findBarCloseRequested();
    }
    // 未处理的键一律转发基类（DACacheWindowTableView）：
    // 恢复 Ctrl+G 定位到行、Ctrl+Home/Ctrl+End 数据首尾、PageUp/PageDown 翻屏、Home/End 行首尾
    // （QTableView 自己的 Home/End 只作用于模型窗口内，滑动窗下等于"窗口首尾"）
    DACacheWindowTableView::keyPressEvent(event);
}

/**
 * @brief 定位到行（007 T1.7）：覆写为只发 gotoRequested 表意信号
 *
 * 基类默认是 QInputDialog（DAPyDataFrameTableView / DAPySeriesTableView 继续使用）；
 * 数据表页由此把 Ctrl+G 与滚动条右键「定位到行」统一引到页面的定位对话框
 *（onGotoRowRequested → promptGotoRow 虚派发走同一条链）。
 */
void DADataTableView::promptGotoRow()
{
    Q_EMIT gotoRequested();
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
 * @brief 当前选区覆盖的数据列数（去重；尾部预留空白列不计）
 *
 * 004 T1.9 分列/合并列的右键启用规则用：单列选区启用分列、多列选区启用合并。
 * 按选区区间展开列号（全选卡顿修复：不再 selectedIndexes 物化整个选区）。
 * @return 数据列数（无模型/无选区返回 0）
 */
int DADataTableView::selectedDataColumnCount() const
{
    QItemSelectionModel* sm = selectionModel();
    DADataTableModel* m     = getDataModel();
    if (!sm || !m) {
        return 0;
    }
    const int dataCols = m->columnCount() - m->getExtraColumnCount();
    QSet< int > cols;
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int c = qMax(0, r.left()); c <= r.right() && c < dataCols; ++c) {
            cols.insert(c);
        }
    }
    return cols.size();
}

/**
 * @brief 是否存在某数据列拥有 >=2 行的连续选中行区间（005 填充启用规则）
 *
 * 区间内同一列的选中行天然连续：高度 ≥2 且覆盖任一数据列即命中（等价于旧的
 * 逐格聚合+排序判定）；高度为 1 的区间才需要按列归并行号找跨区间的相邻对。
 * 尾部预留空白列不计。不足两行的选区向下/向上/ffill/bfill 无从发生。
 * （全选卡顿修复：不再 selectedIndexes 物化整个选区）
 * @return 存在返回 true
 */
bool DADataTableView::hasMultiRowSelectedRun() const
{
    QItemSelectionModel* sm = selectionModel();
    DADataTableModel* m     = getDataModel();
    if (!sm || !m) {
        return false;
    }
    const int dataCols = m->columnCount() - m->getExtraColumnCount();
    QHash< int, QList< int > > colRows;  // 仅高度为 1 的区间需要跨区间归并判定
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        if (r.height() >= 2) {
            if (r.left() < dataCols) {
                return true;  // 区间内同列选中行天然连续
            }
            continue;
        }
        if (r.height() == 1) {
            for (int c = qMax(0, r.left()); c <= r.right() && c < dataCols; ++c) {
                colRows[ c ].append(r.top());
            }
        }
    }
    for (auto it = colRows.constBegin(); it != colRows.constEnd(); ++it) {
        QList< int > rows = it.value();
        std::sort(rows.begin(), rows.end());
        for (int i = 1; i < rows.size(); ++i) {
            if (rows[ i ] == rows[ i - 1 ] + 1) {
                return true;
            }
        }
    }
    return false;
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
    // —— 003：条件格式（R16：表体右键菜单与 Ribbon Table Style 面板同入口；列头菜单不放）——
    // 只读/惰性源也可管理规则（色阶/数据条的可用性限制在对话框内按 §1.4-2 置灰），
    // 无选区时置灰（入口作用于当前选区所在列）
    menu->addSeparator();
    QAction* actConditionalFormat = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/conditional-format.svg")),
                                                    tr("Conditional Format..."));  // cn:条件格式...
    actConditionalFormat->setEnabled(hasSel);
    connect(actConditionalFormat, &QAction::triggered, this, &DADataTableView::conditionalFormatRequested);
    // —— 004：数据工具（计算列/分列/合并列；分组/重塑随各自 WP 追加在本组）——
    // 计算列写数据，只读/惰性源置灰（isTableEditable 为假）
    menu->addSeparator();
    QAction* actComputedColumn = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/computed-column.svg")),
                                                 tr("Computed Column..."));  // cn:计算列...
    actComputedColumn->setEnabled(editable);
    connect(actComputedColumn, &QAction::triggered, this, &DADataTableView::computedColumnRequested);
    // 分列/合并列（004 T1.9）：按选区列数决定启用哪一项——单列选区启用分列，
    // 多列选区启用合并列（计划 §4.3-6）；只读源都置灰
    const int selColCount = selectedDataColumnCount();
    QAction* actSplitColumn = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/split-column.svg")),
                                              tr("Split Column..."));  // cn:分列...
    actSplitColumn->setEnabled(editable && selColCount == 1);
    connect(actSplitColumn, &QAction::triggered, this, &DADataTableView::splitColumnRequested);
    QAction* actMergeColumns = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/merge-columns.svg")),
                                               tr("Merge Columns..."));  // cn:合并列...
    actMergeColumns->setEnabled(editable && selColCount >= 2);
    connect(actMergeColumns, &QAction::triggered, this, &DADataTableView::mergeColumnsRequested);
    // 分组聚合（004 T1.22）：产出新数据集（非破坏），有 dataframe 数据体即可用（只读源也可）
    QAction* actGroupBy = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/group-by.svg")),
                                          tr("Group By..."));  // cn:分组聚合...
    actGroupBy->setEnabled(getData().isDataFrame());
    connect(actGroupBy, &QAction::triggered, this, &DADataTableView::groupByRequested);
    // 转置 / 宽转长（004 T1.27）：默认产出新数据集（非破坏，R14），dataframe 数据体即可用
    QAction* actTranspose = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/transpose.svg")),
                                            tr("Transpose..."));  // cn:转置...
    actTranspose->setEnabled(getData().isDataFrame());
    connect(actTranspose, &QAction::triggered, this, &DADataTableView::transposeRequested);
    QAction* actMelt = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/melt.svg")),
                                       tr("Unpivot (Melt)..."));  // cn:宽转长（melt）...
    actMelt->setEnabled(getData().isDataFrame());
    connect(actMelt, &QAction::triggered, this, &DADataTableView::meltRequested);
    // —— 005：填充子菜单（向下/向上/ffill/bfill/序列…，D6；Ctrl+D 与 fillDownRequested 共用）——
    // 只读源全部置灰；向下/向上/ffill/bfill 需要某列存在 >=2 行的连续选中区间
    //（不足两行无从传递，计划 §3.3-1 选择置灰而非提示）；序列填充有选中即可
    menu->addSeparator();
    QMenu* fillMenu = menu->addMenu(QIcon(QStringLiteral(":/DAGui/icon/fill.svg")), tr("Fill"));  // cn:填充
    const bool canFill     = editable && hasSel;
    const bool multiRowRun = canFill && hasMultiRowSelectedRun();
    QAction* actFillDown   = fillMenu->addAction(QIcon(QStringLiteral(":/DAGui/icon/fill-down.svg")), tr("Fill Down"));     // cn:向下填充
    QAction* actFillUp     = fillMenu->addAction(QIcon(QStringLiteral(":/DAGui/icon/fill-up.svg")), tr("Fill Up"));         // cn:向上填充
    QAction* actFillFwd    = fillMenu->addAction(QIcon(QStringLiteral(":/DAGui/icon/fill-forward.svg")), tr("Fill Forward (ffill)"));   // cn:向下填充空值(ffill)
    QAction* actFillBwd    = fillMenu->addAction(QIcon(QStringLiteral(":/DAGui/icon/fill-backward.svg")), tr("Fill Backward (bfill)"));  // cn:向上填充空值(bfill)
    fillMenu->addSeparator();
    QAction* actFillSeries = fillMenu->addAction(QIcon(QStringLiteral(":/DAGui/icon/fill-series.svg")), tr("Fill Series..."));  // cn:序列填充...
    actFillDown->setEnabled(multiRowRun);
    actFillUp->setEnabled(multiRowRun);
    actFillFwd->setEnabled(multiRowRun);
    actFillBwd->setEnabled(multiRowRun);
    actFillSeries->setEnabled(canFill);
    connect(actFillDown, &QAction::triggered, this, &DADataTableView::fillDownRequested);
    connect(actFillUp, &QAction::triggered, this, &DADataTableView::fillUpRequested);
    connect(actFillFwd, &QAction::triggered, this, &DADataTableView::fillForwardRequested);
    connect(actFillBwd, &QAction::triggered, this, &DADataTableView::fillBackwardRequested);
    connect(actFillSeries, &QAction::triggered, this, &DADataTableView::fillSeriesRequested);
    // —— 005：数据校验（规则管理 / 执行校验，T1.25；与 Ribbon「Data Tools」面板同入口）——
    // 校验是只读操作：pandas dataframe 数据体即可用（只读源也可校验）；惰性源无 df 置灰
    menu->addSeparator();
    QAction* actValidationRules = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/validation.svg")),
                                                  tr("Validation Rules..."));  // cn:校验规则...
    actValidationRules->setEnabled(getData().isDataFrame());
    connect(actValidationRules, &QAction::triggered, this, &DADataTableView::validationRulesRequested);
    QAction* actRunValidation = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/validation-run.svg")),
                                                tr("Run Validation"));  // cn:执行校验
    actRunValidation->setEnabled(getData().isDataFrame());
    connect(actRunValidation, &QAction::triggered, this, &DADataTableView::runValidationRequested);
    // —— 005：单元格批注（T1.26；与 Ribbon「编辑」面板同入口）——
    // 批注是元数据不写数据：只读源也可批注；启用规则按「当前格」（插入=无批注、
    // 编辑/删除=有批注、清除=本表有批注）
    menu->addSeparator();
    int curActualRow = -1, curCol = -1;
    bool hasCur      = false;
    if (QItemSelectionModel* sm2 = selectionModel()) {
        const QModelIndex cidx = sm2->currentIndex();
        DADataTableModel* m    = getDataModel();
        if (cidx.isValid() && m) {
            const int dataCols = m->columnCount() - m->getExtraColumnCount();
            const int dataRows = m->actualRowCount() - m->getExtraRowCount();
            curActualRow       = toActualRow(cidx.row());
            curCol             = cidx.column();
            hasCur             = curCol >= 0 && curCol < dataCols && curActualRow >= 0 && curActualRow < dataRows;
        }
    }
    const bool hasNote  = hasCur && mNoteStore && mNoteStore->hasNote(curActualRow, curCol);
    const int noteCount = mNoteStore ? mNoteStore->count() : 0;
    QAction* actInsertNote = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/cell-note.svg")), tr("Insert Note"));  // cn:插入批注
    QAction* actEditNote   = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/cell-note.svg")), tr("Edit Note"));    // cn:编辑批注
    QAction* actDeleteNote = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/delete.svg")), tr("Delete Note"));     // cn:删除批注
    QAction* actClearNotes = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/note-clear.svg")), tr("Clear All Notes"));  // cn:清除本表批注
    actInsertNote->setEnabled(hasCur && !hasNote);
    actEditNote->setEnabled(hasNote);
    actDeleteNote->setEnabled(hasNote);
    actClearNotes->setEnabled(noteCount > 0);
    connect(actInsertNote, &QAction::triggered, this, &DADataTableView::noteInsertRequested);
    connect(actEditNote, &QAction::triggered, this, &DADataTableView::noteEditRequested);
    connect(actDeleteNote, &QAction::triggered, this, &DADataTableView::noteDeleteRequested);
    connect(actClearNotes, &QAction::triggered, this, &DADataTableView::noteClearAllRequested);
    // —— 后续计划的分组插入点在菜单底部（见函数注释），插入时组间加分隔符 ——
    return menu;
}

/**
 * @brief 注入批注子存储（右键菜单启用规则查询用，非拥有）
 * @param store 批注子存储（可空）
 */
void DADataTableView::setNoteStore(DATableNoteStore* store)
{
    mNoteStore = store;
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
    const QItemSelection selection = sm->selection();
    if (selection.isEmpty()) {
        return false;
    }
    // 包围矩形（按区间求，不物化 selectedIndexes）。成员判定改为「逐行收集选中列
    // 闭区间 + 游标推进」，避免为每个选中格建哈希集合（全选时是百万级插入），
    // 也避免逐格做 O(段数) 的区间判定；复杂度 O(格数 + 行数×段数)
    int minRow = INT_MAX, maxRow = INT_MIN, minCol = INT_MAX, maxCol = INT_MIN;
    for (const QItemSelectionRange& r : selection) {
        minRow = qMin(minRow, r.top());
        maxRow = qMax(maxRow, r.bottom());
        minCol = qMin(minCol, r.left());
        maxCol = qMax(maxCol, r.right());
    }
    QString text;
    QList< QPair< int, int > > rowSpans;  // 复用容器，避免逐行分配
    for (int r = minRow; r <= maxRow; ++r) {
        // 该行被选中的列闭区间（按左端排序后用游标推进，避免逐格做 O(段数) 判定）
        rowSpans.clear();
        for (const QItemSelectionRange& range : selection) {
            if (r >= range.top() && r <= range.bottom()) {
                rowSpans.append(qMakePair(range.left(), range.right()));
            }
        }
        std::sort(rowSpans.begin(), rowSpans.end());
        int spanIdx = 0;
        for (int c = minCol; c <= maxCol; ++c) {
            if (c > minCol) {
                text += QLatin1Char('\t');
            }
            while (spanIdx < rowSpans.size() && c > rowSpans[ spanIdx ].second) {
                ++spanIdx;
            }
            const bool inSpan
                = (spanIdx < rowSpans.size() && c >= rowSpans[ spanIdx ].first && c <= rowSpans[ spanIdx ].second);
            if (inSpan) {
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
