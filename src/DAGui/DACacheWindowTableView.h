#ifndef DACACHEWINDOWTABLEVIEW_H
#define DACACHEWINDOWTABLEVIEW_H
#include "DAGuiAPI.h"
#include <QTableView>
#include <QTableWidget>
namespace DA
{
class DAAbstractCacheWindowTableModel;
class DAVirtualScrollBar;
/**
 * @brief 针对DAAbstractCacheWindowTableModel的view
 *
 * 位置模型（只有一处定义，所有定位/滚动都走它）：
 * @code
 *   T（视口首行的绝对行号） = W（窗口起始行 getCacheWindowStartRow()） + v（纵向滚动条 value）
 * @endcode
 * v 始终是「窗口内行号」，这是 Qt 的固定语义（QHeaderViewPrivate::setScrollOffset 用
 * value 反算表头像素偏移），因此方向键/滚轮/翻页等细粒度滚动一律走 v，天然精确；
 * 当 v 逼近窗口缓冲的两端时整体平移窗口（W 与 v 同时加减同一个增量）——视口内容
 * 一动不动，只是把缓冲搬到数据源的新位置，于是方向键可以无限逐行遍历整表。
 * 纵向滚动条的绘制与拖动走整表坐标（DAVirtualScrollBar），拖到哪就定位到哪。
 */
class DAGUI_API DACacheWindowTableView : public QTableView
{
	Q_OBJECT
public:
	DACacheWindowTableView(QWidget* parent = nullptr);
	~DACacheWindowTableView();
	// 获取缓存窗口model
	DAAbstractCacheWindowTableModel* getCacheModel() const;
	// 获取整表坐标滚动条
	DAVirtualScrollBar* getVirtualScrollBar() const;
	// 显示行：把该绝对行放到视口首行
	void showActualRow(int actualRow);
	// 高亮目标文本框
	void selectActualCell(int row, int col);
	// 真实行转换为逻辑行
	int toLogicalRow(int actualRow) const;
	// 逻辑行转换为真实行
	int toActualRow(int logicalRow) const;
	// 视口首行的绝对行号T
	int getTopActualRow() const;
	// 一屏可见的行数
	int getViewportRowCount() const;
	// 判断真实行是否落在窗口缓冲区内
	bool isActualRowInViewRange(int actualRow) const;
	// 判断真实行是否真的可见（落在视口内）
	bool isActualRowVisible(int actualRow) const;
	// 把视口首行放到指定绝对行（超出数据范围时贴住两端）
	void placeViewportAt(int actualRow);
	// 相对移动视口，deltaRows可正可负（正数向下）
	void moveViewport(int deltaRows);
	// 真实行的名称
	QString actualRowName(int actualRow) const;
	// 真实列的名称
	QString actualColumnName(int actualCol) const;
	// 换模型时重新接线（公开：QAbstractItemView::setModel 是 protected）
	void setModel(QAbstractItemModel* model) override;

protected:
	// 视口滚动后维持窗口缓冲区覆盖视口
	void scrollContentsBy(int dx, int dy) override;
	void keyPressEvent(QKeyEvent* event) override;

private Q_SLOTS:
	// 滚动条请求把目标行置顶（拖动滑块）
	void onSeekRequested(int actualRow);
	// 滚动条请求相对移动若干行（上下箭头）
	void onStepRowsRequested(int rows);
	// 滚动条请求相对翻若干屏（轨道点击、滚动条上的PageUp/PageDown）
	void onPageRowsRequested(int pages);
	// 滚动条请求弹出「定位到行」输入
	void onGotoRowRequested();
	// 模型窗口起始行变化
	void onCacheWindowChanged(int oldStartRow, int newStartRow);
	// 模型结构变化（插删行列）
	void onTableStructureChanged();
	// 模型重置（数据被替换）
	void onModelReset();

private:
	// 窗口不覆盖视口时把它搬回来
	void ensureWindowCoversViewport();
	// v逼近缓冲区边缘时整体平移窗口
	void recenterWindowIfNeeded();
	// 平移窗口：W与v同时加减deltaRows，视口首行不变
	bool shiftWindow(int deltaRows);
	// 窗口平移后同步当前项与选区，使它们仍指向同一批绝对行
	void syncViewStateAfterWindowShift(int deltaRows);
	// 在保持视口首行为topRow的前提下，尽量把当前项也留在窗口内
	int adjustWindowStartForCurrent(int preferW, int topRow) const;
	// 同步滚动条的整表坐标信息
	void syncScrollBarState();
	// 关闭正在编辑的编辑器（搬窗前必须先提交，否则会写到错行）
	bool commitOpenEditor();
	// 正在编辑中的编辑器部件
	QWidget* currentEditor() const;
	// 光标移到指定绝对行/列（不改变视口的纵向位置）
	void setCurrentActualCell(int actualRow, int actualColumn);
	// 「定位到行」输入
	void promptGotoRow();

private:
	bool mAdjustingScrollBar { false };            ///< 正在程序化调整滚动条/窗口，避免重入
	bool mPendingRecenter { false };               ///< 因编辑器打开而延后的窗口重定心
	DAVirtualScrollBar* mVirtualVBar { nullptr };  ///< 整表坐标滚动条（视图拥有）
	QList< QMetaObject::Connection > mModelConnections;  ///< 本类建立的模型连接（换模型时要断开）
};
}
#endif  // DACACHEWINDOWTABLEVIEW_H
