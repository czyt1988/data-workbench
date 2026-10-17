#include "DADataHeaderView.h"
#include "Models/DADataTableModel.h"
#include "DATableComputedColumnStore.h"
#include "DAData.h"
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPainter>
#include <QPolygon>

namespace DA
{

namespace
{
// 行头右缘拖拽热区宽度（像素）
constexpr int c_rowHeaderResizeHotZone = 8;
// 行头最小宽度（像素）
constexpr int c_minRowHeaderWidth = 24;

// 鼠标事件的全局坐标（Qt5/Qt6 兼容）
QPoint globalPosOf(const QMouseEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->globalPos();
#else
    return event->globalPosition().toPoint();
#endif
}
}  // namespace

/**
 * @brief 构造函数
 *
 * vertical 实例沿用 DACacheWindowTableView 构造里的行高/Interactive 设置
 * （替换默认表头后这些设置不会自动带过来，必须在此重建）。
 * @param orientation 方向（水平=列头，垂直=行头）
 * @param parent 父对象（一般为所属表格视图）
 */
DADataHeaderView::DADataHeaderView(Qt::Orientation orientation, QWidget* parent) : QHeaderView(orientation, parent)
{
    // 与 QTableView 构造里创建的默认表头属性对齐：QTableView::setHorizontal/VerticalHeader
    // 只补 sortIndicatorShown 与 stretchLastSection，**不会**重设 sectionsClickable/
    // highlightSections——不在这里恢复，sectionClicked 不再发射，图表「选择序列」拾取链会断
    setSectionsClickable(true);
    setHighlightSections(true);
    if (orientation == Qt::Vertical) {
        // 行高/Interactive 设置沿用 DACacheWindowTableView 构造对默认行头的同款设置
        // （替换默认表头后这些设置不会自动带过来，必须在此重建）
        QFontMetrics fm = fontMetrics();
        setDefaultSectionSize(fm.lineSpacing() * 1.2);
        setSectionResizeMode(QHeaderView::Interactive);
        setMouseTracking(true);  // 行头宽热区光标切换需要无按键移动事件
    }
}

/**
 * @brief 行头宽拖拽的最小宽度
 * @return 像素
 */
int DADataHeaderView::minimumRowHeaderWidth()
{
    return c_minRowHeaderWidth;
}

/**
 * @brief 设置多列排序规格（002 表头点击排序）
 *
 * 规格中的列在段右端画升/降箭头，多列规格时各列左上角画序号角标（1、2…）；
 * 空规格清除全部角标。左下/左侧前导位留给 004 的 ƒx，不占用。
 * @param spec 列逻辑索引+方向，顺序即优先级
 */
void DADataHeaderView::setSortSpec(const QVector< QPair< int, Qt::SortOrder > >& spec)
{
    if (mSortSpec == spec) {
        return;
    }
    mSortSpec = spec;
    viewport()->update();
}

/**
 * @brief 当前多列排序规格
 * @return 列逻辑索引+方向列表（顺序即优先级）
 */
QVector< QPair< int, Qt::SortOrder > > DADataHeaderView::sortSpec() const
{
    return mSortSpec;
}

/**
 * @brief 设置计算列子存储（004 T1.23）
 *
 * 非拥有；浅蓝底色与公式 tooltip 由模型的 BackgroundRole/ToolTipRole 提供，
 * 本类只负责 ƒx 角标绘制与列头菜单的计算列组。设置后重绘视口。
 * @param store 计算列子存储（可空 = 关闭计算列标识）
 */
void DADataHeaderView::setComputedColumnStore(DATableComputedColumnStore* store)
{
    if (mComputedStore == store) {
        return;
    }
    mComputedStore = store;
    viewport()->update();
}

/**
 * @brief 计算列 ƒx 角标的状态色（R9 官方色板）
 *
 * 有效 = 主蓝 #5280C1、待重算 = 金黄 #E6C27C、失效 = 橙红 #CE6043。
 * @param logicalIndex 列逻辑索引
 * @return 非计算列返回无效 QColor
 */
QColor DADataHeaderView::computedColumnBadgeColor(int logicalIndex) const
{
    if (!mComputedStore || orientation() != Qt::Horizontal || logicalIndex < 0) {
        return QColor();
    }
    QAbstractItemModel* m = model();
    if (!m) {
        return QColor();
    }
    const QString colName = m->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole).toString();
    if (colName.isEmpty()) {
        return QColor();
    }
    DATableComputedColumnStore::Entry entry;
    if (!mComputedStore->entryOf(colName, entry)) {
        return QColor();
    }
    switch (entry.state) {
    case DATableComputedColumnStore::State::Valid:
        return QColor(0x52, 0x80, 0xC1);  // 主蓝
    case DATableComputedColumnStore::State::Stale:
        return QColor(0xE6, 0xC2, 0x7C);  // 金黄
    case DATableComputedColumnStore::State::Invalid:
        return QColor(0xCE, 0x60, 0x43);  // 橙红
    }
    return QColor();
}

/**
 * @brief 排序角标与计算列 ƒx 角标绘制（仅水平实例）
 *
 * 在默认绘制之上叠加角标（色值取官方色板，见仓库根目录 DESIGN.md）：
 * - 方向箭头：段右端垂直居中，升序向上/降序向下，深蓝灰 #5D6D7E；
 * - 多列序号角标：仅规格多于 1 列时绘制，段左上角，主蓝 #5280C1 圆底 + 白色数字，
 *   显示该列在规格中的优先级（1 起）；
 * - 计算列 ƒx 角标（004 T1.23 R9）：左侧前导垂直居中，状态色（有效=主蓝 #5280C1 /
 *   待重算=金黄 #E6C27C / 失效=橙红 #CE6043）；浅蓝 #BDCDE8 底色经模型 BackgroundRole
 *   由基类绘制。该列同时带多列排序序号角标（左上）时 ƒx 移到左下，三者共存不重叠
 *   （排序箭头恒在右端）。单元格区域不染色（R9）。
 * @param painter 绘制器
 * @param rect 段矩形
 * @param logicalIndex 列逻辑索引
 */
void DADataHeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    QHeaderView::paintSection(painter, rect, logicalIndex);
    if (orientation() != Qt::Horizontal || !painter || rect.isEmpty()) {
        return;
    }
    int order            = -1;
    Qt::SortOrder dir    = Qt::AscendingOrder;
    for (int i = 0; i < mSortSpec.size(); ++i) {
        if (mSortSpec.at(i).first == logicalIndex) {
            order = i;
            dir   = mSortSpec.at(i).second;
            break;
        }
    }
    // 计算列 ƒx 角标（左侧前导；有左上序号角标时移到左下避让）
    const QColor badgeColor = computedColumnBadgeColor(logicalIndex);
    if (badgeColor.isValid()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QFont f = painter->font();
        f.setBold(true);
        f.setItalic(true);
        f.setPixelSize(qBound(9, rect.height() * 2 / 3, 14));
        painter->setFont(f);
        painter->setPen(badgeColor);
        const bool hasOrderBadge = (order >= 0 && mSortSpec.size() > 1);
        const QRect badgeRect    = hasOrderBadge
                                       ? QRect(rect.left() + 3, rect.center().y() + 1, rect.width() / 2, rect.height() / 2)
                                       : QRect(rect.left() + 3, rect.top(), rect.width() / 2, rect.height());
        painter->drawText(badgeRect, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("\u0192x"));  // ƒx
        painter->restore();
    }
    if (order < 0) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    // 右端方向箭头（深蓝灰 #5D6D7E）
    const qreal arrowHalfW = 4.0;
    const qreal arrowHalfH = 2.5;
    const qreal cx         = rect.right() - arrowHalfW - 5.0;
    const qreal cy         = rect.center().y();
    QPolygonF arrow;
    if (dir == Qt::AscendingOrder) {
        arrow << QPointF(cx - arrowHalfW, cy + arrowHalfH) << QPointF(cx + arrowHalfW, cy + arrowHalfH)
              << QPointF(cx, cy - arrowHalfH);
    } else {
        arrow << QPointF(cx - arrowHalfW, cy - arrowHalfH) << QPointF(cx + arrowHalfW, cy - arrowHalfH)
              << QPointF(cx, cy + arrowHalfH);
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0x5D, 0x6D, 0x7E));
    painter->drawPolygon(arrow);
    // 多列规格：左上角序号角标（主蓝 #5280C1 圆底 + 白字）
    if (mSortSpec.size() > 1) {
        const int r = 7;
        const QPoint center(rect.left() + r + 2, rect.top() + r + 2);
        painter->setBrush(QColor(0x52, 0x80, 0xC1));
        painter->drawEllipse(center, r, r);
        QFont f = painter->font();
        f.setPixelSize(r + 2);
        f.setBold(true);
        painter->setFont(f);
        painter->setPen(Qt::white);
        painter->drawText(QRect(center.x() - r, center.y() - r, 2 * r, 2 * r), Qt::AlignCenter, QString::number(order + 1));
    }
    painter->restore();
}

/**
 * @brief 该列是否允许弹出列头菜单（保持迁移前 APP 版的判定行为）
 *
 * 迁移前判定：右键列有效、数据是 dataframe（df 非 none）、列号在真实数据列范围内
 * （尾部预留空白列不弹）。这里经模型取同一信息：DADataTableModel 的 getData()
 * 与 columnCount()-getExtraColumnCount()。非 DADataTableModel 的通用场景不限制。
 * @param logicalIndex 逻辑列号
 * @return 允许弹出返回 true
 */
bool DADataHeaderView::isColumnMenuAvailable(int logicalIndex) const
{
    if (logicalIndex < 0) {
        return false;
    }
    DADataTableModel* m = qobject_cast< DADataTableModel* >(model());
    if (!m) {
        return true;
    }
    if (!m->getData().isDataFrame()) {
        return false;  // 只读惰性表/非 dataframe：不弹（与迁移前 df.isNone() 判定一致）
    }
    const int dataCols = m->columnCount() - m->getExtraColumnCount();
    return logicalIndex < dataCols;
}

/**
 * @brief 构建列头右键菜单
 *
 * 项与顺序照迁移前现状 **6 项 4 组**（DAAppController::populateColumnContextMenu）：
 * 重命名列 / 删除列 ‖ 复制列名 ‖ 定位最大 / 定位最小 ‖ 描述统计。
 * QAction 由 DAGui 自建（不共享 APP 的 action 对象，避免 Ribbon 与右键的 enabled
 * 互相污染），文案沿用同一英文源串，图标取 :/DAGui/icon/；点击后发
 * columnActionRequested 表意信号，APP 侧接线：先选中该列再执行既有槽逻辑。
 * @param logicalIndex 目标列逻辑索引
 * @return 菜单（parent = this，调用方负责弹出与 deleteLater）
 */
QMenu* DADataHeaderView::buildColumnContextMenu(int logicalIndex)
{
    QMenu* menu = new QMenu(this);
    // 组1：重命名列 / 删除列
    QAction* actRename = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/rename-column.svg")),
                                         tr("Rename Column"));  // cn:重命名此列
    QAction* actRemove = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/delete-column.svg")),
                                         tr("Remove Column"));  // cn:删除列
    menu->addSeparator();
    // 组2：复制列名
    QAction* actCopyName = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/copy.svg")),
                                           tr("Copy Column Name"));  // cn:复制列名
    menu->addSeparator();
    // 组3：定位最大 / 定位最小
    QAction* actGotoMax = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/gotoMax.svg")),
                                          tr("Goto Max"));  // cn:跳转到最大值
    QAction* actGotoMin = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/gotoMin.svg")),
                                          tr("Goto Min"));  // cn:跳转到最小值
    menu->addSeparator();
    // 组4：描述统计
    QAction* actDescribe = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/showDataInList.svg")),
                                           tr("Show Statistics"));  // cn:显示统计信息

    connect(actRename, &QAction::triggered, this, [ this, logicalIndex ]() {
        Q_EMIT columnActionRequested(logicalIndex, ColumnAction::RenameColumn);
    });
    connect(actRemove, &QAction::triggered, this, [ this, logicalIndex ]() {
        Q_EMIT columnActionRequested(logicalIndex, ColumnAction::RemoveColumn);
    });
    connect(actCopyName, &QAction::triggered, this, [ this, logicalIndex ]() {
        Q_EMIT columnActionRequested(logicalIndex, ColumnAction::CopyColumnName);
    });
    connect(actGotoMax, &QAction::triggered, this, [ this, logicalIndex ]() {
        Q_EMIT columnActionRequested(logicalIndex, ColumnAction::GotoMax);
    });
    connect(actGotoMin, &QAction::triggered, this, [ this, logicalIndex ]() {
        Q_EMIT columnActionRequested(logicalIndex, ColumnAction::GotoMin);
    });
    connect(actDescribe, &QAction::triggered, this, [ this, logicalIndex ]() {
        Q_EMIT columnActionRequested(logicalIndex, ColumnAction::ShowColumnDescribe);
    });
    // 组5（004 T1.23）：计算列——仅当该列是计算列时追加「编辑公式 / 重新计算 / 移除公式（保留数据）」
    if (mComputedStore && orientation() == Qt::Horizontal && model()) {
        const QString colName = model()->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole).toString();
        if (!colName.isEmpty() && mComputedStore->hasDefinition(colName)) {
            menu->addSeparator();
            QAction* actEdit = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/rename-column.svg")),
                                               tr("Edit Formula"));  // cn:编辑公式
            QAction* actRecompute = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/computed-column.svg")),
                                                    tr("Recompute"));  // cn:重新计算
            QAction* actRemoveFormula = menu->addAction(QIcon(QStringLiteral(":/DAGui/icon/delete-column.svg")),
                                                        tr("Remove Formula (Keep Data)"));  // cn:移除公式（保留数据）
            connect(actEdit, &QAction::triggered, this, [ this, logicalIndex ]() {
                Q_EMIT columnActionRequested(logicalIndex, ColumnAction::EditComputedFormula);
            });
            connect(actRecompute, &QAction::triggered, this, [ this, logicalIndex ]() {
                Q_EMIT columnActionRequested(logicalIndex, ColumnAction::RecomputeComputedColumn);
            });
            connect(actRemoveFormula, &QAction::triggered, this, [ this, logicalIndex ]() {
                Q_EMIT columnActionRequested(logicalIndex, ColumnAction::RemoveComputedFormula);
            });
        }
    }
    return menu;
}

/**
 * @brief vertical 实例右缘行头宽拖拽热区判定（8px）
 * @param pos 表头内坐标
 * @return 在热区内返回 true
 */
bool DADataHeaderView::isInRowHeaderResizeHotZone(const QPoint& pos) const
{
    if (orientation() != Qt::Vertical) {
        return false;
    }
    return pos.x() >= width() - c_rowHeaderResizeHotZone;
}

/**
 * @brief 恢复热区光标（仅在当前是拖拽光标时复位，不覆盖基类的段边界光标）
 */
void DADataHeaderView::restoreHotZoneCursor()
{
    if (viewport()->cursor().shape() == Qt::SplitHCursor) {
        viewport()->setCursor(Qt::ArrowCursor);
    }
}

/**
 * @brief 右键菜单（水平实例）：列头右键菜单已迁入本类（R12）
 *
 * 垂直实例（行头）右键不弹菜单并 accept 事件——QWidget 默认的 contextMenuEvent
 * 会 ignore 使事件冒泡到父级表格视图，导致表体菜单在行头上弹出；迁移前行头
 * 右键无任何菜单，此处保持该行为。水平实例命中空白列（尾部预留列/非 dataframe）
 * 时同样不弹菜单。
 * @param event 上下文菜单事件
 */
void DADataHeaderView::contextMenuEvent(QContextMenuEvent* event)
{
    if (orientation() != Qt::Horizontal) {
        event->accept();  // 行头右键：不弹菜单，阻止冒泡到表体（保持迁移前行为）
        return;
    }
    const int logical = logicalIndexAt(event->pos());
    if (!isColumnMenuAvailable(logical)) {
        event->accept();
        return;
    }
    QMenu* menu = buildColumnContextMenu(logical);
    menu->popup(event->globalPos());
    // 弹出后菜单独立生命周期，关闭即销毁
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    event->accept();
}

/**
 * @brief 鼠标按下：vertical 热区开始行头宽拖拽；记录按下的 section
 * @param event 鼠标事件
 */
void DADataHeaderView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (isInRowHeaderResizeHotZone(Qt5Qt6Compat_QXXEvent_Pos(event))) {
            mRowHeaderResizing = true;
            mResizeStartX      = globalPosOf(event).x();
            mResizeStartWidth  = width();
            event->accept();
            return;  // 不进基类，避免触发整列/整行选择
        }
        mPressedLogicalIndex = logicalIndexAt(Qt5Qt6Compat_QXXEvent_Pos(event));
    }
    QHeaderView::mousePressEvent(event);
}

/**
 * @brief 鼠标移动：行头宽拖拽改宽（setFixedWidth 落地）；热区光标切换
 * @param event 鼠标事件
 */
void DADataHeaderView::mouseMoveEvent(QMouseEvent* event)
{
    if (mRowHeaderResizing) {
        const int delta = globalPosOf(event).x() - mResizeStartX;
        setFixedWidth(qMax(c_minRowHeaderWidth, mResizeStartWidth + delta));
        event->accept();
        return;
    }
    QHeaderView::mouseMoveEvent(event);
    if (orientation() == Qt::Vertical) {
        if (isInRowHeaderResizeHotZone(Qt5Qt6Compat_QXXEvent_Pos(event))) {
            viewport()->setCursor(Qt::SplitHCursor);
        } else {
            restoreHotZoneCursor();
        }
    }
}

/**
 * @brief 鼠标释放：结束行头宽拖拽并发 rowHeaderWidthChanged；
 * 左键同段按下-释放发 clickedWithModifiers（带修饰键点击信号）
 * @param event 鼠标事件
 */
void DADataHeaderView::mouseReleaseEvent(QMouseEvent* event)
{
    if (mRowHeaderResizing) {
        mRowHeaderResizing = false;
        Q_EMIT rowHeaderWidthChanged(width());
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && mPressedLogicalIndex >= 0) {
        const int logical = logicalIndexAt(Qt5Qt6Compat_QXXEvent_Pos(event));
        if (logical == mPressedLogicalIndex) {
            Q_EMIT clickedWithModifiers(logical, event->modifiers());
        }
    }
    mPressedLogicalIndex = -1;
    QHeaderView::mouseReleaseEvent(event);
}

/**
 * @brief 离开表头：恢复热区光标
 * @param event 事件
 */
void DADataHeaderView::leaveEvent(QEvent* event)
{
    restoreHotZoneCursor();
    QHeaderView::leaveEvent(event);
}

}  // end of namespace DA
