#ifndef DAVIRTUALSCROLLBAR_H
#define DAVIRTUALSCROLLBAR_H
#include "DAGuiAPI.h"
#include <QScrollBar>
#include <QStyleOptionSlider>
namespace DA
{
/**
 * @brief 面向「滑动窗表格」的纵向滚动条
 *
 * 语义分两层，互不干扰：
 * - QScrollBar 自身的 value/range/pageStep 保持「窗口内行号」语义 ——
 *   QTableView / QHeaderView 内部依赖它（value 决定第一个可见行），因此不能改；
 * - 绘制与鼠标/键盘交互按「整个数据集」坐标 —— 滑块位置、拖动落点、翻页与单步
 *   都以数据源绝对行为准，通过信号交给视图去平移滑动窗。
 *
 * 也就是说：本控件画的是「我在整表的哪个位置」，动的是「视图要跳到哪一行」，
 * 而 QScrollBar 的 value 依旧是 Qt 内部用的窗口内行号。
 */
class DAGUI_API DAVirtualScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit DAVirtualScrollBar(QWidget* parent = nullptr);
    ~DAVirtualScrollBar();
    // 由视图同步：首个可见的绝对行、数据总行数、一屏可见行数
    void setDatasetPosition(int firstVisibleActualRow, int totalRows, int viewportRows);
    // 首个可见的绝对行
    int getDatasetFirstVisibleRow() const;
    // 数据总行数
    int getDatasetTotalRows() const;
    // 一屏可见行数
    int getDatasetViewportRows() const;
    // 位置（控件内 y 坐标）换算为整数行号
    int actualRowForPosition(int pos) const;
    // 整数行号换算为滑块中心所在的位置（控件内 y 坐标）
    int positionForActualRow(int actualRow) const;
    // 是否正在拖动滑块
    bool isSeeking() const;
Q_SIGNALS:
    // 拖动滑块：请求把目标行显示为视口首行（拖动过程中连续发出）
    void seekRequested(int actualRow);
    // 拖动结束
    void seekFinished();
    // 请求相对移动若干行（点击上下箭头按钮、滚动条获得焦点后按方向键）
    void stepRowsRequested(int rows);
    // 请求相对翻若干屏（点击轨道空白处、PageUp/PageDown）
    void pageRowsRequested(int pages);
    // 请求弹出「定位到行」输入（右键菜单）
    void gotoRowRequested();

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private Q_SLOTS:
    void onRepeatTimeout();

private:
    // 按整表坐标构造样式选项（绘制与命中判定共用）
    QStyleOptionSlider datasetStyleOption() const;
    // 轨道矩形
    QRect grooveRect(const QStyleOptionSlider& opt) const;
    // 更新鼠标悬停的子控件
    void updateHoverSubControl(const QPoint& pos);
    // 拖动过程中显示「第 N 行 / 共 M 行」提示
    void showDragHud(const QPoint& pos);
    // 开始/停止按住不放的连续滚动
    void startRepeatTimer(QStyle::SubControl sc);
    void stopRepeatTimer();

private:
    int mFirstVisibleRow { 0 };     ///< 视口首行的绝对行号
    int mTotalRows { 0 };           ///< 数据总行数
    int mViewportRows { 1 };        ///< 一屏可见行数
    bool mSeeking { false };        ///< 是否正在拖动滑块
    QStyle::SubControl mHoverSub { QStyle::SC_None };  ///< 当前悬停的子控件
    QStyle::SubControl mRepeatSub { QStyle::SC_None };  ///< 按住不放的子控件
    QTimer* mRepeatTimer { nullptr };                   ///< 按住不放的连续滚动定时器
    int mRepeatTicks { 0 };                             ///< 已重复的次数（首次延迟更长）
};
}
#endif  // DAVIRTUALSCROLLBAR_H
