#ifndef DADATAOPERATEOFDATAFRAMEWIDGET_H
#define DADATAOPERATEOFDATAFRAMEWIDGET_H
#include <QtWidgets/QWidget>
#include <QUndoStack>
#include <QPair>
#include <QVector>
#include <optional>
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include "numpy/DAPyDType.h"
#include "DATableDisplayFormat.h"
#include "DADataOperatePageWidget.h"
#include "Dialog/DADialogFindReplace.h"
namespace Ui
{
class DADataOperateOfDataFrameWidget;
}

namespace DA
{
class DADataTableModel;
class DADataTableView;
class DADialogDataframeColumnCastToNumeric;
class DADialogDataframeColumnCastToDatetime;
class DADialogDataframeColumnDescribe;
class DATableStyleManager;
class DATableStyleItemDelegate;
class DATableMetadataRegistry;
class DATableViewStateStore;
class DATableHighlightOverlay;
class DATableCellStyle;


/**
 * @brief 针对DataFrame的操作窗口
 *
 */
class DAGUI_API DADataOperateOfDataFrameWidget : public DADataOperatePageWidget
{
    Q_OBJECT
public:
    int getDataOperatePageType() const override;

public:
    explicit DADataOperateOfDataFrameWidget(const DAData& d, DATableMetadataRegistry* registry, QWidget* parent = nullptr);
    ~DADataOperateOfDataFrameWidget();
    // 是否存在data
    bool haveData() const;
    // 获取dataframe
    DAPyDataFrame getDataframe() const;
    // 获取Data的引用
    const DAData& data() const;
    // 创建dataframe的数据描述
    DAPyDataFrame createDataDescribe();
    // dataframe表格是否有选中项
    bool isDataframeTableHaveSelection() const;
    // 返回选中的列数，列数不会重复
    QList< int > getSelectedDataframeCoumns(bool ensureInDataframe = true) const;
    // 返回选中的行数（数据源绝对行号），行数不会重复
    QList< int > getSelectedDataframeRows(bool ensureInDataframe = true) const;
    // 返回选中的列/行（行号为数据源绝对行号）
    QList< int > getFullySelectedDataframeColumns(bool ensureInDataframe = true) const;
    QList< int > getFullySelectedDataframeRows(bool ensureInDataframe = true) const;

    // 返回选中的单元格，QPoint(数据源绝对行号, 列号)
    QList< QPoint > getSelectedDataframeCells(bool ensureInDataframe = true) const;
    // 返回选中的某一行的绝对行号
    int getSelectedOneDataframeRow(bool ensureInDataframe = true) const;
    int getSelectedOneDataframeColumn(bool ensureInDataframe = true) const;
    // 获取tableview
    DADataTableView* getDataTableView() const;

    // 获取样式管理器
    DATableStyleManager* styleManager() const;
    // 获取视图状态子存储（列宽/行头宽，借自元数据束，可能为 nullptr）
    DATableViewStateStore* viewStateStore() const;
    // 获取瞬态高亮层（查找/校验高亮，页级持有，非空）
    DATableHighlightOverlay* highlightOverlay() const;
    // 合并样式片段到选中区（fragment 只设部分属性，merge 到目标已有样式）
    // 自动判定层级：整列→列级，整行→行级，否则单元格级
    void mergeStyleToSelection(const DATableCellStyle& fragment);
    // 设置完整样式到选中区（传入什么就设置什么，整体替换，为格式刷准备）
    // 自动判定层级：整列→列级，整行→行级，否则单元格级
    void applyStyleToSelection(const DATableCellStyle& style);
    // 清除选中区样式（自动判定层级）
    void clearStyleSelection();
    // 清除整表所有样式
    void clearStyleAll();
    // 获取选中区代表的样式（各属性独立判断一致性，不一致的属性 valid=false）
    DATableCellStyle getCurrentCellStyle() const;

    // 设置列显示格式到选中列（列级，可撤销）
    void setDisplayFormatToSelection(const DATableDisplayFormat& fmt);
    // 清除选中列的显示格式（列级，保留 bg/fg/font，可撤销）
    void clearDisplayFormatFromSelection();
    // 获取选中列代表的显示格式（不一致返回 invalid）
    DATableDisplayFormat getCurrentColumnDisplayFormat() const;

    // 获取选中的序列，如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，如果用户选中了多列，那么每列作为一个DAData并组成list返回
    QList< DAData > getSlectedSeries() const;
    // 刷新表格
    void refreshTable();
    // 确保列可见
    void ensureColumnVisible(const QString& colName, bool selectCol = true);

    // 表头拾取模式（图表「选择序列」会话期间置位）：拾取模式下表头点击只转发
    // columnHeaderClicked 信号、不排序（§4.1 模式互斥），由 DADataOperateWidget 转发
    void setHeaderPickMode(bool on);
    bool isHeaderPickMode() const;
    // 当前多列排序规格（列逻辑索引+方向，顺序即优先级；不持久化 R13）
    QVector< QPair< int, Qt::SortOrder > > sortSpec() const;

    // 打开查找/替换浮动条（002 T1.4）：expandReplace=true 展开替换行（Ctrl+H 语义）
    void showFindReplace(bool expandReplace);
    // 关闭查找/替换浮动条并清查找高亮
    void closeFindReplace();
    // 当前查找命中数（已装载高亮，≤上限）与当前命中序号（-1=无），供测试与外部查询
    int findHitCount() const;
    int currentFindHitIndex() const;
Q_SIGNALS:
    // 选中区变化时发射当前代表样式，供 ribbon 控件反向同步
    void currentStyleChanged(const DA::DATableCellStyle& style);
    // 选中列变化时发射当前代表显示格式，供 ribbon 格式下拉框反向同步
    void currentDisplayFormatChanged(const DA::DATableDisplayFormat& fmt);
    /**
     * @brief 表格水平表头单击
     * @param logicalIndex 列逻辑索引
     * @note 用于"选择序列"窗口拾取表格列
     */
    void columnHeaderClicked(int logicalIndex);
    // 替换已生效（含超阈值不可撤销降级路径），changedCells=变更格数；APP 接线 setDirty
    void replaceApplied(int changedCells);
public Q_SLOTS:
    void setDAData(const DA::DAData& d);
    // 在选中行后面插入行
    void insertRowAboveBySelect();
    void insertRowBelowBySelect();
    void insertRowAt(int row);
    void insertColumnRightBySelect();
    void insertColumnLeftBySelect();
    void insertColumnAt(int col);
    // 移除选中的行
    int removeSelectRow();
    // 移除选择的列
    int removeSelectColumn();
    // 设置选中单元格为nan,返回设置成功的个数
    int removeSelectCell();
    // 列更名
    void renameColumns();
    // 重命名单列（表头右键用），col 为列位置索引，newName 为新列名
    // 复用 DACommandDataFrame_renameColumns 命令，支持 undo/redo
    // @return 成功返回 true（列为空/重名/越界返回 false）
    bool renameColumn(int col, const QString& newName);
    // 设置选择列的数据类型，发射信号columnTypeChanged
    bool changeSelectColumnType(const DAPyDType& dt);
    // 把选择的行转换为数值，带交互
    void castSelectToNum();
    void castSelectToDatetime();
    // 把选择的列转换为索引
    bool changeSelectColumnToIndex();
    // 显示列统计信息（表头右键），通过 pandas describe 获取
    void showColumnDescribe(int col);
    // 粘贴剪贴板内容到当前选区左上角（仅填充选区，溢出丢弃），可撤销，返回写入单元格数
    int pasteFromClipboard();
    // 剪切选中区（复制 + 置nan），可撤销，返回受影响单元格数
    int cutSelection();
Q_SIGNALS:
    /**
     * @brief 选中的列或者类型发生了变化
     * @param column 列 如果返回空说明影响的列不确定,如果多选，返回
     * @param dt 类型
     * @note 此函数主要是通知主界面ribbon上面的类型变化，调用setDataframeOperateCurrentDType
     */
    void selectTypeChanged(const QList< int >& column, DA::DAPyDType dt);
private Q_SLOTS:
    // 表格点击
    void onTableViewClicked(const QModelIndex& index);
    // 表头带修饰键点击（DADataHeaderView::clickedWithModifiers）：
    // 拾取模式只转发 columnHeaderClicked；否则更新排序规格并排序
    void onHeaderClickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers);
    // 查找条：扫描请求（去抖到期）、上一个/下一个导航、替换当前、全部替换、关闭清高亮
    void onFindSearchRequested();
    void onFindPrevRequested();
    void onFindNextRequested();
    void onFindReplaceRequested();
    void onFindReplaceAllRequested();
    void onFindDialogClosed();

protected:
    void changeEvent(QEvent* e) override;

private:
    // 视图行号（滑动窗内逻辑行）转数据源绝对行号：数据操作唯一换算点
    int toActualRow(int viewRow) const;
    // 数据源真实数据行数/列数（不含尾部预留的空白行列）
    int dataRowCount() const;
    int dataColumnCount() const;
    // 数据源是否可编辑（只读/惰性源上排序入口不生效，判定路径同 DADataTableModel::actualFlags）
    bool isTableEditable() const;
    // 应用排序规格：构造 DACommandDataFrame_sortRows、接线样式/模型/选区/视口回调并 push；
    // 算子失败（如混合类型不可比较）提示并保持原规格，返回 false
    bool applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& newSpec);
    // 按最近应用的置换修正选区（选中行随排序跟随）：appliedPerm 为新→旧置换，
    // 旧行号经逆置换映射到新位置（滑动窗未平移，选中位置号即重排前的数据行号）
    void remapSelectionByPermutation(const QVector< int >& appliedPerm);
    // 执行一次查找扫描（同步 + 忙等待光标 + 请求序号丢弃过期结果，硬约束②），
    // 命中灌入 overlay(Source::Find, Level::Hit) 并导航到首条
    void runFindScan();
    // 导航到第 idx 条命中（循环）：更新 Level::CurrentHit + showActualRow 定位 + 选中
    void gotoFindHit(int idx);
    // 解析扫描范围：全表 → nullopt；选区 → 行/列包围盒（数据源绝对坐标）
    std::optional< QPair< int, int > > findScanRowRange(bool selectionOnly) const;
    std::optional< QPair< int, int > > findScanColRange(bool selectionOnly) const;
    // 更新计数标签（含截断提示）
    void updateFindCountLabel();
    // 应用替换：all=false 替换当前命中这一格，true 替换扫描范围全部；
    // 变更格数 > c_replaceUndoCellLimit 时提示不可撤销并走降级路径（不入栈，§4.11）
    void applyFindReplace(bool all);
    // 替换生效后刷新模型（按变更范围一次性通知）
    void notifyReplaceChanged(const QVector< int >& rows, const QVector< int >& cols);
    // 按列名（+重名消歧 occurrence）从视图状态子存储应用列宽与行头宽；
    // 应用期间置守卫标志屏蔽采集，避免把「应用」当「用户改动」
    void applyViewStateFromStore();
    // 采集用户拖拽的列宽写入视图状态子存储（程序化/布局触发的 sectionResized 不记录）
    void onHeaderSectionResized(int logicalIndex, int newSize);

    Ui::DADataOperateOfDataFrameWidget* ui;
    DAData mData;
    DADataTableModel* mModel { nullptr };
    DATableStyleManager* mStyleManager { nullptr };  ///< 借自 DATableMetadata 束，非拥有（随数据存在，widget 关闭不销毁）
    DATableViewStateStore* mViewStateStore { nullptr };  ///< 借自 DATableMetadata 束，非拥有（视图状态：列宽/行头宽）
    DATableHighlightOverlay* mHighlightOverlay { nullptr };  ///< 瞬态高亮层（页级持有，不序列化、不进撤销栈）
    DATableStyleItemDelegate* mStyleDelegate { nullptr };
    bool mApplyingViewState { false };  ///< 正在应用视图状态（屏蔽 sectionResized 采集）
    bool mHeaderPickMode { false };     ///< 表头拾取模式（图表「选择序列」会话期间：表头点击只转发不排序）
    QVector< QPair< int, Qt::SortOrder > > mSortSpec;  ///< 多列排序规格（列逻辑索引+方向，顺序即优先级；不持久化 R13）

    DADialogDataframeColumnCastToNumeric* mDialogCastNumArgs { nullptr };
    DADialogDataframeColumnCastToDatetime* mDialogCastDatetimeArgs { nullptr };
    DADialogDataframeColumnDescribe* mDialogColumnDescribe { nullptr };
    DADialogFindReplace* mFindReplaceDialog { nullptr };  ///< 非模态浮动查找/替换条（懒建，页级持有）

    // ===== 查找状态（002 T1.4）=====
    QVector< QPair< int, int > > mFindHits;  ///< 当前命中集（数据源绝对行列，行主序，≤上限）
    int mFindHitIndex { -1 };                ///< 当前命中序号（-1=无当前项）
    int mFindTotalHits { 0 };                ///< 真实命中总数（截断时 > mFindHits.size()）
    int mFindRequestSerial { 0 };            ///< 扫描请求序号（硬约束②：结果返回时非最新即丢弃）
    DADialogFindReplace::FindOptions mLastFindOptions;  ///< 最近一次扫描的参数（范围解析用）
};
}  // end of namespace DA
#endif  // DADATAOPERATEOFDATAFRAMEWIDGET_H
