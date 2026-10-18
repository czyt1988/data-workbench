#ifndef DADATAOPERATEOFDATAFRAMEWIDGET_H
#define DADATAOPERATEOFDATAFRAMEWIDGET_H
#include <QtWidgets/QWidget>
#include <QUndoStack>
#include <QHash>
#include <QPair>
#include <QPointer>
#include <QVector>
#include <optional>
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include "numpy/DAPyDType.h"
#include "DATableDisplayFormat.h"
#include "DATableConditionalFormatRule.h"
#include "DATableComputedColumnStore.h"
#include "DATableValidationRule.h"
#include "DADataOperatePageWidget.h"
#include "DADataHeaderView.h"
#include "Dialog/DADialogFindReplace.h"
namespace Ui
{
class DADataOperateOfDataFrameWidget;
}

namespace DA
{
class DADataTableModel;
class DADataTableView;
class DATablePaneContainer;
class DADialogDataframeColumnCastToNumeric;
class DADialogDataframeColumnCastToDatetime;
class DADialogDataframeColumnDescribe;
class DATableStyleManager;
class DATableStyleItemDelegate;
class DATableMetadataRegistry;
class DATableMetadata;
class DATableViewStateStore;
class DATableHighlightOverlay;
class DATableConditionalFormatStore;
class DATableCellStyle;
class DAComputedColumn;
class DATableValidationStore;
class DADialogValidationReport;
class DATableNoteStore;


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
    // 获取条件格式子存储（借自元数据束，可能为 nullptr）
    DATableConditionalFormatStore* conditionalFormatStore() const;
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

    // ===== 列视图（006）=====
    // 列视图直接入口（007 定位的对接面）：只写 store + 重投影，不入撤销栈
    void setColumnHiddenDirectly(const QString& name, int occurrence, bool hidden);
    // 打开列视图对话框（006d）：勾选可见列 + 冻结列数；确定按差异构造一个批量父命令
    void showColumnViewDialog();
    // 目标列是否落在「视图顺序前 m 个可见列」的冻结区（007 判定要不要调 showColumn）；
    // 本单恒 false（冻结投影 006e 收口）；freezeColumnCount()==0 时也恒 false
    bool isColumnInFrozenArea(int logicalColumn) const;
    // 生效冻结列数（本单转发容器，恒 0）
    int effectiveFrozenColumnCount() const;
    // 只读访问器：列轴窗格容器（测试与页面内部接线用；APP 不使用、不改其调用点形状）
    DATablePaneContainer* paneContainer() const;

    // 表头拾取模式（图表「选择序列」会话期间置位）：拾取模式下表头点击只转发
    // columnHeaderClicked 信号、不排序（§4.1 模式互斥），由 DADataOperateWidget 转发
    void setHeaderPickMode(bool on);
    bool isHeaderPickMode() const;
    // 当前多列排序规格（列逻辑索引+方向，顺序即优先级；不持久化 R13）
    QVector< QPair< int, Qt::SortOrder > > sortSpec() const;
    // 按列排序（2026-10-18 交互修订：排序入口=列头右键菜单，点击表头不再排序）：
    // sortColumn 重置规格为「仅该列」；addSecondarySortColumn 保留既有规格追加/切换该列。
    // 只读/惰性源与拾取模式下不执行（只读源 daWarning 提示一次）
    void sortColumn(int logicalIndex, Qt::SortOrder order);
    void addSecondarySortColumn(int logicalIndex, Qt::SortOrder order);

    // 打开条件格式规则编辑对话框（003 T1.5）：作用于当前选区所在列；
    // 编辑期预览叠加，确定后按差异生成增/删/改/移命令入页撤销栈（D4）
    void showConditionalFormatDialog();
    // 清除选中列的条件格式规则（一次命令，可撤销）
    void clearConditionalFormatFromSelection();
    // 打开查找/替换浮动条（002 T1.4）：expandReplace=true 展开替换行（Ctrl+H 语义）
    void showFindReplace(bool expandReplace);
    // 关闭查找/替换浮动条并清查找高亮
    void closeFindReplace();
    // 当前查找命中数（已装载高亮，≤上限）与当前命中序号（-1=无），供测试与外部查询
    int findHitCount() const;
    int currentFindHitIndex() const;

    // ===== 计算列（004 T1.23）=====
    // 获取计算列子存储（借自元数据束，可能为 nullptr）
    DATableComputedColumnStore* computedColumnStore() const;
    // 打开计算列对话框：col<0 为新建，col>=0 为编辑该列既有公式（列头右键「编辑公式」）
    void showComputedColumnDialog(int col = -1);
    // 重算单个计算列（列头右键「重新计算」），col 为列位置索引；成功入栈返回 true
    bool recomputeComputedColumn(int col);
    // 重算全部待重算/失效的计算列（Ribbon「全部重算」），返回成功重算的列数
    int recomputeAllStaleComputedColumns();
    // 移除某列的公式定义（保留列数据，表头右键「移除公式」），可撤销；成功返回 true
    bool removeComputedColumnFormula(int col);

    // ===== 分列 / 合并列（004 T1.9）=====
    // 打开分列对话框：col<0 时取当前选区的唯一数据列（多列/无选区则提示）；
    // 接受后执行拆分并入栈（列级记录撤销，结果列插原列右侧、原列保留）
    void showSplitColumnDialog(int col = -1);
    // 打开合并列对话框（当前选区 >=2 数据列；顺序可在对话框内调整）；
    // 接受后执行合并并入栈（结果列替换被合并列的位置）
    void showMergeColumnsDialog();

    // ===== 分组聚合（004 T1.22）=====
    // 打开分组聚合对话框：产出长格式聚合结果表注册为**新数据集**（命名 xxx_GroupBy，
    // 走 DADataManager::addData_ 可撤销；照透视表 createPivotTable 的真实产出流程），
    // 原表与其元数据完全不动。成功后发 newDataCreated 交 APP 激活新页面
    void showGroupByDialog();

    // ===== 填充（005 T1.24）=====
    // 填充模式（选区按列独立处理，与 Excel 一致；空值判定由 Python 算子完成）
    enum class FillMode
    {
        Down,      ///< 向下填充（每列选区首行值填满其余行，Ctrl+D）
        Up,        ///< 向上填充（每列选区末行值填满其余行）
        Forward,   ///< 选区内向下传递最后一个非空值（ffill）
        Backward   ///< 选区内向上传递下一个非空值（bfill）
    };
    // 选区填充：求值经 da_fill_selection（只返回变化清单），全部列组织成一条
    // DACommandDataFrame_paste（一次填充=一次撤销，D4）；返回写入单元格数
    int fillSelection(FillMode mode);
    // 打开序列填充对话框（数值等差/日期步进，Python 求值+预览）；接受后对选区
    // 每一列各生成一个序列，一条 paste 命令入栈；返回写入单元格数（取消返回 0）
    int showFillSeriesDialog();

    // ===== 数据校验（005 T1.25）=====
    // 单条规则的校验结果（evaluateValidationRules 输出，报告对话框消费）
    struct ValidationRuleOutcome
    {
        DATableValidationRule rule;   ///< 规则定义
        int columnPosition { -1 };    ///< 解析出的列位置（-1 = 列不存在）
        bool columnMissing { false }; ///< 规则引用的列不存在（跳过求值）
        QVector< int > rows;          ///< 违规行（数据源绝对行号，升序）
    };
    // 获取校验规则子存储（借自元数据束，可能为 nullptr）
    DATableValidationStore* validationStore() const;
    // 打开校验规则编辑对话框；确定后按差异生成 setRules 命令入页撤销栈（D4）
    void showValidationRulesDialog();
    // 执行校验（只读，红线2：不改数据、不入撤销栈）：求值全部启用规则、违规装载
    // Source::Validation 高亮（Error/Warning 两级），返回逐规则结果；elapsedMs 非空
    // 时输出求值耗时。唯一性规则命中缓存不重算（§4.9）
    QList< ValidationRuleOutcome > evaluateValidationRules(int* elapsedMs = nullptr);
    // 执行校验并弹出报告对话框（非模态，点击样例行定位）
    void runValidationAndShowReport();
    // 清除违规高亮（只清 Source::Validation，不动查找命中——来源隔离）
    void clearValidationHighlights();
    // 规则显示文本（级别 + 列（第 n 个）+ 类型 + 参数 + 名称；报告与列表共用）
    QString validationRuleDisplayText(const DATableValidationRule& rule) const;

    // ===== 单元格批注（005 T1.26）=====
    // 获取批注子存储（借自元数据束，可能为 nullptr）
    DATableNoteStore* noteStore() const;
    // 打开当前单元格的批注编辑弹层（无批注=插入，有批注=编辑且弹层内可删除）；
    // 弹层跟随单元格右下角定位；保存/删除按旧新条目生成命令入页撤销栈（D4）
    void showNoteEditorForCurrentCell();
    // 删除当前单元格批注（命令可撤销）；无当前格或无批注返回 false
    bool deleteNoteAtCurrentCell();
    // 清除本表全部批注（一次命令，可撤销）；返回清除的批注数
    int clearAllNotes();

    // ===== 转置 / melt 形状变换（004 T1.27）=====
    // 打开形状变换对话框（meltMode=false 转置 / true 宽转长，模式可在对话框内切换）。
    // 默认产出新数据集（xxx_Transposed / xxx_Melted，addData_ 可撤销，R14）；
    // 勾选「就地替换」时先弹确认框列出将被清除的元数据类别与条目数，再经
    // inplaceDataReplaceRequested 交 APP 用 DAAppCommand 建 Swap 命令（统一撤销链，
    // 元数据清理与数据替换同一撤销单元）；超大表降级为「本次不可撤销」并再次确认
    void showReshapeDialog(bool meltMode);
    // 就地替换生效后的整页刷新（模型 reset + 清排序规格；APP 撤销接线回调也用）
    void refreshAfterInplaceReplace();
    // 就地替换撤销内存判定（旧对象驻留 ≈ 1× 表内存，§4.11 策略表）：
    // 单元格数超过 c_inplaceUndoCellLimit 建议降级为不可撤销（静态纯函数，单测覆盖）
    static constexpr qint64 c_inplaceUndoCellLimit = 50000000;
    static bool isInplaceUndoTooLarge(qint64 rows, qint64 cols);

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
    // 表格工具改动了数据（计算列重算等，004）；APP 接线 setDirty
    void dataModified();
    // 产出了新数据集（分组聚合等，004 T1.22；已经 addData_ 注册、可撤销）；
    // APP 接线：setDirty + 抬起数据操作区 + showData 激活新页面（照透视表流程）
    void newDataCreated(const DA::DAData& data);
    // 就地数据替换请求（004 T1.27 转置/melt 就地路径；DAGui 拿不到 DAAppCommand，
    // 按既有 APP↔DAGui 接线走信号）：APP 用 beginDataOperateCommand(data,text,false)
    // 建 Swap 命令（不要自己 new，统一撤销链）→ setPyObject → endDataOperateCommand；
    // metadataSnapshot 为整束元数据 XML 快照——APP 经命令 directional 回调做
    // 「redo 清束 / undo 还原快照」，与数据替换同一撤销单元
    void inplaceDataReplaceRequested(const DA::DAData& target, const DA::DAData& source, const QString& text, const QString& metadataSnapshot);
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
    // ===== 列视图真源操作（006c）：构造命令入栈后由回调重投影，均不动 mSortSpec =====
    // 列头菜单「隐藏此列」（logicalColumn 为逻辑列号）
    void hideColumnAt(int logicalColumn);
    // 列头菜单「显示所有列」
    void showAllColumns();
    // 冻结前 N 个可见列（0 = 取消冻结；入撤销栈）
    void freezeColumnsTo(int visibleCount);
    // 弹「冻结前 N 列」输入框后走 freezeColumnsTo
    void promptFreezeColumns();
    // 容器 columnOrderChanged 驱动：把合并后的全局列序（含隐藏列）转成身份表入栈
    void applyColumnOrderFromView(const QVector< int >& newOrderLogical);
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
    // 「列」子菜单动作分发（006d）：列视图真源操作
    void onHeaderColumnViewActionRequested(int logicalColumn, DA::DADataHeaderView::ColumnViewAction action);
    // 表头带修饰键点击（DADataHeaderView::clickedWithModifiers）：
    // 仅拾取模式下转发 columnHeaderClicked；**点击不排序**（2026-10-18 交互修订，
    // 排序入口=列头右键菜单 sortColumn/addSecondarySortColumn）
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
    // 实际数据列的身份表（数据列序升序；尾部预留的空白列——列名为空——跳过）。
    // occurrence = 该列之前同名列的个数（数据列序中计，006 列视图键的统一口径）
    QList< QPair< QString, int > > actualColumnIdentities() const;
    // 数据源是否可编辑（只读/惰性源上排序入口不生效，判定路径同 DADataTableModel::actualFlags）
    bool isTableEditable() const;
    // 应用排序规格：构造 DACommandDataFrame_sortRows、接线样式/模型/选区/视口回调并 push；
    // 算子失败（如混合类型不可比较）提示并保持原规格，返回 false
    bool applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& newSpec);
    // 排序动作统一入口（菜单驱动）：守卫（拾取模式/边界/只读/空表）→ 构造规格 → applySortSpec；
    // addKey=false 重置为「仅该列」，true 保留既有规格追加/切换该列
    bool applySortAction(int logicalIndex, Qt::SortOrder order, bool addKey);
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
    // 条件格式（003）：规则集与 store 现值差异 → 生成增/删/改/移子命令的父命令；
    // 无差异返回 nullptr（不推空命令）
    QUndoCommand* buildConditionalFormatCommands(const QHash< int, QVector< DATableConditionalFormatRule > >& newRules) const;
    // 色阶/数据条的数据源可用性：惰性加载或不可编辑源无法全列统计 → false（§1.4-2，
    // 判定走模型持有的 DATableDataSource，不新造平行接口）
    bool isColumnStatsSourceAvailable() const;
    // 按列名（+重名消歧 occurrence）从视图状态子存储应用列宽与行头宽；
    // 应用期间置守卫标志屏蔽采集，避免把「应用」当「用户改动」
    void applyViewStateFromStore();
    // 窗格装配唯一路径（006b）：表头替换 → setModel → 委托 → 逐窗格连接；
    // 主窗格与 006e 懒建的冻结窗格都走这里，漏装即该窗格某操作没反应
    void configurePaneView(DADataTableView* pane);
    // 本页所有需要投影的窗格（本单只有主窗格；006e 在末尾追加冻结窗格）
    QList< DADataTableView* > paneViews() const;
    // 全局列序（含隐藏列）的身份表：store 的 columnOrder 按 (名, occurrence) 精确匹配
    // actualColumnIdentities()，每条只匹配一次，未匹配的实际列按 logical 升序追加末尾；
    // store 为空 = logical 升序。任何地方要"全局列序身份表"都调这里（006d 对话框的输入）
    QList< QPair< QString, int > > orderedColumnIdentities() const;
    // 采集用户拖拽的列宽写入视图状态子存储（程序化/布局触发的 sectionResized 不记录）；
    // source = 触发信号的那个窗格的表头（006b 起不再写死 ui->tableView）
    void onHeaderSectionResized(DADataTableView* source, int logicalIndex, int newSize);
    // ===== 计算列私有辅助（004 T1.23）=====
    // 求值 def 并入栈重算命令：isNewColumn=true 走创建路径（undo 删列），
    // false 走重算路径（undo 还原旧列）；成功返回 true（求值失败标失效并提示）
    bool applyComputedColumnEval(const DAComputedColumn& def, bool isNewColumn);
    // 列改名确认：提示「有 N 个计算列引用它」，返回是否继续（无引用直接 true）
    bool confirmComputedColumnRename(const QList< QPair< QString, QString > >& renames) const;
    // 把计算列定义迁移挂进改名命令的 directional 回调（exec 旧→新 / undo 新→旧，
    // 与改名同一撤销单元，R20）
    void wireComputedColumnRenameMigration(class DACommandDataFrame_renameColumns* cmd,
                                           const QList< QPair< QString, QString > >& renames) const;
    // 数据变更后把依赖这些列（或目标列自身被改）的计算列标待重算（actualCols 为变更
    // 列位置；空列表 = 无操作）
    void markComputedColumnsStaleForColumns(const QList< int >& actualCols);
    // 拿不到列信息的整表操作（插删行/列转索引等）保守把全部计算列标待重算（R8）
    void markAllComputedColumnsStale();
    // store 变更后刷新表头（底色/角标/tooltip 重绘）
    void refreshComputedColumnHeaders();
    // 列位置索引 → 列名（越界返回空串）
    QString columnNameAt(int actualCol) const;
    // 注册新数据集（004 T1.22/T1.27 共用：命名 + addData_ 可撤销注册 + newDataCreated）
    void registerNewDataset(const DAPyDataFrame& resultDf, const QString& name, const QString& describe);
    // 就地替换（004 T1.27）：确认框（列清除条目数）→ 超大表降级分支 / 发信号交 APP 建 Swap 命令
    void requestInplaceReplace(const DAPyDataFrame& newDf, const QString& text);
    // ===== 填充私有辅助（005 T1.24）=====
    // 选区按列分组的连续行区间：col -> 若干 [rowStart,rowEnd] 闭区间（绝对行号，升序）。
    // 每列的选中行排序后按连续性切成 run（不规则多选也精确，不填未选中的行）
    QHash< int, QVector< QPair< int, int > > > selectedColumnRowRuns() const;
    // 用填充清单构造一条 DACommandDataFrame_paste 并入栈（值记录，D4；含统计失效/
    // 计算列标脏/模型刷新回调，与 pasteFromClipboard 同款）；返回写入单元格数
    int pushFillPasteCommand(const DAPyDataFrame& df,
                             const QList< int >& rows,
                             const QList< int >& cols,
                             const QList< QVariant >& values,
                             const QString& text);
    // ===== 校验私有辅助（005 T1.25）=====
    // 列改名确认：提示「有 N 条校验规则引用它」，返回是否继续（无引用直接 true）
    bool confirmValidationRuleRename(const QList< QPair< QString, QString > >& renames) const;
    // 把校验规则引用迁移挂进改名命令的 directional 回调（与改名同一撤销单元，R20）
    void wireValidationRuleRenameMigration(class DACommandDataFrame_renameColumns* cmd,
                                           const QList< QPair< QString, QString > >& renames) const;
    // ===== 批注私有辅助（005 T1.26）=====
    // 当前单元格（数据源绝对坐标）：无有效当前格或越出数据范围返回 false
    bool currentActualCell(int* actualRow, int* actualCol) const;
    // 对给定格弹出编辑弹层并落地结果（插入/编辑共用：保存→增/改命令，删除→删除命令）
    void editNoteAtCell(int actualRow, int actualCol);

    Ui::DADataOperateOfDataFrameWidget* ui;
    DAData mData;
    DADataTableModel* mModel { nullptr };
    DATableStyleManager* mStyleManager { nullptr };  ///< 借自 DATableMetadata 束，非拥有（随数据存在，widget 关闭不销毁）
    DATableViewStateStore* mViewStateStore { nullptr };  ///< 借自 DATableMetadata 束，非拥有（视图状态：列宽/行头宽）
    DATableHighlightOverlay* mHighlightOverlay { nullptr };  ///< 瞬态高亮层（页级持有，不序列化、不进撤销栈）
    DATableConditionalFormatStore* mConditionalFormatStore { nullptr };  ///< 借自 DATableMetadata 束，非拥有（条件格式规则，003）
    DATableComputedColumnStore* mComputedColumnStore { nullptr };        ///< 借自 DATableMetadata 束，非拥有（计算列定义，004 T1.23）
    DATableValidationStore* mValidationStore { nullptr };                ///< 借自 DATableMetadata 束，非拥有（校验规则，005 T1.25）
    DATableNoteStore* mNoteStore { nullptr };                            ///< 借自 DATableMetadata 束，非拥有（单元格批注，005 T1.26）
    DATableMetadata* mMetadata { nullptr };                              ///< 元数据束（借自注册表，非拥有；就地形状变换整束快照/清理用，004 T1.27）
    DATableStyleItemDelegate* mStyleDelegate { nullptr };
    DATablePaneContainer* mPaneContainer { nullptr };  ///< 列轴窗格容器（页级拥有，006；主窗格运行时包装进容器）
    bool mApplyingViewState { false };  ///< 正在应用视图状态（屏蔽 sectionResized 采集）
    bool mHeaderPickMode { false };     ///< 表头拾取模式（图表「选择序列」会话期间：表头点击只转发不排序）
    QVector< QPair< int, Qt::SortOrder > > mSortSpec;  ///< 多列排序规格（列逻辑索引+方向，顺序即优先级；不持久化 R13）

    DADialogDataframeColumnCastToNumeric* mDialogCastNumArgs { nullptr };
    DADialogDataframeColumnCastToDatetime* mDialogCastDatetimeArgs { nullptr };
    DADialogDataframeColumnDescribe* mDialogColumnDescribe { nullptr };
    DADialogFindReplace* mFindReplaceDialog { nullptr };  ///< 非模态浮动查找/替换条（懒建，页级持有）
    class DADialogTableColumnView* mDialogColumnView { nullptr };  ///< 列视图对话框（006d，懒建复用）
    QPointer< DADialogValidationReport > mValidationReport;  ///< 非模态校验报告窗（005 T1.25，重跑前先关旧的）

    // ===== 查找状态（002 T1.4）=====
    QVector< QPair< int, int > > mFindHits;  ///< 当前命中集（数据源绝对行列，行主序，≤上限）
    int mFindHitIndex { -1 };                ///< 当前命中序号（-1=无当前项）
    int mFindTotalHits { 0 };                ///< 真实命中总数（截断时 > mFindHits.size()）
    int mFindRequestSerial { 0 };            ///< 扫描请求序号（硬约束②：结果返回时非最新即丢弃）
    DADialogFindReplace::FindOptions mLastFindOptions;  ///< 最近一次扫描的参数（范围解析用）
};
}  // end of namespace DA
#endif  // DADATAOPERATEOFDATAFRAMEWIDGET_H
