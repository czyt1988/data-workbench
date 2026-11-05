#ifndef DADATAOPERATEOFDATAFRAMEWIDGET_H
#define DADATAOPERATEOFDATAFRAMEWIDGET_H
#include <QtWidgets/QWidget>
#include <QUndoStack>
#include <functional>
#include <QHash>
#include <QPair>
#include <QPointer>
#include <QVector>
#include <optional>
#include <memory>
class QItemSelectionModel;
#include "DAGuiAPI.h"
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include "numpy/DAPyDType.h"
#include "Table/Metadata/DATableDisplayFormat.h"
#include "Table/Metadata/DATableConditionalFormatRule.h"
#include "Table/Metadata/DATableComputedColumnStore.h"
#include "DATableValidationRule.h"
#include "Table/Controllers/DATableValidationController.h"
#include "Table/Ops/DATableFillOps.h"
#include "Table/Ops/DATableOperationRegistry.h"
#include "Table/DADataOperatePageWidget.h"
#include "Table/Views/DADataHeaderView.h"
#include "Table/Chart/DATableChartTypes.h"
#include "Table/Dialogs/DADialogFindReplace.h"
#include "Table/Dialogs/DADialogGotoCell.h"
namespace Ui
{
class DADataOperateOfDataFrameWidget;
}
class QMenu;

namespace DA
{
class DADataTableModel;
class DADataTableView;
class DATablePaneContainer;
class DADialogDataframeColumnCastToNumeric;
class DADialogDataframeColumnCastToDatetime;
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
class DATableConditionalFormatRule;
class DATableChildWindowContainer;
class DATableChildWindow;
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;
class DATableNoteController;
class DATableQuickPlotController;
class DATableValidationController;
class DATableComputedColumnController;
class DATableConditionalFormatController;
class DATableFindController;
class DATableStyleController;
class DATableSortController;
class DATableViewStateController;
class DATableColumnViewController;
class DATableRowOps;
class DATableCellOps;
class DATableTypeOps;
class DATableFillOps;
class DATableResultOps;
class DATableReshapeOps;
class DATableColumnCombineOps;
class DATableRenameOps;
class DATableColumnOps;
class DATablePaneAssembler;
class DATableNavigationService;


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
    // 外框线：对选中区包围矩形四边应用边框（单元格选区=逐格外边；整列/整行选区有轻量语义）
    void applyBorderOutlineToSelection();
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
    // 定位单入口（007）：跳到「数据行 actualRow + 列 col」并选中；select=false 只做
    // 「摆位 + 移当前项」，不改选区（查找的「选区范围」导航：选区是替换范围基准，
    // 不能被导航收缩），也不夺焦点；keepHidden=true 时保留目标列的隐藏状态
    //（对话框勾选项「保持隐藏列隐藏」用；默认 false = 自动取消隐藏 + 提示，不入撤销栈）
    void locateActualCell(int actualRow, int col, bool select = true, bool keepHidden = false);
    // 列名重载：重名取第一个同名列（数据列序；对话框路径会另给"还有 N 个"提示）
    void locateActualCell(int actualRow, const QString& columnName, bool select = true, bool keepHidden = false);

    // ===== 列视图（006）=====
    // 列视图直接入口（007 定位的对接面）：只写 store + 重投影，不入撤销栈
    void setColumnHiddenDirectly(const QString& name, int occurrence, bool hidden);
    // 打开列管理对话框：列名编辑 + 列顺序调整 + 隐藏勾选 + 冻结勾选；
    // 确定按差异构造一个批量父命令
    void showManageColumnsDialog();

    // ===== 表内快速绘图（快速图表预览）=====
    // 有序选区：按视图顺序返回选区涉及的列（数据列号），隐藏列跳过
    QList< int > getSelectedDataColumnsInViewOrder(bool ensureInDataframe = true) const;
    // 用当前（有序）选区构造快速绘图请求；列角色走 DATableChartPlanner
    DA::DATableChartRequest buildQuickPlotRequest() const;
    // 用当前选区打开/刷新快速绘图子窗口（贴入表格子窗口容器，可拖动/缩放）；
    // 无法出图时返回 false（已给用户可见提示）
    bool showQuickPlot();
    // 关闭快速绘图子窗口（实例保留复用）
    void closeQuickPlot();
    // 快速绘图子窗口是否正在显示
    bool isQuickPlotVisible() const;
    // 转入绘图区的处理器：APP 能拿到 DAChartOperateWidget，由 APP 注入
    using QuickPlotTransferHandler =
        std::function< bool(const DA::DATableChartRequest&, const DA::DATableChartSpec&, bool /*append*/, QString* /*error*/) >;
    void setQuickPlotTransferHandler(const QuickPlotTransferHandler& handler);
    // 「追加到当前图表」是否可用（APP 依绘图区当前图表注入；决定浮窗菜单项灰显）
    using QuickPlotAppendAvailabilityProvider = std::function< bool() >;
    void setQuickPlotAppendAvailabilityProvider(const QuickPlotAppendAvailabilityProvider& provider);
    // 打开定位对话框（007 T1.7）：Ctrl+G / 滚动条右键 / Ribbon「定位」共用
    void showGotoDialog();
    // 解析定位请求并调定位单入口；返回 false = 未解析成功（已给用户可见提示、未改选区）
    bool resolveGotoRequest(const DADialogGotoCell::Request& req);
    // 当前单元格模型索引：selectionModel()->currentIndex()，无效时取选区左上角
    //（都没有返回无效索引）
    QModelIndex currentOrSelectionTopLeftIndex() const;

    // ===== 选区快照（007 状态栏统计；行号一律数据源绝对行号）=====
    // 选择模型（冻结窗格共享它，只取主窗格）
    QItemSelectionModel* tableSelectionModel() const;
    // 选区段数（0=无选区，>1=多段）
    int selectionSegmentCount() const;
    // 选区包围盒（闭区间；无选区返回 false）
    bool selectionBoundingBox(int* rowBegin, int* rowEnd, int* colBegin, int* colEnd) const;
    // 选中单元数（几何去重；按选区区间累加宽×高，不物化索引）
    int selectedCellCount() const;
    // 当前单元格（数据源绝对坐标；声明在 public——007 状态栏统计用）：无有效当前格或越出数据范围返回 false
    bool currentActualCell(int* actualRow, int* actualCol) const;
    // 目标列是否已冻结（007 判定要不要调 showColumn）；查投影缓存的冻结集合
    bool isColumnInFrozenArea(int logicalColumn) const;
    // 生效冻结列数（= 冻结集合中当前可见的列数，转发容器）
    int effectiveFrozenColumnCount() const;
    // 只读访问器：列轴窗格容器（测试与页面内部接线用；APP 不使用、不改其调用点形状）
    DATablePaneContainer* paneContainer() const;

    // 表头拾取模式（图表「选择序列」会话期间置位）：拾取模式下表头点击只转发
    // columnHeaderClicked 信号、不排序（§4.1 模式互斥），由 DADataOperateWidget 转发
    void setHeaderPickMode(bool on);
    bool isHeaderPickMode() const;
    // ===== 按选中列复制/导出（列头右键「复制选中列为/导出选中列」入口的解析面）=====
    // 当前选中列（视图顺序、去重、剔除尾部预留列）；无选中回退 {fallbackColumn}。
    // fallbackColumn 无效（<0 或越出数据列）且无选中时返回空列表（调用方提示）
    QList< int > selectedColumnsForDataAction(int fallbackColumn) const;
    // 当前多列排序规格（列逻辑索引+方向，顺序即优先级；不持久化 R13）。
    // 列号 DATableSortController::c_indexSortColumn(-1) 表示「索引」键
    QVector< QPair< int, Qt::SortOrder > > sortSpec() const;
    // 按列排序（2026-10-18 交互修订：排序入口=列头右键菜单/Ribbon「排序」面板，点击表头不再排序）：
    // sortColumn 重置规格为「仅该列」；addSecondarySortColumn 保留既有规格追加/切换该列。
    // 只读/惰性源与拾取模式下不执行（只读源 daWarning 提示一次）
    void sortColumn(int logicalIndex, Qt::SortOrder order);
    void addSecondarySortColumn(int logicalIndex, Qt::SortOrder order);
    // 排序面板的目标列：当前单元格所在列 > 选区首列 > -1（无目标）
    int currentDataColumnForSort() const;
    // 应用完整排序规格（自定义排序对话框：多条件、顺序即优先级）；成功 push 返回 true
    bool applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& spec);
    // 按索引升序恢复原始行顺序（「清除排序」入口；可撤销）
    void restoreOriginalSortOrder();
    // 打开自定义排序对话框：presetColumn>=0 时以该列为唯一主键预置（表头右键入口），
    // 否则按当前排序规格预填；确定后整份规格作为一条撤销命令入栈
    void showSortDialog(int presetColumn = -1);
    // 数据源是否可编辑（只读/惰性源上排序入口不生效；Ribbon「排序」面板据此置灰）
    bool isTableEditable() const;

    // 打开条件格式规则编辑对话框（003 T1.5）：作用于当前选区所在列；
    // 编辑期预览叠加，确定后按差异生成增/删/改/移命令入页撤销栈（D4）；
    // initialType 为 DATableConditionalFormatRule::Type 整数值（-1=不预设，菜单「其他规则」入口传对应类型）
    void showConditionalFormatDialog(int initialType = -1);
    // 清除选中列的条件格式规则（一次命令，可撤销）
    void clearConditionalFormatFromSelection();
    // 填充条件格式菜单（WPS 式：数据条/色阶/突出显示单元格规则/清除规则/自定义设置；
    // Ribbon 条件格式按钮与表体右键子菜单共用；每次 aboutToShow 调用先 clear 再填充）
    void populateConditionalFormatMenu(QMenu* menu);
    // 打开查找/替换浮动条（002 T1.4）：expandReplace=true 展开替换行（Ctrl+H 语义）；
    // prefillFromCurrentCell=true（右键菜单入口）时查找框预填当前单格显示文本（D9）
    void showFindReplace(bool expandReplace, bool prefillFromCurrentCell = false);
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

    // ===== 填充（005 T1.24；实现迁入 DATableFillOps，008 期4）=====
    // 填充模式（选区按列独立处理，与 Excel 一致；空值判定由 Python 算子完成）；
    // 枚举定义已随填充迁入算子，此处 using 别名保留旧 public 名
    using FillMode = DATableFillOps::FillMode;
    // 选区填充：求值经 da_fill_selection（只返回变化清单），全部列组织成一条
    // DACommandDataFrame_paste（一次填充=一次撤销，D4）；返回写入单元格数
    int fillSelection(FillMode mode);
    // 打开序列填充对话框（数值等差/日期步进，Python 求值+预览）；接受后对选区
    // 每一列各生成一个序列，一条 paste 命令入栈；返回写入单元格数（取消返回 0）
    int showFillSeriesDialog();

    // ===== 数据校验（005 T1.25；实现迁入 DATableValidationController，008 期3）=====
    // 单条规则的校验结果（evaluateValidationRules 输出，报告对话框消费）；
    // 结构随实现迁入 DATableValidationOutcome，此处 using 别名保留旧 public 名
    using ValidationRuleOutcome = DATableValidationOutcome;
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
    // 选区或当前项变化（007 状态栏统计触发源；两窗格共享 selectionModel 只连主窗格）
    void tableSelectionChanged();
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
    // 移除指定列（表头右键/树右键用，绕过选区与焦点依赖）；冻结列被删除时
    // 冻结集合同步剔除，全部冻结列删光后冻结窗格隐藏
    int removeColumnAt(int col);
    // 删列命令管线（上面两个入口共用）：视图状态整店快照随命令 undo/redo 往返
    int removeColumns(const QList< int >& columns);
    // 设置选中单元格为nan,返回设置成功的个数
    int removeSelectCell();
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
    // ===== 列视图真源操作（006c）：构造命令入栈后由回调重投影，均不动 mSortSpec =====
    // 列头菜单「隐藏此列」（logicalColumn 为逻辑列号）
    void hideColumnAt(int logicalColumn);
    // 列头菜单「显示所有列」
    void showAllColumns();
    // 冻结 / 取消冻结指定列（表头右键「冻结此列/取消冻结此列」；入撤销栈）
    void setColumnFrozen(int logicalColumn, bool frozen);
    // 冻结全局列序第一个可见列（Ribbon「冻结首列」）
    void freezeFirstColumn();
    // 取消全部冻结（Ribbon「取消全部冻结」）
    void unfreezeAllColumns();
    // 弹「冻结前 N 列」输入框：把全局列序前 N 列并入冻结集合（追加，0 = 不做修改）
    void promptFreezeColumns();
    // 容器 columnOrderChanged 驱动：把合并后的全局列序（含隐藏列）转成身份表入栈
    void applyColumnOrderFromView(const QVector< int >& newOrderLogical);
    // 容器 frozenOrderChanged 驱动：冻结区内部拖动顺序入栈
    void applyFrozenOrderFromView(const QVector< int >& newFrozenOrderLogical);
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
    // 「列」子菜单动作分发（006d）：列视图真源操作（转发列视图控制器，008 期3）
    void onHeaderColumnViewActionRequested(int logicalColumn, DA::DADataHeaderView::ColumnViewAction action);
    // 表头带修饰键点击（DADataHeaderView::clickedWithModifiers）：
    // 仅拾取模式下转发 columnHeaderClicked；**点击不排序**（2026-10-18 交互修订，
    // 排序入口=列头右键菜单 sortColumn/addSecondarySortColumn）
    void onHeaderClickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers);
    //（查找条六个槽 onFind* 已随查找/替换迁入 DATableFindController，008 期3）

protected:
    void changeEvent(QEvent* e) override;

private:
    // ===== 条件格式菜单（WPS 式预设入口；实现迁入 DATableConditionalFormatController，008 期3）=====
    // 视图行号（滑动窗内逻辑行）转数据源绝对行号：数据操作唯一换算点
    int toActualRow(int viewRow) const;
    //（选区是否与行级/单元格级样式相交已随样式迁入 DATableStyleController，008 期3；
    //  判定实现在选区服务 selectionIntersectsRowStyles/CellStyles）
    // 数据源真实数据行数/列数（不含尾部预留的空白行列）
    int dataRowCount() const;
    int dataColumnCount() const;
    // 实际数据列的身份表（数据列序升序；尾部预留的空白列——列名为空——跳过）。
    // occurrence = 该列之前同名列的个数（数据列序中计，006 列视图键的统一口径）
    QList< QPair< QString, int > > actualColumnIdentities() const;
    //（applySortAction/applySortSpec/remapSelectionByPermutation/mSortSpec 已随排序迁入
    //  DATableSortController，008 期3；isTableEditable 转发上下文 isDataSourceEditable，已置 public）
    //（查找/替换状态机：runFindScan/gotoFindHit/扫描范围/计数标签/applyFindReplace/
    //  notifyReplaceChanged 已随查找/替换迁入 DATableFindController，008 期3）
    //（条件格式差异命令/统计端点可用性已随条件格式迁入 DATableConditionalFormatController，
    //  可用性判定经上下文 isColumnStatsSourceAvailable，008 期3）
    // 按列名（+重名消歧 occurrence）从视图状态子存储应用列宽与行头宽；
    // 应用期间置守卫标志屏蔽采集，避免把「应用」当「用户改动」（转发视图状态控制器）
    void applyViewStateFromStore();
    // 窗格装配唯一路径（006b）：表头替换 → setModel → 委托 → 逐窗格连接；
    // 主窗格与 006e 懒建的冻结窗格都走这里，漏装即该窗格某操作没反应
    void configurePaneView(DADataTableView* pane);
    // 本页所有需要投影的窗格（主窗格在前；冻结窗格懒建后追加）
    QList< DADataTableView* > paneViews() const;
    // 遍历全部窗格的水平表头（排序角标/ƒx 角标等按实例刷新用，006e）
    void forEachPaneHeader(const std::function< void(DADataHeaderView*) >& fn);
    //（列宽套用/内容自适应/自适应宽度/列宽落库/行头宽归属/行高/表头高已随视图状态
    //  迁入 DATableViewStateController，008 期3）
    // 当前选区为单格时取其显示文本（Qt::DisplayRole），否则空串（D9 预填用）
    QString currentSingleCellDisplayText() const;
    //（全局列序身份表 orderedColumnIdentities 已随列视图迁入 DATableColumnViewController，008 期3）
    // 采集用户拖拽的列宽写入视图状态子存储（程序化/布局触发的 sectionResized 不记录）；
    // source = 触发信号的那个窗格的表头（006b 起不再写死 ui->tableView）
    void onHeaderSectionResized(DADataTableView* source, int logicalIndex, int newSize);
    // ===== 计算列私有辅助（004 T1.23；主体迁入 DATableComputedColumnController，008 期3）=====
    // 列改名确认：提示「有 N 个计算列引用它」，返回是否继续（无引用直接 true）
    bool confirmComputedColumnRename(const QList< QPair< QString, QString > >& renames) const;
    // 把计算列定义迁移挂进改名命令的 directional 回调（exec 旧→新 / undo 新→旧，
    // 与改名同一撤销单元，R20）
    void wireComputedColumnRenameMigration(class DACommandDataFrame_renameColumns* cmd,
                                           const QList< QPair< QString, QString > >& renames) const;
    // 数据变更后把依赖这些列（或目标列自身被改）的计算列标待重算（actualCols 为变更
    // 列位置；空列表 = 无操作；实现转发协调器）
    void markComputedColumnsStaleForColumns(const QList< int >& actualCols);
    //（applyEval/refreshHeaders/columnNameAt/markAllComputedColumnsStale 已随计算列
    //  迁入 DATableComputedColumnController，008 期3）
    // 注册新数据集（004 T1.22/T1.27 共用：命名 + addData_ 可撤销注册 + newDataCreated）
    void registerNewDataset(const DAPyDataFrame& resultDf, const QString& name, const QString& describe);
    // 就地替换（004 T1.27）：确认框（列清除条目数）→ 超大表降级分支 / 发信号交 APP 建 Swap 命令
    void requestInplaceReplace(const DAPyDataFrame& newDf, const QString& text);
    // ===== 填充私有辅助（005 T1.24；实现已随填充迁入 DATableFillOps，008 期4）=====
    // 选区按列分组的连续行区间：col -> 若干 [rowStart,rowEnd] 闭区间（绝对行号，升序）。
    // 每列的选中行排序后按连续性切成 run（不规则多选也精确，不填未选中的行）
    QHash< int, QVector< QPair< int, int > > > selectedColumnRowRuns() const;
    //（pushFillPasteCommand 已随填充迁入 DATableFillOps，008 期4）
    // ===== 校验私有辅助（005 T1.25）=====
    // 列改名确认：提示「有 N 条校验规则引用它」，返回是否继续（无引用直接 true）
    bool confirmValidationRuleRename(const QList< QPair< QString, QString > >& renames) const;
    // 把校验规则引用迁移挂进改名命令的 directional 回调（与改名同一撤销单元，R20）
    void wireValidationRuleRenameMigration(class DACommandDataFrame_renameColumns* cmd,
                                           const QList< QPair< QString, QString > >& renames) const;
    // ===== PIMPL（008 期5）：全部数据成员已迁入 cpp 的 PrivateData（DA_DECLARE_PRIVATE 宏展开 d_ptr/d_func）=====
    DA_DECLARE_PRIVATE(DADataOperateOfDataFrameWidget)
};
}  // end of namespace DA
#endif  // DADATAOPERATEOFDATAFRAMEWIDGET_H
