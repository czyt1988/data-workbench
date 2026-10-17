#ifndef DADATAHEADERVIEW_H
#define DADATAHEADERVIEW_H
#include "DAGuiAPI.h"
#include <QHeaderView>

class QMenu;
class QMouseEvent;
class QContextMenuEvent;

namespace DA
{
/**
 * @brief 数据表格头（水平/垂直共用，按 orientation 分支）
 *
 * 表头重写范围（R5/D7）只有四项，本类承载其中三项（排序交互 002 接入）：
 * 1. 带修饰键的点击信号 clickedWithModifiers（001 只发信号不改消费方，
 *    页面继续走既有 sectionClicked 链路保证图表「选择序列」拾取零回归；002 切换）；
 * 2. 列头右键菜单（从 APP 的 setupDataFrameHeaderContextMenu 迁入，R12）：
 *    菜单内容（哪些项、顺序、启用规则）单点定义在本类，QAction 由 DAGui 自建
 *    （不共享 APP 的 action 对象，避免 enabled 互相污染），文案沿用同一英文源串，
 *    图标取 :/DAGui/icon/；点击后发 columnActionRequested 表意信号，APP 接线执行；
 * 3. 行头宽度可拖拽并持久化（vertical 实例右缘 8px 热区，setFixedWidth 落地——
 *    Qt 纵向表头宽度 = clamp(qMax(minimumWidth, sizeHint), maximumWidth)，
 *    setFixedWidth 令 min=max=w 恒为 w，不会被视图布局覆盖；释放时发
 *    rowHeaderWidthChanged，由页面写入 DATableViewStateStore）。
 *
 * 不做：表头默认宽度与内容自适应口径的重新设计（R5）；滑动窗与块缓存不动（D7）。
 */
class DAGUI_API DADataHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    // 列头右键菜单动作表意（点击后发信号，执行逻辑由 APP 接线到既有槽）
    enum class ColumnAction
    {
        RenameColumn,        ///< 重命名列
        RemoveColumn,        ///< 删除列
        CopyColumnName,      ///< 复制列名
        GotoMax,             ///< 定位最大
        GotoMin,             ///< 定位最小
        ShowColumnDescribe   ///< 描述统计
    };
    Q_ENUM(ColumnAction)

    explicit DADataHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

    // 构建列头右键菜单（6 项 4 组，与迁移前 APP 注入版逐项一致；调用方负责弹出与销毁）
    QMenu* buildColumnContextMenu(int logicalIndex);

    // 行头宽拖拽的最小宽度（像素）
    static int minimumRowHeaderWidth();

Q_SIGNALS:
    // 左键点击命中 section（带修饰键；002 排序交互消费，001 不接线）
    void clickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers);
    // 列头右键菜单项被点击（APP 接线：先选中该列再执行既有槽逻辑）
    void columnActionRequested(int logicalIndex, DA::DADataHeaderView::ColumnAction action);
    // 行头宽度拖拽结束（仅 vertical 实例发射，页面据此写视图状态）
    void rowHeaderWidthChanged(int width);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // vertical 实例右缘行头宽拖拽热区（8px）判定
    bool isInRowHeaderResizeHotZone(const QPoint& pos) const;
    // 恢复热区光标
    void restoreHotZoneCursor();
    // 该列是否允许弹出列头菜单（数据边界判定，保持迁移前行为）
    bool isColumnMenuAvailable(int logicalIndex) const;

    int mPressedLogicalIndex { -1 };   ///< 左键按下的 section（释放时同段才发 clickedWithModifiers）
    bool mRowHeaderResizing { false }; ///< 正在拖拽行头宽
    int mResizeStartX { 0 };           ///< 拖拽起点 x
    int mResizeStartWidth { 0 };       ///< 拖拽起点宽度
};
}  // end of namespace DA
#endif  // DADATAHEADERVIEW_H
