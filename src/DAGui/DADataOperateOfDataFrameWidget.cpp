#include "DADataOperateOfDataFrameWidget.h"
#include "ui_DADataOperateOfDataFrameWidget.h"
#include "Models/DADataTableModel.h"
#include "Models/DAAbstractCacheWindowTableModel.h"
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
#include "DAWaitCursorScoped.h"
#include "DALogCategory.h"
#include "DASlowLog.h"
#include "DATableDataSource.h"
#include "DATableKeyRemap.h"
#include "DAPybind11QtCaster.hpp"
// stl
#include <memory>
#include <algorithm>
// qt
#include <QTableView>
#include <QHeaderView>
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
#include "Commands/DACommandsDataFrame.h"
#include "Commands/DACommandsTableStyle.h"
#include "Commands/DACommandsTableColumnFormat.h"
#include "Commands/DACommandsConditionalFormat.h"
#include "Commands/DACommandsTableColumnView.h"
#include "DADataManager.h"
#include "DACallBackInterface.h"

#include <QDomDocument>
// table metadata / style
#include "DATableStyleManager.h"
#include "DATableStyleItemDelegate.h"
#include "DATableMetadataRegistry.h"
#include "DATableMetadata.h"
#include "DATableViewStateStore.h"
#include "DATableHighlightOverlay.h"
#include "DATableConditionalFormatStore.h"
#include "DATableComputedColumnStore.h"
#include "DATableValidationStore.h"
#include "Commands/DACommandsTableValidation.h"
#include "DATableNoteStore.h"
#include "DATableNoteEditDialog.h"
#include "DADataHeaderView.h"
#include "DATablePaneContainer.h"
// 表内快速绘图
#include "TableChart/DATableChartPlanner.h"
#include "TableChart/DATableQuickPlotWindow.h"
// 表格子窗口容器（快速绘图的承载框架，也可贴入其它窗口）
#include "TableChildWindow/DATableChildWindow.h"
#include "TableChildWindow/DATableChildWindowContainer.h"
// 表格页核心（008 期1：页面上下文 + 选区服务；期2：编辑后效协调器）
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
// 表格页特性控制器（008 期3）
#include "TableNote/DATableNoteController.h"
#include "TableQuickPlot/DATableQuickPlotController.h"
#include "TableComputedColumn/DATableComputedColumnController.h"
#include "TableConditionalFormat/DATableConditionalFormatController.h"
#include "TableFind/DATableFindController.h"
#include "TableStyle/DATableStyleController.h"
#include "TableSort/DATableSortController.h"
#include "TableViewState/DATableViewStateController.h"
#include "TableColumnView/DATableColumnViewController.h"
#include "TableOps/DATableRowOps.h"
#include "TableOps/DATableCellOps.h"
// Dialog
#include "Dialog/DARenameColumnsNameDialog.h"
#include "Dialog/DADialogDataframeColumnCastToNumeric.h"
#include "Dialog/DADialogDataframeColumnCastToDatetime.h"
#include "Dialog/DADialogInsertNewColumn.h"
#include "Dialog/DADialogTableColumnView.h"
#include "Dialog/DADialogFindReplace.h"
#include "Dialog/DADialogConditionalFormat.h"
#include "Dialog/DADialogConditionalFormatQuickRule.h"
#include "DAConditionalFormatPresets.h"
#include "Dialog/DADialogComputedColumn.h"
#include "Dialog/DADialogSplitColumn.h"
#include "Dialog/DADialogMergeColumns.h"
#include "Dialog/DADialogGroupBy.h"
#include "Dialog/DADialogReshape.h"
#include "Dialog/DADialogFillSeries.h"
// python operator wrapper（查找/替换）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include <climits>

//===================================================
// 008 表格页重构 · 分区索引（迁移路线见 tmp/plan-表格页重构与职责拆分.md §5）
//
// 期1 已迁出（src/DAGui/TableCore/）：
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
    : DADataOperatePageWidget(parent)
    , ui(new Ui::DADataOperateOfDataFrameWidget)
    , mDialogCastNumArgs(nullptr)
    , mDialogCastDatetimeArgs(nullptr)
{
    ui->setupUi(this);

    // 运行时包装（006b）：把 .ui 里已嵌入的视图作为主窗格接管。.ui 不动；
    // getDataTableView() 返回值与语义不变（= 主窗格），APP/插件调用点零改动。
    // 必须在任何 configurePaneView() 之前（configurePaneView 里 006c/006d 要往
    // mPaneContainer 上连线；先装配后建容器 = 那些 if (mPaneContainer) 全部静默跳过）
    ui->verticalLayout->removeWidget(ui->tableView);
    mPaneContainer = new DATablePaneContainer(this);
    ui->verticalLayout->addWidget(mPaneContainer);
    mPaneContainer->setMainView(ui->tableView);
    // 拖动落库（006c）：容器合并出全局列序后交页面入栈重投影（拖动 = 一个撤销单元）
    connect(mPaneContainer, &DATablePaneContainer::columnOrderChanged, this, &DADataOperateOfDataFrameWidget::applyColumnOrderFromView);
    // 冻结区内部拖动落库：冻结列之间的先后顺序（不改全局列序），入栈重投影
    connect(mPaneContainer, &DATablePaneContainer::frozenOrderChanged, this, &DADataOperateOfDataFrameWidget::applyFrozenOrderFromView);
    // 冻结分界条（宽随列宽改造）：拖动结束写 store（与表头拖拽同键 (名, occurrence)），
    // 双击对末冻结列做内容自适应；两信号均在容器已确定末冻结列的前提下发出
    //（视图状态采集/投影经视图状态控制器，008 期3）
    connect(mPaneContainer, &DATablePaneContainer::frozenBoundaryResized, this,
            [ this ](int logicalColumn, int newWidth) {
        mViewStateController->storeColumnWidthAt(logicalColumn, newWidth);
    });
    connect(mPaneContainer, &DATablePaneContainer::frozenBoundaryAutoFitRequested, this, [ this ](int logicalColumn) {
        if (!mPaneContainer || !mPaneContainer->frozenView()) {
            return;
        }
        const int w = mViewStateController->contentFittedColumnWidth(mPaneContainer->frozenView(), logicalColumn);
        if (w <= 0) {
            return;
        }
        mPaneContainer->frozenView()->horizontalHeader()->resizeSection(logicalColumn, w);
        mViewStateController->storeColumnWidthAt(logicalColumn, w);
    });
    // 冻结窗格懒建（006e）：容器建好窗格后交页面装配同一条 configurePaneView，
    // 装配完成后再补列宽/行头宽；转发契约（冻结 → 主）也在此接线（APP 只连主窗格）
    connect(mPaneContainer, &DATablePaneContainer::frozenViewCreated, this, [ this ](DA::DADataTableView* frozen) {
        configurePaneView(frozen);
        mViewStateController->applyColumnWidthsToPane(frozen);
        mViewStateController->autoFitUnrecordedColumnWidths(frozen);  // 无记录列内容自适应（冻结窗格首次懒建时视口可能还没行，靠投影尾轮兜底）
        mViewStateController->applyRowHeaderWidthToOwner();
        // 表头动作转发：重命名/删除/定位最大最小/描述统计/计算列三项（APP 只连主窗格）
        DADataHeaderView* fh = qobject_cast< DADataHeaderView* >(frozen->horizontalHeader());
        DADataHeaderView* mh = qobject_cast< DADataHeaderView* >(ui->tableView->horizontalHeader());
        if (fh && mh) {
            connect(fh, &DADataHeaderView::columnActionRequested, mh, &DADataHeaderView::columnActionRequested);
            // 按选中列复制/导出（冻结窗格表头触发 → 主窗格同名信号，APP 只连主窗格）
            connect(fh, &DADataHeaderView::columnDataCopyRequested, mh, &DADataHeaderView::columnDataCopyRequested);
            connect(fh, &DADataHeaderView::columnDataExportRequested, mh, &DADataHeaderView::columnDataExportRequested);
        }
        // 表体剪贴板三信号转发到主窗格同名信号
        connect(frozen, &DADataTableView::contextMenuCutRequested, ui->tableView, &DADataTableView::contextMenuCutRequested);
        connect(frozen, &DADataTableView::contextMenuCopyRequested, ui->tableView, &DADataTableView::contextMenuCopyRequested);
        connect(frozen, &DADataTableView::contextMenuPasteRequested, ui->tableView, &DADataTableView::contextMenuPasteRequested);
        // 列宽/冻结数变化都会触发容器重投影与几何重算
        connect(frozen->horizontalHeader(), &QHeaderView::sectionResized, this, [ this ]() {
            if (mPaneContainer) {
                mPaneContainer->updateFrozenGeometry();
            }
        });
        // 冻结窗格行头宽变化：行头归属冻结窗格，宽度变化要同步几何
        if (DADataHeaderView* fvh = qobject_cast< DADataHeaderView* >(frozen->verticalHeader())) {
            connect(fvh, &DADataHeaderView::rowHeaderWidthChanged, this, [ this ](int width) {
                if (mViewStateStore) {
                    mViewStateStore->setRowHeaderWidth(width);
                }
                if (mPaneContainer) {
                    mPaneContainer->updateFrozenGeometry();
                }
            });
        }
    });

    mModel = new DADataTableModel(getUndoStack(), this);

    // 表格元数据束与各子存储、delegate
    // 元数据束借自会话级 DATableMetadataRegistry（随数据存在，widget 关闭后保留，重开可见）。
    // 行列增删的样式键偏移同步在 §7 的回调中接入（onRowsInserted 等系列）。
    DATableMetadata* metadata = registry ? registry->getOrCreate(d) : nullptr;
    mMetadata                 = metadata;
    mStyleManager             = metadata ? metadata->styleManager() : nullptr;
    mViewStateStore           = metadata ? metadata->viewState() : nullptr;
    mConditionalFormatStore   = metadata ? metadata->conditionalFormat() : nullptr;
    mComputedColumnStore      = metadata ? metadata->computedColumns() : nullptr;
    mValidationStore          = metadata ? metadata->validations() : nullptr;
    mNoteStore                = metadata ? metadata->notes() : nullptr;
    // 瞬态高亮层：页级持有（查找/校验两个来源共用），不序列化、不进撤销栈；
    // 注入 delegate 绘制管线③（瞬态装饰）
    mHighlightOverlay = new DATableHighlightOverlay(this);
    mStyleDelegate    = new DATableStyleItemDelegate(mStyleManager, mHighlightOverlay, this);
    // 条件格式子存储注入 delegate 绘制管线②（持久装饰）与④（数据条），003 T1.5
    mStyleDelegate->setConditionalFormatStore(mConditionalFormatStore);
    // 校验规则子存储注入 delegate（枚举列下拉编辑器 createEditor，005 T1.25）
    mStyleDelegate->setValidationStore(mValidationStore);
    // 批注子存储注入 delegate（层⑤之后角标 + helpEvent tooltip，005 T1.26）
    //（视图侧 setNoteStore 在 configurePaneView 里，006b 起随窗格装配）
    mStyleDelegate->setNoteStore(mNoteStore);
    // 表格页上下文（008 期1）：模型/主窗格/元数据束聚合为依赖注入载体；
    // 选区/坐标查询全部收口到 DATableSelectionService（纯 const 查询，不持有窗口）
    mContext = std::make_unique< DATablePageContext >();
    mContext->setModel(mModel);
    mContext->setMainView(ui->tableView);
    mContext->setPaneContainer(mPaneContainer);
    mContext->setMetadata(mMetadata);
    mContext->setHighlightOverlay(mHighlightOverlay);
    mContext->setHostWidget(this);
    mContext->setData(d);
    mSelection = std::make_unique< DATableSelectionService >(*mContext);
    // 宿主桥注入（008 期2）：协作者经上下文的 std::function 触达页面级动作，
    // 不持窗口指针（notifyHostValueChange/closeFloatingWindows 同款设计见计划 §6.1）
    mContext->notifyHostValueChange = [ this ]() {
        if (DADataManager* mgr = mData.getDataManager()) {
            mgr->notifyDataChangedSignal(mData, DADataManager::ChangeValue);
        }
    };
    mContext->notifyHostColumnRename = [ this ]() {
        if (DADataManager* mgr = mData.getDataManager()) {
            mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
        }
    };
    mContext->closeFloatingWindows = [ this ]() { closeQuickPlot(); };
    // 撤销栈推入桥（push 即激活本页撤销栈；控制器经此入栈，不持窗口指针）
    mContext->pushCommand = [ this ](QUndoCommand* cmd) { push(cmd); };
    // 定位桥（007 定位单入口：隐藏列自动取消隐藏 + 冻结区处置；select=false 只摆位
    // 不收缩选区；报告窗等控制器用）
    mContext->locateActualCell = [ this ](int row, int col, bool select) { locateActualCell(row, col, select); };
    // 脏标桥（计算列重算等改数据的操作经此发 dataModified，与页面信号同源）
    mContext->notifyDataModified = [ this ]() { Q_EMIT dataModified(); };
    // 拾取模式查询桥（图表「选择序列」会话期间排序入口不生效）
    mContext->isHeaderPickMode = [ this ]() -> bool { return mHeaderPickMode; };
    // 编辑后效协调器（008 期2）：commit() 是唯一的「编辑后」扇出点，
    // 此前 ~20 处手抄的 模型刷新→统计失效→计算列标脏→浮窗/宿主 收口于此；
    // attach() 一次性订阅批注/高亮层/样式/条件格式/校验缓存的「变更→失效/刷新」
    // 连接（008 期2 Observer 收口，此前散在本构造里 ~160 行）
    mMutation = new DATableMutationCoordinator(*mContext, this);
    mMutation->attach();
    // 特性控制器（008 期3）：批注（构造注入上下文与选区服务，QObject 父子持有）
    mNoteController = new DATableNoteController(*mContext, *mSelection, this);
    // 快速绘图（构造注入上下文与选区服务；数据变更经本页 dataModified 信号转发）
    mQuickPlotController = new DATableQuickPlotController(*mContext, *mSelection, this);
    connect(this, &DADataOperateOfDataFrameWidget::dataModified, mQuickPlotController, &DATableQuickPlotController::notifyDataChanged);
    // 数据校验（构造注入上下文；报告窗定位经上下文定位桥回本页单入口）
    mValidationController = new DATableValidationController(*mContext, this);
    // 计算列（构造注入上下文；定义/状态变更→表头重绘的连接在控制器构造内订阅）
    mComputedColumnController = new DATableComputedColumnController(*mContext, this);
    // 条件格式（构造注入上下文与选区服务；WPS 式菜单/规则对话框/差异命令）
    mConditionalFormatController = new DATableConditionalFormatController(*mContext, *mSelection, this);
    // 查找/替换（构造注入上下文/选区服务/编辑后效协调器；替换生效经 replaceApplied 转发）
    mFindController = new DATableFindController(*mContext, *mSelection, mMutation, this);
    connect(mFindController, &DATableFindController::replaceApplied, this, &DADataOperateOfDataFrameWidget::replaceApplied);
    // 样式/显示格式（构造注入上下文与选区服务）
    mStyleController = new DATableStyleController(*mContext, *mSelection, this);
    // 排序（构造注入上下文；插删列/换数据集后规格清理由控制器 clearSpec 提供）
    mSortController = new DATableSortController(*mContext, this);
    // 视图状态（构造注入上下文与选区服务；列宽采集/投影/冻结集合查表）
    mViewStateController = new DATableViewStateController(*mContext, *mSelection, this);
    // 列视图（构造注入上下文/选区服务 + 视图状态/计算列/校验控制器；列视图真源操作）
    mColumnViewController = new DATableColumnViewController(*mContext, *mSelection, mViewStateController,
                                                            mComputedColumnController, mValidationController, this);
    // 数据操作算子（008 期4：一个操作一个文件；参数化入口由 Facade 直调具体算子）
    mRowOps = new DATableRowOps(*mContext, *mSelection, mMutation, this);
    mCellOps = new DATableCellOps(*mContext, *mSelection, mMutation, this);
    // 窗格装配（006b 抽取的唯一装配点）：表头替换 → setModel → 委托 → 逐窗格连接；
    // 必须在 mStyleDelegate 创建之后（里面要用它），在 setDAData/applyViewStateFromStore 之前
    configurePaneView(ui->tableView);
    // 模型读取列显示格式（actualData 的 DisplayRole 分支）
    mModel->setStyleManager(mStyleManager);
    // 计算列子存储注入模型（表头浅蓝底色 BackgroundRole + 公式 tooltip ToolTipRole，004 T1.23）
    mModel->setComputedColumnStore(mComputedColumnStore);
    // 视图状态（列宽按列名 + 行头宽，R6）：
    // 结构变化（模型重置/插删列）后按列名重新应用，保证列宽不随位置错位；
    // 采集只认用户拖拽（sectionResized + 左键按住在表头上），程序化改动不写 store
    //（期3 ViewStateController 迁移候选，暂留 Facade）
    connect(mModel, &QAbstractItemModel::modelReset, this, [ this ]() {
        applyViewStateFromStore();
    });
    connect(mModel, &QAbstractItemModel::columnsInserted, this, [ this ]() {
        applyViewStateFromStore();
        // 插列后排序规格里的列位置失效，清除规格（数据顺序不变，只清指示器）
        mSortController->clearSpec();
    });
    connect(mModel, &QAbstractItemModel::columnsRemoved, this, [ this ]() {
        applyViewStateFromStore();
        // 删列后排序规格里的列位置失效，清除规格
        mSortController->clearSpec();
    });
    // 关闭不必要的绘制特性
    setDAData(d);
    // 构造完成后应用一次视图状态（工程加载的宽度回填在注册表里，此时已可借用）
    applyViewStateFromStore();
    // 选中变化时反向同步 ribbon 控件（只在主窗格连：006e 冻结窗格共享同一 selection model）
    if (auto sm = ui->tableView->selectionModel()) {
        connect(sm, &QItemSelectionModel::selectionChanged, this, [ this ]() {
            DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.selectionChanged", [this]() {
                return QStringLiteral("rows=%1 cols=%2").arg(dataRowCount()).arg(dataColumnCount());
            });
            Q_EMIT currentStyleChanged(getCurrentCellStyle());
            Q_EMIT currentDisplayFormatChanged(getCurrentColumnDisplayFormat());
            // 查找条处于「当前选区」范围时，选区变化经去抖触发重扫（范围基准随选区更新；
            // 实现迁入 DATableFindController，008 期3）
            mFindController->restartDebounceIfVisible();
            Q_EMIT tableSelectionChanged();  // 007：状态栏统计触发源
        });
    }
}

DADataOperateOfDataFrameWidget::~DADataOperateOfDataFrameWidget()
{
    // 条件格式子存储是会话级（借自注册表，比 widget 长寿）：清空统计提供者，
    // 避免持有已销毁页面的回调（QPointer 已防悬挂，此处彻底释放 lambda 捕获）
    if (mConditionalFormatStore) {
        mConditionalFormatStore->setStatsProvider(nullptr);
        mConditionalFormatStore->setDuplicatesProvider(nullptr);
    }
    // 子窗口容器先断回调（008 期0 基线修复，随快速绘图迁入控制器）：容器的
    // boundsProvider 回调读 ui->tableView，delete ui 之后页面仍在析构（~QWidget 阶段），
    // 被监听部件（主视口等）的 Hide 事件会触发 eventFilter → clampAllWindows → 回调 →
    // 读已释放的 ui（use-after-free，DATableQuickPlotTest 在页面析构路径偶现 0xC0000005 崩溃）
    if (mQuickPlotController) {
        mQuickPlotController->detach();
    }
    delete ui;
    ui = nullptr;
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
    // 1) 表头替换：用 DADataHeaderView（列头右键菜单/带修饰键点击信号/行头宽拖拽）。
    //    行高/Interactive 设置在 DADataHeaderView(Vertical) 构造内重建
    //    （参照 DACacheWindowTableView 构造对默认行头的同款设置）
    pane->setHorizontalHeader(new DADataHeaderView(Qt::Horizontal, pane));
    pane->setVerticalHeader(new DADataHeaderView(Qt::Vertical, pane));
    // 2) 模型与委托（同一 delegate 实例注入多个视图，delegate 属页面）；
    //    冻结窗格共享主窗格的 selectionModel（006e，顺序：先 setModel 再共享）
    pane->setModel(mModel);
    if (pane != ui->tableView) {
        pane->setSelectionModel(ui->tableView->selectionModel());
    }
    pane->setItemDelegate(mStyleDelegate);
    // 3) 批注子存储注入视图（右键菜单「插入/编辑/删除批注」的启用规则查当前格）
    pane->setNoteStore(mNoteStore);
    // 4) 水平表头：计算列 store 注入（ƒx 角标 + 列头菜单计算列组）+ 信号。
    //    006c 在此追加 sectionMoved → 容器槽；006d 在此追加 columnViewActionRequested
    if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(pane->horizontalHeader())) {
        hh->setComputedColumnStore(mComputedColumnStore);
        // 冻结状态查询：列头菜单据此显示「冻结此列」或「取消冻结此列」
        hh->setFrozenColumnChecker([ this ](int logical) { return isColumnInFrozenArea(logical); });
        connect(hh, &DADataHeaderView::clickedWithModifiers, this, &DADataOperateOfDataFrameWidget::onHeaderClickedWithModifiers);
        // 列宽采集：用触发信号的那个窗格的表头（不再写死 ui->tableView；采集经视图状态控制器，008 期3）
        connect(hh, &QHeaderView::sectionResized, this, [ this, pane ](int logicalIndex, int, int newSize) {
            mViewStateController->onHeaderSectionResized(pane, logicalIndex, newSize);
        });
        // 列序拖动落库（006c）：必须在表头替换之后（hh 是新表头）；每装一个窗格连一份
        if (mPaneContainer) {
            connect(hh, &QHeaderView::sectionMoved, mPaneContainer, &DATablePaneContainer::onPaneSectionMoved);
        }
        // 「列」子菜单动作（006d）：列视图真源操作，消费方是页面自身（不经 APP）
        connect(hh, &DADataHeaderView::columnViewActionRequested,
                this, &DADataOperateOfDataFrameWidget::onHeaderColumnViewActionRequested);
        // 表头高拖拽落库（底边热区）：写视图状态 + 套到各窗格（冻结窗格行对齐要求同高）
        connect(hh, &DADataHeaderView::horizontalHeaderHeightChanged, this, [ this ](int height) {
            if (mViewStateStore) {
                mViewStateStore->setHorizontalHeaderHeight(height);
            }
            mViewStateController->applyHeaderHeightToPanes(height);
        });
    }
    // 5) 垂直表头：行头宽拖拽 → 写视图状态 + 冻结几何重算（006e）
    if (DADataHeaderView* vh = qobject_cast< DADataHeaderView* >(pane->verticalHeader())) {
        connect(vh, &DADataHeaderView::rowHeaderWidthChanged, this, [ this ](int width) {
            if (mViewStateStore) {
                mViewStateStore->setRowHeaderWidth(width);
            }
            if (mPaneContainer) {
                mPaneContainer->updateFrozenGeometry();
            }
        });
        // 统一行高拖拽落库（行边界热区）：写视图状态 + 套到各窗格
        connect(vh, &DADataHeaderView::rowHeightChanged, this, [ this ](int height) {
            if (mViewStateStore) {
                mViewStateStore->setRowHeight(height);
            }
            mViewStateController->applyRowHeightToPanes(height);
        });
    }
    // 6) 视图表意入口（快捷键/右键菜单共用）：每装一个窗格连一份
    connect(pane, &QTableView::clicked, this, &DADataOperateOfDataFrameWidget::onTableViewClicked);
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
}

/**
 * @brief 是否存在data
 * @return
 */
bool DADataOperateOfDataFrameWidget::haveData() const
{
    // dataframe与通用表格数据（如数据库惰性表）都视为有数据；series维持原语义不开数据页
    return mData.isDataFrame() || (mData.isTable() && !mData.isSeries());
}

/**
 * @brief 获取dataframe
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::getDataframe() const
{
    return mData.toDataFrame();
}

/**
 * @brief 获取Data的引用
 * @return
 */
const DAData& DADataOperateOfDataFrameWidget::data() const
{
    return mData;
}

void DADataOperateOfDataFrameWidget::setDAData(const DA::DAData& d)
{
    // 切换数据集后浮窗绑定的列身份与数据快照都失效，直接关闭（实施方案 §5.3）
    closeQuickPlot();
    mData = d;
    mContext->setData(d);  // 上下文随宿主换数据（008 期1）
    // 数据替换后旧排序规格（列位置+方向）失去意义，清除（排序规格不持久化，R13）
    mSortController->clearSpec();
    // 通用表格数据（DATableDataSource，如数据库惰性表）也可显示；series维持原语义
    if (d.isDataFrame() || (d.isTable() && !d.isSeries())) {
        ui->tableView->setData(d);
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
    mRowOps->insertRowAt(row);
}

/**
 * @brief 选中列右边插入新列
 */
void DADataOperateOfDataFrameWidget::insertColumnRightBySelect()
{
    int c = getSelectedOneDataframeColumn();
    if (c < 0) {
        return;
    }
    insertColumnAt(c + 1);
}

/**
 * @brief 选中列左边插入新列
 */
void DADataOperateOfDataFrameWidget::insertColumnLeftBySelect()
{
    int c = getSelectedOneDataframeColumn();
    if (c < 0) {
        return;
    }
    insertColumnAt(c);
}

/**
 * @brief 在col位置插入列
 * @param col
 */
void DADataOperateOfDataFrameWidget::insertColumnAt(int col)
{
    if (mData.toDataFrame().isNone()) {
        // 非dataframe数据（如只读的数据库惰性表）不支持插入列，弹窗前先拦截
        return;
    }
    DADialogInsertNewColumn dlg(this);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    std::unique_ptr< DACommandDataFrame_insertColumn > cmd;
    QString name = dlg.getName();
    if (name.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Warning"),                                                     // cn:警告
                             tr("The name of the new column to be inserted must be specified")  // cn:必须指定列的名字
        );
        return;
    }
    DAPyDType dt = dlg.getDType();
    if (dlg.isRangeMode()) {
        cmd.reset(
            new DACommandDataFrame_insertColumn(mData.toDataFrame(), col, name, dlg.getStartValue(), dlg.getStopValue()));
    } else {
        cmd.reset(new DACommandDataFrame_insertColumn(mData.toDataFrame(), col, name, dlg.getDefaultValue()));
    }
    QPointer< DADataTableModel > modle = mModel;
    // 编辑后效（008 期2）：插列使列位置整体后移 → 全量统计失效 + 计算列全标脏（策略表：列增删）
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Structure;
    mutation.modelRefresh = [ modle, col ]() {
        if (modle) {
            // 此操作会删除一列，添加一列，整个modelreflash（沿用旧通知形状）
            modle->notifyColumnsRemoved({ col });
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    // 行列增删样式键同步：插入列时 >=col 的键 +1；撤销时按删除处理
    // 条件格式规则同为装饰类位置键（R20），随列插删同款重映射（003）
    DATableStyleManager* styleMgr = mStyleManager;
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    DATableNoteStore* noteStore   = mNoteStore;
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, col ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsRemoved({ col });
            } else {
                styleMgr->onColumnsInserted({ col });
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsRemoved({ col });
            } else {
                cfStore->onColumnsInserted({ col });
            }
        }
        // 批注列维同款重映射（005 T1.26）
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsRemoved({ col });
            } else {
                noteStore->onColumnsInserted({ col });
            }
        }
    });
    if (!cmd->exec()) {
        return;
    }
    push(cmd.release());
}

/**
 * @brief 移除选中的行（转发行算子）
 * @return 返回成功移除的行数
 */
int DADataOperateOfDataFrameWidget::removeSelectRow()
{
    return mRowOps->removeSelectedRows();
}

/**
 * @brief 移除选中的列
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectColumn()
{
    QList< int > columns = getSelectedDataframeCoumns();
    if (columns.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return 0;
    }
    return removeColumns(columns);
}

/**
 * @brief 移除指定列（表头右键「删除列」直接入口）
 *
 * 与 removeSelectColumn 的差异：不依赖选区——右键菜单弹出瞬间焦点被菜单窗口
 * 抢走，APP 侧 getCurrentDataFrameOperateWidget 的焦点检查会失败，选区也不一定
 * 在目标列上；本入口直接按列号操作本页。
 * 冻结列被删除时冻结集合同步剔除（主区列不补位），全部冻结列删光后
 * 冻结窗格随之隐藏（removeColumns 内投影收敛）。
 * @param col 列位置索引
 * @return 成功移除的列数（0 = 失败/列号无效）
 */
int DADataOperateOfDataFrameWidget::removeColumnAt(int col)
{
    if (col < 0 || col >= dataColumnCount()) {
        return 0;
    }
    return removeColumns(QList< int >({ col }));
}

/**
 * @brief 按列号列表删除列（removeSelectColumn / removeColumnAt 共用命令管线）
 *
 * 命令：DACommandDataFrame_dropIColumn（临时 pickle 快照，undo 整表还原）。
 * 联动（对照重命名路径补全）：
 * 1) 视图状态（列宽/列序/隐藏/冻结）：删除前整店 XML 快照，redo 清被删列条目
 *    （冻结集合随 removeColumn 一并剔除），undo 从快照整店还原（同一撤销单元）；
 * 2) 数据管理器树：notifyDataChangedSignal(ChangeValue) 重建列子项；
 * 3) 样式/条件格式/批注/计算列重映射与失效提示：与旧 removeSelectColumn 一致。
 * @param columns 列位置索引列表
 * @return 成功移除的列数
 */
int DADataOperateOfDataFrameWidget::removeColumns(const QList< int >& columns)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    if (columns.isEmpty()) {
        return 0;
    }
    // —— 删除前解析：被删列名（视图状态按名字清条目，冻结集合随之剔除） ——
    QStringList removedNames;
    const QList< QString > colNames = df.columns();
    for (int c : columns) {
        if (c >= 0 && c < colNames.size()) {
            removedNames.append(colNames.at(c));
        }
    }
    // 视图状态整店快照（undo 还原；QDomDocument 按引用持有，lambda 里 shared_ptr 保活）
    auto snapDoc = std::make_shared< QDomDocument >();
    QDomElement snapRoot = snapDoc->createElement(QStringLiteral("snapshot"));
    snapDoc->appendChild(snapRoot);  // 必须挂进文档，否则 documentElement() 读不到
    if (mViewStateStore) {
        mViewStateStore->toXml(*snapDoc, snapRoot);
    }
    std::unique_ptr< DACommandDataFrame_dropIColumn > cmd(new DACommandDataFrame_dropIColumn(mData.toDataFrame(), columns));
    // 编辑后效（008 期2）：结构通知 + 全量统计失效 + 计算列全标脏 + 关浮窗 + 宿主树刷新，
    // 收口到 commit（undo/redo 同走此回调；宿主树刷新随 undo 同步，与旧「仅 exec 后通知」
    // 相比把撤销后的树也刷新了——列子项随 undo 一致重建）
    DATableMutation mutation;
    mutation.kind               = DATableMutation::Structure;
    mutation.closeQuickPlot      = true;   // 删列后浮窗绑定的列身份失效，直接关闭（实施方案 §5.3）
    mutation.notifyHostValueChange = true;  // 数据管理器树重建列子项
    QPointer< DADataTableModel > modle = mModel;
    mutation.modelRefresh        = [ modle, columns ]() {
        if (modle) {
            modle->notifyColumnsRemoved(columns);
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    // 行列增删样式键同步：删除列时 ==col 的键删除，>col 的键 -1；撤销时按插入处理
    // 注意：被删除列的样式在删除时丢失，撤销(load恢复dataframe)无法恢复样式（已知限制，见 spec §7.3）
    // 条件格式规则同为装饰类位置键（R20），随列删除同款重映射（被删列的规则一并丢弃，003）
    DATableStyleManager* styleMgr = mStyleManager;
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    DATableNoteStore* noteStore   = mNoteStore;
    // 计算列联动（004 T1.23 R20）：被删列名在 exec 前解析；定义集合先快照——
    // undo 整店恢复（含被移除的定义与原状态），redo 重放删列联动
    //（目标列被删 → 定义移除；依赖列被删 → 置 Invalid，不静默清空列数据）
    DATableComputedColumnStore* ccStore = mComputedColumnStore;
    DATableViewStateStore* viewStore = mViewStateStore;
    QList< DATableComputedColumnStore::Entry > ccSnapshot;
    if (ccStore) {
        ccSnapshot = ccStore->snapshot();
    }
    auto invalidatedTargets = std::make_shared< QStringList >();
    auto self = QPointer< DADataOperateOfDataFrameWidget >(this);
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, ccStore, viewStore, snapDoc, columns,
                                   removedNames, ccSnapshot, invalidatedTargets, self ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsInserted(columns);
            } else {
                styleMgr->onColumnsRemoved(columns);
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsInserted(columns);
            } else {
                cfStore->onColumnsRemoved(columns);
            }
        }
        // 批注列维同款重映射（005 T1.26，被删列的批注一并丢弃）
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsInserted(columns);
            } else {
                noteStore->onColumnsRemoved(columns);
            }
        }
        if (ccStore) {
            if (isUndo) {
                ccStore->restore(ccSnapshot);
            } else {
                invalidatedTargets->clear();
                ccStore->onColumnsRemoved(removedNames, nullptr, invalidatedTargets.get());
            }
        }
        // 视图状态随命令往返：redo 清被删列条目（含冻结集合）；undo 整店快照还原
        if (viewStore) {
            if (isUndo) {
                const QDomElement vs = snapDoc->documentElement().firstChildElement(QStringLiteral("view-state"));
                if (!vs.isNull()) {
                    viewStore->fromXml(vs);
                } else {
                    viewStore->clear();
                }
            } else {
                for (const QString& n : removedNames) {
                    viewStore->removeColumn(n);
                }
            }
        }
        if (self) {
            self->applyViewStateFromStore();  // 冻结投影收敛（冻结列删光 → 窗格隐藏）
        }
    });
    if (!cmd->exec()) {
        return 0;
    }
    push(cmd.release());
    // 浮窗关闭与宿主树刷新已随 commit（exec 内）执行；此处只剩用户提示
    // 删列后提示失效的计算列（exec 内的 directional 回调已填充；不静默，R9）
    if (!invalidatedTargets->isEmpty()) {
        daWarning << tr("Computed column(s) %1 became invalid because their dependency column(s) were removed")
                         .arg(invalidatedTargets->join(QStringLiteral(", ")));  // cn:计算列 %1 因依赖列被删除已失效
    }
    return columns.size();
}

/**
 * @brief 设置选中单元格为nan,返回设置成功的个数（转发单元格算子）
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectCell()
{
    return mCellOps->removeSelectedCells();
}

/**
 * @brief 粘贴剪贴板内容到当前选区左上角（转发单元格算子）
 * @return 成功写入的单元格数（0 表示未写入）
 */
int DADataOperateOfDataFrameWidget::pasteFromClipboard()
{
    return mCellOps->pasteFromClipboard();
}

/**
 * @brief 剪切选中区（转发单元格算子）
 * @return 受影响单元格数
 */
int DADataOperateOfDataFrameWidget::cutSelection()
{
    return mCellOps->cutSelection();
}

/**
 * @brief 更改列名
 */
void DADataOperateOfDataFrameWidget::renameColumns()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    QList< QString > oldcols = df.columns();
    if (oldcols.size() <= 0) {
        daWarning << tr("Table has no columns");  // cn:表格没有列
        return;
    }
    DARenameColumnsNameDialog dlg(this);
    dlg.setDataName(mData.getName());
    dlg.setColumnsName(oldcols);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    QList< QString > cols = dlg.getColumnsName();
    // 唯一判断在DARenameColumnsNameDialog里进行
    // 改名对（old→new，仅变化的）
    QList< QPair< QString, QString > > renames;
    const int n = qMin(cols.size(), oldcols.size());
    for (int i = 0; i < n; ++i) {
        if (cols[ i ] != oldcols[ i ]) {
            renames.append(qMakePair(oldcols[ i ], cols[ i ]));
        }
    }
    // 计算列引用联动（004 T1.23 R20）：先提示影响面，确认后随改名命令同一撤销单元批量迁移
    if (!confirmComputedColumnRename(renames)) {
        return;
    }
    // 校验规则引用联动（005 T1.25 R20）：同款提示与迁移
    if (!confirmValidationRuleRename(renames)) {
        return;
    }
    // 表头刷新走模型通知入口（命令不再持有 QHeaderView*）
    DACommandDataFrame_renameColumns* cmd = new DACommandDataFrame_renameColumns(df, cols, oldcols, mModel);
    wireComputedColumnRenameMigration(cmd, renames);
    wireValidationRuleRenameMigration(cmd, renames);
    if (!cmd->exec()) {
        delete cmd;
        return;
    }
    // 列改名迁移：视图状态按列名引用（R20），宽度记录随改名同步迁移
    if (mViewStateStore) {
        for (const auto& p : std::as_const(renames)) {
            mViewStateStore->renameColumn(p.first, p.second);
        }
    }
    //! 通知datamanager
    if (DADataManager* mgr = mData.getDataManager()) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
    }

    push(cmd);
}

/**
 * @brief 重命名单列
 *
 * 构造"全表列名列表，仅修改目标列"传给 DACommandDataFrame_renameColumns，
 * 复用现有命令的 undo/redo 与 headerDataChanged 通知机制。
 * @param col 列位置索引
 * @param newName 新列名
 * @return 成功返回 true；列为空/重名/越界/exec 失败返回 false
 */
bool DADataOperateOfDataFrameWidget::renameColumn(int col, const QString& newName)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return false;
    }
    QList< QString > oldcols = df.columns();
    if (col < 0 || col >= oldcols.size()) {
        return false;
    }
    if (newName.isEmpty()) {
        daWarning << tr("Column name cannot be empty");  // cn:列名不能为空
        return false;
    }
    if (oldcols.contains(newName)) {
        daWarning << tr("Column name \"%1\" already exists, please use another name").arg(newName);  // cn:列名"%1"已存在，请使用其他名称
        return false;
    }
    QList< QString > newcols = oldcols;
    newcols[ col ]           = newName;
    // 计算列引用联动（004 T1.23 R20）：先提示影响面，确认后随改名命令同一撤销单元批量迁移
    const QList< QPair< QString, QString > > renames { qMakePair(oldcols[ col ], newName) };
    if (!confirmComputedColumnRename(renames)) {
        return false;
    }
    // 校验规则引用联动（005 T1.25 R20）
    if (!confirmValidationRuleRename(renames)) {
        return false;
    }
    std::unique_ptr< DACommandDataFrame_renameColumns > cmd(
        new DACommandDataFrame_renameColumns(df, newcols, oldcols, mModel));
    wireComputedColumnRenameMigration(cmd.get(), renames);
    wireValidationRuleRenameMigration(cmd.get(), renames);
    // 视图状态迁移（列宽/列序/隐藏按列名键）：挂 directional 回调 redo/undo 双向
    // 迁移（链式包裹前两个迁移，同 wireValidationRuleRenameMigration 手法）——
    // 旧实现只在页面 exec 后单向迁移，undo 后 store 里还是新列名，后续投影按
    // 孤儿键跳过，列宽/隐藏/冻结状态错位
    if (mViewStateStore) {
        DATableViewStateStore* vsStore = mViewStateStore;
        DACallBackInterface::DirectionalCallBack prev = cmd->getDirectionalCallBack();
        cmd->setDirectionalCallBack([ prev, vsStore, renames ](bool isUndo) {
            if (prev) {
                prev(isUndo);
            }
            for (const auto& p : std::as_const(renames)) {
                vsStore->renameColumn(isUndo ? p.second : p.first, isUndo ? p.first : p.second);
            }
        });
    }
    if (!cmd->exec()) {
        return false;
    }
    // 列改名后模型列名缓存随命令 exec/undo 的 refreshData 重建（命令内完成），
    // 这里补视图状态投影（模型 reset 触发的 applyViewStateFromStore 用新键命中）
    if (DADataManager* mgr = mData.getDataManager()) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
    }
    push(cmd.release());
    applyViewStateFromStore();
    return true;
}

/**
 * @brief 设置选择列的数据类型
 * @param dtypeName
 * @return 成功改变类型返回true
 */
bool DADataOperateOfDataFrameWidget::changeSelectColumnType(const DAPyDType& dt)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        emit selectTypeChanged({ }, DAPyDType());
        return false;
    }
    QList< int > selColumns = getSelectedDataframeCoumns();
    if (selColumns.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        emit selectTypeChanged({ }, DAPyDType());
        return false;
    }
    std::unique_ptr< DACommandDataFrame_astype > cmd(new DACommandDataFrame_astype(df, selColumns, dt));
    // 编辑后效（008 期2）：类型转换改整列值 → 受影响列统计失效 + 计算列标脏，收口到 commit
    QPointer< DADataTableModel > modle = mModel;
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedCols = selColumns;
    mutation.modelRefresh = [ modle, selColumns ]() {
        if (modle) {
            for (int c : std::as_const(selColumns)) {
                modle->notifyColumnChanged(c);
            }
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        // 说明没有设置成功
        emit selectTypeChanged({ }, DAPyDType());
        return false;
    }
    push(cmd.release());  // 成功，push会执行redo但会跳过
    emit selectTypeChanged(selColumns, dt);
    // 这里说明设置成功了
    return true;
}

/**
 * @brief 把选择的列转换为数值
 */
void DADataOperateOfDataFrameWidget::castSelectToNum()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    QList< int > colsIndex = getSelectedDataframeCoumns();
    if (colsIndex.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    if (mDialogCastNumArgs == nullptr) {
        mDialogCastNumArgs = new DADialogDataframeColumnCastToNumeric(this);
    }
    if (QDialog::Accepted != mDialogCastNumArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypeObject(colsIndex.first());
    pybind11::dict args = mDialogCastNumArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castNum > cmd(new DACommandDataFrame_castNum(df, colsIndex, args));
    // 编辑后效（008 期2）：数值转换改整列值 → 受影响列统计失效 + 计算列标脏，收口到 commit
    QPointer< DADataTableModel > modle = mModel;
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedCols = colsIndex;
    mutation.modelRefresh = [ modle, colsIndex ]() {
        if (modle) {
            for (int c : std::as_const(colsIndex)) {
                modle->notifyColumnChanged(c);
            }
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        return;
    }
    push(cmd.release());  // 推入后不会执行redo逻辑部分
    DAPyDType dt2 = df.dtypeObject(colsIndex.first());
    if (dt != dt2) {
        emit selectTypeChanged(colsIndex, dt2);
    }
}

/**
 * @brief 把选择的列转换为日期
 */
void DADataOperateOfDataFrameWidget::castSelectToDatetime()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    QList< int > colsIndex = getSelectedDataframeCoumns();
    if (colsIndex.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    if (mDialogCastDatetimeArgs == nullptr) {
        mDialogCastDatetimeArgs = new DADialogDataframeColumnCastToDatetime(this);
    }
    if (QDialog::Accepted != mDialogCastDatetimeArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypeObject(colsIndex.first());
    pybind11::dict args = mDialogCastDatetimeArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castDatetime > cmd(new DACommandDataFrame_castDatetime(df, colsIndex, args));
    // 编辑后效（008 期2）：日期转换改整列值 → 受影响列统计失效 + 计算列标脏，收口到 commit
    QPointer< DADataTableModel > modle = mModel;
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedCols = colsIndex;
    mutation.modelRefresh = [ modle, colsIndex ]() {
        if (modle) {
            for (int c : std::as_const(colsIndex)) {
                modle->notifyColumnChanged(c);
            }
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        return;
    }
    push(cmd.release());
    DAPyDType dt2 = df.dtypeObject(colsIndex.first());
    if (dt != dt2) {
        emit selectTypeChanged(colsIndex, dt2);
    }
}

/**
 * @brief 把选择的列转换为索引
 * @return
 */
bool DADataOperateOfDataFrameWidget::changeSelectColumnToIndex()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return false;
    }
    QList< int > colsIndex = getSelectedDataframeCoumns();
    if (colsIndex.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return false;
    }
    std::unique_ptr< DACommandDataFrame_setIndex > cmd = std::make_unique< DACommandDataFrame_setIndex >(df, colsIndex);
    // 编辑后效（008 期2）：列转索引是结构性整表操作 → 模型 reset + 全量失效 +
    // 计算列全标脏，收口到 commit（FullReset）
    DATableMutation mutation;
    mutation.kind = DATableMutation::FullReset;
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        return false;
    }
    push(cmd.release());  // 推入后不会执行redo逻辑部分
    return true;
}

/**
 * @brief 创建一个数据描述
 * @return
 */
DAPyDataFrame DADataOperateOfDataFrameWidget::createDataDescribe()
{
    if (!mData.isDataFrame()) {
        return DAPyDataFrame();
    }
    DAPyDataFrame df_describe = mData.toDataFrame().describe();
    return df_describe;
}

/**
 * @brief dataframe表格是否有选中项
 * @return
 */
bool DADataOperateOfDataFrameWidget::isDataframeTableHaveSelection() const
{
    return mSelection->haveSelection();
}

/**
 * @brief 获取dataframe表格的tableview
 *
 * @return DADataTableView*
 */
DADataTableView* DADataOperateOfDataFrameWidget::getDataTableView() const
{
    return ui ? ui->tableView : nullptr;
}

/**
 * @brief 返回当前选中单元格所包含的列数，列数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeCoumns(bool ensureInDataframe) const
{
    return mSelection->selectedDataframeColumns(ensureInDataframe);
}

/**
 * @brief 返回当前选中单元格所包含的行数，行数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeRows(bool ensureInDataframe) const
{
    return mSelection->selectedDataframeRows(ensureInDataframe);
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
    return mContext->toActualRow(viewRow);
}

/**
 * @brief 数据源真实数据行数（不含尾部预留的空白行）
 * @return 行数
 */
int DADataOperateOfDataFrameWidget::dataRowCount() const
{
    return mContext->dataRowCount();
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
    return mSelection->fullySelectedDataframeColumns(ensureInDataframe);
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
    return mSelection->fullySelectedDataframeRows(ensureInDataframe);
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeRow(bool ensureInDataframe) const
{
    return mSelection->oneSelectedDataframeRow(ensureInDataframe);
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeColumn(bool ensureInDataframe) const
{
    return mSelection->oneSelectedDataframeColumn(ensureInDataframe);
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
    return mSelection->selectedSeries();
}

void DADataOperateOfDataFrameWidget::refreshTable()
{
    if (mModel) {
        mModel->refreshData();
    }
}

/**
 * @brief 确保列可见
 * @param colName
 */
void DADataOperateOfDataFrameWidget::ensureColumnVisible(const QString& colName, bool selectCol)
{
    if (colName.isEmpty()) {
        return;
    }
    QTableView* tv = ui->tableView;
    if (!tv || !tv->model()) {
        return;
    }

    // 直接找列号
    int col       = -1;
    const auto* m = tv->model();
    for (int c = 0, total = m->columnCount(); c < total; ++c) {
        if (m->headerData(c, Qt::Horizontal).toString() == colName) {
            col = c;
            break;
        }
    }
    if (col < 0)
        return;

    if (mViewStateStore && !colName.isEmpty()) {
        // 隐藏列收口（007）：图表拾取/表头右键过来的"确保整列可见"若落在隐藏列上，
        // 不处理会静默失效——先走 006 的不入栈入口取消隐藏（occ = 0，第一同名）再整列选中
        if (mViewStateStore->isColumnHidden(colName, 0)) {
            setColumnHiddenDirectly(colName, 0, false);
            daInfo << tr("The column \"%1\" was hidden and has been made visible").arg(colName);  // cn:列"%1"已隐藏，现已显示（与 locateActualCell 复用同一句串，不造第二份）
        }
    }

    tv->scrollTo(m->index(0, col), QAbstractItemView::EnsureVisible);
    if (selectCol) {
        QItemSelectionModel* sel = tv->selectionModel();
        if (!sel) {
            return;
        }
        QModelIndex topLeft     = m->index(0, col);
        QModelIndex bottomRight = m->index(m->rowCount() - 1, col);
        QItemSelection selection(topLeft, bottomRight);

        sel->clearSelection();  // 去掉旧选区
        sel->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Columns);
    }
}

/**
 * @brief 定位单入口（007）：跳到「数据行 actualRow + 列 col」并选中
 *
 * 四步语义（顺序不可颠倒）：①越界夹取（无数据直接返回）→ ②隐藏列处置
 * （只有「要选中且不要求保持隐藏」才取消隐藏 + 提示，不入撤销栈）→ ③纵向一律
 * 主窗格 showActualRow（冻结窗格绝不调——placeViewportAt 会改模型窗口起始行，
 * 与主窗格抢窗口）→ ④横向按冻结区分流。
 * 两窗格共享同一个 selectionModel 时 setCurrentIndex 会同时惊动两个视图的
 * currentChanged → scrollTo；而 Qt 的 QTableView::scrollTo() 对本窗格隐藏的列
 * 直接 return（Qt 6.7.3 源码核对：函数开头 isColumnHidden 早退）——所以
 * "主区不为冻结列横滚""冻结区不为主区列横滚"天然成立，不需要额外守卫。
 * 只读：不写数据、不入撤销栈、不写工程；唯一例外是取消隐藏（setColumnHiddenDirectly）。
 * @param actualRow 数据源绝对行号（越界自动夹到数据范围）
 * @param col 列号
 * @param select false = 只摆位 + 移当前项（选区范围导航不收缩选区、不夺焦点）
 * @param keepHidden true = 保留目标列的隐藏状态（对话框勾选项）
 */
void DADataOperateOfDataFrameWidget::locateActualCell(int actualRow, int col, bool select, bool keepHidden)
{
    // ① 越界夹取：无数据直接返回（用户可见提示走对话框侧，见 007b）
    const int rows = dataRowCount();
    const int cols = dataColumnCount();
    if (rows <= 0 || cols <= 0) {
        return;
    }
    actualRow = qBound(0, actualRow, rows - 1);
    col       = qBound(0, col, cols - 1);

    DADataTableView* mainView = getDataTableView();  // 006 约定 = 主窗格（模型窗口与滚动位置的唯一驱动者）
    if (!mainView || !mainView->model()) {
        return;
    }

    // ② 隐藏列处置：只有「要选中、且不要求保持隐藏」的定位才取消隐藏
    //（select=false 的查找选区导航、keepHidden=true 的对话框勾选项都保持原可见性）
    if (select && !keepHidden && mViewStateStore) {
        const QString name = mainView->actualColumnName(col);
        if (!name.isEmpty()) {  // 空列名（尾部预留空白列）无身份，先判掉再查 store
            int occ = 0;        // occurrence = 该列之前同名列个数（数据列序中计，
            for (int c = 0; c < col; ++c) {  // 与 applyViewStateFromStore/onHeaderSectionResized 同法，
                if (mainView->model()->headerData(c, Qt::Horizontal).toString() == name) {
                    ++occ;  // 不要按视图顺序算）
                }
            }
            if (mViewStateStore->isColumnHidden(name, occ)) {
                setColumnHiddenDirectly(name, occ, false);  // 006 的不入栈入口：写真源 + 统一重投影
                daInfo << tr("The column \"%1\" was hidden and has been made visible").arg(name);  // cn:列"%1"已隐藏，现已显示
            }
        }
    }

    // ③ 纵向一律走主窗格：showActualRow = placeViewportAt（目标行落到视口首行，
    //    贴数据两端时收紧）
    mainView->showActualRow(actualRow);

    // ④ 横向：冻结区判定用 006 的 isColumnInFrozenArea（真源唯一，不要自己重算）；
    //    判定放在 ② 之后——取消隐藏会改变"视图顺序前 m 个可见列"，冻结集合随之变
    if (isColumnInFrozenArea(col)) {
        // 冻结区：不调 showColumn。QTableView::showColumn = horizontalHeader()->showSection()，
        // 会静默解除该列在主窗格的隐藏（绕开真源与投影）→ 手动三步：摆位（已做）→
        // 共享 selectionModel 的 setCurrentIndex（flags 随 select）→ 焦点给包含该列的窗格
        //（设计稿 §3.4.4：焦点决定 Ctrl+C/编辑在哪个窗格生效）
        mainView->selectionModel()->setCurrentIndex(
            mainView->model()->index(mainView->toLogicalRow(actualRow), col),
            select ? QItemSelectionModel::ClearAndSelect : QItemSelectionModel::NoUpdate);
        if (select) {
            if (DATablePaneContainer* c = paneContainer()) {
                if (DADataTableView* frozen = c->frozenView()) {
                    frozen->setFocus();
                }
            }
        }
    } else if (select) {
        // 主区：复用既有语义（内部 showActualRow + showColumn + setCurrentIndex(ClearAndSelect)）；
        // 此分支成立的前提是 ② 已把列解除隐藏。
        // ⚠️ 已知边界：keepHidden=true 且列既隐藏又不在冻结区时，这里的 showColumn 会把
        //    该列重新显示——勾选项的语义是"别主动解隐藏"，而主区定位本身要求列可见；
        //    v1 接受这个结果，不要为此在主区分支手工改写可见性（那会变成第二条真源路径）
        mainView->selectActualCell(actualRow, col);
    } else {
        // select=false（查找的"选区范围"导航）：不调 showColumn、不收缩选区，只移当前项；
        // 焦点不动（与既有 gotoFindHit 的选区范围分支一致，查找条继续响应上下导航键）
        mainView->selectionModel()->setCurrentIndex(
            mainView->model()->index(mainView->toLogicalRow(actualRow), col),
            QItemSelectionModel::NoUpdate);
    }
}

/**
 * @brief 定位单入口的列名重载：重名取第一个同名列（数据列序）
 *
 * 找不到列名时静默返回——用户可见提示归对话框路径（007b 的 resolveGotoRequest）。
 * @param actualRow 数据源绝对行号
 * @param columnName 列名
 * @param select 同列号重载
 * @param keepHidden 同列号重载
 */
void DADataOperateOfDataFrameWidget::locateActualCell(int actualRow, const QString& columnName, bool select,
                                                      bool keepHidden)
{
    if (columnName.isEmpty() || !mModel) {
        return;
    }
    int col = -1;  // 重名取第一个同名列（数据列序）
    for (int c = 0; c < dataColumnCount(); ++c) {
        if (mModel->headerData(c, Qt::Horizontal).toString() == columnName) {
            col = c;
            break;
        }
    }
    if (col < 0) {
        return;
    }
    locateActualCell(actualRow, col, select, keepHidden);
}

QList< QPoint > DADataOperateOfDataFrameWidget::getSelectedDataframeCells(bool ensureInDataframe) const
{
    return mSelection->selectedDataframeCells(ensureInDataframe);
}

/**
 * @brief 数据源真实数据列数（不含尾部预留的空白列）
 * @return 列数
 */
int DADataOperateOfDataFrameWidget::dataColumnCount() const
{
    return mContext->dataColumnCount();
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
    return mSelection->actualColumnIdentities();
}

/**
 * @brief 表格点击
 * @param index
 */
void DADataOperateOfDataFrameWidget::onTableViewClicked(const QModelIndex& index)
{
    if (!mData.isDataFrame()) {
        emit selectTypeChanged({ index.column() }, DAPyDType());
        return;
    }
    DAPyDataFrame df = mData.toDataFrame();
    if (index.column() >= (int)df.shape().second) {
        emit selectTypeChanged({ index.column() }, DAPyDType());
        return;
    }
    DAPyDType t;
    try {
        t = df.dtypeObject(index.column());
    } catch (const std::exception& e) {
        qCritical() << e.what();
    }

    emit selectTypeChanged({ index.column() }, t);
}

/**
 * @brief 获取样式管理器
 * @return 样式管理器指针
 */
DATableStyleManager* DADataOperateOfDataFrameWidget::styleManager() const
{
    return mStyleManager;
}

/**
 * @brief 获取视图状态子存储
 * @return 视图状态子存储指针（借自元数据束，registry 为空时为 nullptr）
 */
DATableViewStateStore* DADataOperateOfDataFrameWidget::viewStateStore() const
{
    return mViewStateStore;
}

/**
 * @brief 获取瞬态高亮层
 * @return 高亮层指针（页级持有，非空）
 */
DATableHighlightOverlay* DADataOperateOfDataFrameWidget::highlightOverlay() const
{
    return mHighlightOverlay;
}

/**
 * @brief 获取条件格式子存储
 * @return 条件格式子存储指针（借自元数据束，registry 为空时为 nullptr）
 */
DATableConditionalFormatStore* DADataOperateOfDataFrameWidget::conditionalFormatStore() const
{
    return mConditionalFormatStore;
}

/**
 * @brief 选择模型（选区快照 API，007 状态栏统计）
 *
 * 只取主窗格（冻结窗格共享同一个 selectionModel）。
 * @return 选择模型指针（无视图时 nullptr）
 */
QItemSelectionModel* DADataOperateOfDataFrameWidget::tableSelectionModel() const
{
    return mSelection->selectionModel();
}

/**
 * @brief 选区段数
 *
 * 选区按连续段组织（Ctrl+多点形成多段）；0=无选区，>1=多段（状态栏只给几何计数）。
 * @return 段数
 */
int DADataOperateOfDataFrameWidget::selectionSegmentCount() const
{
    return mSelection->selectionSegmentCount();
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
    return mSelection->selectionBoundingBox(rowBegin, rowEnd, colBegin, colEnd);
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
    return mSelection->selectedCellCount();
}

/**
 * @brief 从视图状态子存储投影到窗格（转发视图状态控制器；唯一投影路径）
 */
void DADataOperateOfDataFrameWidget::applyViewStateFromStore()
{
    mViewStateController->applyFromStore();
}

/**
 * @brief 当前选区为单格时取其显示文本（D9 预填用）
 * @return 单格显示文本（Qt::DisplayRole）；选区非单格/无选区返回空串
 */
QString DADataOperateOfDataFrameWidget::currentSingleCellDisplayText() const
{
    return mSelection->currentSingleCellDisplayText();
}

/**
 * @brief 本页所有需要投影的窗格
 *
 * 主窗格在前；冻结窗格懒建后追加（006e）。
 * @return 窗格列表（非空）
 */
QList< DADataTableView* > DADataOperateOfDataFrameWidget::paneViews() const
{
    return mContext->panes();
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
    return mViewStateController->isColumnInFrozenArea(logicalColumn);
}

/**
 * @brief 列轴窗格容器只读访问器（测试与页面内部接线用；APP 不使用、不改其调用点形状）
 */
DATablePaneContainer* DADataOperateOfDataFrameWidget::paneContainer() const
{
    return mPaneContainer;
}

/**
 * @brief 列视图直接入口：只写 store + 重投影，不入撤销栈（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::setColumnHiddenDirectly(const QString& name, int occurrence, bool hidden)
{
    mColumnViewController->setColumnHiddenDirectly(name, occurrence, hidden);
}

/**
 * @brief 生效冻结列数（转发列视图控制器）
 */
int DADataOperateOfDataFrameWidget::effectiveFrozenColumnCount() const
{
    return mColumnViewController->effectiveFrozenColumnCount();
}

/**
 * @brief 隐藏指定列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::hideColumnAt(int logicalColumn)
{
    mColumnViewController->hideColumnAt(logicalColumn);
}

/**
 * @brief 显示所有列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::showAllColumns()
{
    mColumnViewController->showAllColumns();
}

/**
 * @brief 冻结/取消冻结指定列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::setColumnFrozen(int logicalColumn, bool frozen)
{
    mColumnViewController->setColumnFrozen(logicalColumn, frozen);
}

/**
 * @brief 冻结首列（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::freezeFirstColumn()
{
    mColumnViewController->freezeFirstColumn();
}

/**
 * @brief 取消全部冻结（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::unfreezeAllColumns()
{
    mColumnViewController->unfreezeAllColumns();
}

/**
 * @brief 弹「冻结前 N 列」输入框（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::promptFreezeColumns()
{
    mColumnViewController->promptFreezeColumns();
}

/**
 * @brief 拖动落库入栈（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::applyColumnOrderFromView(const QVector< int >& newOrderLogical)
{
    mColumnViewController->applyColumnOrderFromView(newOrderLogical);
}

/**
 * @brief 冻结区内部拖动落库入栈（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::applyFrozenOrderFromView(const QVector< int >& newFrozenOrderLogical)
{
    mColumnViewController->applyFrozenOrderFromView(newFrozenOrderLogical);
}

/**
 * @brief 「列」子菜单动作分发（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::onHeaderColumnViewActionRequested(int logicalColumn,
                                                                        DA::DADataHeaderView::ColumnViewAction action)
{
    mColumnViewController->onHeaderActionRequested(logicalColumn, static_cast< int >(action));
}

/**
 * @brief 打开列视图对话框（转发列视图控制器）
 */
void DADataOperateOfDataFrameWidget::showColumnViewDialog()
{
    mColumnViewController->showDialog();
}

/**
 * @brief 解析定位请求并调定位单入口（007 T1.7）
 *
 * 行解析 → 列解析 → 只调 locateActualCell（不自己写滚动/定位）。返回 false =
 * 未解析成功（已给用户可见提示、未改选区）。行列口径只用 dataRowCount()/
 * dataColumnCount()；currentIndex() 的行是滑动窗逻辑行，经 toActualRow 换算。
 * @param req 定位请求
 * @return 是否定位成功
 */
bool DADataOperateOfDataFrameWidget::resolveGotoRequest(const DADialogGotoCell::Request& req)
{
    if (!mModel || dataRowCount() <= 0 || dataColumnCount() <= 0) {
        return false;
    }
    // ① 行解析
    int actualRow = 0;
    switch (req.rowKind) {
    case DADialogGotoCell::Request::RowKind::FirstRow:
        actualRow = 0;
        break;
    case DADialogGotoCell::Request::RowKind::LastRow:
        actualRow = dataRowCount() - 1;
        break;
    case DADialogGotoCell::Request::RowKind::Position:
        if (req.rowPosition < 1 || req.rowPosition > dataRowCount()) {
            daWarning << tr("Row number is out of range");  // cn:行号超出范围
            return false;
        }
        actualRow = req.rowPosition - 1;
        break;
    case DADialogGotoCell::Request::RowKind::None: {
        // 保持当前行（当前项无效 → 第 0 行）
        const QModelIndex cur = currentOrSelectionTopLeftIndex();
        actualRow = cur.isValid() ? toActualRow(cur.row()) : 0;
        break;
    }
    case DADialogGotoCell::Request::RowKind::IndexLabel: {
        DAPyIndex index                    = getDataframe().index();
        DAPyIndex::IndexLabelLocations loc = index.findLabelLocations(DA::PY::toPyObject(req.indexLabel));
        if (!loc.found) {  // 数值索引：字符串→整数/浮点再试一次（同一个查询路径）
            bool ok = false;
            const long long asInt = req.indexLabel.toLongLong(&ok);
            if (ok) {
                loc = index.findLabelLocations(pybind11::int_(asInt));
            } else {
                const double asDouble = req.indexLabel.toDouble(&ok);
                if (ok) {
                    loc = index.findLabelLocations(pybind11::float_(asDouble));
                }
            }
        }
        if (!loc.found) {
            daWarning << tr("Index label not found: %1").arg(req.indexLabel);  // cn:未找到索引标签：%1
            return false;
        }
        if (loc.count > 1) {
            daInfo << tr("There are %1 rows with this index label; jumped to the first one").arg(loc.count);  // cn:有 %1 行使用该索引标签，已跳到第一个
        }
        actualRow = static_cast< int >(loc.firstPos);
        break;
    }
    }
    // ② 列解析
    int col = 0;
    switch (req.columnKind) {
    case DADialogGotoCell::Request::ColumnKind::FirstColumn:
        col = 0;
        break;
    case DADialogGotoCell::Request::ColumnKind::LastColumn:
        col = dataColumnCount() - 1;
        break;
    case DADialogGotoCell::Request::ColumnKind::Name: {
        int found    = -1;  // 重名取第一个（数据列序）
        int nameHits = 0;
        for (int c = 0; c < dataColumnCount(); ++c) {
            if (mModel->headerData(c, Qt::Horizontal).toString() == req.columnName) {
                if (found < 0) {
                    found = c;
                }
                ++nameHits;
            }
        }
        if (found < 0) {
            daWarning << tr("Column not found: %1").arg(req.columnName);  // cn:未找到列名：%1
            return false;
        }
        if (nameHits > 1) {
            daInfo << tr("There are %1 columns with this name; used the first one").arg(nameHits);  // cn:有 %1 个同名列，已取第一个
        }
        col = found;
        break;
    }
    case DADialogGotoCell::Request::ColumnKind::None: {
        // 保持当前列（当前项无效 → 第 0 列）
        const QModelIndex cur = currentOrSelectionTopLeftIndex();
        col = cur.isValid() ? cur.column() : 0;
        break;
    }
    }
    // ③ 收尾：定位单入口（keepHidden 透传）
    locateActualCell(actualRow, col, /*select=*/true, /*keepHidden=*/req.keepHidden);
    return true;
}

/**
 * @brief 当前单元格模型索引
 *
 * selectionModel()->currentIndex()，无效时取选区左上角；都没有返回无效索引。
 * @return 模型索引
 */
QModelIndex DADataOperateOfDataFrameWidget::currentOrSelectionTopLeftIndex() const
{
    return mSelection->currentOrSelectionTopLeftIndex();
}

/**
 * @brief 打开定位对话框（007 T1.7）
 *
 * 懒建复用；setup 重置初值（不回显残留）；exec 模态，确定后解析请求。
 * 无数据页静默返回。
 */
void DADataOperateOfDataFrameWidget::showGotoDialog()
{
    if (!mModel || dataRowCount() <= 0 || dataColumnCount() <= 0) {
        return;
    }
    if (!mGotoDialog) {
        mGotoDialog = new DADialogGotoCell(this);
    }
    QStringList columnNames;  // 重名列保留重复项（不做去重）
    for (int c = 0; c < dataColumnCount(); ++c) {
        columnNames.append(mModel->headerData(c, Qt::Horizontal).toString());
    }
    const QModelIndex cur        = currentOrSelectionTopLeftIndex();
    const int currentRowPosition = cur.isValid() ? toActualRow(cur.row()) + 1 : 1;
    const QString currentColName = cur.isValid() ? mModel->headerData(cur.column(), Qt::Horizontal).toString() : QString();
    const bool hasHiddenColumns  = (mViewStateStore && !mViewStateStore->hiddenColumns().isEmpty());
    mGotoDialog->setup(dataRowCount(), columnNames, currentRowPosition, currentColName, hasHiddenColumns);
    if (mGotoDialog->exec() != QDialog::Accepted) {
        return;
    }
    resolveGotoRequest(mGotoDialog->request());
}

/**
 * @brief 采集列宽改动写入视图状态子存储（转发视图状态控制器，008 期3）
 */
void DADataOperateOfDataFrameWidget::onHeaderSectionResized(DADataTableView* source, int logicalIndex, int newSize)
{
    mViewStateController->onHeaderSectionResized(source, logicalIndex, newSize);
}

// ============================================================
// 样式与列显示格式（实现迁入 DATableStyleController，008 期3）
// ============================================================

/**
 * @brief 合并样式片段到选中区（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::mergeStyleToSelection(const DATableCellStyle& fragment)
{
    mStyleController->mergeStyleToSelection(fragment);
}

/**
 * @brief 设置完整样式到选中区（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::applyStyleToSelection(const DATableCellStyle& style)
{
    mStyleController->applyStyleToSelection(style);
}

/**
 * @brief 清除选中区样式（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::clearStyleSelection()
{
    mStyleController->clearStyleSelection();
}

/**
 * @brief 清除整表所有样式（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::clearStyleAll()
{
    mStyleController->clearStyleAll();
}

/**
 * @brief 获取选中区代表的样式（转发样式控制器）
 */
DATableCellStyle DADataOperateOfDataFrameWidget::getCurrentCellStyle() const
{
    return mStyleController->getCurrentCellStyle();
}

/**
 * @brief 设置列显示格式到选中列（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::setDisplayFormatToSelection(const DATableDisplayFormat& fmt)
{
    mStyleController->setDisplayFormatToSelection(fmt);
}

/**
 * @brief 清除选中列的显示格式（转发样式控制器）
 */
void DADataOperateOfDataFrameWidget::clearDisplayFormatFromSelection()
{
    mStyleController->clearDisplayFormatFromSelection();
}

/**
 * @brief 获取选中列代表的显示格式（转发样式控制器）
 */
DATableDisplayFormat DADataOperateOfDataFrameWidget::getCurrentColumnDisplayFormat() const
{
    return mStyleController->getCurrentColumnDisplayFormat();
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
    mHeaderPickMode = on;
}

/**
 * @brief 是否处于表头拾取模式
 * @return 拾取模式返回 true
 */
bool DADataOperateOfDataFrameWidget::isHeaderPickMode() const
{
    return mHeaderPickMode;
}

/**
 * @brief 当前多列排序规格
 * @return 列逻辑索引+方向列表（顺序即优先级；不持久化 R13）
 */
QVector< QPair< int, Qt::SortOrder > > DADataOperateOfDataFrameWidget::sortSpec() const
{
    return mSortController->sortSpec();
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
    return mContext->isDataSourceEditable();
}

/**
 * @brief 表头带修饰键点击
 *
 * 2026-10-18 交互修订：**点击表头不再触发排序**（误触即整表重排、大表卡顿数秒，
 * 用户反馈修订；排序入口收敛到列头右键菜单）。本槽仅保留拾取模式转发：
 * - 拾取模式：转发 columnHeaderClicked（图表「选择序列」拾取链路零回归）；
 * - 非拾取模式：不做任何事（Qt 默认的列选中/高亮行为保留）。
 * @param logicalIndex 列逻辑索引
 * @param modifiers 修饰键
 */
void DADataOperateOfDataFrameWidget::onHeaderClickedWithModifiers(int logicalIndex, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers);
    if (mHeaderPickMode) {
        // 拾取模式：只转发信号（修饰键不吞，拾取侧按列填充表达式）
        Q_EMIT columnHeaderClicked(logicalIndex);
    }
}

// ============================================================
// 排序（实现迁入 DATableSortController，008 期3）
// ============================================================

/**
 * @brief 按列排序（转发排序控制器）
 */
void DADataOperateOfDataFrameWidget::sortColumn(int logicalIndex, Qt::SortOrder order)
{
    mSortController->sortColumn(logicalIndex, order);
}

/**
 * @brief 追加为次级排序列（转发排序控制器）
 */
void DADataOperateOfDataFrameWidget::addSecondarySortColumn(int logicalIndex, Qt::SortOrder order)
{
    mSortController->addSecondarySortColumn(logicalIndex, order);
}

// ============================================================
// 查找/替换（实现迁入 DATableFindController，008 期3）
// ============================================================

/**
 * @brief 打开查找/替换浮动条（转发查找控制器）
 */
void DADataOperateOfDataFrameWidget::showFindReplace(bool expandReplace, bool prefillFromCurrentCell)
{
    mFindController->showFindBar(expandReplace, prefillFromCurrentCell);
}

/**
 * @brief 关闭查找/替换浮动条（转发查找控制器）
 */
void DADataOperateOfDataFrameWidget::closeFindReplace()
{
    mFindController->closeFindBar();
}

/**
 * @brief 当前查找命中数（转发查找控制器）
 */
int DADataOperateOfDataFrameWidget::findHitCount() const
{
    return mFindController->hitCount();
}

/**
 * @brief 当前命中序号（转发查找控制器）
 */
int DADataOperateOfDataFrameWidget::currentFindHitIndex() const
{
    return mFindController->currentHitIndex();
}

// ============================================================
// 条件格式（实现迁入 DATableConditionalFormatController，008 期3）
// ============================================================

/**
 * @brief 打开条件格式规则编辑对话框（转发条件格式控制器）
 */
void DADataOperateOfDataFrameWidget::showConditionalFormatDialog(int initialType)
{
    mConditionalFormatController->showDialog(initialType);
}

/**
 * @brief 清除选中列的条件格式规则（转发条件格式控制器）
 */
void DADataOperateOfDataFrameWidget::clearConditionalFormatFromSelection()
{
    mConditionalFormatController->clearFromSelection();
}

/**
 * @brief 填充条件格式菜单（转发条件格式控制器）
 */
void DADataOperateOfDataFrameWidget::populateConditionalFormatMenu(QMenu* menu)
{
    mConditionalFormatController->populateMenu(menu);
}

void DADataOperateOfDataFrameWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
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
    return mComputedColumnStore;
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
    mMutation->markComputedColumnsStale(actualCols);
}

//（以下计算列能力转发 DATableComputedColumnController，008 期3；实现已迁入控制器）
/**
 * @brief 打开计算列对话框（转发计算列控制器）
 */
void DADataOperateOfDataFrameWidget::showComputedColumnDialog(int col)
{
    mComputedColumnController->showDialog(col);
}

/**
 * @brief 重算单个计算列（转发计算列控制器）
 */
bool DADataOperateOfDataFrameWidget::recomputeComputedColumn(int col)
{
    return mComputedColumnController->recompute(col);
}

/**
 * @brief 重算全部待重算/失效的计算列（转发计算列控制器）
 */
int DADataOperateOfDataFrameWidget::recomputeAllStaleComputedColumns()
{
    return mComputedColumnController->recomputeAllStale();
}

/**
 * @brief 移除某列的公式定义（转发计算列控制器）
 */
bool DADataOperateOfDataFrameWidget::removeComputedColumnFormula(int col)
{
    return mComputedColumnController->removeFormula(col);
}

/**
 * @brief 列改名确认（转发计算列控制器）
 */
bool DADataOperateOfDataFrameWidget::confirmComputedColumnRename(const QList< QPair< QString, QString > >& renames) const
{
    return mComputedColumnController->confirmRename(renames);
}

/**
 * @brief 把计算列定义迁移挂进改名命令的 directional 回调（转发计算列控制器）
 */
void DADataOperateOfDataFrameWidget::wireComputedColumnRenameMigration(
    DACommandDataFrame_renameColumns* cmd, const QList< QPair< QString, QString > >& renames) const
{
    mComputedColumnController->wireRenameMigration(cmd, renames);
}

//===================================================
// 分列 / 合并列（004 T1.9）
//===================================================

/**
 * @brief 打开分列对话框并执行（004 T1.9）
 *
 * col<0 时取当前选区的唯一数据列（多列/无选区提示）。落位规则：结果列插在原列
 * 右侧、原列保留（算子注释同步写明）。撤销用列级记录：命令持源列快照 + 结果列名
 * 清单，undo 删结果列并写回源列快照（D4，内存代价=一列）。预览与执行同一算子
 * （对话框已用 da_head_copy + da_split_column 试算，红线5）。
 * @param col 源列位置（-1=从选区解析）
 */
void DADataOperateOfDataFrameWidget::showSplitColumnDialog(int col)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !isTableEditable()) {
        daWarning << tr("Split Column requires an editable table");  // cn:分列需要可编辑的表格
        return;
    }
    int sourceCol = col;
    if (sourceCol < 0) {
        const QList< int > sel = getSelectedDataframeCoumns();
        if (sel.size() != 1) {
            daWarning << tr("Please select exactly one column to split");  // cn:请选择要拆分的单个列
            return;
        }
        sourceCol = sel.first();
    }
    DADialogSplitColumn dlg(df, sourceCol, this);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    const DACommandDataFrame_splitColumn::SplitParams params = dlg.params();
    DAPyScriptsDataFrame& py                                 = DAPyScripts::getDataFrame();
    // 源列快照（列级撤销记录，D4）
    const DAPySeries oldSource = py.getColumnCopy(df, sourceCol);
    if (oldSource.isNone()) {
        daWarning << tr("Failed to snapshot the source column");  // cn:源列快照失败
        return;
    }
    auto cmd = std::make_unique< DACommandDataFrame_splitColumn >(df, params, oldSource);
    QPointer< DADataTableModel > modle     = mModel;
    DATableStyleManager* styleMgr          = mStyleManager;
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    DATableNoteStore* noteStore            = mNoteStore;
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    DACommandDataFrame_splitColumn* cmdRaw = cmd.get();
    const int srcCol                       = sourceCol;
    cmd->setCallBack([ modle, cfStore, self, srcCol ]() {
        if (modle) {
            modle->notifyColumnsInserted({ srcCol + 1 });  // 结构刷新（cacheShape + refreshWindow）
        }
        if (cfStore) {
            cfStore->invalidateAllStats();  // 列位置整体位移，统计缓存保守全失效
        }
        if (self) {
            Q_EMIT self->dataModified();
        }
    });
    // 位置键元数据重映射（装饰类 R20）：结果列插入原列右侧 → >=插入点的键右移；undo 反向
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, cmdRaw, srcCol ](bool isUndo) {
        const int n = cmdRaw->insertedNames().size();
        if (n <= 0) {
            return;
        }
        QList< int > inserted;
        inserted.reserve(n);
        for (int i = 1; i <= n; ++i) {
            inserted.append(srcCol + i);
        }
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsRemoved(inserted);
            } else {
                styleMgr->onColumnsInserted(inserted);
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsRemoved(inserted);
            } else {
                cfStore->onColumnsInserted(inserted);
            }
        }
        // 批注列维同款重映射（005 T1.26）
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsRemoved(inserted);
            } else {
                noteStore->onColumnsInserted(inserted);
            }
        }
    });
    {
        DAWaitCursorScoped waitCursor;  // 10 万行分列实测亚秒级，仍加忙等待（验收 §4.4）
        if (!cmd->exec()) {
            daWarning << tr("Split column failed: %1").arg(cmd->lastErrorMessage());  // cn:分列失败：%1
            return;
        }
    }
    push(cmd.release());
}

/**
 * @brief 打开合并列对话框并执行（004 T1.9）
 *
 * 源列 = 当前选区数据列（>=2，顺序可在对话框内调整）。落位规则：结果列替换被合并
 * 列的位置（插到最左源列处，源列删除）。撤销用列级记录：命令持全部源列快照，
 * undo 删结果列并按原位置升序写回源列（D4）。预览与执行同一算子（红线5）。
 * 被合并列若被计算列依赖 → 相关计算列置失效并提示（与删列联动同款，004 T1.23）。
 */
void DADataOperateOfDataFrameWidget::showMergeColumnsDialog()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !isTableEditable()) {
        daWarning << tr("Merge Columns requires an editable table");  // cn:合并列需要可编辑的表格
        return;
    }
    QList< int > sel = getSelectedDataframeCoumns();
    if (sel.size() < 2) {
        daWarning << tr("Please select at least two columns to merge");  // cn:请选择至少两列进行合并
        return;
    }
    std::sort(sel.begin(), sel.end());
    DADialogMergeColumns dlg(df, sel, this);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    const DACommandDataFrame_mergeColumns::MergeParams params = dlg.params();
    DAPyScriptsDataFrame& py                                  = DAPyScripts::getDataFrame();
    // 源列快照（与 params.cols 顺序一一对应，列级撤销记录 D4）
    QList< DAPySeries > oldSources;
    oldSources.reserve(params.cols.size());
    for (int c : params.cols) {
        const DAPySeries s = py.getColumnCopy(df, c);
        if (s.isNone()) {
            daWarning << tr("Failed to snapshot the source columns");  // cn:源列快照失败
            return;
        }
        oldSources.append(s);
    }
    // 计算列联动（004 T1.23）：源列被删 → 依赖它的计算列失效；快照供 undo 恢复
    const QList< QString > colNames = df.columns();
    QStringList removedNames;
    for (int c : params.cols) {
        if (c >= 0 && c < colNames.size()) {
            removedNames.append(colNames.at(c));
        }
    }
    QList< DATableComputedColumnStore::Entry > ccSnapshot;
    QStringList invalidatedTargets;
    if (mComputedColumnStore) {
        ccSnapshot = mComputedColumnStore->snapshot();
        for (const QString& n : std::as_const(removedNames)) {
            const QStringList refs = mComputedColumnStore->definitionsReferencing(n);
            for (const QString& t : refs) {
                if (!invalidatedTargets.contains(t) && !removedNames.contains(t)) {
                    invalidatedTargets.append(t);
                }
            }
        }
    }
    auto cmd = std::make_unique< DACommandDataFrame_mergeColumns >(df, params, oldSources);
    QPointer< DADataTableModel > modle     = mModel;
    DATableStyleManager* styleMgr          = mStyleManager;
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    DATableNoteStore* noteStore            = mNoteStore;
    DATableComputedColumnStore* ccStore    = mComputedColumnStore;
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    const int insertPos                    = cmd->insertPosition();
    QList< int > srcCols                   = params.cols;
    std::sort(srcCols.begin(), srcCols.end());
    cmd->setCallBack([ modle, cfStore, self, insertPos ]() {
        if (modle) {
            modle->notifyColumnsRemoved({ insertPos });  // 结构刷新（cacheShape + refreshWindow）
        }
        if (cfStore) {
            cfStore->invalidateAllStats();  // 列位置整体位移，统计缓存保守全失效
        }
        if (self) {
            Q_EMIT self->dataModified();
        }
    });
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, ccStore, srcCols, insertPos, ccSnapshot, removedNames ](bool isUndo) {
        // 位置键元数据（装饰类 R20）：exec=删源列+插结果列；undo=删结果列+回插源列
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsRemoved({ insertPos });
                styleMgr->onColumnsInserted(srcCols);
            } else {
                styleMgr->onColumnsRemoved(srcCols);
                styleMgr->onColumnsInserted({ insertPos });
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsRemoved({ insertPos });
                cfStore->onColumnsInserted(srcCols);
            } else {
                cfStore->onColumnsRemoved(srcCols);
                cfStore->onColumnsInserted({ insertPos });
            }
        }
        // 批注列维同款重映射（005 T1.26）：源列批注随删列丢弃，结果列位置右移的键回位
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsRemoved({ insertPos });
                noteStore->onColumnsInserted(srcCols);
            } else {
                noteStore->onColumnsRemoved(srcCols);
                noteStore->onColumnsInserted({ insertPos });
            }
        }
        // 计算列（语义类，列名引用）：exec=删列联动；undo=整店快照恢复
        if (ccStore) {
            if (isUndo) {
                ccStore->restore(ccSnapshot);
            } else {
                ccStore->onColumnsRemoved(removedNames);
            }
        }
    });
    {
        DAWaitCursorScoped waitCursor;
        if (!cmd->exec()) {
            daWarning << tr("Merge columns failed: %1").arg(cmd->lastErrorMessage());  // cn:合并列失败：%1
            return;
        }
    }
    push(cmd.release());
    // 失效提示（不静默，004 T1.23 R9）
    if (!invalidatedTargets.isEmpty()) {
        daWarning << tr("Computed column(s) %1 became invalid because their dependency column(s) were merged")
                         .arg(invalidatedTargets.join(QStringLiteral(", ")));  // cn:计算列 %1 因依赖列被合并已失效
    }
}

//===================================================
// 分组聚合（004 T1.22）
//===================================================

/**
 * @brief 打开分组聚合对话框并产出新数据集（004 T1.22，v1 只出长格式结果表）
 *
 * 产出流程照透视表的真实流程（DataframeOperateWorker::createPivotTable）：
 * 聚合结果封装为 DAData → 命名 xxx_GroupBy（重名由 addData_ 的既有唯一化策略处理）→
 * DADataManager::addData_ 注册（可撤销：注册命令进 DADataManager 自己的撤销栈，
 * 该栈已由 APP 接入统一撤销组）→ 发 newDataCreated 交 APP 抬起数据区并激活新页面。
 * 原表与其元数据完全不动（非破坏，R14 同款口径）；不做折叠视图（R15）。
 */
void DADataOperateOfDataFrameWidget::showGroupByDialog()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        // 开发诊断：非 dataframe（只读惰性表无 python 数据体）
        qWarning() << "DADataOperateOfDataFrameWidget::showGroupByDialog: dataframe unavailable";
        return;
    }
    DADialogGroupBy dlg(df, this);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    DAPyDataFrame result = dlg.result();
    if (result.isNone()) {
        return;
    }
    // 注册新数据集（addData_ 可撤销 + newDataCreated 交 APP 激活新页面，与转置/melt 共用管线）
    registerNewDataset(result,
                       tr("%1_GroupBy").arg(mData.getName()),                      // cn:%1_分组聚合
                       tr("Group-by aggregation of %1").arg(mData.getName()));      // cn:生成%1的分组聚合结果
}

//===================================================
// 转置 / melt 形状变换（004 T1.27）
//===================================================

/**
 * @brief 就地替换撤销内存判定（静态纯函数，单测覆盖）
 *
 * Swap 命令旧对象驻留内存 ≈ 1× 表内存（§4.11 策略表）；单元格数超阈值时
 * 建议降级为「本次操作不可撤销」（二次确认后直接替换）。
 */
bool DADataOperateOfDataFrameWidget::isInplaceUndoTooLarge(qint64 rows, qint64 cols)
{
    if (rows <= 0 || cols <= 0) {
        return false;
    }
    return rows * cols > c_inplaceUndoCellLimit;
}

/**
 * @brief 注册新数据集（分组聚合/转置/melt 默认路径共用）
 *
 * 照透视表 createPivotTable 的真实产出流程：命名（重名由 addData_ 的既有唯一化
 * 策略处理）→ DADataManager::addData_（可撤销注册，命令进管理器自己的撤销栈，
 * 已由 APP 接入统一撤销组）→ 发 newDataCreated 交 APP 置脏/抬区/激活新页面。
 */
void DADataOperateOfDataFrameWidget::registerNewDataset(const DAPyDataFrame& resultDf, const QString& name, const QString& describe)
{
    if (resultDf.isNone()) {
        return;
    }
    DAData newData(resultDf);
    newData.setName(name);
    newData.setDescribe(describe);
    DADataManager* mgr = mData.getDataManager();
    if (!mgr) {
        // 开发诊断：无数据管理器（正常 APP 链路数据都来自管理器）
        qWarning() << "DADataOperateOfDataFrameWidget::registerNewDataset: no data manager, result not registered";
        return;
    }
    mgr->addData_(newData);  // 可撤销注册（别用不带下划线的 addData，计划 §5.3-2/§6.3-1）
    Q_EMIT newDataCreated(newData);
}

/**
 * @brief 就地替换（004 T1.27 勾选「就地替换当前表」路径）
 *
 * 流程：① 确认框**列出将被清除的元数据类别与条目数**（§6.3-2 强制）→
 * ② 整束元数据 XML 快照 → ③ 超大表（单元格数 > c_inplaceUndoCellLimit）提示降级
 * 「本次操作不可撤销」，确认后直接替换 + 清束（§4.11 策略表）→ ④ 常规路径发
 * inplaceDataReplaceRequested 交 APP 经 DAAppCommand::beginDataOperateCommand
 * (isObjectPersist=false) 建 Swap 命令（统一撤销链，不在 DAGui new），元数据清理
 * 与数据替换经命令 directional 回调构成同一撤销单元。
 */
void DADataOperateOfDataFrameWidget::requestInplaceReplace(const DAPyDataFrame& newDf, const QString& text)
{
    if (newDf.isNone()) {
        return;
    }
    // ① 确认框：列出将被清除的元数据类别与条目数（不能只说一句"将清除样式"）
    QStringList lines;
    if (mMetadata) {
        const QList< QPair< QString, int > > counts = mMetadata->entryCountsByCategory();
        for (const auto& p : counts) {
            lines << QStringLiteral("%1: %2").arg(p.first).arg(p.second);
        }
    }
    const QString detail = lines.isEmpty() ? tr("(no table metadata to clear)")  // cn:（无需要清除的表元数据）
                                           : lines.join(QLatin1Char('\n'));
    const auto answer    = QMessageBox::question(
        this,
        tr("Replace In Place"),  // cn:就地替换
        tr("Replacing in place will clear the following table metadata:\n%1\n\nData and metadata share one undo unit (Ctrl+Z restores both). Continue?")
            .arg(detail),  // cn:就地替换将清除本表的以下元数据：\n%1\n\n数据与元数据共用一个撤销单元（Ctrl+Z 一并还原）。是否继续？
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    // ② 整束快照（undo 还原用）
    const QString snapshot = mMetadata ? mMetadata->snapshotToString() : QString();
    // ③ 超大表降级：旧对象驻留内存 ≈ 1× 表内存，超阈值提示「本次不可撤销」
    const DAPyDataFrame curDf = getDataframe();
    qint64 rows               = 0;
    qint64 cols               = 0;
    if (!curDf.isNone()) {
        const auto s = curDf.shape();
        rows         = static_cast< qint64 >(s.first);
        cols         = static_cast< qint64 >(s.second);
    }
    if (isInplaceUndoTooLarge(rows, cols)) {
        const auto answer2 = QMessageBox::question(
            this,
            tr("Replace In Place"),  // cn:就地替换
            tr("The table is very large (%1 cells). Keeping the old data for undo would double memory usage, so this in-place replacement will NOT be undoable. Continue?")
                .arg(rows * cols),  // cn:表格过大（%1 单元格）。保留旧数据用于撤销将使内存翻倍，本次就地替换将**不可撤销**。是否继续？
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer2 != QMessageBox::Yes) {
            return;
        }
        mData.setPyObject(newDf.object());
        if (mMetadata) {
            mMetadata->clearAll();
        }
        refreshAfterInplaceReplace();
        Q_EMIT dataModified();
        return;
    }
    // ④ 可撤销路径：交 APP 建 Swap 命令（统一撤销链）
    DAData source(newDf);
    Q_EMIT inplaceDataReplaceRequested(mData, source, text, snapshot);
}

/**
 * @brief 打开形状变换对话框并执行（004 T1.27）
 * @param meltMode true=宽转长（melt）初始模式，false=转置初始模式（对话框内可切换）
 */
void DADataOperateOfDataFrameWidget::showReshapeDialog(bool meltMode)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        // 开发诊断：非 dataframe（只读惰性表无 python 数据体）
        qWarning() << "DADataOperateOfDataFrameWidget::showReshapeDialog: dataframe unavailable";
        return;
    }
    DADialogReshape dlg(df, meltMode, this);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QString err;
    DAPyDataFrame result;
    {
        DAWaitCursorScoped waitCursor;  // 全表变换忙等待
        if (dlg.isMeltMode()) {
            result = py.melt(df, dlg.idVars(), dlg.valueVars(), dlg.varName(), dlg.valueName(), &err);
        } else {
            result = py.transpose(df, &err);
        }
    }
    if (result.isNone()) {
        daWarning << tr("Reshape failed: %1").arg(err);  // cn:形状变换失败：%1
        return;
    }
    if (dlg.isInplace()) {
        requestInplaceReplace(result,
                              dlg.isMeltMode() ? tr("Melt table in place")    // cn:就地宽转长
                                               : tr("Transpose table in place"));  // cn:就地转置
        return;
    }
    // 默认产出新数据集（R14），命名 xxx_Transposed / xxx_Melted（与 _PivotTable/_GroupBy 对称）
    if (dlg.isMeltMode()) {
        registerNewDataset(result, tr("%1_Melted").arg(mData.getName()), tr("Melt (wide to long) of %1").arg(mData.getName()));  // cn:%1_宽转长 / 生成%1的宽转长结果
    } else {
        registerNewDataset(result, tr("%1_Transposed").arg(mData.getName()), tr("Transpose of %1").arg(mData.getName()));  // cn:%1_转置 / 生成%1的转置结果
    }
}

/**
 * @brief 就地替换生效后的整页刷新（模型 reset + 清排序规格；APP 撤销接线回调也用）
 */
void DADataOperateOfDataFrameWidget::refreshAfterInplaceReplace()
{
    // 行列互换后旧排序规格（列位置+方向）失去意义，清除（同 setDAData 口径）
    mSortController->clearSpec();
    if (mModel) {
        mModel->refreshData();  // 全模型 reset（shape/列名/块缓存重建，滑窗回顶部）
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
    return mSelection->selectedColumnRowRuns();
}

/**
 * @brief 用填充清单构造一条 paste 命令并入栈（值记录，D4）
 *
 * 与 pasteFromClipboard 同款回调：模型逐格 notifyDataChanged、失效受影响列的条件格式
 * 统计、按变更列把依赖的计算列标 Stale。一次填充 = 一条命令 = 一步撤销。
 * @param df 数据
 * @param rows 行位置（绝对）
 * @param cols 列位置（与 rows/values 一一对应）
 * @param values 新值
 * @param text 命令文案（已翻译）
 * @return 写入单元格数（0 表示未写入）
 */
int DADataOperateOfDataFrameWidget::pushFillPasteCommand(const DAPyDataFrame& df,
                                                         const QList< int >& rows,
                                                         const QList< int >& cols,
                                                         const QList< QVariant >& values,
                                                         const QString& text)
{
    if (rows.isEmpty()) {
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_paste > cmd(new DACommandDataFrame_paste(df, rows, cols, values));
    cmd->setText(text);
    // 编辑后效（008 期2）：模型逐格刷新 + 受影响列统计失效 + 计算列标脏，收口到 commit
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedRows = rows;
    mutation.changedCols = cols;
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        daWarning << tr("Fill failed: the content does not match the column data type");  // cn:填充失败：内容与列数据类型不匹配
        return 0;
    }
    push(cmd.release());
    return rows.size();
}

/**
 * @brief 选区填充（向下/向上/ffill/bfill）
 *
 * 按列独立处理：每列在自己的连续选中行区间内传递（selectedColumnRowRuns 切 run）。
 * 求值全部交 Python 算子 da_fill_selection（空值 NaN/NaT/None 判定在 pandas 侧才可靠，
 * 设计稿 §4.8 硬要求），只返回变化的单元格；全部列的变化清单汇总成**一条** paste 命令
 * 入栈（一次填充 = 一次撤销，D4）。只读/惰性源不生效（入口已置灰，此处二次守卫）。
 * @param mode 填充模式
 * @return 写入单元格数（0 表示无变化/无选区/只读源）
 */
int DADataOperateOfDataFrameWidget::fillSelection(FillMode mode)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    if (!isTableEditable()) {
        daWarning << tr("Cannot fill a read-only data source");  // cn:只读数据源不能填充
        return 0;
    }
    const QHash< int, QVector< QPair< int, int > > > runs = selectedColumnRowRuns();
    if (runs.isEmpty()) {
        return 0;
    }
    QString modeStr;
    QString text;
    switch (mode) {
    case FillMode::Down:
        modeStr = QStringLiteral("down");
        text    = tr("Fill down");  // cn:向下填充
        break;
    case FillMode::Up:
        modeStr = QStringLiteral("up");
        text    = tr("Fill up");  // cn:向上填充
        break;
    case FillMode::Forward:
        modeStr = QStringLiteral("ffill");
        text    = tr("Fill forward (ffill)");  // cn:向下填充空值(ffill)
        break;
    case FillMode::Backward:
        modeStr = QStringLiteral("bfill");
        text    = tr("Fill backward (bfill)");  // cn:向上填充空值(bfill)
        break;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QList< int > allRows, allCols;
    QList< QVariant > allValues;
    // const 遍历（红线：非 const Qt 容器禁范围迭代）
    for (auto cit = runs.constBegin(); cit != runs.constEnd(); ++cit) {
        const int col = cit.key();
        const QVector< QPair< int, int > >& colRuns = cit.value();
        for (const QPair< int, int >& run : colRuns) {
            const DAPyScriptsDataFrame::FillSelectionResult r = py.fillSelection(df, col, run.first, run.second, modeStr);
            if (!r.ok) {
                continue;
            }
            for (int i = 0; i < r.rows.size(); ++i) {
                allRows.append(r.rows[ i ]);
                allCols.append(col);
                allValues.append(i < r.values.size() ? r.values[ i ] : QVariant());
            }
        }
    }
    return pushFillPasteCommand(df, allRows, allCols, allValues, text);
}

/**
 * @brief 序列填充对话框（数值等差 / 日期步进）
 *
 * 对话框收集参数并实时预览（da_fill_series，Python 求值，预览与写入同一算子，红线5）。
 * 接受后对选区**每一列**各生成一个序列（Excel 语义）：每列按自己选中行的个数生成等长
 * 序列，写入自己选中的行（选中行升序，非连续也按序填）。全部列汇总成一条 paste 命令入栈。
 * @return 写入单元格数（取消/无选区/只读源返回 0）
 */
int DADataOperateOfDataFrameWidget::showFillSeriesDialog()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    if (!isTableEditable()) {
        daWarning << tr("Cannot fill a read-only data source");  // cn:只读数据源不能填充
        return 0;
    }
    // 每列的选中行（升序去重），序列按此顺序写入
    const QList< QPoint > cells = getSelectedDataframeCells();
    if (cells.isEmpty()) {
        daWarning << tr("Please select cells to fill");  // cn:请先选中要填充的单元格
        return 0;
    }
    QHash< int, QList< int > > colRows;
    for (const QPoint& p : cells) {
        colRows[ p.y() ].append(p.x());
    }
    for (auto it = colRows.begin(); it != colRows.end(); ++it) {
        std::sort(it.value().begin(), it.value().end());
        it.value().erase(std::unique(it.value().begin(), it.value().end()), it.value().end());
    }
    // 预览个数取首列选中行数
    int previewCount = 5;
    for (auto it = colRows.constBegin(); it != colRows.constEnd(); ++it) {
        previewCount = it.value().size();
        break;
    }
    DADialogFillSeries dlg(previewCount, this);
    if (QDialog::Accepted != dlg.exec()) {
        return 0;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QList< int > allRows, allCols;
    QList< QVariant > allValues;
    for (auto it = colRows.constBegin(); it != colRows.constEnd(); ++it) {
        const int col          = it.key();
        const QList< int > rows = it.value();
        bool ok                = false;
        const QVariantList seq = py.fillSeries(rows.size(), dlg.kind(), dlg.startText(), dlg.stepText(), dlg.stopText(), dlg.dateUnit(), &ok);
        if (!ok) {
            daWarning << tr("Failed to generate the fill series");  // cn:生成填充序列失败
            continue;
        }
        const int n = qMin(rows.size(), seq.size());
        for (int i = 0; i < n; ++i) {
            allRows.append(rows[ i ]);
            allCols.append(col);
            allValues.append(seq[ i ]);
        }
    }
    return pushFillPasteCommand(df, allRows, allCols, allValues, tr("Fill series"));  // cn:序列填充
}

//===================================================
// 数据校验（005 T1.25，设计稿 §4.9）
//===================================================

/**
 * @brief 校验规则子存储（借自元数据束）
 * @return 可能为 nullptr（无注册表时）
 */
DATableValidationStore* DADataOperateOfDataFrameWidget::validationStore() const
{
    return mValidationStore;
}

/**
 * @brief 规则显示文本（转发校验控制器；实现迁入 DATableValidationController，008 期3）
 */
QString DADataOperateOfDataFrameWidget::validationRuleDisplayText(const DATableValidationRule& rule) const
{
    return mValidationController->validationRuleDisplayText(rule);
}

/**
 * @brief 打开校验规则编辑对话框（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::showValidationRulesDialog()
{
    mValidationController->showRulesDialog();
}

/**
 * @brief 执行校验求值（转发校验控制器；返回结构经 ValidationRuleOutcome 别名保持兼容）
 */
QList< DADataOperateOfDataFrameWidget::ValidationRuleOutcome > DADataOperateOfDataFrameWidget::evaluateValidationRules(int* elapsedMs)
{
    return mValidationController->evaluateRules(elapsedMs);
}

/**
 * @brief 执行校验并弹出报告对话框（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::runValidationAndShowReport()
{
    mValidationController->runAndShowReport();
}

/**
 * @brief 清除违规高亮（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::clearValidationHighlights()
{
    mValidationController->clearHighlights();
}

/**
 * @brief 列改名确认（转发校验控制器）
 */
bool DADataOperateOfDataFrameWidget::confirmValidationRuleRename(const QList< QPair< QString, QString > >& renames) const
{
    return mValidationController->confirmRuleRename(renames);
}

/**
 * @brief 把校验规则引用迁移挂进改名命令的 directional 回调（转发校验控制器）
 */
void DADataOperateOfDataFrameWidget::wireValidationRuleRenameMigration(
    DACommandDataFrame_renameColumns* cmd, const QList< QPair< QString, QString > >& renames) const
{
    mValidationController->wireRuleRenameMigration(cmd, renames);
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
    return mNoteStore;
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
    return mSelection->currentActualCell(actualRow, actualCol);
}

/**
 * @brief 打开当前单元格的批注编辑弹层（无批注=插入，有批注=编辑且弹层内可删除）
 *（实现迁入 DATableNoteController，008 期3）
 */
void DADataOperateOfDataFrameWidget::showNoteEditorForCurrentCell()
{
    mNoteController->showNoteEditorForCurrentCell();
}

/**
 * @brief 删除当前单元格批注（命令可撤销；实现迁入 DATableNoteController，008 期3）
 * @return 成功入栈返回 true；无当前格或无批注返回 false
 */
bool DADataOperateOfDataFrameWidget::deleteNoteAtCurrentCell()
{
    return mNoteController->deleteNoteAtCurrentCell();
}

/**
 * @brief 清除本表全部批注（一次命令，可撤销；实现迁入 DATableNoteController，008 期3）
 * @return 清除的批注数（无批注返回 0，不推空命令）
 */
int DADataOperateOfDataFrameWidget::clearAllNotes()
{
    return mNoteController->clearAllNotes();
}

// ============================================================
// 表内快速绘图（快速图表预览；实现迁入 DATableQuickPlotController，008 期3）
// ============================================================

/**
 * @brief 用当前（有序）选区构造快速绘图请求（转发快速绘图控制器）
 */
DA::DATableChartRequest DADataOperateOfDataFrameWidget::buildQuickPlotRequest() const
{
    return mQuickPlotController->buildQuickPlotRequest();
}

/**
 * @brief 用当前选区打开/刷新快速绘图子窗口（转发快速绘图控制器）
 */
bool DADataOperateOfDataFrameWidget::showQuickPlot()
{
    return mQuickPlotController->showQuickPlot();
}

/**
 * @brief 关闭快速绘图子窗口（实例保留复用；转发快速绘图控制器）
 */
void DADataOperateOfDataFrameWidget::closeQuickPlot()
{
    mQuickPlotController->closeQuickPlot();
}

/**
 * @brief 快速绘图浮窗是否正在显示（转发快速绘图控制器）
 */
bool DADataOperateOfDataFrameWidget::isQuickPlotVisible() const
{
    return mQuickPlotController->isQuickPlotVisible();
}

/**
 * @brief 设置转入绘图区的处理器（APP 注入；转发快速绘图控制器）
 */
void DADataOperateOfDataFrameWidget::setQuickPlotTransferHandler(const QuickPlotTransferHandler& handler)
{
    mQuickPlotController->setTransferHandler(handler);
}

/**
 * @brief 设置「追加到当前图表」的可用性提供者（APP 注入；转发快速绘图控制器）
 */
void DADataOperateOfDataFrameWidget::setQuickPlotAppendAvailabilityProvider(
    const QuickPlotAppendAvailabilityProvider& provider)
{
    mQuickPlotController->setAppendAvailabilityProvider(provider);
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
    return mSelection->selectedDataColumnsInViewOrder(ensureInDataframe);
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
    return mSelection->selectedColumnsForDataAction(fallbackColumn);
}

