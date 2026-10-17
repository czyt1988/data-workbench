#include "DADataHeaderView.h"
#include "Models/DADataTableModel.h"
#include "DAData.h"
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QContextMenuEvent>

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
 * @param event 上下文菜单事件
 */
void DADataHeaderView::contextMenuEvent(QContextMenuEvent* event)
{
    if (orientation() != Qt::Horizontal) {
        QHeaderView::contextMenuEvent(event);
        return;
    }
    const int logical = logicalIndexAt(event->pos());
    if (!isColumnMenuAvailable(logical)) {
        return;
    }
    QMenu* menu = buildColumnContextMenu(logical);
    menu->popup(event->globalPos());
    // 弹出后菜单独立生命周期，关闭即销毁
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
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
