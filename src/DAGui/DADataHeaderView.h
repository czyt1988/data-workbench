#ifndef DADATAHEADERVIEW_H
#define DADATAHEADERVIEW_H
#include "DAGuiAPI.h"
#include <QHeaderView>
#include <QPair>
#include <QVector>

class QMenu;
class QMouseEvent;
class QContextMenuEvent;
class QPainter;

namespace DA
{
class DATableComputedColumnStore;
/**
 * @brief 数据表格头（水平/垂直共用，按 orientation 分支）
 *
 * 表头重写范围（R5/D7）只有四项，本类承载全部四项：
 * 1. 带修饰键的点击信号 clickedWithModifiers（**点击不排序**：排序入口在列头右键
 *    菜单，见第 2 项——2026-10-18 用户反馈修订，避免误触整表重排；本信号仅供
 *    拾取模式下页面转发 columnHeaderClicked 使用），并绘制多列排序角标
 *    （setSortSpec：方向箭头在段右端、多列序号在左上角，左下/左侧前导位是
 *    004 的 ƒx）；
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
        SortAscending,             ///< 升序排序（重置排序规格为仅该列）
        SortDescending,            ///< 降序排序（重置排序规格为仅该列）
        AddSecondarySortAscending,   ///< 追加为次级排序列（升序，保留既有规格）
        AddSecondarySortDescending,  ///< 追加为次级排序列（降序，保留既有规格）
        RenameColumn,          ///< 重命名列
        RemoveColumn,          ///< 删除列
        CopyColumnName,        ///< 复制列名
        GotoMax,               ///< 定位最大
        GotoMin,               ///< 定位最小
        ShowColumnDescribe,    ///< 描述统计
        EditComputedFormula,   ///< 编辑公式（计算列，004 T1.23）
        RecomputeComputedColumn,  ///< 重新计算（计算列，004 T1.23）
        RemoveComputedFormula  ///< 移除公式保留数据（计算列，004 T1.23）
    };
    Q_ENUM(ColumnAction)

    // 「列」子菜单动作表意（006d）：与既有 ColumnAction 分开——消费方是 DAGui
    // 页面自身（列视图真源操作），不进 APP 的「先选中该列再执行」链路
    enum class ColumnViewAction
    {
        HideColumn,           ///< 隐藏此列
        ShowAllColumns,       ///< 显示所有列
        FreezeToColumn,       ///< 冻结到此列
        Unfreeze,             ///< 取消冻结
        OpenColumnViewDialog  ///< 列视图…
    };
    Q_ENUM(ColumnViewAction)

    explicit DADataHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

    // 设置计算列子存储（ƒx 角标三态绘制与列头菜单计算列组，非拥有，004 T1.23；
    // 浅蓝底色与公式 tooltip 走模型 BackgroundRole/ToolTipRole，本类只画角标）
    void setComputedColumnStore(DATableComputedColumnStore* store);

    // 构建列头右键菜单（排序 4 项 + 原 6 项 4 组；排序组在最前：升序/降序/追加次级升/追加次级降，
    // 2026-10-18 交互修订——点击表头不再排序；计算列追加「编辑公式/重新计算/移除公式（保留数据）」组，
    // 004 T1.23；调用方负责弹出与销毁）
    QMenu* buildColumnContextMenu(int logicalIndex);

    // 行头宽拖拽的最小宽度（像素）
    static int minimumRowHeaderWidth();
    // 行头宽拖拽热区宽度（像素：vertical 实例右缘与角格右缘共用同一口径）
    static int rowHeaderResizeHotZoneWidth();
    // 段边界拖拽热区宽度（像素：列宽/行高边界两侧各这么宽，Excel 式手感）
    static int sectionResizeHotZoneWidth();
    // 行头宽直接设值（只 setFixedWidth 不发信号；角格拖拽移动期用）
    void setRowHeaderWidthDirect(int width);
    // 行头宽落库（发 rowHeaderWidthChanged；拖拽释放时调）
    void commitRowHeaderWidth();

    // 设置多列排序规格（列逻辑索引+方向，顺序即优先级；002 排序角标绘制）：
    // 规格中的列在右端画升/降箭头，多列时各列左上角画序号角标（1、2…）；
    // 空规格清除全部角标。左下/左侧前导位留给 004 的 ƒx，不占用
    void setSortSpec(const QVector< QPair< int, Qt::SortOrder > >& spec);
    QVector< QPair< int, Qt::SortOrder > > sortSpec() const;

Q_SIGNALS:
    // 左键点击命中 section（带修饰键；仅拾取模式下由页面转发 columnHeaderClicked，点击不排序）
    void clickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers);
    // 列头右键菜单项被点击（APP 接线：排序类动作直接执行；其余先选中该列再执行既有槽逻辑）
    void columnActionRequested(int logicalIndex, DA::DADataHeaderView::ColumnAction action);
    // 「列」子菜单动作被点击（006d：消费者是页面自身——列视图真源操作，不经 APP）
    void columnViewActionRequested(int logicalIndex, DA::DADataHeaderView::ColumnViewAction action);
    // 行头宽度拖拽结束（仅 vertical 实例发射，页面据此写视图状态）
    void rowHeaderWidthChanged(int width);
    // 行边界拖拽改统一行高结束（仅 vertical 实例发射；滑动窗下行高按单值语义持久化，
    // 页面据此写视图状态并套到各窗格）
    void rowHeightChanged(int height);
    // 水平表头底边拖拽改表头高结束（仅 horizontal 实例发射；页面据此写视图状态并套到各窗格）
    void horizontalHeaderHeightChanged(int height);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    // 排序角标/计算列 ƒx 角标绘制（仅水平实例）：基类 paintSection 之上叠加右端方向
    // 箭头与左上角多列序号。基类调用必须 save/restore 包裹：QCommonStyle CE_Header
    // 收尾会把裁剪恢复成「空区域」（启用裁剪+空区域=全裁掉），Qt 5.15 的
    // QHeaderView::paintSection 不做 save/restore，污染态会吞掉之后的全部直接绘制
    // （实测排序箭头/序号角标/ƒx 角标不显示的根因）
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;

private:
    // 自定义段边界拖拽类别（列宽/统一行高/水平表头高；None=未拖拽）
    enum class ResizeKind
    {
        None,
        ColumnWidth,
        RowHeight,
        HeaderHeight
    };
    // vertical 实例右缘行头宽拖拽热区（8px）判定
    bool isInRowHeaderResizeHotZone(const QPoint& pos) const;
    // horizontal 实例底边表头高拖拽热区判定
    bool isInHeaderHeightHotZone(const QPoint& pos) const;
    // 段「尾端边界」热区命中：返回该段 logical（列宽=右边界、行高=下边界），未命中 -1
    int sectionEndBoundaryAt(const QPoint& pos) const;
    // 热区光标反馈（viewport 上管理：列边界/行头宽=SplitH，行边界/表头高=SplitV，其余复位）
    void updateHotZoneCursor(const QPoint& pos);
    // 恢复热区光标
    void restoreHotZoneCursor();
    // 该列是否允许弹出列头菜单（数据边界判定，保持迁移前行为）
    bool isColumnMenuAvailable(int logicalIndex) const;

    // 计算列 ƒx 角标状态查询（logicalIndex → 状态色）；非计算列返回无效 QColor
    QColor computedColumnBadgeColor(int logicalIndex) const;
    // 单段角标叠加绘制（排序方向箭头 + 多列序号角标 + 计算列 ƒx 角标）
    void paintSectionOverlays(QPainter* painter, const QRect& rect, int logicalIndex) const;

    DATableComputedColumnStore* mComputedStore { nullptr };  ///< 计算列子存储（非拥有，004 T1.23）
    int mPressedLogicalIndex { -1 };   ///< 左键按下的 section（释放时同段且未拖动才发 clickedWithModifiers）
    QPoint mPressedPos;                ///< 左键按下的位置（拖动阈值比较基准，006c）
    bool mDragMoved { false };         ///< 本次按下-释放期间是否真正拖动过（等价于 Qt 的"拖动指示器已显示"，006c）
    bool mRowHeaderResizing { false }; ///< 正在拖拽行头宽
    int mResizeStartX { 0 };           ///< 拖拽起点 x
    int mResizeStartWidth { 0 };       ///< 拖拽起点宽度
    // 自定义段边界拖拽状态（按下热区起、移动应用、释放落库）
    ResizeKind mResizeKind { ResizeKind::None };  ///< 当前拖拽类别
    int mResizeLogical { -1 };                    ///< 拖拽的段 logical（ColumnWidth/RowHeight）
    int mResizeStartPos { 0 };                    ///< 拖拽起点全局坐标（沿表头轴向）
    int mResizeStartSize { 0 };                   ///< 拖拽起点尺寸（宽/高像素）
    QVector< QPair< int, Qt::SortOrder > > mSortSpec;  ///< 多列排序规格（列逻辑索引+方向，顺序即优先级；不持久化 R13）
};
}  // end of namespace DA
#endif  // DADATAHEADERVIEW_H
