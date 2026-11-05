#include "Table/DADataOperateOfDataFrameWidget.h"
#include "ui_DADataOperateOfDataFrameWidget.h"
#include "Table/Models/DADataTableModel.h"
#include "Table/Models/DAAbstractCacheWindowTableModel.h"
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
#include "DAWaitCursorScoped.h"
#include "DALogCategory.h"
#include "DASlowLog.h"
#include "DATableDataSource.h"
#include "Table/Metadata/DATableKeyRemap.h"
#include "DAPybind11QtCaster.hpp"
// stl
#include <memory>
#include <algorithm>
#include <utility>
// qt
#include <QTableView>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QSharedPointer>
#include <QMessageBox>
#include <QPointer>
#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QMenu>
#include <QBrush>
// cmd
#include "Table/Ops/DACommandsDataFrame.h"
#include "Table/Commands/DACommandsTableStyle.h"
#include "Table/Commands/DACommandsTableColumnFormat.h"
#include "Table/Commands/DACommandsConditionalFormat.h"
#include "Table/Commands/DACommandsTableColumnView.h"
#include "DADataManager.h"
#include "DACallBackInterface.h"

#include <QDomDocument>
// table metadata / style
#include "Table/Metadata/DATableStyleManager.h"
#include "Table/Views/DATableStyleItemDelegate.h"
#include "Table/Metadata/DATableMetadataRegistry.h"
#include "Table/Metadata/DATableMetadata.h"
#include "Table/Metadata/DATableViewStateStore.h"
#include "Table/Views/DATableHighlightOverlay.h"
#include "Table/Metadata/DATableConditionalFormatStore.h"
#include "Table/Metadata/DATableComputedColumnStore.h"
#include "Table/Metadata/DATableValidationStore.h"
#include "Table/Commands/DACommandsTableValidation.h"
#include "Table/Metadata/DATableNoteStore.h"
#include "Table/Dialogs/DATableNoteEditDialog.h"
#include "Table/Views/DADataHeaderView.h"
#include "Table/Views/DATablePaneContainer.h"
// 表内快速绘图
#include "Table/Chart/DATableChartPlanner.h"
#include "Table/Chart/DATableQuickPlotWindow.h"
// 表格子窗口容器（快速绘图的承载框架，也可贴入其它窗口）
#include "Table/Views/DATableChildWindow.h"
#include "Table/Views/DATableChildWindowContainer.h"
// 表格页核心（008 期1：页面上下文 + 选区服务；期2：编辑后效协调器）
#include "Table/Core/DATablePageContext.h"
#include "Table/Core/DATableSelectionService.h"
#include "Table/Core/DATableMutationCoordinator.h"
// 表格页特性控制器（008 期3）
#include "Table/Controllers/DATableNoteController.h"
#include "Table/Controllers/DATableQuickPlotController.h"
#include "Table/Controllers/DATableComputedColumnController.h"
#include "Table/Controllers/DATableConditionalFormatController.h"
#include "Table/Controllers/DATableFindController.h"
#include "Table/Controllers/DATableStyleController.h"
#include "Table/Controllers/DATableSortController.h"
#include "Table/Controllers/DATableViewStateController.h"
#include "Table/Controllers/DATableColumnViewController.h"
#include "Table/Ops/DATableRowOps.h"
#include "Table/Ops/DATableCellOps.h"
#include "Table/Ops/DATableTypeOps.h"
#include "Table/Ops/DATableFillOps.h"
#include "Table/Ops/DATableResultOps.h"
#include "Table/Ops/DATableReshapeOps.h"
#include "Table/Ops/DATableColumnCombineOps.h"
#include "Table/Ops/DATableRenameOps.h"
#include "Table/Ops/DATableColumnOps.h"
#include "Table/Ops/DATableOperationRegistry.h"
#include "Table/Core/DATablePaneAssembler.h"
#include "Table/Controllers/DATableNavigationService.h"
#include "Table/Controllers/DATableNavigationService.h"
// Dialog
#include "Table/Dialogs/DADialogDataframeColumnCastToNumeric.h"
#include "Table/Dialogs/DADialogDataframeColumnCastToDatetime.h"
#include "Table/Dialogs/DADialogInsertNewColumn.h"
#include "Table/Dialogs/DADialogManageColumns.h"
#include "Table/Dialogs/DADialogFindReplace.h"
#include "Table/Dialogs/DADialogConditionalFormat.h"
#include "Table/Dialogs/DADialogConditionalFormatQuickRule.h"
#include "Table/Metadata/DAConditionalFormatPresets.h"
#include "Table/Dialogs/DADialogComputedColumn.h"
#include "Table/Dialogs/DADialogSplitColumn.h"
#include "Table/Dialogs/DADialogMergeColumns.h"
#include "Table/Dialogs/DADialogGroupBy.h"
#include "Table/Dialogs/DADialogReshape.h"
#include "Table/Dialogs/DADialogFillSeries.h"
#include "Table/Dialogs/DATableSortDialog.h"
// python operator wrapper（查找/替换）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include <climits>

//===================================================
// 008 表格页重构 · 分区索引（迁移路线见 tmp/plan-表格页重构与职责拆分.md §5）
//
// 期1 已迁出（src/DAGui/Table/Core/）：
//   DATablePageContext      —— 依赖注入载体：模型/主窗格/元数据束聚合
//   DATableSelectionService —— 选区/坐标纯查询（~20 个选区方法，本文件保留 public 一行转发）
//
// 本文件分区（自上而下，期号 = 计划迁出的目标归属）：
//   匿名空间工具      —— toUniqueSorted/toIdentSet/mergeFrozenOrder（列视图与粘贴用）
//   构造/析构         —— 窗格装配接线 + store→模型连接（期2 收口 DATableMutationCoordinator）
//   数据操作          —— insertRow/removeColumns/paste/cut/rename/cast（期4 Strategy 算子）
//   选区/坐标（转发） —— getSelected*/selection*/currentActualCell → DATableSelectionService
//   导航             —— ensureColumnVisible/locateActualCell/showGotoDialog（期3 NavigationService）
//   视图状态          —— applyViewStateFromStore/列宽/行高（期3 ViewStateController）
//   列视图            —— 隐藏/冻结/列序/列视图对话框（期3 ColumnViewController）
//   样式/显示格式      —— mergeStyleToSelection 等（期3 StyleController）
//   排序             —— sortSpec/applySortAction（期3 SortController）
//   查找/替换         —— showFindReplace/runFindScan/applyFindReplace（期3 FindController）
//   条件格式          —— showConditionalFormatDialog/populateConditionalFormatMenu（期3 CF 控制器）
//   ==== 计算列 ====        ——（期3 ComputedColumnController）
//   ==== 分列 / 合并列 ====  ——（期4 算子）
//   ==== 分组聚合 ====      ——（期4 算子）
//   ==== 转置 / melt ====   ——（期4 算子）
//   ==== 填充 ====          ——（期4 算子）
//   ==== 数据校验 ====      ——（期3 ValidationController）
//   ==== 单元格批注 ====    ——（期3 NoteController）
//   ==== 表内快速绘图 ====  ——（期3 QuickPlotController）
//   表格子窗口容器    —— tableChildWindowContainer/tableChildWindowBounds/closeQuickPlot
//===================================================

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;
//===================================================
// DADataOperateOfDataFrameWidget::PrivateData（008 期5 PIMPL）
//===================================================
class DADataOperateOfDataFrameWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DADataOperateOfDataFrameWidget)

public:
    PrivateData(DADataOperateOfDataFrameWidget* q)
        : q_ptr(q), ui(new Ui::DADataOperateOfDataFrameWidget)
    {
    }
    ~PrivateData()
    {
        if (ui) {
            delete ui;
            ui = nullptr;
        }
    }

public:
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
    bool mHeaderPickMode { false };     ///< 表头拾取模式（图表「选择序列」会话期间：表头点击只转发不排序）
    int mNormalRowHeight { -1 };        ///< 常规行高记忆（紧凑/普通切换恢复用；-1 = 未记录）
    std::unique_ptr< DATablePageContext > mContext;          ///< 依赖注入载体：模型/主窗格/元数据束聚合
    std::unique_ptr< DATableSelectionService > mSelection;   ///< 选区/坐标纯查询服务（构造注入上下文）
    DATableMutationCoordinator* mMutation { nullptr };  ///< 编辑后效协调器（008 期2，QObject 父子持有）
    DATableNoteController* mNoteController { nullptr };  ///< 批注控制器（008 期3，QObject 父子持有）
    DATableQuickPlotController* mQuickPlotController { nullptr };  ///< 快速绘图控制器（008 期3，QObject 父子持有）
    DATableValidationController* mValidationController { nullptr };  ///< 数据校验控制器（008 期3，QObject 父子持有）
    DATableComputedColumnController* mComputedColumnController { nullptr };  ///< 计算列控制器（008 期3，QObject 父子持有）
    DATableConditionalFormatController* mConditionalFormatController { nullptr };  ///< 条件格式控制器（008 期3，QObject 父子持有）
    DATableFindController* mFindController { nullptr };  ///< 查找/替换控制器（008 期3，QObject 父子持有）
    DATableStyleController* mStyleController { nullptr };  ///< 样式/显示格式控制器（008 期3，QObject 父子持有）
    DATableSortController* mSortController { nullptr };  ///< 排序控制器（008 期3，QObject 父子持有）
    DATableViewStateController* mViewStateController { nullptr };  ///< 视图状态控制器（008 期3，QObject 父子持有）
    DATableColumnViewController* mColumnViewController { nullptr };  ///< 列视图控制器（008 期3，QObject 父子持有）
    DATableRowOps* mRowOps { nullptr };  ///< 行操作算子（008 期4，页级持有）
    DATableCellOps* mCellOps { nullptr };  ///< 单元格操作算子（008 期4，页级持有）
    DATableTypeOps* mTypeOps { nullptr };  ///< 类型转换算子（008 期4，页级持有）
    DATableFillOps* mFillOps { nullptr };  ///< 填充算子（008 期4，页级持有）
    DATableResultOps* mResultOps { nullptr };  ///< 结果落库算子（008 期4，页级持有）
    DATableReshapeOps* mReshapeOps { nullptr };  ///< 形状变换算子（008 期4，页级持有）
    DATableColumnCombineOps* mColumnCombineOps { nullptr };  ///< 分列/合并列算子（008 期4，页级持有）
    DATableRenameOps* mRenameOps { nullptr };  ///< 列改名算子（008 期4，页级持有）
    DATableColumnOps* mColumnOps { nullptr };  ///< 列增删算子（008 期4，页级持有）
    DATableOperationRegistry mOpRegistry;  ///< 算子注册表（008 期4：id → 唯一实例）
    DATablePaneAssembler* mPaneAssembler { nullptr };  ///< 窗格装配器（008 期5：装配段收口，页级持有）
    DATableNavigationService* mNavigation { nullptr };  ///< 导航服务（008 期5：定位对话框/请求解析，页级持有）
};


namespace
{
//（toUniqueSorted/toIdentSet/mergeFrozenOrder/c_replaceUndoCellLimit 已随各控制器迁出：
//  选区查询 → DATableSelectionService；查找/替换 → DATableFindController；
//  列视图合并 → DATableColumnViewController；替换阈值 → DATableFindController，008 期1-3）
}

//===================================================
// DADataOperateOfDataFrameWidget
//===================================================
int DADataOperateOfDataFrameWidget::getDataOperatePageType() const
{
    return DADataOperatePageWidget::DataOperateOfDataFrame;
}

DADataOperateOfDataFrameWidget::DADataOperateOfDataFrameWidget(const DAData& d, DATableMetadataRegistry* registry, QWidget* parent)
    : DADataOperatePageWidget(parent), DA_PIMPL_CONSTRUCT
{
    d_func()->ui->setupUi(this);

    // 运行时包装（006b）：把 .ui 里已嵌入的视图作为主窗格接管。.ui 不动；
    // getDataTableView() 返回值与语义不变（= 主窗格），APP/插件调用点零改动。
    // 必须在任何 configurePaneView() 之前（configurePaneView 里 006c/006d 要往
    // mPaneContainer 上连线；先装配后建容器 = 那些 if (mPaneContainer) 全部静默跳过）
    d_func()->ui->verticalLayout->removeWidget(d_func()->ui->tableView);
    d_func()->mPaneContainer = new DATablePaneContainer(this);
    d_func()->ui->verticalLayout->addWidget(d_func()->mPaneContainer);
    d_func()->mPaneContainer->setMainView(d_func()->ui->tableView);
    // 拖动落库（006c）：容器合并出全局列序后交页面入栈重投影（拖动 = 一个撤销单元）
    connect(d_func()->mPaneContainer, &DATablePaneContainer::columnOrderChanged, this, &DADataOperateOfDataFrameWidget::applyColumnOrderFromView);
    // 冻结区内部拖动落库：冻结列之间的先后顺序（不改全局列序），入栈重投影
    connect(d_func()->mPaneContainer, &DATablePaneContainer::frozenOrderChanged, this, &DADataOperateOfDataFrameWidget::applyFrozenOrderFromView);
    // 冻结分界条（宽随列宽改造）：拖动结束写 store（与表头拖拽同键 (名, occurrence)），
    // 双击对末冻结列做内容自适应；两信号均在容器已确定末冻结列的前提下发出
    //（视图状态采集/投影经视图状态控制器，008 期3）
    connect(d_func()->mPaneContainer, &DATablePaneContainer::frozenBoundaryResized, this,
            [ this ](int logicalColumn, int newWidth) {
        d_func()->mViewStateController->storeColumnWidthAt(logicalColumn, newWidth);
    });
    connect(d_func()->mPaneContainer, &DATablePaneContainer::frozenBoundaryAutoFitRequested, this, [ this ](int logicalColumn) {
        if (!d_func()->mPaneContainer || !d_func()->mPaneContainer->frozenView()) {
            return;
        }
        const int w = d_func()->mViewStateController->contentFittedColumnWidth(d_func()->mPaneContainer->frozenView(), logicalColumn);
        if (w <= 0) {
            return;
        }
        d_func()->mPaneContainer->frozenView()->horizontalHeader()->resizeSection(logicalColumn, w);
        d_func()->mViewStateController->storeColumnWidthAt(logicalColumn, w);
    });
    // 冻结窗格懒建（006e）：容器建好窗格后交页面装配同一条 configurePaneView，
    // 装配完成后再补列宽/行头宽；转发契约（冻结 → 主）也在此接线（APP 只连主窗格）
    connect(d_func()->mPaneContainer, &DATablePaneContainer::frozenViewCreated, this, [ this ](DA::DADataTableView* frozen) {
        configurePaneView(frozen);
        d_func()->mViewStateController->applyColumnWidthsToPane(frozen);
        d_func()->mViewStateController->autoFitUnrecordedColumnWidths(frozen);  // 无记录列内容自适应（冻结窗格首次懒建时视口可能还没行，靠投影尾轮兜底）
        d_func()->mViewStateController->applyRowHeaderWidthToOwner();
        // 表头动作转发：重命名/删除/定位最大最小/描述统计/计算列三项（APP 只连主窗格）
        DADataHeaderView* fh = qobject_cast< DADataHeaderView* >(frozen->horizontalHeader());
        DADataHeaderView* mh = qobject_cast< DADataHeaderView* >(d_func()->ui->tableView->horizontalHeader());
        if (fh && mh) {
            connect(fh, &DADataHeaderView::columnActionRequested, mh, &DADataHeaderView::columnActionRequested);
            // 快速绘图子菜单（冻结窗格表头触发 → 主窗格同名信号，APP 只连主窗格）
            connect(fh, &DADataHeaderView::quickPlotRequested, mh, &DADataHeaderView::quickPlotRequested);
            // 按选中列复制/导出（冻结窗格表头触发 → 主窗格同名信号，APP 只连主窗格）
            connect(fh, &DADataHeaderView::columnDataCopyRequested, mh, &DADataHeaderView::columnDataCopyRequested);
            connect(fh, &DADataHeaderView::columnDataExportRequested, mh, &DADataHeaderView::columnDataExportRequested);
        }
        // 表体剪贴板三信号转发到主窗格同名信号
        connect(frozen, &DADataTableView::contextMenuCutRequested, d_func()->ui->tableView, &DADataTableView::contextMenuCutRequested);
        connect(frozen, &DADataTableView::contextMenuCopyRequested, d_func()->ui->tableView, &DADataTableView::contextMenuCopyRequested);
        connect(frozen, &DADataTableView::contextMenuPasteRequested, d_func()->ui->tableView, &DADataTableView::contextMenuPasteRequested);
        // 列宽/冻结数变化都会触发容器重投影与几何重算
        connect(frozen->horizontalHeader(), &QHeaderView::sectionResized, this, [ this ]() {
            if (d_func()->mPaneContainer) {
                d_func()->mPaneContainer->updateFrozenGeometry();
            }
        });
        // 冻结窗格行头宽变化：行头归属冻结窗格，宽度变化要同步几何
        if (DADataHeaderView* fvh = qobject_cast< DADataHeaderView* >(frozen->verticalHeader())) {
            connect(fvh, &DADataHeaderView::rowHeaderWidthChanged, this, [ this ](int width) {
                if (d_func()->mViewStateStore) {
                    d_func()->mViewStateStore->setRowHeaderWidth(width);
                }
                if (d_func()->mPaneContainer) {
                    d_func()->mPaneContainer->updateFrozenGeometry();
                }
            });
        }
    });

    d_func()->mModel = new DADataTableModel(getUndoStack(), this);

    // 表格元数据束与各子存储、delegate
    // 元数据束借自会话级 DATableMetadataRegistry（随数据存在，widget 关闭后保留，重开可见）。
    // 行列增删的样式键偏移同步在 §7 的回调中接入（onRowsInserted 等系列）。
    DATableMetadata* metadata = registry ? registry->getOrCreate(d) : nullptr;
    d_func()->mMetadata                 = metadata;
    d_func()->mStyleManager             = metadata ? metadata->styleManager() : nullptr;
    d_func()->mViewStateStore           = metadata ? metadata->viewState() : nullptr;
    d_func()->mConditionalFormatStore   = metadata ? metadata->conditionalFormat() : nullptr;
    d_func()->mComputedColumnStore      = metadata ? metadata->computedColumns() : nullptr;
    d_func()->mValidationStore          = metadata ? metadata->validations() : nullptr;
    d_func()->mNoteStore                = metadata ? metadata->notes() : nullptr;
    // 瞬态高亮层：页级持有（查找/校验两个来源共用），不序列化、不进撤销栈；
    // 注入 delegate 绘制管线③（瞬态装饰）
    d_func()->mHighlightOverlay = new DATableHighlightOverlay(this);
    d_func()->mStyleDelegate    = new DATableStyleItemDelegate(d_func()->mStyleManager, d_func()->mHighlightOverlay, this);
    // 条件格式子存储注入 delegate 绘制管线②（持久装饰）与④（数据条），003 T1.5
    d_func()->mStyleDelegate->setConditionalFormatStore(d_func()->mConditionalFormatStore);
    // 校验规则子存储注入 delegate（枚举列下拉编辑器 createEditor，005 T1.25）
    d_func()->mStyleDelegate->setValidationStore(d_func()->mValidationStore);
    // 批注子存储注入 delegate（层⑤之后角标 + helpEvent tooltip，005 T1.26）
    //（视图侧 setNoteStore 在 configurePaneView 里，006b 起随窗格装配）
    d_func()->mStyleDelegate->setNoteStore(d_func()->mNoteStore);
    // 表格页上下文（008 期1）：模型/主窗格/元数据束聚合为依赖注入载体；
    // 选区/坐标查询全部收口到 DATableSelectionService（纯 const 查询，不持有窗口）
    d_func()->mContext = std::make_unique< DATablePageContext >();
    d_func()->mContext->setModel(d_func()->mModel);
    d_func()->mContext->setMainView(d_func()->ui->tableView);
    d_func()->mContext->setPaneContainer(d_func()->mPaneContainer);
    d_func()->mContext->setMetadata(d_func()->mMetadata);
    d_func()->mContext->setHighlightOverlay(d_func()->mHighlightOverlay);
    d_func()->mContext->setHostWidget(this);
    d_func()->mContext->setData(d);
    d_func()->mSelection = std::make_unique< DATableSelectionService >(*d_func()->mContext);
    // 宿主桥注入（008 期2）：协作者经上下文的 std::function 触达页面级动作，
    // 不持窗口指针（notifyHostValueChange/closeFloatingWindows 同款设计见计划 §6.1）
    d_func()->mContext->notifyHostValueChange = [ this ]() {
        if (DADataManager* mgr = d_func()->mData.getDataManager()) {
            mgr->notifyDataChangedSignal(d_func()->mData, DADataManager::ChangeValue);
        }
    };
    d_func()->mContext->notifyHostColumnRename = [ this ]() {
        if (DADataManager* mgr = d_func()->mData.getDataManager()) {
            mgr->notifyDataChangedSignal(d_func()->mData, DADataManager::ChangeDataframeColumnName);
        }
    };
    d_func()->mContext->closeFloatingWindows = [ this ]() { closeQuickPlot(); };
    // 撤销栈推入桥（push 即激活本页撤销栈；控制器经此入栈，不持窗口指针）
    d_func()->mContext->pushCommand = [ this ](QUndoCommand* cmd) { push(cmd); };
    // 定位桥（007 定位单入口：隐藏列自动取消隐藏 + 冻结区处置；select=false 只摆位
    // 不收缩选区；报告窗等控制器用）
    d_func()->mContext->locateActualCell = [ this ](int row, int col, bool select) { locateActualCell(row, col, select); };
    // 脏标桥（计算列重算等改数据的操作经此发 dataModified，与页面信号同源）
    d_func()->mContext->notifyDataModified = [ this ]() { Q_EMIT dataModified(); };
    // 拾取模式查询桥（图表「选择序列」会话期间排序入口不生效）
    d_func()->mContext->isHeaderPickMode = [ this ]() -> bool { return d_func()->mHeaderPickMode; };
    // 编辑后效协调器（008 期2）：commit() 是唯一的「编辑后」扇出点，
    // 此前 ~20 处手抄的 模型刷新→统计失效→计算列标脏→浮窗/宿主 收口于此；
    // attach() 一次性订阅批注/高亮层/样式/条件格式/校验缓存的「变更→失效/刷新」
    // 连接（008 期2 Observer 收口，此前散在本构造里 ~160 行）
    d_func()->mMutation = new DATableMutationCoordinator(*d_func()->mContext, this);
    d_func()->mMutation->attach();
    // 特性控制器（008 期3）：批注（构造注入上下文与选区服务，QObject 父子持有）
    d_func()->mNoteController = new DATableNoteController(*d_func()->mContext, *d_func()->mSelection, this);
    // 快速绘图（构造注入上下文与选区服务；数据变更经本页 dataModified 信号转发）
    d_func()->mQuickPlotController = new DATableQuickPlotController(*d_func()->mContext, *d_func()->mSelection, this);
    connect(this, &DADataOperateOfDataFrameWidget::dataModified, d_func()->mQuickPlotController, &DATableQuickPlotController::notifyDataChanged);
    // 数据校验（构造注入上下文；报告窗定位经上下文定位桥回本页单入口）
    d_func()->mValidationController = new DATableValidationController(*d_func()->mContext, this);
    // 计算列（构造注入上下文；定义/状态变更→表头重绘的连接在控制器构造内订阅）
    d_func()->mComputedColumnController = new DATableComputedColumnController(*d_func()->mContext, this);
    // 条件格式（构造注入上下文与选区服务；WPS 式菜单/规则对话框/差异命令）
    d_func()->mConditionalFormatController = new DATableConditionalFormatController(*d_func()->mContext, *d_func()->mSelection, this);
    // 查找/替换（构造注入上下文/选区服务/编辑后效协调器；替换生效经 replaceApplied 转发）
    d_func()->mFindController = new DATableFindController(*d_func()->mContext, *d_func()->mSelection, d_func()->mMutation, this);
    connect(d_func()->mFindController, &DATableFindController::replaceApplied, this, &DADataOperateOfDataFrameWidget::replaceApplied);
    // 样式/显示格式（构造注入上下文与选区服务）
    d_func()->mStyleController = new DATableStyleController(*d_func()->mContext, *d_func()->mSelection, this);
    // 排序（构造注入上下文；插删列/换数据集后规格清理由控制器 clearSpec 提供）
    d_func()->mSortController = new DATableSortController(*d_func()->mContext, this);
    // 视图状态（构造注入上下文与选区服务；列宽采集/投影/冻结集合查表）
    d_func()->mViewStateController = new DATableViewStateController(*d_func()->mContext, *d_func()->mSelection, this);
    // 列视图（构造注入上下文/选区服务 + 视图状态/计算列/校验控制器；列视图真源操作）
    d_func()->mColumnViewController = new DATableColumnViewController(*d_func()->mContext, *d_func()->mSelection, d_func()->mViewStateController,
                                                            d_func()->mComputedColumnController, d_func()->mValidationController, this);
    // 数据操作算子（008 期4：一个操作一个文件；参数化入口由 Facade 直调具体算子）
    d_func()->mRowOps = new DATableRowOps(*d_func()->mContext, *d_func()->mSelection, d_func()->mMutation, this);
    d_func()->mCellOps = new DATableCellOps(*d_func()->mContext, *d_func()->mSelection, d_func()->mMutation, this);
    d_func()->mTypeOps = new DATableTypeOps(*d_func()->mContext, *d_func()->mSelection, d_func()->mMutation, this);
    connect(d_func()->mTypeOps, &DATableTypeOps::selectTypeChanged, this, &DADataOperateOfDataFrameWidget::selectTypeChanged);
    d_func()->mFillOps = new DATableFillOps(*d_func()->mContext, *d_func()->mSelection, d_func()->mMutation, this);
    // 结果落库（新数据集注册/就地替换；三类信号转发回页面同名信号 → APP 连接点零改动）
    d_func()->mResultOps = new DATableResultOps(*d_func()->mContext, this);
    connect(d_func()->mResultOps, &DATableResultOps::newDataCreated, this, &DADataOperateOfDataFrameWidget::newDataCreated);
    connect(d_func()->mResultOps, &DATableResultOps::inplaceDataReplaceRequested, this, &DADataOperateOfDataFrameWidget::inplaceDataReplaceRequested);
    connect(d_func()->mResultOps, &DATableResultOps::dataModified, this, &DADataOperateOfDataFrameWidget::dataModified);
    // 就地替换降级路径的整页刷新桥（清排序规格 + 模型 reset）
    d_func()->mContext->refreshAfterInplaceReplace = [ this ]() { refreshAfterInplaceReplace(); };
    // 形状变换（分组聚合/转置/melt；落库经结果算子）
    d_func()->mReshapeOps = new DATableReshapeOps(*d_func()->mContext, d_func()->mResultOps, this);
    // 分列/合并列（列级撤销记录 + 计算列联动失效）
    d_func()->mColumnCombineOps = new DATableColumnCombineOps(*d_func()->mContext, *d_func()->mSelection, d_func()->mMutation, this);
    // 列改名（影响面确认/引用迁移经计算列+校验控制器，Mediator-light）
    d_func()->mRenameOps = new DATableRenameOps(*d_func()->mContext, d_func()->mViewStateController, d_func()->mComputedColumnController, d_func()->mValidationController, this);
    // 列增删（插列对话框/删列视图状态快照重映射；删列后重投影经视图状态控制器）
    d_func()->mColumnOps = new DATableColumnOps(*d_func()->mContext, *d_func()->mSelection, d_func()->mMutation, d_func()->mViewStateController, this);
    // 算子注册表（008 期4：id → 唯一实例；Agent/插件的 id 驱动入口与菜单启用规则共用）
    d_func()->mOpRegistry.add(d_func()->mRowOps);
    d_func()->mOpRegistry.add(d_func()->mCellOps);
    d_func()->mOpRegistry.add(d_func()->mTypeOps);
    d_func()->mOpRegistry.add(d_func()->mFillOps);
    d_func()->mOpRegistry.add(d_func()->mResultOps);
    d_func()->mOpRegistry.add(d_func()->mReshapeOps);
    d_func()->mOpRegistry.add(d_func()->mColumnCombineOps);
    d_func()->mOpRegistry.add(d_func()->mRenameOps);
    d_func()->mOpRegistry.add(d_func()->mColumnOps);
    // 窗格装配器（008 期5：装配段收口；表意信号连接留在页面——消费方是页面自身的槽）
    d_func()->mPaneAssembler = new DATablePaneAssembler(*d_func()->mContext, d_func()->mViewStateController, d_func()->mStyleDelegate);
    // 导航服务（008 期5：定位对话框/请求解析/定位执行/点击分发）
    d_func()->mNavigation = new DATableNavigationService(*d_func()->mContext, *d_func()->mSelection, d_func()->mViewStateController, d_func()->mColumnViewController, this);
    connect(d_func()->mNavigation, &DATableNavigationService::selectTypeChanged, this, &DADataOperateOfDataFrameWidget::selectTypeChanged);
    connect(d_func()->mNavigation, &DATableNavigationService::columnHeaderClicked, this, &DADataOperateOfDataFrameWidget::columnHeaderClicked);
    // 窗格装配（006b 抽取的唯一装配点）：表头替换 → setModel → 委托 → 逐窗格连接；
    // 必须在 mStyleDelegate 创建之后（里面要用它），在 setDAData/applyViewStateFromStore 之前
    configurePaneView(d_func()->ui->tableView);
    // 模型读取列显示格式（actualData 的 DisplayRole 分支）
    d_func()->mModel->setStyleManager(d_func()->mStyleManager);
    // 计算列子存储注入模型（表头浅蓝底色 BackgroundRole + 公式 tooltip ToolTipRole，004 T1.23）
    d_func()->mModel->setComputedColumnStore(d_func()->mComputedColumnStore);
    // 视图状态（列宽按列名 + 行头宽，R6）：
    // 结构变化（模型重置/插删列）后按列名重新应用，保证列宽不随位置错位；
    // 采集只认用户拖拽（sectionResized + 左键按住在表头上），程序化改动不写 store
    //（期3 ViewStateController 迁移候选，暂留 Facade）
    connect(d_func()->mModel, &QAbstractItemModel::modelReset, this, [ this ]() {
        applyViewStateFromStore();
    });
    connect(d_func()->mModel, &QAbstractItemModel::columnsInserted, this, [ this ]() {
        applyViewStateFromStore();
        // 插列后排序规格里的列位置失效，清除规格（数据顺序不变，只清指示器）
        d_func()->mSortController->clearSpec();
    });
    connect(d_func()->mModel, &QAbstractItemModel::columnsRemoved, this, [ this ]() {
        applyViewStateFromStore();
        // 删列后排序规格里的列位置失效，清除规格
        d_func()->mSortController->clearSpec();
    });
    // 关闭不必要的绘制特性
    setDAData(d);
    // 构造完成后应用一次视图状态（工程加载的宽度回填在注册表里，此时已可借用）
    applyViewStateFromStore();
    // 选中变化时反向同步 ribbon 控件（只在主窗格连：006e 冻结窗格共享同一 selection model）
    if (auto sm = d_func()->ui->tableView->selectionModel()) {
        connect(sm, &QItemSelectionModel::selectionChanged, this, [ this ]() {
            DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.selectionChanged", [this]() {
                return QStringLiteral("rows=%1 cols=%2").arg(dataRowCount()).arg(dataColumnCount());
            });
            Q_EMIT currentStyleChanged(getCurrentCellStyle());
            Q_EMIT currentDisplayFormatChanged(getCurrentColumnDisplayFormat());
            // 查找条处于「当前选区」范围时，选区变化经去抖触发重扫（范围基准随选区更新；
            // 实现迁入 DATableFindController，008 期3）
            d_func()->mFindController->restartDebounceIfVisible();
            Q_EMIT tableSelectionChanged();  // 007：状态栏统计触发源
        });
    }
}

DADataOperateOfDataFrameWidget::~DADataOperateOfDataFrameWidget()
{
    // 条件格式子存储是会话级（借自注册表，比 widget 长寿）：清空统计提供者，
    // 避免持有已销毁页面的回调（QPointer 已防悬挂，此处彻底释放 lambda 捕获）
    if (d_func()->mConditionalFormatStore) {
        d_func()->mConditionalFormatStore->setStatsProvider(nullptr);
        d_func()->mConditionalFormatStore->setDuplicatesProvider(nullptr);
    }
    // 子窗口容器先断回调（008 期0 基线修复，随快速绘图迁入控制器）：容器的
    // boundsProvider 回调读 ui->tableView，delete ui 之后页面仍在析构（~QWidget 阶段），
    // 被监听部件（主视口等）的 Hide 事件会触发 eventFilter → clampAllWindows → 回调 →
    // 读已释放的 ui（use-after-free，DATableQuickPlotTest 在页面析构路径偶现 0xC0000005 崩溃）。
    // dev 0c87de48 同款修复：事件过滤与允许区域提供者也一并解除（detachFromHost）
    if (d_func()->mQuickPlotController) {
        d_func()->mQuickPlotController->detach();
    }
    // 窗格装配器（008 期5）：表头接线引用本页成员，先于 delete ui 释放
    delete d_func()->mPaneAssembler;
    d_func()->mPaneAssembler = nullptr;
    delete d_func()->ui;
    d_func()->ui = nullptr;
}

/**
 * @brief 窗格装配唯一路径（006b）
 *
 * 表头替换 → setModel → 委托 → 批注 store 注入 → 水平表头（计算列 store + 表意信号
 * + 列宽采集按触发窗格）→ 垂直表头（行头宽拖拽）→ 视图表意入口连接。
 * 主窗格与 006e 懒建的冻结窗格都走这里；漏一处 = 该窗格某操作没反应。
 * 表头替换必须在 setModel 之前（QTableView::setModel 会把模型接到"当前"表头上）。
 * @param pane 目标窗格
 */
void DADataOperateOfDataFrameWidget::configurePaneView(DADataTableView* pane)
{
    if (!pane) {
        return;
    }
    // 1)~5) 装配段（表头替换 → setModel/selectionModel 共享 → 委托 → 批注/计算列
    // store 注入 → 表头接线到视图状态控制器/窗格容器/store）——收口到装配器，008 期5
    d_func()->mPaneAssembler->assemble(pane);
    // 6) 视图表意入口（快捷键/右键菜单共用）：每装一个窗格连一份（消费方是页面自身的槽）
    connect(pane, &QTableView::clicked, this, &DADataOperateOfDataFrameWidget::onTableViewClicked);
    // 表头表意入口（页面自身的槽）：带修饰键点击 + 「列」子菜单动作（006d）
    if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(pane->horizontalHeader())) {
        connect(hh, &DADataHeaderView::clickedWithModifiers, this, &DADataOperateOfDataFrameWidget::onHeaderClickedWithModifiers);
        connect(hh, &DADataHeaderView::columnViewActionRequested,
                this, &DADataOperateOfDataFrameWidget::onHeaderColumnViewActionRequested);
        // 快速绘图右键子菜单：按「右键列是否在选区」实时规划可选图型（v3）
        hh->setQuickPlotPlanProvider([ this ](int logicalIndex) { return quickPlotPlanForColumn(logicalIndex); });
    }
    // 查找/替换入口（002 T1.4）：视图快捷键 Ctrl+F/Ctrl+H 与表体右键菜单共用表意信号；
    // 右键菜单入口带 prefill=true（查找框预填当前单格内容，D9）
    connect(pane, &DADataTableView::findRequested, this, [ this ](bool prefill) {
        showFindReplace(false, prefill);
    });
    connect(pane, &DADataTableView::replaceRequested, this, [ this ](bool prefill) {
        showFindReplace(true, prefill);
    });
    // 定位入口（007 T1.7）：视图只发表意信号、页面弹对话框；滚动条右键经
    // promptGotoRow 虚派发走同一条链（每装一个窗格连一份，冻结窗格 Ctrl+G 同样可用）
    connect(pane, &DADataTableView::gotoRequested, this, &DADataOperateOfDataFrameWidget::showGotoDialog);
    connect(pane, &DADataTableView::findBarCloseRequested, this, &DADataOperateOfDataFrameWidget::closeFindReplace);
    // 条件格式入口（003 T1.5）：表体右键菜单与 Ribbon Table Style 面板共用同一打开逻辑
    connect(pane, &DADataTableView::conditionalFormatRequested, this, [ this ]() {
        showConditionalFormatDialog();
    });
    // 右键条件格式子菜单：页面填充 WPS 式预设（aboutToShow 每次重填，保证作用列/可用性实时）
    connect(pane, &DADataTableView::conditionalFormatMenuAboutToShow, this,
            &DADataOperateOfDataFrameWidget::populateConditionalFormatMenu);
    // 计算列入口（004 T1.23）：表体右键菜单与 Ribbon「Data Tools」面板共用同一打开逻辑
    connect(pane, &DADataTableView::computedColumnRequested, this, [ this ]() {
        showComputedColumnDialog(-1);
    });
    // 分列/合并列入口（004 T1.9）：表体右键菜单与 Ribbon「Data Tools」面板共用同一打开逻辑
    connect(pane, &DADataTableView::splitColumnRequested, this, [ this ]() {
        showSplitColumnDialog(-1);
    });
    connect(pane, &DADataTableView::mergeColumnsRequested, this, &DADataOperateOfDataFrameWidget::showMergeColumnsDialog);
    // 分组聚合入口（004 T1.22）：表体右键菜单与 Ribbon「Data Tools」面板共用同一打开逻辑
    connect(pane, &DADataTableView::groupByRequested, this, &DADataOperateOfDataFrameWidget::showGroupByDialog);
    // 转置/宽转长入口（004 T1.27）：表体右键菜单与 Ribbon「Data Tools」面板共用同一打开逻辑
    connect(pane, &DADataTableView::transposeRequested, this, [ this ]() {
        showReshapeDialog(false);
    });
    connect(pane, &DADataTableView::meltRequested, this, [ this ]() {
        showReshapeDialog(true);
    });
    // 填充入口（005 T1.24）：Ctrl+D 与表体右键「填充」子菜单共用表意信号
    connect(pane, &DADataTableView::fillDownRequested, this, [ this ]() {
        fillSelection(FillMode::Down);
    });
    connect(pane, &DADataTableView::fillUpRequested, this, [ this ]() {
        fillSelection(FillMode::Up);
    });
    connect(pane, &DADataTableView::fillForwardRequested, this, [ this ]() {
        fillSelection(FillMode::Forward);
    });
    connect(pane, &DADataTableView::fillBackwardRequested, this, [ this ]() {
        fillSelection(FillMode::Backward);
    });
    connect(pane, &DADataTableView::fillSeriesRequested, this, &DADataOperateOfDataFrameWidget::showFillSeriesDialog);
    // 数据校验入口（005 T1.25）：表体右键菜单与 Ribbon「Data Tools」面板共用同一逻辑
    connect(pane, &DADataTableView::validationRulesRequested, this, &DADataOperateOfDataFrameWidget::showValidationRulesDialog);
    connect(pane, &DADataTableView::runValidationRequested, this, &DADataOperateOfDataFrameWidget::runValidationAndShowReport);
    // 批注入口（005 T1.26）：插入/编辑共用弹层（按有无批注自适应），删除/清除各自命令
    connect(pane, &DADataTableView::noteInsertRequested, this, &DADataOperateOfDataFrameWidget::showNoteEditorForCurrentCell);
    connect(pane, &DADataTableView::noteEditRequested, this, &DADataOperateOfDataFrameWidget::showNoteEditorForCurrentCell);
    connect(pane, &DADataTableView::noteDeleteRequested, this, &DADataOperateOfDataFrameWidget::deleteNoteAtCurrentCell);
    connect(pane, &DADataTableView::noteClearAllRequested, this, &DADataOperateOfDataFrameWidget::clearAllNotes);
    // 单元格大小入口（A3）：表体右键「单元格大小」子菜单 → 页面命令（不进撤销栈）
    connect(pane, &DADataTableView::columnWidthAutoFitRequested, this, [ this ]() {
        autoFitSelectedColumnWidths();
    });
    connect(pane, &DADataTableView::columnWidthPromptRequested, this, &DADataOperateOfDataFrameWidget::showSetColumnWidthDialog);
    connect(pane, &DADataTableView::rowHeightPromptRequested, this, &DADataOperateOfDataFrameWidget::showSetRowHeightDialog);
}

/**
 * @brief 是否存在data
 * @return
 */
bool DADataOperateOfDataFrameWidget::haveData() const
{
    // dataframe与通用表格数据（如数据库惰性表）都视为有数据；series维持原语义不开数据页
    return d_func()->mData.isDataFrame() || (d_func()->mData.isTable() && !d_func()->mData.isSeries());
}

/**
 * @brief 获取dataframe
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::getDataframe() const
{
    return d_func()->mData.toDataFrame();
}

/**
 * @brief 获取Data的引用
 * @return
 */
const DAData& DADataOperateOfDataFrameWidget::data() const
{
    return d_func()->mData;
}

void DADataOperateOfDataFrameWidget::setDAData(const DA::DAData& d)
{
    // 切换数据集后浮窗绑定的列身份与数据快照都失效，直接关闭（实施方案 §5.3）
    closeQuickPlot();
    d_func()->mData = d;
    d_func()->mContext->setData(d);  // 上下文随宿主换数据（008 期1）
    // 数据替换后旧排序规格（列位置+方向）失去意义，清除（排序规格不持久化，R13）
    d_func()->mSortController->clearSpec();
    // 通用表格数据（DATableDataSource，如数据库惰性表）也可显示；series维持原语义
    if (d.isDataFrame() || (d.isTable() && !d.isSeries())) {
        d_func()->ui->tableView->setData(d);
    }
}

void DADataOperateOfDataFrameWidget::insertRowAboveBySelect()
{
    int r = getSelectedOneDataframeRow();
    if (r < 0) {
        return;
    }
    insertRowAt(r);
}

void DADataOperateOfDataFrameWidget::insertRowBelowBySelect()
{
    int r = getSelectedOneDataframeRow();
    if (r < 0) {
        return;
    }
    insertRowAt(r + 1);
}

/**
 * @brief 在所选位置插入行（转发行算子）
 * @param row
 */
void DADataOperateOfDataFrameWidget::insertRowAt(int row)
{
    d_func()->mRowOps->insertRowAt(row);
}

// ============================================================
// 列增删（实现迁入 DATableColumnOps，008 期4）
// ============================================================
/**
 * @brief 选中列右边插入新列（转发列算子）
 */
void DADataOperateOfDataFrameWidget::insertColumnRightBySelect()
{
    d_func()->mColumnOps->insertColumnRightBySelect();
}

/**
 * @brief 选中列左边插入新列（转发列算子）
 */
void DADataOperateOfDataFrameWidget::insertColumnLeftBySelect()
{
    d_func()->mColumnOps->insertColumnLeftBySelect();
}

/**
 * @brief 在col位置插入列（转发列算子）
 * @param col
 */
void DADataOperateOfDataFrameWidget::insertColumnAt(int col)
{
    d_func()->mColumnOps->insertColumnAt(col);
}

/**
 * @brief 移除选中的列（转发列算子）
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectColumn()
{
    return d_func()->mColumnOps->removeSelectColumn();
}

/**
 * @brief 移除指定列（表头右键「删除列」直接入口；转发列算子）
 * @param col 列位置索引
 * @return 成功移除的列数（0 = 失败/列号无效）
 */
int DADataOperateOfDataFrameWidget::removeColumnAt(int col)
{
    return d_func()->mColumnOps->removeColumnAt(col);
}

/**
 * @brief 按列号列表删除列（转发列算子）
 * @param columns 列位置索引列表
 * @return 成功移除的列数
 */
int DADataOperateOfDataFrameWidget::removeColumns(const QList< int >& columns)
{
    return d_func()->mColumnOps->removeColumns(columns);
}

/**
 * @brief 移除选中的行（转发行算子）
 * @return 返回成功移除的行数
 */
int DADataOperateOfDataFrameWidget::removeSelectRow()
{
    return d_func()->mRowOps->removeSelectedRows();
}

/**
 * @brief 设置选中单元格为nan,返回设置成功的个数（转发单元格算子）
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectCell()
{
    return d_func()->mCellOps->removeSelectedCells();
}

/**
 * @brief 粘贴剪贴板内容到当前选区左上角（转发单元格算子）
 * @return 成功写入的单元格数（0 表示未写入）
 */
int DADataOperateOfDataFrameWidget::pasteFromClipboard()
{
    return d_func()->mCellOps->pasteFromClipboard();
}

/**
 * @brief 剪切选中区（转发单元格算子）
 * @return 受影响单元格数
 */
int DADataOperateOfDataFrameWidget::cutSelection()
{
    return d_func()->mCellOps->cutSelection();
}

/**
 * @brief 重命名单列（转发列改名算子）
 * @param col 列位置索引
 * @param newName 新列名
 * @return 成功返回 true；列为空/重名/越界/exec 失败返回 false
 */
bool DADataOperateOfDataFrameWidget::renameColumn(int col, const QString& newName)
{
    return d_func()->mRenameOps->renameColumn(col, newName);
}

/**
 * @brief 设置选择列的数据类型（转发类型算子）
 * @param dtypeName
 * @return 成功改变类型返回true
 */
bool DADataOperateOfDataFrameWidget::changeSelectColumnType(const DAPyDType& dt)
{
    return d_func()->mTypeOps->changeSelectColumnType(dt);
}

/**
 * @brief 把选择的列转换为数值（转发类型算子）
 */
void DADataOperateOfDataFrameWidget::castSelectToNum()
{
    d_func()->mTypeOps->castSelectToNum();
}

/**
 * @brief 把选择的列转换为日期（转发类型算子）
 */
void DADataOperateOfDataFrameWidget::castSelectToDatetime()
{
    d_func()->mTypeOps->castSelectToDatetime();
}

/**
 * @brief 把选择的列转换为索引（转发类型算子）
 * @return
 */
bool DADataOperateOfDataFrameWidget::changeSelectColumnToIndex()
{
    return d_func()->mTypeOps->changeSelectColumnToIndex();
}

/**
 * @brief 创建一个数据描述
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::createDataDescribe()
{
    if (!d_func()->mData.isDataFrame()) {
        return DAPyDataFrame();
    }
    DAPyDataFrame df_describe = d_func()->mData.toDataFrame().describe();
    return df_describe;
}

/**
 * @brief dataframe表格是否有选中项
 * @return
 */
bool DADataOperateOfDataFrameWidget::isDataframeTableHaveSelection() const
{
    return d_func()->mSelection->haveSelection();
}

/**
 * @brief 获取dataframe表格的tableview
 *
 * @return DADataTableView*
 */
DADataTableView* DADataOperateOfDataFrameWidget::getDataTableView() const
{
    return d_func()->ui ? d_func()->ui->tableView : nullptr;
}

/**
 * @brief 返回当前选中单元格所包含的列数，列数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeCoumns(bool ensureInDataframe) const
{
    return d_func()->mSelection->selectedDataframeColumns(ensureInDataframe);
}

/**
 * @brief 返回当前选中单元格所包含的行数，行数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeRows(bool ensureInDataframe) const
{
    return d_func()->mSelection->selectedDataframeRows(ensureInDataframe);
}

/**
 * @brief 视图行号转换为数据源绝对行号
 *
 * 表格视图带滑动窗，选中项的行号是窗口内行号；凡是拿行号去读写 dataframe、
 * 样式表、undo 命令的地方都必须先经过这里，否则滚动之后就会操作到别的行。
 * @param viewRow 视图行号（滑动窗内逻辑行）
 * @return 数据源绝对行号
 */
int DADataOperateOfDataFrameWidget::toActualRow(int viewRow) const
{
    return d_func()->mContext->toActualRow(viewRow);
}

/**
 * @brief 数据源真实数据行数（不含尾部预留的空白行）
 * @return 行数
 */
int DADataOperateOfDataFrameWidget::dataRowCount() const
{
    return d_func()->mContext->dataRowCount();
}

/**
 * @brief 返回表格中完全选中的一整列的列数
 *
 * 按选区区间做「逐列行区间合并」判定（全选卡顿修复：不用
 * QItemSelectionModel::selectedColumns——它对每列逐行判定，复杂度 O(列×行)，
 * 200 列×2 万行实测 55ms，宽表下线性放大到秒级；本实现是 O(段数×选中列数)）。
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getFullySelectedDataframeColumns(bool ensureInDataframe) const
{
    return d_func()->mSelection->fullySelectedDataframeColumns(ensureInDataframe);
}

/**
 * @brief 返回表格中完全选中的一整行的行数
 *
 * 同 getFullySelectedDataframeColumns：按选区区间做「逐行列区间合并」判定，
 * 不用 QItemSelectionModel::selectedRows（它对每行逐列判定，复杂度 O(行×列)）。
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getFullySelectedDataframeRows(bool ensureInDataframe) const
{
    return d_func()->mSelection->fullySelectedDataframeRows(ensureInDataframe);
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeRow(bool ensureInDataframe) const
{
    return d_func()->mSelection->oneSelectedDataframeRow(ensureInDataframe);
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeColumn(bool ensureInDataframe) const
{
    return d_func()->mSelection->oneSelectedDataframeColumn(ensureInDataframe);
}

/**
 * @brief 获取当前表格操作选中的数据
 *
 * 如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，
 * 如果用户选中了多列，那么每列作为一个DAData，最后组成一个QList<DAData>返回,如果用户打开了表格，但没选择任何列，这个函数返回一个空list
 *
 * 如果用户没有选择列，但选中了单元格，那么相当于选中了单元格对应的列
 *
 * 如果什么都没选中，那么返回一个空的list
 *
 * @return
 */
QList< DAData > DADataOperateOfDataFrameWidget::getSlectedSeries() const
{
    return d_func()->mSelection->selectedSeries();
}

void DADataOperateOfDataFrameWidget::refreshTable()
{
    if (d_func()->mModel) {
        d_func()->mModel->refreshData();
    }
}

/**
 * @brief 确保整列可见并滚动到位（转发导航服务；隐藏列先取消隐藏+提示）
 * @param colName 列名
 * @param selectCol true = 整列选中
 */
void DADataOperateOfDataFrameWidget::ensureColumnVisible(const QString& colName, bool selectCol)
{
    d_func()->mNavigation->ensureColumnVisible(colName, selectCol);
}
/**
 * @brief 定位单入口（转发导航服务；007 语义：数据行 + 列 + 是否选中 + 是否保持隐藏）
 */
void DADataOperateOfDataFrameWidget::locateActualCell(int actualRow, int col, bool select, bool keepHidden)
{
    d_func()->mNavigation->locateActualCell(actualRow, col, select, keepHidden);
}
/**
 * @brief 定位单入口的列名重载（转发导航服务；重名取第一个同名列，数据列序）
 */
void DADataOperateOfDataFrameWidget::locateActualCell(int actualRow, const QString& columnName, bool select,
                                                      bool keepHidden)
{
    d_func()->mNavigation->locateActualCell(actualRow, columnName, select, keepHidden);
}
QList< QPoint > DADataOperateOfDataFrameWidget::getSelectedDataframeCells(bool ensureInDataframe) const
{
    return d_func()->mSelection->selectedDataframeCells(ensureInDataframe);
}

/**
 * @brief 数据源真实数据列数（不含尾部预留的空白列）
 * @return 列数
 */
int DADataOperateOfDataFrameWidget::dataColumnCount() const
{
    return d_func()->mContext->dataColumnCount();
}

/**
 * @brief 实际数据列的身份表
 *
 * 数据列序升序收集 (列名, occurrence)；尾部预留的空白列（列名为空）跳过。
 * occurrence 只在数据列区间内计（occurrence = 该列之前同名列的个数），
 * 与 applyViewStateFromStore/onHeaderSectionResized 的口径一致（空白列列名
 * 为空、两处都 continue，结果等价）。
 * @return 身份表（006 列视图键的统一口径）
 */
QList< QPair< QString, int > > DADataOperateOfDataFrameWidget::actualColumnIdentities() const
{
    return d_func()->mSelection->actualColumnIdentities();
}

/**
 * @brief 表格点击（转发导航服务：类型上报）
 * @param index
 */
void DADataOperateOfDataFrameWidget::onTableViewClicked(const QModelIndex& index)
{
    d_func()->mNavigation->onTableViewClicked(index);
}
/**
 * @brief 获取样式管理器
 * @return 样式管理器指针
 */
DATableStyleManager* DADataOperateOfDataFrameWidget::styleManager() const
{
    return d_func()->mStyleManager;
}

/**
 * @brief 获取视图状态子存储
 * @return 视图状态子存储指针（借自元数据束，registry 为空时为 nullptr）
 */
DATableViewStateStore* DADataOperateOfDataFrameWidget::viewStateStore() const
{
    return d_func()->mViewStateStore;
}

/**
 * @brief 获取瞬态高亮层
 * @return 高亮层指针（页级持有，非空）
 */
DATableHighlightOverlay* DADataOperateOfDataFrameWidget::highlightOverlay() const
{
    return d_func()->mHighlightOverlay;
}

/**
 * @brief 获取条件格式子存储
 * @return 条件格式子存储指针（借自元数据束，registry 为空时为 nullptr）
 */
DATableConditionalFormatStore* DADataOperateOfDataFrameWidget::conditionalFormatStore() const
{
    return d_func()->mConditionalFormatStore;
}

/**
 * @brief 选择模型（选区快照 API，007 状态栏统计）
 *
 * 只取主窗格（冻结窗格共享同一个 selectionModel）。
 * @return 选择模型指针（无视图时 nullptr）
 */
QItemSelectionModel* DADataOperateOfDataFrameWidget::tableSelectionModel() const
{
    return d_func()->mSelection->selectionModel();
}

/**
 * @brief 选区段数
 *
 * 选区按连续段组织（Ctrl+多点形成多段）；0=无选区，>1=多段（状态栏只给几何计数）。
 * @return 段数
 */
int DADataOperateOfDataFrameWidget::selectionSegmentCount() const
{
    return d_func()->mSelection->selectionSegmentCount();
}

/**
 * @brief 选区包围盒（闭区间，数据源绝对行号）
 *
 * 走 selection() 段列表（不要 selectedIndexes——会物化）。列维度没有滑动窗，
 * column() 即数据列序号。
 * @return 无选区返回 false
 */
bool DADataOperateOfDataFrameWidget::selectionBoundingBox(int* rowBegin, int* rowEnd, int* colBegin, int* colEnd) const
{
    return d_func()->mSelection->selectionBoundingBox(rowBegin, rowEnd, colBegin, colEnd);
}

/**
 * @brief 选中单元数（几何去重）
 *
 * 按选区区间累加宽×高（selection() 的区间互不重叠，无重复计数；
 * 全选卡顿修复：不再物化 selectedIndexes）。
 * @return 单元数
 */
int DADataOperateOfDataFrameWidget::selectedCellCount() const
{
    return d_func()->mSelection->selectedCellCount();
}

/**
 * @brief 从视图状态子存储投影到窗格（转发视图状态控制器；唯一投影路径）
 */
void DADataOperateOfDataFrameWidget::applyViewStateFromStore()
{
    d_func()->mViewStateController->applyFromStore();
}

// ============================================================
// 单元格大小（A3）：列宽自适应 / 设置列宽 / 设置行高
// 三者都直接改视图状态并重投影，不进撤销栈（与拖拽列宽/行高一致）
// ============================================================

namespace
{
// 设置列宽允许范围（像素）：下限保证可读，上限防误操作把列拉爆
constexpr int c_minColumnWidth = 20;
constexpr int c_maxColumnWidth = 2000;
// 设置行高允许范围（像素）：下限放宽到 4px，允许「紧凑」级别的手动设置
constexpr int c_minRowHeight = 4;
constexpr int c_maxRowHeight = 500;
}  // namespace

/**
 * @brief 选区覆盖的列（整列选中优先，否则取选中单元格涉及列）
 * @param sel 选区服务
 * @return 逻辑列号列表（可空）
 */
static QList< int > selectedColumnsForCellSize(const DATableSelectionService& sel)
{
    QList< int > cols = sel.fullySelectedDataframeColumns(false);
    if (cols.isEmpty()) {
        cols = sel.selectedDataframeColumns(false);
    }
    return cols;
}

/**
 * @brief 列宽自适应：选中列按内容宽度落库并重投影（不进撤销栈）
 * @return 至少改了一列返回 true；无选中列返回 false
 */
bool DADataOperateOfDataFrameWidget::autoFitSelectedColumnWidths()
{
    const QList< int > cols = selectedColumnsForCellSize(*d_func()->mSelection);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return false;
    }
    DATableViewStateController* vc = d_func()->mViewStateController;
    DADataTableView* pane          = getDataTableView();
    bool changed                   = false;
    for (int col : std::as_const(cols)) {
        const int w = vc->contentFittedColumnWidth(pane, col);
        if (w > 0) {
            vc->storeColumnWidthAt(col, w);
            changed = true;
        }
    }
    if (changed) {
        applyViewStateFromStore();  // 唯一投影路径（含冻结窗格几何重算）
    }
    return changed;
}

/**
 * @brief 设置选中列列宽（像素，钳制后落库并重投影；不进撤销栈）
 * @param width 目标宽度（像素）
 * @return 至少改了一列返回 true；无选中列返回 false
 */
bool DADataOperateOfDataFrameWidget::setSelectedColumnWidth(int width)
{
    const QList< int > cols = selectedColumnsForCellSize(*d_func()->mSelection);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return false;
    }
    const int w = qBound(c_minColumnWidth, width, c_maxColumnWidth);
    for (int col : std::as_const(cols)) {
        d_func()->mViewStateController->storeColumnWidthAt(col, w);
    }
    applyViewStateFromStore();
    return true;
}

/**
 * @brief 设置行高（像素，整表统一；落库 + 套到全部窗格；不进撤销栈）
 *
 * 整表设置 = 写默认行高 + 清除全部逐行覆盖（与 Excel「设置行高」一致：
 * 单行例外被整表设置覆盖掉）。用户显式设定的高度记为「常规行高」，
 * 供「紧凑/普通」切换时恢复。
 * @param height 目标行高（像素，钳制后应用）
 */
void DADataOperateOfDataFrameWidget::setTableRowHeight(int height)
{
    d_func()->mNormalRowHeight = qBound(c_minRowHeight, height, c_maxRowHeight);
    setTableRowHeightNoRemember(height);
}

/**
 * @brief 整表行高落地（不改「常规行高」记忆）
 *
 * 写默认行高 + 清除全部逐行覆盖，然后统一投影到所有窗格并重算冻结区几何。
 * @param height 目标行高（像素，钳制后应用）
 */
void DADataOperateOfDataFrameWidget::setTableRowHeightNoRemember(int height)
{
    const int h = qBound(c_minRowHeight, height, c_maxRowHeight);
    if (d_func()->mViewStateStore) {
        // 屏蔽中间态：默认值与覆盖一起改完后由 applyRowHeightToPanes 统一投影
        QSignalBlocker blocker(d_func()->mViewStateStore);
        d_func()->mViewStateStore->setRowHeight(h);
        d_func()->mViewStateStore->clearRowHeightOverrides();
    }
    d_func()->mViewStateController->applyRowHeightToPanes(h);
    // 行高变化会改变行头与冻结区几何，重算一次
    if (d_func()->mPaneContainer) {
        d_func()->mPaneContainer->updateFrozenGeometry();
    }
}

/**
 * @brief 紧凑行高：行高 = 视图字体高度 + 2px（Ribbon「视图」面板下拉「紧凑」）
 *
 * 首次进入紧凑时把当时的行高记为「常规行高」，切回「普通」时恢复。
 * 与整表设置一样会清除逐行覆盖（紧凑是整表视图状态）。
 */
void DADataOperateOfDataFrameWidget::setCompactRowHeight()
{
    DADataTableView* pane = getDataTableView();
    if (!pane) {
        return;
    }
    if (d_func()->mNormalRowHeight <= 0) {
        d_func()->mNormalRowHeight = currentRowHeight();
    }
    const int h = qBound(c_minRowHeight, pane->fontMetrics().height() + 2, c_maxRowHeight);
    // 不覆盖「常规行高」：普通模式要恢复的是进入紧凑前的高度
    setTableRowHeightNoRemember(h);
}

/**
 * @brief 普通行高：恢复进入紧凑前的常规行高（未记录时回退程序默认行高）
 */
void DADataOperateOfDataFrameWidget::setNormalRowHeight()
{
    int h = d_func()->mNormalRowHeight;
    if (h <= 0) {
        DADataTableView* pane = getDataTableView();
        h = pane ? DADataHeaderView::defaultRowHeightFor(pane->fontMetrics()) : 24;
    }
    d_func()->mNormalRowHeight = h;
    setTableRowHeightNoRemember(h);
}

/**
 * @brief 当前行高（设置行高对话框默认值）
 * @return 行高（像素）；取不到返回默认行高
 */
int DADataOperateOfDataFrameWidget::currentRowHeight() const
{
    if (DADataTableView* pane = const_cast< DADataOperateOfDataFrameWidget* >(this)->getDataTableView()) {
        if (QHeaderView* vh = pane->verticalHeader()) {
            return vh->defaultSectionSize();
        }
    }
    return 24;
}

/**
 * @brief 当前选中列的代表列宽（设置列宽对话框默认值）
 * @return 列宽（像素）；无选中列返回 -1
 */
int DADataOperateOfDataFrameWidget::currentSelectedColumnWidth() const
{
    const QList< int > cols = selectedColumnsForCellSize(*d_func()->mSelection);
    if (cols.isEmpty()) {
        return -1;
    }
    DADataTableView* pane = const_cast< DADataOperateOfDataFrameWidget* >(this)->getDataTableView();
    if (!pane || !pane->horizontalHeader()) {
        return -1;
    }
    return pane->horizontalHeader()->sectionSize(cols.first());
}

/**
 * @brief 右键菜单「设置列宽…」：弹输入框（默认当前宽度），确定后应用
 */
void DADataOperateOfDataFrameWidget::showSetColumnWidthDialog()
{
    const int cur = currentSelectedColumnWidth();
    if (cur < 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    bool ok = false;
    const int w = QInputDialog::getInt(this,
                                       tr("Set Column Width"),  // cn:设置列宽
                                       tr("Width (pixels):"),    // cn:列宽（像素）：
                                       cur, c_minColumnWidth, c_maxColumnWidth, 1, &ok);
    if (ok) {
        setSelectedColumnWidth(w);
    }
}

/**
 * @brief 右键菜单「设置行高…」：弹输入框（默认当前行高），确定后应用到全表
 */
void DADataOperateOfDataFrameWidget::showSetRowHeightDialog()
{
    bool ok = false;
    const int h = QInputDialog::getInt(this,
                                       tr("Set Row Height"),  // cn:设置行高
                                       tr("Height (pixels):"),  // cn:行高（像素）：
                                       currentRowHeight(), c_minRowHeight, c_maxRowHeight, 1, &ok);
    if (ok) {
        setTableRowHeight(h);
    }
}

/**
 * @brief 当前选区为单格时取其显示文本（D9 预填用）
 * @return 单格显示文本（Qt::DisplayRole）；选区非单格/无选区返回空串
 */
QString DADataOperateOfDataFrameWidget::currentSingleCellDisplayText() const
{
    return d_func()->mSelection->currentSingleCellDisplayText();
}

/**
 * @brief 本页所有需要投影的窗格
 *
 * 主窗格在前；冻结窗格懒建后追加（006e）。
 * @return 窗格列表（非空）
 */
QList< DADataTableView* > DADataOperateOfDataFrameWidget::paneViews() const
{
    return d_func()->mContext->panes();
}

/**
 * @brief 遍历全部窗格的水平表头（排序角标/ƒx 角标等按实例刷新用，006e）
 * @param fn 对每个表头执行的回调
 */
void DADataOperateOfDataFrameWidget::forEachPaneHeader(const std::function< void(DADataHeaderView*) >& fn)
{
    for (DADataTableView* pane : paneViews()) {
        if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(pane->horizontalHeader())) {
            fn(hh);
        }
    }
}


// ============================================================
// 列视图（实现迁入 DATableColumnViewController，008 期3）
// ============================================================

/**
 * @brief 目标列是否落在冻结区（转发视图状态控制器，纯查表）
 */
bool DADataOperateOfDataFrameWidget::isColumnInFrozenArea(int logicalColumn) const
{
    return d_func()->mViewStateController->isColumnInFrozenArea(logicalColumn);
}

/**
 * @brief 列轴窗格容器只读访问器（测试与页面内部接线用；APP 不使用、不改其调用点形状）
 */
DATablePaneContainer* DADataOperateOfDataFrameWidget::paneContainer() const
{
    return d_func()->mPaneContainer;
}

/**
 * @brief 列视图直接入口：只写 store + 重投影，不入撤销栈（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::setColumnHiddenDirectly(const QString& name, int occurrence, bool hidden)
{
    d_func()->mColumnViewController->setColumnHiddenDirectly(name, occurrence, hidden);
}

/**
 * @brief 生效冻结列数（转发列视图控制器）
 */
int DADataOperateOfDataFrameWidget::effectiveFrozenColumnCount() const
{
    return d_func()->mColumnViewController->effectiveFrozenColumnCount();
}

/**
 * @brief 隐藏指定列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::hideColumnAt(int logicalColumn)
{
    d_func()->mColumnViewController->hideColumnAt(logicalColumn);
}

/**
 * @brief 显示所有列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::showAllColumns()
{
    d_func()->mColumnViewController->showAllColumns();
}

/**
 * @brief 冻结/取消冻结指定列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::setColumnFrozen(int logicalColumn, bool frozen)
{
    d_func()->mColumnViewController->setColumnFrozen(logicalColumn, frozen);
}

/**
 * @brief 冻结首列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::freezeFirstColumn()
{
    d_func()->mColumnViewController->freezeFirstColumn();
}

/**
 * @brief 取消全部冻结（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::unfreezeAllColumns()
{
    d_func()->mColumnViewController->unfreezeAllColumns();
}

/**
 * @brief 弹「冻结前 N 列」输入框（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::promptFreezeColumns()
{
    d_func()->mColumnViewController->promptFreezeColumns();
}

/**
 * @brief 拖动落库入栈（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::applyColumnOrderFromView(const QVector< int >& newOrderLogical)
{
    d_func()->mColumnViewController->applyColumnOrderFromView(newOrderLogical);
}

/**
 * @brief 冻结区内部拖动落库入栈（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::applyFrozenOrderFromView(const QVector< int >& newFrozenOrderLogical)
{
    d_func()->mColumnViewController->applyFrozenOrderFromView(newFrozenOrderLogical);
}

/**
 * @brief 「列」子菜单动作分发（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::onHeaderColumnViewActionRequested(int logicalColumn,
                                                                        DA::DADataHeaderView::ColumnViewAction action)
{
    d_func()->mColumnViewController->onHeaderActionRequested(logicalColumn, static_cast< int >(action));
}

/**
 * @brief 打开列管理对话框（列名/顺序/隐藏/冻结，转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::showManageColumnsDialog()
{
    d_func()->mColumnViewController->showManageColumnsDialog();
}

/**
 * @brief 解析定位请求并调定位单入口（转发导航服务；007 T1.7）
 * @param req 定位请求
 * @return 是否定位成功
 */
bool DADataOperateOfDataFrameWidget::resolveGotoRequest(const DADialogGotoCell::Request& req)
{
    return d_func()->mNavigation->resolveGotoRequest(req);
}

/**
 * @brief 当前单元格模型索引
 *
 * selectionModel()->currentIndex()，无效时取选区左上角；都没有返回无效索引。
 * @return 模型索引
 */
QModelIndex DADataOperateOfDataFrameWidget::currentOrSelectionTopLeftIndex() const
{
    return d_func()->mSelection->currentOrSelectionTopLeftIndex();
}

/**
 * @brief 打开定位对话框（转发导航服务；007 T1.7）
 */
void DADataOperateOfDataFrameWidget::showGotoDialog()
{
    d_func()->mNavigation->showGotoDialog();
}

/**
 * @brief 采集列宽改动写入视图状态子存储（转发视图状态控制器，008 期3）
 */
void DADataOperateOfDataFrameWidget::onHeaderSectionResized(DADataTableView* source, int logicalIndex, int newSize)
{
    d_func()->mViewStateController->onHeaderSectionResized(source, logicalIndex, newSize);
}

// ============================================================
// 样式与列显示格式（实现迁入 DATableStyleController，008 期3）
// ============================================================

/**
 * @brief 合并样式片段到选中区（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::mergeStyleToSelection(const DATableCellStyle& fragment)
{
    d_func()->mStyleController->mergeStyleToSelection(fragment);
}

/**
 * @brief 外框线：对选中区包围矩形四边应用边框（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::applyBorderOutlineToSelection()
{
    d_func()->mStyleController->applyBorderOutlineToSelection();
}

/**
 * @brief 设置完整样式到选中区（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::applyStyleToSelection(const DATableCellStyle& style)
{
    d_func()->mStyleController->applyStyleToSelection(style);
}

/**
 * @brief 清除选中区样式（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::clearStyleSelection()
{
    d_func()->mStyleController->clearStyleSelection();
}

/**
 * @brief 清除整表所有样式（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::clearStyleAll()
{
    d_func()->mStyleController->clearStyleAll();
}

/**
 * @brief 获取选中区代表的样式（转发样式控制器）
 */
DATableCellStyle DADataOperateOfDataFrameWidget::getCurrentCellStyle() const
{
    return d_func()->mStyleController->getCurrentCellStyle();
}

/**
 * @brief 设置列显示格式到选中列（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::setDisplayFormatToSelection(const DATableDisplayFormat& fmt)
{
    d_func()->mStyleController->setDisplayFormatToSelection(fmt);
}

/**
 * @brief 清除选中列的显示格式（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::clearDisplayFormatFromSelection()
{
    d_func()->mStyleController->clearDisplayFormatFromSelection();
}

/**
 * @brief 获取选中列代表的显示格式（转发样式控制器）
 */
DATableDisplayFormat DADataOperateOfDataFrameWidget::getCurrentColumnDisplayFormat() const
{
    return d_func()->mStyleController->getCurrentColumnDisplayFormat();
}

/**
 * @brief 表头拾取模式（图表「选择序列」会话期间置位）
 *
 * 拾取模式下表头点击只转发 columnHeaderClicked 信号（拾取链路保持不变）、不排序；
 * 由 DADataOperateWidget::setHeaderPickMode 转发（拾取器 connect/disconnectDataWidgets 切换）。
 * @param on true=进入拾取模式
 */
void DADataOperateOfDataFrameWidget::setHeaderPickMode(bool on)
{
    d_func()->mHeaderPickMode = on;
}

/**
 * @brief 是否处于表头拾取模式
 * @return 拾取模式返回 true
 */
bool DADataOperateOfDataFrameWidget::isHeaderPickMode() const
{
    return d_func()->mHeaderPickMode;
}

/**
 * @brief 当前多列排序规格
 * @return 列逻辑索引+方向列表（顺序即优先级；不持久化 R13）
 */
QVector< QPair< int, Qt::SortOrder > > DADataOperateOfDataFrameWidget::sortSpec() const
{
    return d_func()->mSortController->sortSpec();
}

/**
 * @brief 数据源是否可编辑（只读/惰性源上排序入口不生效，D1）
 *
 * 判定路径同 DADataTableModel::actualFlags 的只读源分支：tableSource 非空且
 * isTableEditable() 为假（如数据库惰性表）→ 只读。
 * @return 可编辑返回 true
 */
bool DADataOperateOfDataFrameWidget::isTableEditable() const
{
    return d_func()->mContext->isDataSourceEditable();
}

/**
 * @brief 表头带修饰键点击（转发导航服务；2026-10-18 交互修订：点击表头不排序）
 * @param logicalIndex 列逻辑索引
 * @param modifiers 修饰键
 */
void DADataOperateOfDataFrameWidget::onHeaderClickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers)
{
    d_func()->mNavigation->onHeaderClickedWithModifiers(logicalIndex, modifiers);
}
// ============================================================
// 排序（实现迁入 DATableSortController，008 期3）
// ============================================================

/**
 * @brief 按列排序（转发排序控制器）
 */
void DADataOperateOfDataFrameWidget::sortColumn(int logicalIndex, Qt::SortOrder order)
{
    d_func()->mSortController->sortColumn(logicalIndex, order);
}

/**
 * @brief 追加为次级排序列（转发排序控制器）
 */
void DADataOperateOfDataFrameWidget::addSecondarySortColumn(int logicalIndex, Qt::SortOrder order)
{
    d_func()->mSortController->addSecondarySortColumn(logicalIndex, order);
}

/**
 * @brief 排序面板的目标列：当前单元格所在列 > 选区首列 > -1
 *
 * Ribbon「升序/降序」按钮作用于「当前列」（与 Excel 一致：光标在哪一列就排哪一列），
 * 不预选整列（整列选择会产生上万 range，拖慢排序的选区重映射）。
 * @return 数据列号；无有效目标返回 -1
 */
int DADataOperateOfDataFrameWidget::currentDataColumnForSort() const
{
    const int selCol = getSelectedOneDataframeColumn();
    if (selCol >= 0) {
        return selCol;
    }
    int actualRow = -1;
    int actualCol = -1;
    if (currentActualCell(&actualRow, &actualCol) && actualCol >= 0) {
        return actualCol;
    }
    return -1;
}

/**
 * @brief 应用完整排序规格（自定义排序对话框确定后转发排序控制器）
 * @param spec 列逻辑索引（或 c_indexSortColumn）+ 方向，顺序即优先级
 * @return 成功入栈返回 true
 */
bool DADataOperateOfDataFrameWidget::applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& spec)
{
    return d_func()->mSortController->applySpec(spec);
}

/**
 * @brief 按索引升序恢复原始行顺序（「清除排序」；转发排序控制器，一条可撤销命令）
 */
void DADataOperateOfDataFrameWidget::restoreOriginalSortOrder()
{
    d_func()->mSortController->restoreOriginalOrder();
}

/**
 * @brief 打开自定义排序对话框（多条件；确定后整份规格一条撤销命令）
 *
 * presetColumn>=0（表头右键入口）时以该列为唯一主键预置，否则按当前规格预填
 * （再次打开可继续调整，与 Excel 的习惯一致）。只读源直接提示、不弹窗。
 * @param presetColumn 预置的主键列（-1 = 用当前排序规格）
 */
void DADataOperateOfDataFrameWidget::showSortDialog(int presetColumn)
{
    DAPyDataFrame frame = getDataframe();
    if (frame.isNone()) {
        return;
    }
    if (!isTableEditable()) {
        daWarning << tr("This data source is read-only and cannot be sorted.");  // cn:该数据源为只读，无法排序
        return;
    }
    const QStringList columnNames = frame.columns();
    if (columnNames.isEmpty()) {
        return;
    }
    QVector< QPair< int, Qt::SortOrder > > initial;
    if (presetColumn >= 0 && presetColumn < columnNames.size()) {
        initial.append(qMakePair(presetColumn, Qt::AscendingOrder));
    } else {
        initial = sortSpec();
        if (initial.isEmpty()) {
            // 没有既有规格：以当前列为唯一主键预置（与「升序/降序」按钮的作用列一致）
            const int currentCol = currentDataColumnForSort();
            if (currentCol >= 0 && currentCol < columnNames.size()) {
                initial.append(qMakePair(currentCol, Qt::AscendingOrder));
            }
        }
    }
    DATableSortDialog dlg(columnNames, initial, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const QVector< QPair< int, Qt::SortOrder > > spec = dlg.sortSpec();
    if (spec.isEmpty() || spec == sortSpec()) {
        return;  // 无有效条件 / 与当前规格相同：不重复执行
    }
    d_func()->mSortController->applySpec(spec);
}

// ============================================================
// 查找/替换（实现迁入 DATableFindController，008 期3）
// ============================================================

/**
 * @brief 打开查找/替换浮动条（转发查找控制器）
 */
void DADataOperateOfDataFrameWidget::showFindReplace(bool expandReplace, bool prefillFromCurrentCell)
{
    d_func()->mFindController->showFindBar(expandReplace, prefillFromCurrentCell);
}

/**
 * @brief 关闭查找/替换浮动条（转发查找控制器）
 */
void DADataOperateOfDataFrameWidget::closeFindReplace()
{
    d_func()->mFindController->closeFindBar();
}

/**
 * @brief 当前查找命中数（转发查找控制器）
 */
int DADataOperateOfDataFrameWidget::findHitCount() const
{
    return d_func()->mFindController->hitCount();
}

/**
 * @brief 当前命中序号（转发查找控制器）
 */
int DADataOperateOfDataFrameWidget::currentFindHitIndex() const
{
    return d_func()->mFindController->currentHitIndex();
}

// ============================================================
// 条件格式（实现迁入 DATableConditionalFormatController，008 期3）
// ============================================================

/**
 * @brief 打开条件格式规则编辑对话框（转发条件格式控制器）
 */
void DADataOperateOfDataFrameWidget::showConditionalFormatDialog(int initialType)
{
    d_func()->mConditionalFormatController->showDialog(initialType);
}

/**
 * @brief 清除选中列的条件格式规则（转发条件格式控制器）
 */
void DADataOperateOfDataFrameWidget::clearConditionalFormatFromSelection()
{
    d_func()->mConditionalFormatController->clearFromSelection();
}

/**
 * @brief 填充条件格式菜单（转发条件格式控制器）
 */
void DADataOperateOfDataFrameWidget::populateConditionalFormatMenu(QMenu* menu)
{
    d_func()->mConditionalFormatController->populateMenu(menu);
}

void DADataOperateOfDataFrameWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        d_func()->ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

//===================================================
// 计算列（004 T1.23）
//===================================================

/**
 * @brief 计算列子存储（借自元数据束，可能为 nullptr）
 */
DATableComputedColumnStore* DADataOperateOfDataFrameWidget::computedColumnStore() const
{
    return d_func()->mComputedColumnStore;
}

/**
 * @brief 数据变更后把依赖这些列（或目标列自身被改）的计算列标待重算（R8 手动重算）
 *
 * 只标状态不求值；排序路径不调用本函数（R13 行顺序变化不影响列值）。
 * 实现收口到 DATableMutationCoordinator（008 期2，唯一实现点）。
 * @param actualCols 变更列位置（空列表 = 无操作）
 */
void DADataOperateOfDataFrameWidget::markComputedColumnsStaleForColumns(const QList< int >& actualCols)
{
    d_func()->mMutation->markComputedColumnsStale(actualCols);
}

//（以下计算列能力转发 DATableComputedColumnController，008 期3；实现已迁入控制器）
/**
 * @brief 打开计算列对话框（转发计算列控制器）
 */
void DADataOperateOfDataFrameWidget::showComputedColumnDialog(int col)
{
    d_func()->mComputedColumnController->showDialog(col);
}

/**
 * @brief 重算单个计算列（转发计算列控制器）
 */
bool DADataOperateOfDataFrameWidget::recomputeComputedColumn(int col)
{
    return d_func()->mComputedColumnController->recompute(col);
}

/**
 * @brief 重算全部待重算/失效的计算列（转发计算列控制器）
 */
int DADataOperateOfDataFrameWidget::recomputeAllStaleComputedColumns()
{
    return d_func()->mComputedColumnController->recomputeAllStale();
}

/**
 * @brief 移除某列的公式定义（转发计算列控制器）
 */
bool DADataOperateOfDataFrameWidget::removeComputedColumnFormula(int col)
{
    return d_func()->mComputedColumnController->removeFormula(col);
}

/**
 * @brief 列改名确认（转发计算列控制器）
 */
bool DADataOperateOfDataFrameWidget::confirmComputedColumnRename(const QList< QPair< QString, QString > >& renames) const
{
    return d_func()->mComputedColumnController->confirmRename(renames);
}

/**
 * @brief 把计算列定义迁移挂进改名命令的 directional 回调（转发计算列控制器）
 */
void DADataOperateOfDataFrameWidget::wireComputedColumnRenameMigration(
    DACommandDataFrame_renameColumns* cmd, const QList< QPair< QString, QString > >& renames) const
{
    d_func()->mComputedColumnController->wireRenameMigration(cmd, renames);
}

// ============================================================
// 分列 / 合并列（实现迁入 DATableColumnCombineOps，008 期4）
// ============================================================

/**
 * @brief 打开分列对话框并执行（转发分列/合并列算子）
 * @param col 源列位置（-1=从选区解析）
 */
void DADataOperateOfDataFrameWidget::showSplitColumnDialog(int col)
{
    d_func()->mColumnCombineOps->showSplitColumnDialog(col);
}

/**
 * @brief 打开合并列对话框并执行（转发分列/合并列算子）
 */
void DADataOperateOfDataFrameWidget::showMergeColumnsDialog()
{
    d_func()->mColumnCombineOps->showMergeColumnsDialog();
}

//===================================================
// 分组聚合（004 T1.22）
//===================================================

/**
 * @brief 就地替换撤销内存判定（转发结果落库算子的静态纯函数）
 */
bool DADataOperateOfDataFrameWidget::isInplaceUndoTooLarge(qint64 rows, qint64 cols)
{
    return DATableResultOps::isInplaceUndoTooLarge(rows, cols);
}

/**
 * @brief 注册新数据集（转发结果落库算子）
 */
void DADataOperateOfDataFrameWidget::registerNewDataset(const DAPyDataFrame& resultDf, const QString& name, const QString& describe)
{
    d_func()->mResultOps->registerNewDataset(resultDf, name, describe);
}

/**
 * @brief 就地替换（转发结果落库算子）
 */
void DADataOperateOfDataFrameWidget::requestInplaceReplace(const DAPyDataFrame& newDf, const QString& text)
{
    d_func()->mResultOps->requestInplaceReplace(newDf, text);
}

/**
 * @brief 打开分组聚合对话框（转发形状变换算子）
 */
void DADataOperateOfDataFrameWidget::showGroupByDialog()
{
    d_func()->mReshapeOps->showGroupByDialog();
}

/**
 * @brief 打开形状变换对话框并执行（转发形状变换算子）
 * @param meltMode true=宽转长（melt）初始模式，false=转置初始模式（对话框内可切换）
 */
void DADataOperateOfDataFrameWidget::showReshapeDialog(bool meltMode)
{
    d_func()->mReshapeOps->showReshapeDialog(meltMode);
}

/**
 * @brief 就地替换生效后的整页刷新（模型 reset + 清排序规格；APP 撤销接线回调也用）
 */
void DADataOperateOfDataFrameWidget::refreshAfterInplaceReplace()
{
    // 行列互换后旧排序规格（列位置+方向）失去意义，清除（同 setDAData 口径）
    d_func()->mSortController->clearSpec();
    if (d_func()->mModel) {
        d_func()->mModel->refreshData();  // 全模型 reset（shape/列名/块缓存重建，滑窗回顶部）
    }
}

//===================================================
// 填充（005 T1.24，设计稿 §4.8）
//===================================================

/**
 * @brief 选区按列分组的连续行区间
 *
 * 取选中单元格（getSelectedDataframeCells 返回 QPoint(绝对行, 列)，已确保在 dataframe 内），
 * 按列聚合后各列的选中行升序，再按连续性切成若干 [rowStart,rowEnd] 闭区间（run）。
 * 不规则多选（Ctrl 点选的非连续行）会切成多个 run，只填选中的行、不碰未选中的行——
 * 与 Excel「填充作用于每个连续区域」一致。尾部预留空白列/行不计（ensureInDataframe）。
 * @return col -> 连续行区间列表（升序）；无选区返回空
 */
QHash< int, QVector< QPair< int, int > > > DADataOperateOfDataFrameWidget::selectedColumnRowRuns() const
{
    return d_func()->mSelection->selectedColumnRowRuns();
}

/**
 * @brief 选区填充（转发填充算子）
 * @param mode 填充模式
 * @return 写入单元格数（0 表示无变化/无选区/只读源）
 */
int DADataOperateOfDataFrameWidget::fillSelection(FillMode mode)
{
    return d_func()->mFillOps->fillSelection(mode);
}
/**
 * @brief 打开序列填充对话框（转发填充算子）
 * @return 写入单元格数（取消/无选区/只读源返回 0）
 */
int DADataOperateOfDataFrameWidget::showFillSeriesDialog()
{
    return d_func()->mFillOps->showFillSeriesDialog();
}

/**
 * @brief 校验规则子存储（借自元数据束）
 * @return 可能为 nullptr（无注册表时）
 */
DATableValidationStore* DADataOperateOfDataFrameWidget::validationStore() const
{
    return d_func()->mValidationStore;
}

/**
 * @brief 规则显示文本（转发校验控制器；实现迁入 DATableValidationController，008 期3）
 */
QString DADataOperateOfDataFrameWidget::validationRuleDisplayText(const DATableValidationRule& rule) const
{
    return d_func()->mValidationController->validationRuleDisplayText(rule);
}

/**
 * @brief 打开校验规则编辑对话框（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::showValidationRulesDialog()
{
    d_func()->mValidationController->showRulesDialog();
}

/**
 * @brief 执行校验求值（转发校验控制器；返回结构经 ValidationRuleOutcome 别名保持兼容）
 */
QList< DADataOperateOfDataFrameWidget::ValidationRuleOutcome > DADataOperateOfDataFrameWidget::evaluateValidationRules(int* elapsedMs)
{
    return d_func()->mValidationController->evaluateRules(elapsedMs);
}

/**
 * @brief 执行校验并弹出报告对话框（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::runValidationAndShowReport()
{
    d_func()->mValidationController->runAndShowReport();
}

/**
 * @brief 清除违规高亮（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::clearValidationHighlights()
{
    d_func()->mValidationController->clearHighlights();
}

/**
 * @brief 列改名确认（转发校验控制器）
 */
bool DADataOperateOfDataFrameWidget::confirmValidationRuleRename(const QList< QPair< QString, QString > >& renames) const
{
    return d_func()->mValidationController->confirmRuleRename(renames);
}

/**
 * @brief 把校验规则引用迁移挂进改名命令的 directional 回调（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::wireValidationRuleRenameMigration(
    DACommandDataFrame_renameColumns* cmd, const QList< QPair< QString, QString > >& renames) const
{
    d_func()->mValidationController->wireRuleRenameMigration(cmd, renames);
}


//===================================================
// 单元格批注（005 T1.26，设计稿 §4.10）
//===================================================

/**
 * @brief 批注子存储（借自元数据束）
 * @return 可能为 nullptr（无注册表时）
 */
DATableNoteStore* DADataOperateOfDataFrameWidget::noteStore() const
{
    return d_func()->mNoteStore;
}

/**
 * @brief 当前单元格（数据源绝对坐标）
 *
 * 视图行经 toActualRow 翻译成绝对行；越出数据范围（尾部预留空白行列）返回 false。
 * @param actualRow 非空时输出绝对行
 * @param actualCol 非空时输出列位置
 * @return 有有效当前格返回 true
 */
bool DADataOperateOfDataFrameWidget::currentActualCell(int* actualRow, int* actualCol) const
{
    return d_func()->mSelection->currentActualCell(actualRow, actualCol);
}

/**
 * @brief 打开当前单元格的批注编辑弹层（无批注=插入，有批注=编辑且弹层内可删除）
 *（实现迁入 DATableNoteController，008 期3）
 */
void DADataOperateOfDataFrameWidget::showNoteEditorForCurrentCell()
{
    d_func()->mNoteController->showNoteEditorForCurrentCell();
}

/**
 * @brief 删除当前单元格批注（命令可撤销；实现迁入 DATableNoteController，008 期3）
 * @return 成功入栈返回 true；无当前格或无批注返回 false
 */
bool DADataOperateOfDataFrameWidget::deleteNoteAtCurrentCell()
{
    return d_func()->mNoteController->deleteNoteAtCurrentCell();
}

/**
 * @brief 清除本表全部批注（一次命令，可撤销；实现迁入 DATableNoteController，008 期3）
 * @return 清除的批注数（无批注返回 0，不推空命令）
 */
int DADataOperateOfDataFrameWidget::clearAllNotes()
{
    return d_func()->mNoteController->clearAllNotes();
}

// ============================================================
// 表内快速绘图（快速图表预览；实现迁入 DATableQuickPlotController，008 期3）
// ============================================================

/**
 * @brief 用当前（有序）选区构造快速绘图请求（转发快速绘图控制器）
 */
DA::DATableChartRequest DADataOperateOfDataFrameWidget::buildQuickPlotRequest() const
{
    return d_func()->mQuickPlotController->buildQuickPlotRequest();
}

/**
 * @brief 按右键列规划快速绘图能力（转发快速绘图控制器）
 */
DA::DATableChartPlan DADataOperateOfDataFrameWidget::quickPlotPlanForColumn(int logicalIndex) const
{
    return d_func()->mQuickPlotController->quickPlotPlanForColumn(logicalIndex);
}

/**
 * @brief 用当前选区打开/刷新快速绘图子窗口（转发快速绘图控制器）
 */
bool DADataOperateOfDataFrameWidget::showQuickPlot(DA::DATableChartType preferredType)
{
    return d_func()->mQuickPlotController->showQuickPlot(preferredType);
}

/**
 * @brief 关闭快速绘图子窗口（实例保留复用；转发快速绘图控制器）
 */
void DADataOperateOfDataFrameWidget::closeQuickPlot()
{
    d_func()->mQuickPlotController->closeQuickPlot();
}

/**
 * @brief 快速绘图浮窗是否正在显示（转发快速绘图控制器）
 */
bool DADataOperateOfDataFrameWidget::isQuickPlotVisible() const
{
    return d_func()->mQuickPlotController->isQuickPlotVisible();
}

/**
 * @brief 设置转入绘图区的处理器（APP 注入；转发快速绘图控制器）
 */
void DADataOperateOfDataFrameWidget::setQuickPlotTransferHandler(const QuickPlotTransferHandler& handler)
{
    d_func()->mQuickPlotController->setTransferHandler(handler);
}

/**
 * @brief 设置「追加到当前图表」的可用性提供者（APP 注入；转发快速绘图控制器）
 */
void DADataOperateOfDataFrameWidget::setQuickPlotAppendAvailabilityProvider(
    const QuickPlotAppendAvailabilityProvider& provider)
{
    d_func()->mQuickPlotController->setAppendAvailabilityProvider(provider);
}

/**
 * @brief 有序选区：按视图顺序返回选区涉及的列（数据列号）
 *
 * 现有 getSelectedDataframeCoumns 走 QSet::values()，顺序不保证；快速绘图需要
 * 「第 1 列当 X、第 2 列当 Y」这类稳定语义，因此按窗格容器的可见列顺序重排，
 * 隐藏列按视图状态跳过（实施方案 §8.1）。
 * @param ensureInDataframe 是否要求列号落在 dataframe 范围内（惰性源传 false）
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataColumnsInViewOrder(bool ensureInDataframe) const
{
    return d_func()->mSelection->selectedDataColumnsInViewOrder(ensureInDataframe);
}

/**
 * @brief 按选中列复制/导出的列范围解析（列头右键「复制选中列为/导出选中列」入口）
 *
 * 有选中列 → 视图顺序返回（所见即所得：导出列序与用户看到的列序一致）；
 * 无选中 → 回退右键所在列 {fallbackColumn}（菜单项文案虽写「选中列」，右键
 * 在某列表头上本身就是明确的指向，Excel 同款语义）；两者皆无有效列返回空
 * 列表（调用方提示用户）。
 */
QList< int > DADataOperateOfDataFrameWidget::selectedColumnsForDataAction(int fallbackColumn) const
{
    return d_func()->mSelection->selectedColumnsForDataAction(fallbackColumn);
}

