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
// Dialog
#include "Dialog/DARenameColumnsNameDialog.h"
#include "Dialog/DADialogDataframeColumnCastToNumeric.h"
#include "Dialog/DADialogDataframeColumnCastToDatetime.h"
#include "Dialog/DADialogInsertNewColumn.h"
#include "Dialog/DADialogDataframeColumnDescribe.h"
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
#include "Dialog/DADialogValidationRules.h"
#include "Dialog/DADialogValidationReport.h"
// python operator wrapper（查找/替换）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include <climits>

//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

namespace
{
// 替换撤销阈值（格）：变更格数超过此值降级为「不可撤销 + 确认提示」（设计稿 §4.11 策略表，默认 100 万格）
constexpr int c_replaceUndoCellLimit = 1000000;

// 列集合去重排序（003 统计失效挂点用：变更清单的列收敛为唯一列集合）
QList< int > toUniqueSorted(const QList< int >& in)
{
    QList< int > out = in;
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// 身份列表 → 身份集合（Qt5.14 无 QSet 迭代器区间构造，循环插入）
QSet< QPair< QString, int > > toIdentSet(const QList< QPair< QString, int > >& idents)
{
    QSet< QPair< QString, int > > res;
    for (const QPair< QString, int >& ident : idents) {
        res.insert(ident);
    }
    return res;
}

// 一维闭区间的合并覆盖长度（「整列/整行是否全选」判定用）：排序后归并相邻/重叠段再累加，
// 对重叠选区段也不会重复计数。按值传入以便就地排序（段数 = 触及该列/行的选区段数，很小）
int mergedSpanCoverage(QVector< QPair< int, int > > spans)
{
    if (spans.isEmpty()) {
        return 0;
    }
    std::sort(spans.begin(), spans.end());
    int covered  = 0;
    int curStart = spans.first().first;
    int curEnd   = spans.first().second;
    for (int i = 1; i < spans.size(); ++i) {
        const QPair< int, int >& s = spans.at(i);
        if (s.first > curEnd + 1) {
            covered += curEnd - curStart + 1;
            curStart  = s.first;
            curEnd    = s.second;
        } else {
            curEnd = qMax(curEnd, s.second);
        }
    }
    covered += curEnd - curStart + 1;
    return covered;
}
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
    // 冻结窗格懒建（006e）：容器建好窗格后交页面装配同一条 configurePaneView，
    // 装配完成后再补列宽/行头宽；转发契约（冻结 → 主）也在此接线（APP 只连主窗格）
    connect(mPaneContainer, &DATablePaneContainer::frozenViewCreated, this, [ this ](DA::DADataTableView* frozen) {
        configurePaneView(frozen);
        applyColumnWidthsToPane(frozen);
        applyRowHeaderWidthToOwner();
        // 表头动作转发：重命名/删除/定位最大最小/描述统计/计算列三项（APP 只连主窗格）
        DADataHeaderView* fh = qobject_cast< DADataHeaderView* >(frozen->horizontalHeader());
        DADataHeaderView* mh = qobject_cast< DADataHeaderView* >(ui->tableView->horizontalHeader());
        if (fh && mh) {
            connect(fh, &DADataHeaderView::columnActionRequested, mh, &DADataHeaderView::columnActionRequested);
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
    if (mNoteStore) {
        // 批注变更只影响角标（不经模型 dataChanged 链路），重绘视口即可
        connect(mNoteStore, &DATableNoteStore::notesChanged, this, [ this ]() {
            ui->tableView->viewport()->update();
        });
    }
    // 窗格装配（006b 抽取的唯一装配点）：表头替换 → setModel → 委托 → 逐窗格连接；
    // 必须在 mStyleDelegate 创建之后（里面要用它），在 setDAData/applyViewStateFromStore 之前
    configurePaneView(ui->tableView);
    // 高亮层变更时重绘两窗格视口（瞬态装饰不经模型 dataChanged 链路，006e 扩冻结窗格）
    connect(mHighlightOverlay, &DATableHighlightOverlay::changed, this, [ this ]() {
        for (DADataTableView* pane : paneViews()) {
            pane->viewport()->update();
        }
    });
    // 模型读取列显示格式（actualData 的 DisplayRole 分支）
    mModel->setStyleManager(mStyleManager);
    // 计算列子存储注入模型（表头浅蓝底色 BackgroundRole + 公式 tooltip ToolTipRole，004 T1.23）
    mModel->setComputedColumnStore(mComputedColumnStore);
    if (mComputedColumnStore) {
        // 定义/状态变更 → 表头重绘（底色经模型 headerDataChanged，角标经视口 update）
        connect(mComputedColumnStore, &DATableComputedColumnStore::definitionsChanged, this, &DADataOperateOfDataFrameWidget::refreshComputedColumnHeaders);
        connect(mComputedColumnStore, &DATableComputedColumnStore::stateChanged, this, [ this ](const QString&, DATableComputedColumnStore::State) {
            refreshComputedColumnHeaders();
        });
    }
    // 样式变更时触发 view 刷新（actualRow 转回 logical row 通知 model）
    connect(mStyleManager, &DATableStyleManager::styleChanged, this, [ this ](int actualRow, int actualCol) {
        if (!mModel) {
            return;
        }
        mModel->notifyDataChanged(actualRow, actualCol);
    });
    connect(mStyleManager, &DATableStyleManager::styleRangeChanged, this, [ this ](int rowStart, int colStart, int rowEnd, int colEnd) {
        if (!mModel) {
            return;
        }
        // -1 表示整个维度；行号统一用数据源绝对行，由 model 换算到窗口内
        const int rStart = (rowStart < 0) ? 0 : rowStart;
        const int rEnd   = (rowEnd < 0) ? qMax(0, mModel->actualRowCount() - 1) : rowEnd;
        const int cStart = (colStart < 0) ? 0 : colStart;
        const int cEnd   = (colEnd < 0) ? mModel->columnCount() - 1 : colEnd;
        mModel->notifyDataChanged(rStart, cStart, rEnd, cEnd);
    });
    connect(mStyleManager, &DATableStyleManager::styleReset, this, [ this ]() {
        if (mModel) {
            // 全表刷新
            mModel->notifyDataChanged(0, 0, qMax(0, mModel->actualRowCount() - 1), mModel->columnCount() - 1);
        }
    });
    // 条件格式（003 T1.5）：规则变更 → 重绘对应列（-1 整表）；重映射 → 整表刷新
    if (mConditionalFormatStore) {
        connect(mConditionalFormatStore, &DATableConditionalFormatStore::rulesChanged, this, [ this ](int column) {
            if (!mModel) {
                return;
            }
            const int rEnd = qMax(0, mModel->actualRowCount() - 1);
            if (column < 0) {
                mModel->notifyDataChanged(0, 0, rEnd, qMax(0, mModel->columnCount() - 1));
            } else {
                mModel->notifyDataChanged(0, column, rEnd, column);
            }
        });
        connect(mConditionalFormatStore, &DATableConditionalFormatStore::ruleReset, this, [ this ]() {
            if (mModel) {
                mModel->notifyDataChanged(0, 0, qMax(0, mModel->actualRowCount() - 1), qMax(0, mModel->columnCount() - 1));
            }
        });
        // 列统计提供者（色阶/数据条端点）：store 是会话级（比 widget 长寿），
        // lambda 用 QPointer 防悬挂，widget 析构时另行清空提供者
        QPointer< DADataOperateOfDataFrameWidget > self(this);
        mConditionalFormatStore->setStatsProvider([ self ](int column, DATableConditionalFormatStore::ColumnStats& out) {
            if (!self) {
                return false;
            }
            DAPyDataFrame df = self->getDataframe();
            if (df.isNone()) {
                return false;
            }
            DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
            const DAPyScriptsDataFrame::ColumnStatsResult r = py.columnStats(df, column);
            if (!r.ok) {
                return false;
            }
            out.minValue    = r.minValue;
            out.maxValue    = r.maxValue;
            out.medianValue = r.medianValue;
            return true;
        });
        // 重复键提供者（重复/唯一条件）：与统计同款 QPointer 防悬挂 + 惰性缓存语义
        mConditionalFormatStore->setDuplicatesProvider([ self ](int column, QSet< QString >& out) {
            if (!self) {
                return false;
            }
            DAPyDataFrame df = self->getDataframe();
            if (df.isNone()) {
                return false;
            }
            DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
            QList< QVariant > values;
            if (!py.columnDuplicateValues(df, column, values)) {
                return false;
            }
            for (const QVariant& v : std::as_const(values)) {
                const QString key = DATableConditionalFormatRule::duplicateKey(v);
                if (!key.isEmpty()) {
                    out.insert(key);
                }
            }
            return true;
        });
        // 统计失效挂点（widget 侧，计划 WP2 步骤 5）：单元格编辑命令在模型内部创建，
        // 不走 widget 命令回调，用模型的 cellValueChanged 精确信号失效对应列。
        // 不用 dataChanged：滑窗滚动（setCacheWindowStartRow）与样式刷新也发整窗
        // dataChanged，会在滚动时每步误失效统计（破坏"滚动千行无卡顿"验收）；
        // 排序不发本信号（不改列值集合，不失效——性能决策）
        connect(mModel, &DADataTableModel::cellValueChanged, this, [ this ](int actualRow, int actualColumn) {
            Q_UNUSED(actualRow);
            if (mConditionalFormatStore) {
                mConditionalFormatStore->invalidateColumnStats(actualColumn);
            }
        });
    }
    // 计算列待重算标记（004 T1.23 R8 手动重算）：单元格编辑走 cellValueChanged 精确信号，
    // 只把依赖该列（或目标列自身被改）的计算列标 Stale；排序不发本信号（R13 不触发）
    connect(mModel, &DADataTableModel::cellValueChanged, this, [ this ](int actualRow, int actualColumn) {
        Q_UNUSED(actualRow);
        markComputedColumnsStaleForColumns({ actualColumn });
    });
    // 视图状态（列宽按列名 + 行头宽，R6）：
    // 结构变化（模型重置/插删列）后按列名重新应用，保证列宽不随位置错位；
    // 采集只认用户拖拽（sectionResized + 左键按住在表头上），程序化改动不写 store
    connect(mModel, &QAbstractItemModel::modelReset, this, [ this ]() {
        applyViewStateFromStore();
    });
    // 唯一性违规缓存失效挂点（005 T1.25 §4.9）：数据/结构变更后重复集合可能变化，
    // 保守全失效（缓存只为「同一份数据重复点校验」省算，正确性优先）
    if (mValidationStore) {
        DATableValidationStore* vaStore = mValidationStore;
        connect(mModel, &QAbstractItemModel::modelReset, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(mModel, &QAbstractItemModel::rowsInserted, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(mModel, &QAbstractItemModel::rowsRemoved, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(mModel, &QAbstractItemModel::columnsInserted, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(mModel, &QAbstractItemModel::columnsRemoved, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(mModel, &DADataTableModel::cellValueChanged, this, [ vaStore ](int, int) {
            vaStore->invalidateUniqueCache();
        });
    }
    connect(mModel, &QAbstractItemModel::columnsInserted, this, [ this ]() {
        applyViewStateFromStore();
        // 插列后排序规格里的列位置失效，清除规格（数据顺序不变，只清指示器）
        mSortSpec.clear();
        forEachPaneHeader([ this ](DADataHeaderView* hh) { hh->setSortSpec(mSortSpec); });
    });
    connect(mModel, &QAbstractItemModel::columnsRemoved, this, [ this ]() {
        applyViewStateFromStore();
        // 删列后排序规格里的列位置失效，清除规格
        mSortSpec.clear();
        forEachPaneHeader([ this ](DADataHeaderView* hh) { hh->setSortSpec(mSortSpec); });
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
            // 查找条处于「当前选区」范围时，选区变化经去抖触发重扫（范围基准随选区更新）
            if (mFindReplaceDialog && mFindReplaceDialog->isVisible() && mFindReplaceDialog->options().selectionOnly) {
                mFindReplaceDialog->restartDebounce();
            }
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
    delete ui;
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
        connect(hh, &DADataHeaderView::clickedWithModifiers, this, &DADataOperateOfDataFrameWidget::onHeaderClickedWithModifiers);
        // 列宽采集：用触发信号的那个窗格的表头（不再写死 ui->tableView）
        connect(hh, &QHeaderView::sectionResized, this, [ this, pane ](int logicalIndex, int, int newSize) {
            onHeaderSectionResized(pane, logicalIndex, newSize);
        });
        // 列序拖动落库（006c）：必须在表头替换之后（hh 是新表头）；每装一个窗格连一份
        if (mPaneContainer) {
            connect(hh, &QHeaderView::sectionMoved, mPaneContainer, &DATablePaneContainer::onPaneSectionMoved);
        }
        // 「列」子菜单动作（006d）：列视图真源操作，消费方是页面自身（不经 APP）
        connect(hh, &DADataHeaderView::columnViewActionRequested,
                this, &DADataOperateOfDataFrameWidget::onHeaderColumnViewActionRequested);
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
    }
    // 6) 视图表意入口（快捷键/右键菜单共用）：每装一个窗格连一份
    connect(pane, &QTableView::clicked, this, &DADataOperateOfDataFrameWidget::onTableViewClicked);
    // 查找/替换入口（002 T1.4）：视图快捷键 Ctrl+F/Ctrl+H 与表体右键菜单共用表意信号
    connect(pane, &DADataTableView::findRequested, this, [ this ]() {
        showFindReplace(false);
    });
    connect(pane, &DADataTableView::replaceRequested, this, [ this ]() {
        showFindReplace(true);
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
    mData = d;
    // 数据替换后旧排序规格（列位置+方向）失去意义，清除（排序规格不持久化，R13）
    mSortSpec.clear();
    forEachPaneHeader([ this ](DADataHeaderView* hh) { hh->setSortSpec(mSortSpec); });
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
 * @brief 在所选位置插入行
 * @param row
 */
void DADataOperateOfDataFrameWidget::insertRowAt(int row)
{
    DAPyDataFrame df = mData.toDataFrame();
    if (df.isNone()) {
        // 非dataframe数据（如只读的数据库惰性表）不支持插入行
        return;
    }
    std::unique_ptr< DACommandDataFrame_insertNanRow > cmd(new DACommandDataFrame_insertNanRow(df, row));
    QPointer< DADataTableModel > modle = mModel;
    // 统计失效挂点（003 WP2 步骤 5）：插行保守失效全部列统计（exec/undo 都触发）
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    // 计算列挂点（004 T1.23）：插行是拿不到列信息的整表操作 → 保守全标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, row, cfStore, self ]() {
        if (modle) {
            modle->notifyRowsInserted({ row });
        }
        if (cfStore) {
            cfStore->invalidateAllStats();
        }
        if (self) {
            self->markAllComputedColumnsStale();
        }
    });
    // 行列增删样式键同步：插入行时 >=row 的键 +1；撤销时按删除处理（>=row 的键 -1）
    DATableStyleManager* styleMgr = mStyleManager;
    DATableNoteStore* noteStore   = mNoteStore;
    cmd->setDirectionalCallBack([ styleMgr, noteStore, row ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onRowsRemoved({ row });
            } else {
                styleMgr->onRowsInserted({ row });
            }
        }
        // 批注与样式同源位置键，插删行同款重映射（005 T1.26「批注跟着行走」）
        if (noteStore) {
            if (isUndo) {
                noteStore->onRowsRemoved({ row });
            } else {
                noteStore->onRowsInserted({ row });
            }
        }
    });
    if (!cmd->exec()) {
        return;
    }
    push(cmd.release());
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
    cmd->setCallBack([ modle, col ]() {
        if (modle) {
            // 此操作会删除一列，添加一列，整个modelreflash
            modle->notifyColumnsRemoved({ col });
        }
    });
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
 * @brief 移除选中的行
 * @return 返回成功移除的行数
 */
int DADataOperateOfDataFrameWidget::removeSelectRow()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    QList< int > rows = getSelectedDataframeRows();
    if (rows.size() <= 0) {
        daWarning << tr("Please select valid data cells");  // cn:请选择正确的行
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_dropIRow > cmd(new DACommandDataFrame_dropIRow(mData.toDataFrame(), rows));
    QPointer< DADataTableModel > modle = mModel;
    // 统计失效挂点（003）：删行可能改变列 min/max，保守失效全部列统计
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    // 计算列挂点（004 T1.23）：删行是整表操作 → 保守全标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, rows, cfStore, self ]() {
        if (modle) {
            modle->notifyRowsInserted(rows);
        }
        if (cfStore) {
            cfStore->invalidateAllStats();
        }
        if (self) {
            self->markAllComputedColumnsStale();
        }
    });
    // 行列增删样式键同步：删除行时 ==row 的键删除，>row 的键 -1；撤销时按插入处理
    // 注意：被删除行的样式在删除时丢失，撤销(load恢复dataframe)无法恢复样式（已知限制，见 spec §7.3）
    DATableStyleManager* styleMgr = mStyleManager;
    DATableNoteStore* noteStore   = mNoteStore;
    cmd->setDirectionalCallBack([ styleMgr, noteStore, rows ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onRowsInserted(rows);
            } else {
                styleMgr->onRowsRemoved(rows);
            }
        }
        // 批注同款重映射（005 T1.26）：被删行的批注在删除时丢失（与样式同一已知限制，
        // undo 走 load 恢复 dataframe 无法恢复位置键元数据，见 spec §7.3）
        if (noteStore) {
            if (isUndo) {
                noteStore->onRowsInserted(rows);
            } else {
                noteStore->onRowsRemoved(rows);
            }
        }
    });
    if (!cmd->exec()) {
        return 0;
    }
    push(cmd.release());
    return rows.size();
}

/**
 * @brief 移除选中的列
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectColumn()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    QList< int > columns = getSelectedDataframeCoumns();
    if (columns.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_dropIColumn > cmd(new DACommandDataFrame_dropIColumn(mData.toDataFrame(), columns));
    QPointer< DADataTableModel > modle = mModel;
    cmd->setCallBack([ modle, columns ]() {
        if (modle) {
            modle->notifyColumnsRemoved(columns);
        }
    });
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
    QStringList removedNames;
    QList< DATableComputedColumnStore::Entry > ccSnapshot;
    if (ccStore) {
        const QList< QString > colNames = df.columns();
        for (int c : std::as_const(columns)) {
            if (c >= 0 && c < colNames.size()) {
                removedNames.append(colNames.at(c));
            }
        }
        ccSnapshot = ccStore->snapshot();
    }
    auto invalidatedTargets = std::make_shared< QStringList >();
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, ccStore, columns, removedNames, ccSnapshot, invalidatedTargets ](bool isUndo) {
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
    });
    if (!cmd->exec()) {
        return 0;
    }
    push(cmd.release());
    // 删列后提示失效的计算列（exec 内的 directional 回调已填充；不静默，R9）
    if (!invalidatedTargets->isEmpty()) {
        daWarning << tr("Computed column(s) %1 became invalid because their dependency column(s) were removed")
                         .arg(invalidatedTargets->join(QStringLiteral(", ")));  // cn:计算列 %1 因依赖列被删除已失效
    }
    return columns.size();
}

/**
 * @brief 设置选中单元格为nan,返回设置成功的个数
 * @return
 */
int DADataOperateOfDataFrameWidget::removeSelectCell()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    const QList< QPoint > cells = getSelectedDataframeCells();
    if (cells.size() <= 0) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return 0;
    }
    QList< int > rows, cols;
    rows.reserve(cells.size());
    cols.reserve(cells.size());
    for (const auto p : cells) {
        rows.append(p.x());
        cols.append(p.y());
    }
    std::unique_ptr< DACommandDataFrame_setnan > cmd(new DACommandDataFrame_setnan(df, rows, cols));
    QPointer< DADataTableModel > modle = mModel;
    // 统计失效挂点（003）：置 nan 改变列值集合，失效受影响列的统计
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    // 计算列挂点（004 T1.23）：按变更列比对依赖集合标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, rows, cols, cfStore, self ]() {
        if (modle) {
            const auto size = qMin(rows.size(), cols.size());
            for (int i = 0; i < size; ++i) {
                modle->notifyDataChanged(rows[ i ], cols[ i ]);
            }
        }
        if (cfStore) {
            const QList< int > uniqueCols = toUniqueSorted(cols);
            for (int c : uniqueCols) {
                cfStore->invalidateColumnStats(c);
            }
        }
        if (self) {
            self->markComputedColumnsStaleForColumns(cols);
        }
    });
    if (!cmd->exec()) {
        return 0;
    }
    push(cmd.release());
    return cells.size();
}

/**
 * @brief 粘贴剪贴板内容到当前选区左上角
 *
 * 解析剪贴板 TSV 文本（Tab 分列、\n 分行，与 copySelectionToClipboard 格式对称），
 * 从当前选中区左上角起填充；超出数据表边界的部分丢弃并 daWarning 提示丢弃规模（D9）。
 * 数值列遇非数值文本时按 iat 现有转换链路处理（写入失败则整条命令回滚）。
 * 撤销命令走 DADataOperatePageWidget::push 统一入口（push 即激活栈）。
 * @return 成功写入的单元格数（0 表示未写入）
 */
int DADataOperateOfDataFrameWidget::pasteFromClipboard()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    QClipboard* cb = QGuiApplication::clipboard();
    if (!cb) {
        return 0;
    }
    QString text = cb->text();
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.paste", [&text]() {
        return QStringLiteral("clipboard_len=%1").arg(text.size());
    });
    if (text.isEmpty()) {
        daWarning << tr("Clipboard is empty");  // cn:剪贴板为空
        return 0;
    }
    // 定位选区左上角；无选区时用当前光标格
    QModelIndex anchor;
    if (QItemSelectionModel* sm = ui->tableView->selectionModel()) {
        // 段列表取首个区间左上角（等价于旧 selectedIndexes().first()，不物化整个选区）
        const QItemSelection sel = sm->selection();
        anchor                   = sel.isEmpty() ? sm->currentIndex() : sel.first().topLeft();
    }
    if (!anchor.isValid()) {
        daWarning << tr("Please select a cell to paste into");  // cn:请先选中要粘贴的起始单元格
        return 0;
    }
    auto shape  = df.shape();
    const int dfRows = static_cast< int >(shape.first);
    const int dfCols = static_cast< int >(shape.second);
    // 视图行号是滑动窗内逻辑行，写 dataframe 前必须换成绝对行
    const int startRow = toActualRow(anchor.row());
    const int startCol = anchor.column();
    if (startRow >= dfRows || startCol >= dfCols) {
        return 0;
    }
    // 解析 TSV（Tab 分列、\n 分行、\r 行尾剔除）
    const QStringList lines = text.split(QLatin1Char('\n'));
    QList< int > rows, cols;
    QList< QVariant > values;
    int droppedRows = 0, droppedCols = 0;
    int contentLineCount = 0;
    for (int r = 0; r < lines.size(); ++r) {
        QString line = lines[ r ];
        // 结尾换行产生的末尾空行跳过；中间空行保留占位（与复制格式对称）
        if (r == lines.size() - 1 && line.isEmpty()) {
            break;
        }
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
        const QStringList fields = line.split(QLatin1Char('\t'));
        const bool rowOverflow = (startRow + contentLineCount >= dfRows);
        int lineDroppedCols = 0;
        for (int c = 0; c < fields.size(); ++c) {
            if (startCol + c >= dfCols) {
                ++lineDroppedCols;
                continue;
            }
            if (rowOverflow) {
                continue;
            }
            rows.append(startRow + contentLineCount);
            cols.append(startCol + c);
            values.append(fields[ c ]);
        }
        if (rowOverflow) {
            ++droppedRows;
        } else {
            droppedCols += lineDroppedCols;
        }
        ++contentLineCount;
    }
    if (rows.isEmpty()) {
        daWarning << tr("Nothing to paste: the clipboard content exceeds the table boundary");  // cn:无可粘贴内容：剪贴板内容超出表格边界
        return 0;
    }
    if (droppedRows > 0 || droppedCols > 0) {
        daWarning << tr("Paste partially dropped: %1 row(s) and %2 cell(s) outside the table were ignored")
                        .arg(droppedRows)
                        .arg(droppedCols);  // cn:粘贴部分丢弃：%1 行与 %2 个单元格超出表格范围被忽略
    }
    std::unique_ptr< DACommandDataFrame_paste > cmd(new DACommandDataFrame_paste(df, rows, cols, values));
    QPointer< DADataTableModel > modle = mModel;
    // 统计失效挂点（003）：粘贴改变列值集合，失效受影响列的统计
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    // 计算列挂点（004 T1.23）：按变更列比对依赖集合标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, rows, cols, cfStore, self ]() {
        if (modle) {
            const auto size = qMin(rows.size(), cols.size());
            for (int i = 0; i < size; ++i) {
                modle->notifyDataChanged(rows[ i ], cols[ i ]);
            }
        }
        if (cfStore) {
            const QList< int > uniqueCols = toUniqueSorted(cols);
            for (int c : uniqueCols) {
                cfStore->invalidateColumnStats(c);
            }
        }
        if (self) {
            self->markComputedColumnsStaleForColumns(cols);
        }
    });
    if (!cmd->exec()) {
        // 数值列写入非数值等场景：整条命令回滚
        daWarning << tr("Paste failed: the content does not match the column data type");  // cn:粘贴失败：内容与列数据类型不匹配
        return 0;
    }
    push(cmd.release());
    return rows.size();
}

/**
 * @brief 剪切选中区
 *
 * 先按复制格式导出到剪贴板，再把选中区置 nan（复用 DACommandDataFrame_setnan，可撤销）
 * @return 受影响单元格数
 */
int DADataOperateOfDataFrameWidget::cutSelection()
{
    // 先复制（失败说明无选中）
    if (!ui->tableView->copySelectionToClipboard()) {
        daWarning << tr("No cells selected to cut");  // cn:没有选中可剪切的单元格
        return 0;
    }
    // 复制成功说明有选中，置 nan（同 removeSelectCell 语义）
    return removeSelectCell();
}

/**
 * @brief 激活此窗口的UndoStack
 */

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
    if (!cmd->exec()) {
        return false;
    }
    // 列改名迁移：视图状态按列名引用（R20），宽度记录随改名同步迁移；
    // 放在 refreshData 之前，保证 modelReset 触发 applyViewStateFromStore 时按新列名命中
    if (mViewStateStore) {
        mViewStateStore->renameColumn(oldcols[ col ], newName);
    }
    if (DADataManager* mgr = mData.getDataManager()) {
        mgr->notifyDataChangedSignal(mData, DADataManager::ChangeDataframeColumnName);
    }
    push(cmd.release());
    // DADataTableModel 在缓存模式下缓存了列名，headerDataChanged 信号无法刷新缓存，
    // 需显式调用 refreshData() 重置模型以刷新表头显示
    if (DADataTableModel* m = ui->tableView->getDataModel()) {
        m->refreshData();
    }
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
    QPointer< DADataTableModel > modle = mModel;
    // 计算列挂点（004 T1.23）：类型转换改变列值 → 依赖该列的计算列标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, selColumns, self ]() {
        if (modle) {
            for (int c : std::as_const(selColumns)) {
                modle->notifyColumnChanged(c);
            }
        }
        if (self) {
            self->markComputedColumnsStaleForColumns(selColumns);
        }
    });
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
    QPointer< DADataTableModel > modle = mModel;
    // 计算列挂点（004 T1.23）：数值转换改变列值 → 依赖该列的计算列标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, colsIndex, self ]() {
        if (modle) {
            for (int c : std::as_const(colsIndex)) {
                modle->notifyColumnChanged(c);
            }
        }
        if (self) {
            self->markComputedColumnsStaleForColumns(colsIndex);
        }
    });
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
    QPointer< DADataTableModel > modle = mModel;
    // 计算列挂点（004 T1.23）：日期转换改变列值 → 依赖该列的计算列标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, colsIndex, self ]() {
        if (modle) {
            for (int c : std::as_const(colsIndex)) {
                modle->notifyColumnChanged(c);
            }
        }
        if (self) {
            self->markComputedColumnsStaleForColumns(colsIndex);
        }
    });
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
    DADataTableModel* modle                            = mModel;
    // 计算列挂点（004 T1.23）：列转索引是结构性整表操作 → 保守全标 Stale
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, self ]() {
        if (modle) {
            modle->refreshData();
        }
        if (self) {
            self->markAllComputedColumnsStale();
        }
    });
    if (!cmd->exec()) {
        return false;
    }
    push(cmd.release());  // 推入后不会执行redo逻辑部分
    return true;
}

/**
 * @brief 显示列统计信息
 *
 * 通过 pandas Series.describe() 获取指定列的统计信息（count、mean、std、min、25%、50%、75%、max 等），
 * 在弹出的对话框中展示。
 * @param col 列位置索引
 */
void DADataOperateOfDataFrameWidget::showColumnDescribe(int col)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    auto shape = df.shape();
    if (col < 0 || col >= (int)shape.second) {
        return;
    }
    QString colName = df.columnName(col);
    QString dtypeStr;
    QList<QPair<QString, QString>> stats;
    pybind11::gil_scoped_acquire gil;
    try {
        DAPySeries s      = df[ colName ];
        dtypeStr          = s.dtypeString();
        DAPySeries desc   = s.describe();
        QStringList statNames = desc.indexAsStringList();
        for (int i = 0; i < statNames.size(); ++i) {
            stats.append({ statNames[ i ], desc.valueAsString(i) });
        }
    } catch (const std::exception& e) {
        qCritical() << e.what();
        daWarning << tr("Unable to get statistics for this column");  // cn:无法获取此列的统计信息
        return;
    }
    if (stats.isEmpty()) {
        daWarning << tr("Unable to get statistics for this column");  // cn:无法获取此列的统计信息
        return;
    }
    if (!mDialogColumnDescribe) {
        mDialogColumnDescribe = new DADialogDataframeColumnDescribe(this);
    }
    mDialogColumnDescribe->setColumnName(colName);
    mDialogColumnDescribe->setColumnDType(dtypeStr);
    mDialogColumnDescribe->setStatistics(stats);
    mDialogColumnDescribe->exec();
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
    const QItemSelectionModel* selectionModel = ui->tableView->selectionModel();
    if (!selectionModel) {
        return false;
    }
    return selectionModel->hasSelection();
}

/**
 * @brief 获取dataframe表格的tableview
 *
 * @return DADataTableView*
 */
DADataTableView* DADataOperateOfDataFrameWidget::getDataTableView() const
{
    return ui->tableView;
}

/**
 * @brief 返回当前选中单元格所包含的列数，列数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeCoumns(bool ensureInDataframe) const
{
    QList< int > res;
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return res;
    }
    // 按选区区间展开列号（全选卡顿修复：不再 selectedIndexes 物化整个选区，
    // 全选时开销从 窗口行数×列数 降为 选中列数）
    int dfCols = -1;
    if (ensureInDataframe) {
        // 确保返回的列数都在dataframe里
        DAPyDataFrame df = getDataframe();
        if (df.isNone()) {
            return res;
        }
        dfCols = static_cast< int >(df.shape().second);
    }
    QSet< int > colSet;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int c = r.left(); c <= r.right(); ++c) {
            if (dfCols >= 0 && c >= dfCols) {
                continue;
            }
            colSet.insert(c);
        }
    }
    return colSet.values();
}

/**
 * @brief 返回当前选中单元格所包含的行数，行数不会重复
 * @param ensureInDataframe 此参数代表确保返回的值在dataframe的列范围里面
 * @return
 */
QList< int > DADataOperateOfDataFrameWidget::getSelectedDataframeRows(bool ensureInDataframe) const
{
    QList< int > res;
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return res;
    }
    // 按选区区间展开行号（全选卡顿修复：不再 selectedIndexes 物化整个选区）
    const int rowCount = dataRowCount();
    QSet< int > rowSet;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        // 视图行号是滑动窗内的逻辑行，数据操作一律用数据源绝对行
        for (int vr = r.top(); vr <= r.bottom(); ++vr) {
            const int actualRow = toActualRow(vr);
            if (ensureInDataframe && actualRow >= rowCount) {
                continue;
            }
            rowSet.insert(actualRow);
        }
    }
    return rowSet.values();
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
    if (!ui->tableView) {
        return viewRow;
    }
    return ui->tableView->toActualRow(viewRow);
}

/**
 * @brief 数据源真实数据行数（不含尾部预留的空白行）
 * @return 行数
 */
int DADataOperateOfDataFrameWidget::dataRowCount() const
{
    if (mModel) {
        // actualRowCount() 本就不含尾部预留空行（rowCount=窗口行数，total=actual+extra
        // 仅用于滚动条上界）——此前误减 getExtraRowCount() 会把 N 行表算成 N-1 行（007 修复）
        return qMax(0, mModel->actualRowCount());
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    return static_cast< int >(df.shape().first);
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
    QList< int > res;
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return res;
    }
    QAbstractItemModel* m = ui->tableView->model();
    if (!m) {
        return res;
    }
    const int rowCount = m->rowCount();
    const int colCount = m->columnCount();
    if (rowCount <= 0 || colCount <= 0) {
        return res;
    }
    int colLimit = colCount;
    if (ensureInDataframe) {
        // 确保返回的列数都在dataframe里
        DAPyDataFrame df = getDataframe();
        if (df.isNone()) {
            return res;
        }
        colLimit = qMin(colCount, static_cast< int >(df.shape().second));
    }
    // 逐列收集选中的行闭区间（只收 colLimit 内的列）
    QHash< int, QVector< QPair< int, int > > > colSpans;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int c = qMax(0, r.left()); c <= r.right() && c < colLimit; ++c) {
            colSpans[ c ].append(qMakePair(r.top(), r.bottom()));
        }
    }
    // 合并区间后覆盖满全部模型行 → 整列选中
    for (auto it = colSpans.constBegin(); it != colSpans.constEnd(); ++it) {
        if (mergedSpanCoverage(it.value()) >= rowCount) {
            res.append(it.key());
        }
    }
    return res;
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
    QList< int > res;
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return res;
    }
    QAbstractItemModel* m = ui->tableView->model();
    if (!m) {
        return res;
    }
    const int colCount = m->columnCount();
    if (colCount <= 0) {
        return res;
    }
    // 逐行（窗口内逻辑行）收集选中的列闭区间
    QHash< int, QVector< QPair< int, int > > > rowSpans;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int vr = r.top(); vr <= r.bottom(); ++vr) {
            rowSpans[ vr ].append(qMakePair(r.left(), r.right()));
        }
    }
    const int dataRows = dataRowCount();
    for (auto it = rowSpans.constBegin(); it != rowSpans.constEnd(); ++it) {
        if (mergedSpanCoverage(it.value()) < colCount) {
            continue;
        }
        // 视图行号是滑动窗内的逻辑行，返回值一律用数据源绝对行
        const int actualRow = toActualRow(it.key());
        if (ensureInDataframe && actualRow >= dataRows) {
            continue;
        }
        res.append(actualRow);
    }
    return res;
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeRow(bool ensureInDataframe) const
{
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return -1;
    }
    // 区间遍历（全选卡顿修复：不再 selectedIndexes 物化整个选区）。
    // 旧实现按 selectedIndexes 的行主序取第一个满足过滤的行：区间内行号自顶向下
    // 递增、toActualRow 单调，故区间首行不满足过滤时整个区间都不满足，直接跳下一段，
    // 结果与逐格遍历逐位一致。
    const int rowCount = ensureInDataframe ? dataRowCount() : 0;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        const int actualRow = toActualRow(r.top());
        if (!ensureInDataframe || actualRow < rowCount) {
            return actualRow;
        }
    }
    return -1;
}

int DADataOperateOfDataFrameWidget::getSelectedOneDataframeColumn(bool ensureInDataframe) const
{
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return -1;
    }
    // 区间遍历（全选卡顿修复：不再 selectedIndexes 物化整个选区）。
    // 旧实现按 selectedIndexes 的行主序取第一个满足过滤的列：区间内列号自左向右
    // 递增，故区间首列不满足过滤时整个区间都不满足，直接跳下一段，结果逐位一致。
    int dfCols = -1;
    if (ensureInDataframe) {
        // 确保返回的列数都在dataframe里
        DAPyDataFrame df = getDataframe();
        if (df.isNone()) {
            return -1;
        }
        dfCols = static_cast< int >(df.shape().second);
    }
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        if (dfCols < 0 || r.left() < dfCols) {
            return r.left();
        }
    }
    return -1;
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
    QList< DAData > res;
    const QList< int > indexs = getSelectedDataframeCoumns(true);
    if (indexs.isEmpty()) {
        return res;
    }
    DAPyDataFrame df = mData.toDataFrame();
    auto shape       = df.shape();
    for (int i : indexs) {
        if (i < shape.second) {
            auto series = df[ i ];
            DAData d(series);
            res.append(d);
        }
    }
    return res;
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
    QList< QPoint > res;
    QItemSelectionModel* selModel = ui->tableView->selectionModel();
    if (!selModel) {
        return res;
    }
    const int rowCount    = dataRowCount();
    const int columnCount = dataColumnCount();
    const QModelIndexList selindexs = selModel->selectedIndexes();
    for (const QModelIndex& index : selindexs) {
        const int actualRow = toActualRow(index.row());
        if (ensureInDataframe && (actualRow >= rowCount || index.column() >= columnCount)) {
            continue;
        }
        res.append(QPoint(actualRow, index.column()));
    }
    return res;
}

/**
 * @brief 数据源真实数据列数（不含尾部预留的空白列）
 * @return 列数
 */
int DADataOperateOfDataFrameWidget::dataColumnCount() const
{
    if (mModel) {
        return qMax(0, mModel->columnCount() - mModel->getExtraColumnCount());
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return 0;
    }
    return static_cast< int >(df.shape().second);
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
    QList< QPair< QString, int > > res;
    if (!mModel) {
        return res;
    }
    QHash< QString, int > seen;  // 列名 -> 已出现次数（= 下一列的 occurrence）
    const int cols = dataColumnCount();
    res.reserve(cols);
    for (int c = 0; c < cols; ++c) {
        const QString name = mModel->headerData(c, Qt::Horizontal).toString();
        if (name.isEmpty()) {
            continue;  // 空列名先判掉：不要让空名进 seen/occurrence 计数
        }
        const int occ = seen.value(name, 0);
        seen[ name ]  = occ + 1;
        res.append(qMakePair(name, occ));
    }
    return res;
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
    return ui->tableView->selectionModel();
}

/**
 * @brief 选区段数
 *
 * 选区按连续段组织（Ctrl+多点形成多段）；0=无选区，>1=多段（状态栏只给几何计数）。
 * @return 段数
 */
int DADataOperateOfDataFrameWidget::selectionSegmentCount() const
{
    QItemSelectionModel* sm = tableSelectionModel();
    return sm ? sm->selection().size() : 0;
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
    QItemSelectionModel* sm = tableSelectionModel();
    const QItemSelection sel = sm ? sm->selection() : QItemSelection();
    if (sel.isEmpty()) {
        return false;
    }
    int top = INT_MAX, bottom = -1, left = INT_MAX, right = -1;
    for (const QItemSelectionRange& r : sel) {
        top    = qMin(top, r.top());
        bottom = qMax(bottom, r.bottom());
        left   = qMin(left, r.left());
        right  = qMax(right, r.right());
    }
    if (rowBegin) {
        *rowBegin = toActualRow(top);
    }
    if (rowEnd) {
        *rowEnd = toActualRow(bottom);
    }
    if (colBegin) {
        *colBegin = left;
    }
    if (colEnd) {
        *colEnd = right;
    }
    return bottom >= 0;
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
    QItemSelectionModel* sm = tableSelectionModel();
    if (!sm) {
        return 0;
    }
    qint64 count = 0;
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        count += qint64(r.width()) * qint64(r.height());
    }
    return static_cast< int >(qMin< qint64 >(count, INT_MAX));
}

/**
 * @brief 从视图状态子存储应用列宽与行头宽
 *
 * 列宽按「列名 + 第几次出现（occurrence）」匹配逻辑列号后 resizeSection——
 * 按列名匹配保证插列/删列后宽度不错位（R6/R20）。应用期间置守卫标志，
 * 屏蔽 sectionResized 采集，避免把「应用」当「用户改动」写回 store。
 * 行头宽用 setFixedWidth 落地（Qt 纵向表头宽度=clamp(qMax(min,sizeHint),max)，
 * min=max=w 恒为 w，不会被视图布局覆盖）。
 * 首次调用先做一次加载收敛（normalizeToColumns：孤儿剪除 + 新列追加列序末尾），
 * 有丢弃时发一条用户可见提示；列序/隐藏/冻结的投影由 006b/006e 落地。
 */
void DADataOperateOfDataFrameWidget::applyViewStateFromStore()
{
    if (!mViewStateStore || !mModel) {
        return;
    }
    // 加载收敛（只做一次）：把已被删除的列从视图状态里剪掉、把新列追加到列序末尾。
    // 释放后不再收敛（E6：不在列插删时收敛，避免与"删除列"的 Ctrl+Z 互踩）
    if (!mViewStateStore->isColumnSetNormalized() && dataColumnCount() > 0) {
        if (mViewStateStore->normalizeToColumns(actualColumnIdentities())) {
            daWarning << tr("Some column view settings were dropped because the table columns have changed.");
            // cn:表格的列已变化，部分列视图设置已被丢弃
        }
    }
    mApplyingViewState = true;
    // 1) 列宽：对每个窗格按列名（+occurrence）套用（Qt 按段存宽度，隐藏段写进
    //    hiddenSectionSize、显示时恢复，安全）
    for (DADataTableView* pane : paneViews()) {
        applyColumnWidthsToPane(pane);
    }
    // 2) 行头宽：设给行头归属方（冻结时归属冻结窗格，否则主窗格）
    applyRowHeaderWidthToOwner();
    // 3) 列序 / 隐藏 / 冻结 → 容器投影（冻结集合 = ordered 前 freeze 个非隐藏列）
    const QList< QPair< QString, int > > actual       = actualColumnIdentities();
    const QList< QPair< QString, int > > storedOrder  = orderedColumnIdentities();  // 全局列序身份表
    const QList< QPair< QString, int > > storedHidden = mViewStateStore->hiddenColumns();
    QList< int > actualLogical;                            // 身份 → logical：与 actualColumnIdentities() 同口径
    QHash< QPair< QString, int >, int > logicalOfIdent;    // 重扫列头（跳过空列名），与 actual 逐条对齐
    {
        QHash< QString, int > seen;
        const int cols = dataColumnCount();
        actualLogical.reserve(cols);
        for (int c = 0; c < cols; ++c) {
            const QString name = mModel->headerData(c, Qt::Horizontal).toString();
            if (name.isEmpty()) {
                continue;
            }
            const int occ = seen.value(name, 0);
            seen[ name ]  = occ + 1;
            actualLogical.append(c);
            logicalOfIdent.insert(qMakePair(name, occ), c);
        }
        Q_ASSERT(actualLogical.size() == actual.size());
    }
    QVector< int > ordered;  // store 列序里能匹配的列 ++ 未匹配的实际列（数据列序追加末尾）
    QSet< int > used;
    ordered.reserve(actualLogical.size());
    for (const QPair< QString, int >& ident : storedOrder) {
        const auto it = logicalOfIdent.constFind(ident);
        if (it == logicalOfIdent.constEnd() || used.contains(it.value())) {
            continue;  // 孤儿由 006a 收敛剪掉，这里只跳过（防御）
        }
        ordered.append(it.value());
        used.insert(it.value());
    }
    for (const int logical : actualLogical) {
        if (!used.contains(logical)) {
            ordered.append(logical);
        }
    }
    QSet< int > hidden;  // store 隐藏集合按身份匹配成 logical 集合（孤儿由 006a 收敛剪掉）
    for (const QPair< QString, int >& ident : storedHidden) {
        const auto it = logicalOfIdent.constFind(ident);
        if (it != logicalOfIdent.constEnd()) {
            hidden.insert(it.value());
        }
    }
    int visibleCount = 0;  // freeze：投影期夹紧，不写回 store（列数临时变少时不丢用户设置）
    for (int logical : ordered) {
        if (!hidden.contains(logical)) {
            ++visibleCount;
        }
    }
    const int freeze = qBound(0, mViewStateStore->freezeColumnCount(), visibleCount);
    // 冻结被夹小时发一条用户可见提示（只在生效值变化时发，避免重复投影刷屏）
    if (freeze < mViewStateStore->freezeColumnCount() && mLastProjectedFreeze != freeze) {
        daWarning << tr("Frozen columns were reduced because visible columns changed.");
        // cn:可见列已变化，冻结列数被自动缩减
    }
    mLastProjectedFreeze = freeze;
    // 冻结列集合缓存（isColumnInFrozenArea 纯查表，唯一写入时机在这里）
    mFrozenColumnsLogical.clear();
    for (int i = 0; i < ordered.size() && mFrozenColumnsLogical.size() < freeze; ++i) {
        const int logical = ordered.at(i);
        if (!hidden.contains(logical)) {
            mFrozenColumnsLogical.insert(logical);  // 前 freeze 个非隐藏列
        }
    }
    if (mPaneContainer) {
        mPaneContainer->applyColumnState(ordered, hidden, freeze);
    }
    mApplyingViewState = false;
}

/**
 * @brief 把 store 里的列宽套到指定窗格（列宽段唯一实现）
 *
 * 按「列名 + occurrence」匹配逻辑列号后 resizeSection；尾部预留空白列跳过。
 * 主窗格与冻结窗格共用（006e）。
 * @param pane 目标窗格
 */
void DADataOperateOfDataFrameWidget::applyColumnWidthsToPane(DADataTableView* pane)
{
    if (!pane || !mViewStateStore || !mModel) {
        return;
    }
    QHeaderView* hh = pane->horizontalHeader();
    if (!hh) {
        return;
    }
    const int totalCols = mModel->columnCount();
    QHash< QString, int > seen;  // 列名 -> 已出现次数（即下一列的 occurrence）
    for (int c = 0; c < totalCols; ++c) {
        const QString name = mModel->headerData(c, Qt::Horizontal).toString();
        const int occ      = seen.value(name, 0);
        seen[ name ]       = occ + 1;
        if (name.isEmpty()) {
            continue;  // 尾部预留空白列无列名，不应用
        }
        const int w = mViewStateStore->columnWidth(name, occ);
        if (w > 0) {
            hh->resizeSection(c, w);
        }
    }
}

/**
 * @brief 行头宽只设给归属方
 *
 * 冻结时行头归属冻结窗格（[行号][冻结列][主区]），否则归属主窗格。
 * 只对归属方 setFixedWidth，不给非归属方的隐藏行头写宽度（切回时留脏值）。
 */
void DADataOperateOfDataFrameWidget::applyRowHeaderWidthToOwner()
{
    if (!mViewStateStore) {
        return;
    }
    const int rhw = mViewStateStore->rowHeaderWidth();
    if (rhw <= 0) {
        return;
    }
    QHeaderView* owner = ui->tableView->verticalHeader();
    if (mPaneContainer && mPaneContainer->frozenView()) {
        owner = mPaneContainer->frozenView()->verticalHeader();
    }
    if (owner) {
        owner->setFixedWidth(rhw);
    }
}

/**
 * @brief 本页所有需要投影的窗格
 *
 * 主窗格在前；冻结窗格懒建后追加（006e）。
 * @return 窗格列表（非空）
 */
QList< DADataTableView* > DADataOperateOfDataFrameWidget::paneViews() const
{
    QList< DADataTableView* > res;
    res.append(ui->tableView);
    if (mPaneContainer) {
        if (DADataTableView* frozen = mPaneContainer->frozenView()) {
            res.append(frozen);
        }
    }
    return res;
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

/**
 * @brief 全局列序（含隐藏列）的身份表
 *
 * 口径 = store 的 columnOrder 按 (名, occurrence) 精确匹配 actualColumnIdentities()，
 * 每条只匹配一次，未匹配的实际列按 logical 升序追加末尾；store 为空 = logical 升序。
 * 任何地方要"全局列序身份表"都调这里（006d 列视图对话框的输入），不要在调用点重写匹配。
 * @return 全局列序身份表
 */
QList< QPair< QString, int > > DADataOperateOfDataFrameWidget::orderedColumnIdentities() const
{
    const QList< QPair< QString, int > > actual = actualColumnIdentities();
    QList< QPair< QString, int > > res;
    res.reserve(actual.size());
    if (!mViewStateStore) {
        return actual;
    }
    QSet< QPair< QString, int > > used;
    const QList< QPair< QString, int > > storedOrder = mViewStateStore->columnOrder();
    for (const QPair< QString, int >& ident : storedOrder) {
        if (actual.contains(ident) && !used.contains(ident)) {  // 精确匹配，每条只匹配一次
            res.append(ident);
            used.insert(ident);
        }
    }
    for (const QPair< QString, int >& ident : actual) {
        if (!used.contains(ident)) {
            res.append(ident);
        }
    }
    return res;
}

/**
 * @brief 列视图直接入口：只写 store + 重投影，不入撤销栈
 *
 * 007 定位的对接面（恢复隐藏列后跳转）。唯一投影路径 applyViewStateFromStore，
 * 不另写第二条，也不偷偷 push 命令。
 * @param name 列名
 * @param occurrence 重名消歧位
 * @param hidden 是否隐藏
 */
void DADataOperateOfDataFrameWidget::setColumnHiddenDirectly(const QString& name, int occurrence, bool hidden)
{
    if (!mViewStateStore || name.isEmpty()) {
        return;
    }
    mViewStateStore->setColumnHidden(name, occurrence, hidden);
    applyViewStateFromStore();
}

/**
 * @brief 目标列是否落在冻结区
 *
 * 纯查表（mFrozenColumnsLogical 在 applyViewStateFromStore 尾部与投影同一次
 * 写入）；007 不得自己重算这份语义（真源唯一）。
 * @param logicalColumn 逻辑列号
 * @return 是否在冻结区
 */
bool DADataOperateOfDataFrameWidget::isColumnInFrozenArea(int logicalColumn) const
{
    return mFrozenColumnsLogical.contains(logicalColumn);
}

/**
 * @brief 生效冻结列数
 *
 * 转发容器（本单恒 0，冻结投影 006e 收口）。
 * @return 生效冻结列数
 */
int DADataOperateOfDataFrameWidget::effectiveFrozenColumnCount() const
{
    return mPaneContainer ? mPaneContainer->effectiveFrozenColumnCount() : 0;
}

/**
 * @brief 列轴窗格容器只读访问器
 *
 * 测试与页面内部接线用；APP 不使用、不改其调用点形状。
 * @return 容器指针（构造后非空）
 */
DATablePaneContainer* DADataOperateOfDataFrameWidget::paneContainer() const
{
    return mPaneContainer;
}

/**
 * @brief 隐藏指定列（列头菜单「隐藏此列」）
 *
 * 构造隐藏集合变更命令入栈；已隐藏（集合无差异）不推空命令。不动 mSortSpec。
 * @param logicalColumn 逻辑列号
 */
void DADataOperateOfDataFrameWidget::hideColumnAt(int logicalColumn)
{
    if (!mViewStateStore || !mModel) {
        return;
    }
    if (logicalColumn < 0 || logicalColumn >= dataColumnCount()) {
        return;
    }
    const QString name = mModel->headerData(logicalColumn, Qt::Horizontal).toString();
    if (name.isEmpty()) {
        return;  // 尾部预留空白列不参与列视图
    }
    int occ = 0;
    for (int c = 0; c < logicalColumn; ++c) {
        if (mModel->headerData(c, Qt::Horizontal).toString() == name) {
            ++occ;
        }
    }
    const QSet< QPair< QString, int > > oldHidden = toIdentSet(mViewStateStore->hiddenColumns());
    QSet< QPair< QString, int > > newHidden       = oldHidden;
    newHidden.insert(qMakePair(name, occ));
    if (newHidden == oldHidden) {
        return;  // 已隐藏，不推空命令
    }
    push(new DACommandTableColumnVisibility(mViewStateStore, oldHidden, newHidden,
                                            [ this ]() { applyViewStateFromStore(); }));
}

/**
 * @brief 显示所有列（列头菜单「显示所有列」）
 *
 * 隐藏集合清空入栈；空集合（无差异）不推空命令。不动 mSortSpec。
 */
void DADataOperateOfDataFrameWidget::showAllColumns()
{
    if (!mViewStateStore) {
        return;
    }
    const QSet< QPair< QString, int > > oldHidden = toIdentSet(mViewStateStore->hiddenColumns());
    if (oldHidden.isEmpty()) {
        return;
    }
    push(new DACommandTableColumnVisibility(mViewStateStore, oldHidden, QSet< QPair< QString, int > >(),
                                            [ this ]() { applyViewStateFromStore(); }));
}

/**
 * @brief 冻结前 N 个可见列（0 = 取消冻结）
 *
 * 入撤销栈；与当前计数相同不推空命令。不动 mSortSpec。
 * @param visibleCount 冻结的可见列数
 */
void DADataOperateOfDataFrameWidget::freezeColumnsTo(int visibleCount)
{
    if (!mViewStateStore) {
        return;
    }
    const int newCount = qMax(0, visibleCount);
    if (newCount == mViewStateStore->freezeColumnCount()) {
        return;
    }
    push(new DACommandTableFreezeColumns(mViewStateStore, mViewStateStore->freezeColumnCount(), newCount,
                                         [ this ]() { applyViewStateFromStore(); }));
}

/**
 * @brief 弹「冻结前 N 列」输入框（Ribbon 冻结列主体入口）
 */
void DADataOperateOfDataFrameWidget::promptFreezeColumns()
{
    if (!mViewStateStore) {
        return;
    }
    const int maxFreeze = qMax(0, dataColumnCount() - static_cast< int >(mViewStateStore->hiddenColumns().size()));
    bool ok    = false;
    const int n = QInputDialog::getInt(this,
                                      tr("Freeze Columns"),  // cn:冻结列
                                      tr("Freeze the first N visible columns (0 = no freeze):"),  // cn:冻结前 N 个可见列（0 = 不冻结）：
                                      mViewStateStore->freezeColumnCount(),
                                      0,
                                      maxFreeze,
                                      1,
                                      &ok);
    if (!ok) {
        return;
    }
    freezeColumnsTo(n);
}

/**
 * @brief 容器 columnOrderChanged 驱动：拖动落库入栈
 *
 * 收到的是容器**合并后的全局列序**（含隐藏列）。logical → (列名, occurrence)
 * 转换按数据列序算 occurrence（不按传入顺序）；无差异不推命令。不动 mSortSpec。
 * @param newOrderLogical 新全局列序（logical，含隐藏列）
 */
void DADataOperateOfDataFrameWidget::applyColumnOrderFromView(const QVector< int >& newOrderLogical)
{
    if (!mViewStateStore || !mModel) {
        return;
    }
    QList< QPair< QString, int > > newOrder;
    newOrder.reserve(newOrderLogical.size());
    for (int logical : newOrderLogical) {
        if (logical < 0 || logical >= dataColumnCount()) {
            continue;
        }
        const QString name = mModel->headerData(logical, Qt::Horizontal).toString();
        if (name.isEmpty()) {
            continue;  // 尾部预留空白列不进列序
        }
        int occ = 0;
        for (int c = 0; c < logical; ++c) {
            if (mModel->headerData(c, Qt::Horizontal).toString() == name) {
                ++occ;
            }
        }
        newOrder.append(qMakePair(name, occ));
    }
    QList< QPair< QString, int > > oldOrder = mViewStateStore->columnOrder();
    if (oldOrder.isEmpty()) {
        oldOrder = actualColumnIdentities();  // store 为空 → 旧值视作"数据列序"
    }
    if (oldOrder == newOrder) {
        return;  // 无差异不推命令
    }
    push(new DACommandTableColumnOrder(mViewStateStore, oldOrder, newOrder,
                                       [ this ]() { applyViewStateFromStore(); }));
}

/**
 * @brief 「列」子菜单动作分发（006d）
 *
 * 「冻结到此列」的序号口径 = 该列在可见列序中的位置 + 1（容器
 * visibleColumnOrderLogical() 是"列序剔隐藏"的唯一实现，页面不另写一份）。
 * @param logicalColumn 触发菜单的列（逻辑索引）
 * @param action 子菜单动作
 */
void DADataOperateOfDataFrameWidget::onHeaderColumnViewActionRequested(int logicalColumn,
                                                                       DADataHeaderView::ColumnViewAction action)
{
    switch (action) {
    case DADataHeaderView::ColumnViewAction::HideColumn:
        hideColumnAt(logicalColumn);
        break;
    case DADataHeaderView::ColumnViewAction::ShowAllColumns:
        showAllColumns();
        break;
    case DADataHeaderView::ColumnViewAction::FreezeToColumn: {
        const QVector< int > visibleOrder
            = mPaneContainer ? mPaneContainer->visibleColumnOrderLogical() : QVector< int >();
        const int pos = visibleOrder.indexOf(logicalColumn);  // 隐藏列的表头点不到，必 >= 0
        if (pos >= 0) {
            freezeColumnsTo(pos + 1);
        }
        break;
    }
    case DADataHeaderView::ColumnViewAction::Unfreeze:
        freezeColumnsTo(0);
        break;
    case DADataHeaderView::ColumnViewAction::OpenColumnViewDialog:
        showColumnViewDialog();
        break;
    }
}

/**
 * @brief 打开列视图对话框（006d）
 *
 * 输入 = 全局列序（含隐藏列）的身份表（orderedColumnIdentities() 唯一实现）；
 * 确定后按差异构造一个批量父命令（隐藏集合变更 + 冻结数变更 = 一个撤销单元），
 * 取消不改动；冻结数比较基准用 store 真源旧值（不是夹紧后的生效值）。
 */
void DADataOperateOfDataFrameWidget::showColumnViewDialog()
{
    if (!mViewStateStore || !mModel) {
        return;
    }
    if (!mDialogColumnView) {
        mDialogColumnView = new DADialogTableColumnView(this);
    }
    const QList< QPair< QString, int > > ordered = orderedColumnIdentities();
    if (ordered.isEmpty()) {
        return;
    }
    QSet< QPair< QString, int > > oldHidden = toIdentSet(mViewStateStore->hiddenColumns());
    mDialogColumnView->setup(ordered, oldHidden, effectiveFrozenColumnCount());
    if (mDialogColumnView->exec() != QDialog::Accepted) {
        return;  // 取消不改动任何东西
    }
    const QSet< QPair< QString, int > > newHidden = mDialogColumnView->hiddenColumns();
    const int newFreeze = mDialogColumnView->frozenColumnCount();
    const int oldFreeze = mViewStateStore->freezeColumnCount();
    if (newHidden == oldHidden && newFreeze == oldFreeze) {
        return;  // 无差异不推空命令
    }
    DACommandTableColumnViewBatch* batch = new DACommandTableColumnViewBatch(tr("Column View..."));  // cn:列视图
    if (newHidden != oldHidden) {
        new DACommandTableColumnVisibility(mViewStateStore, oldHidden, newHidden,
                                           [ this ]() { applyViewStateFromStore(); }, QString(), batch);
    }
    if (newFreeze != oldFreeze) {
        new DACommandTableFreezeColumns(mViewStateStore, oldFreeze, newFreeze,
                                        [ this ]() { applyViewStateFromStore(); }, batch);
    }
    push(batch);
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
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (!sm) {
        return QModelIndex();
    }
    const QModelIndex cur = sm->currentIndex();
    if (cur.isValid()) {
        return cur;
    }
    const QItemSelection sel = sm->selection();  // 段列表（不要 selectedIndexes 物化）
    if (sel.isEmpty()) {
        return QModelIndex();
    }
    return sel.first().topLeft();
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
 * @brief 采集列宽改动写入视图状态子存储
 *
 * 只记录用户拖拽：左键按住且鼠标在表头上；应用守卫期间、程序化/布局触发的
 * sectionResized（模型重置、窗口缩放拉伸末列等）一律不记录。尾部预留空白列
 * 与空列名不记录。occurrence = 该列之前同名列的个数（重名列消歧，R20）。
 * source = 触发信号的那个窗格（006b 起不再写死 ui->tableView）。
 * @param source 触发窗格
 * @param logicalIndex 逻辑列号
 * @param newSize 新宽度（像素）
 */
void DADataOperateOfDataFrameWidget::onHeaderSectionResized(DADataTableView* source, int logicalIndex, int newSize)
{
    if (mApplyingViewState || !mViewStateStore || !mModel || !source) {
        return;
    }
    QHeaderView* hh = source->horizontalHeader();
    if (!hh || !(QGuiApplication::mouseButtons() & Qt::LeftButton) || !hh->underMouse()) {
        return;  // 非用户拖拽
    }
    if (logicalIndex < 0 || logicalIndex >= dataColumnCount()) {
        return;  // 尾部预留空白列不记录
    }
    const QString name = mModel->headerData(logicalIndex, Qt::Horizontal).toString();
    if (name.isEmpty()) {
        return;
    }
    int occ = 0;
    for (int c = 0; c < logicalIndex; ++c) {
        if (mModel->headerData(c, Qt::Horizontal).toString() == name) {
            ++occ;
        }
    }
    mViewStateStore->setColumnWidth(name, occ, newSize);
    // 冻结列宽变化 → 重算冻结窗格宽度（006e；非冻结列宽只影响主区，重算幂等无害）
    if (mPaneContainer) {
        mPaneContainer->updateFrozenGeometry();
    }
}

/**
 * @brief 合并样式片段到选中区
 *
 * 自动判定层级：选中整列→列级，选中整行→行级，否则单元格级。
 * fragment 只设部分属性（如只设 background），merge 到目标已有样式，未设属性保留。
 * @param fragment 样式片段
 */
void DADataOperateOfDataFrameWidget::mergeStyleToSelection(const DATableCellStyle& fragment)
{
    if (!mStyleManager) {
        return;
    }
    // 判定应用层级
    QList< int > fullCols = getFullySelectedDataframeColumns(false);
    QList< int > fullRows = getFullySelectedDataframeRows(false);
    const QList< QPoint > cells = getSelectedDataframeCells(false);
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.style.mergeSelection", [&cells, &fullCols, &fullRows]() {
        return QStringLiteral("cells=%1 fullCols=%2 fullRows=%3")
            .arg(cells.size())
            .arg(fullCols.size())
            .arg(fullRows.size());
    });

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));

    if (!fullCols.isEmpty()) {
        // 列级：合并 fragment 到已有列样式
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, newStyle, true);
        }
    } else if (!fullRows.isEmpty()) {
        // 行级：合并 fragment 到已有行样式
        for (int row : std::as_const(fullRows)) {
            const int actualRow       = row;
            DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, newStyle, true);
        }
    } else {
        // 单元格级：合并 fragment 到已有单元格样式
        for (const QPoint& p : cells) {
            // QPoint(actualRow, column)
            const int actualRow       = p.x();
            const int col             = p.y();
            DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, newStyle, true);
        }
    }

    push(cmd.release());
}

/**
 * @brief 设置完整样式到选中区（整体替换，为格式刷准备）
 *
 * 自动判定层级：选中整列→列级，选中整行→行级，否则单元格级。
 * 传入什么样式就设置什么样式，完全替换目标已有样式（不做 merge）。
 * style 中 valid=false 的属性会让目标对应属性变为"未设置"。
 * @param style 完整样式
 */
void DADataOperateOfDataFrameWidget::applyStyleToSelection(const DATableCellStyle& style)
{
    if (!mStyleManager) {
        return;
    }
    QList< int > fullCols = getFullySelectedDataframeColumns(false);
    QList< int > fullRows = getFullySelectedDataframeRows(false);
    const QList< QPoint > cells = getSelectedDataframeCells(false);
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.style.applySelection", [&cells, &fullCols, &fullRows]() {
        return QStringLiteral("cells=%1 fullCols=%2 fullRows=%3")
            .arg(cells.size())
            .arg(fullCols.size())
            .arg(fullRows.size());
    });

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));

    if (!fullCols.isEmpty()) {
        // 列级：直接替换
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, style, false);
        }
    } else if (!fullRows.isEmpty()) {
        // 行级：直接替换
        for (int row : std::as_const(fullRows)) {
            const int actualRow       = row;
            DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, style, false);
        }
    } else {
        // 单元格级：直接替换
        for (const QPoint& p : cells) {
            const int actualRow       = p.x();
            const int col             = p.y();
            DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, style, false);
        }
    }

    push(cmd.release());
}

/**
 * @brief 清除选中区样式
 *
 * 自动判定层级，仅清除有样式的目标的样式。
 */
void DADataOperateOfDataFrameWidget::clearStyleSelection()
{
    if (!mStyleManager) {
        return;
    }
    QList< int > fullCols = getFullySelectedDataframeColumns(false);
    QList< int > fullRows = getFullySelectedDataframeRows(false);
    const QList< QPoint > cells = getSelectedDataframeCells(false);

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));

    if (!fullCols.isEmpty()) {
        for (int col : std::as_const(fullCols)) {
            if (mStyleManager->hasColumnStyle(col)) {
                DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
                cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else if (!fullRows.isEmpty()) {
        for (int row : std::as_const(fullRows)) {
            const int actualRow = row;
            if (mStyleManager->hasRowStyle(actualRow)) {
                DATableCellStyle oldStyle = mStyleManager->getRowStyle(actualRow);
                cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else {
        for (const QPoint& p : cells) {
            const int actualRow = p.x();
            const int col       = p.y();
            if (mStyleManager->hasCellStyle(actualRow, col)) {
                DATableCellStyle oldStyle = mStyleManager->getCellStyle(actualRow, col);
                cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, DATableCellStyle(), false);
            }
        }
    }

    // 避免推送空命令到 undo 栈（选中区无样式时不产生 undo 步骤）
    if (cmd->isEmpty()) {
        return;
    }
    push(cmd.release());
}

/**
 * @brief 清除整表所有样式（可撤销）
 *
 * 记录所有现有样式的 oldStyle，构造撤销命令。
 */
void DADataOperateOfDataFrameWidget::clearStyleAll()
{
    if (!mStyleManager || mStyleManager->isEmpty()) {
        return;
    }
    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(mStyleManager));
    // 收集列级样式
    const auto& cs = mStyleManager->styledColumns();
    for (int col : cs) {
        DATableCellStyle oldStyle = mStyleManager->getColumnStyle(col);
        cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, DATableCellStyle(), false);
    }

    // 收集行级样式
    const auto& rs = mStyleManager->styledRows();
    for (int row : rs) {
        DATableCellStyle oldStyle = mStyleManager->getRowStyle(row);
        cmd->addChange(DACommandTableStyle::Row, row, 0, oldStyle, DATableCellStyle(), false);
    }
    // 收集单元格级样式
    const auto& cells = mStyleManager->styledCells();
    for (const QPair< int, int >& key : cells) {
        DATableCellStyle oldStyle = mStyleManager->getCellStyle(key.first, key.second);
        cmd->addChange(DACommandTableStyle::Cell, key.first, key.second, oldStyle, DATableCellStyle(), false);
    }
    push(cmd.release());
}

namespace
{
// 小选区逐格精确比对的格数上限：选区不超过它且存在行级/单元格级样式时才逐格
// （全选卡顿修复）；超过则不做逐格一致性证明（见 getCurrentCellStyle 注释）
constexpr qint64 c_styleExactScanCellLimit = 10000;

// 逐属性一致性流式累加器：不物化选区样式列表（全选时是 窗口行数×列数 的
// QList<DATableCellStyle>，宽表下千万级堆分配直接冻结界面），只保留首项样式
// 与三个属性的一致标志；判定语义与旧实现的逐属性全量扫描一致
class CellStyleConsensus
{
public:
    void add(const DATableCellStyle& s)
    {
        if (!mHaveFirst) {
            mFirst     = s;
            mHaveFirst = true;
            return;
        }
        mBgOK   = mBgOK && (s.backgroundValid() == mFirst.backgroundValid())
                && (!mFirst.backgroundValid() || s.background() == mFirst.background());
        mFgOK   = mFgOK && (s.foregroundValid() == mFirst.foregroundValid())
                && (!mFirst.foregroundValid() || s.foreground() == mFirst.foreground());
        mFontOK = mFontOK && (s.fontValid() == mFirst.fontValid())
                && (!mFirst.fontValid() || s.font() == mFirst.font());
    }

    DATableCellStyle take()
    {
        DATableCellStyle result;
        if (!mHaveFirst) {
            return result;
        }
        if (mBgOK && mFirst.backgroundValid()) {
            result.setBackground(mFirst.background());
        }
        if (mFgOK && mFirst.foregroundValid()) {
            result.setForeground(mFirst.foreground());
        }
        if (mFontOK && mFirst.fontValid()) {
            result.setFont(mFirst.font());
        }
        return result;
    }

private:
    DATableCellStyle mFirst;
    bool mHaveFirst { false };
    bool mBgOK { true };
    bool mFgOK { true };
    bool mFontOK { true };
};
}  // namespace

/**
 * @brief 获取选中区代表的样式
 *
 * 对每个属性（background/foreground/font）独立判断选中区所有单元格的一致性：
 * - 所有单元格该属性值相同 → 该属性 valid，返回具体值
 * - 该属性在不同单元格有不同值 → 该属性 invalid（ribbon 不调整该控件）
 * 无选中时返回空样式。
 *
 * @note 性能（全选卡顿修复）：旧实现把选区物化成逐格列表再逐格 resolveCellStyle，
 * 全选时是 窗口行数×列数 的 QModelIndex/QPoint/DATableCellStyle 三份大列表
 * （宽表下千万级堆分配，界面冻结数十秒）。现按三级策略计算：
 * 1) 快路径（精确）：选区内没有行级/单元格级样式 → 每格最终样式只由列级样式决定，
 *    一致性只需比较各选中列的列样式（含"无样式"档），O(选中列数)；
 * 2) 小选区（精确）：选区格数 ≤ c_styleExactScanCellLimit 且有行级/单元格级样式，
 *    走逐格流式比对（CellStyleConsensus，不物化列表）；
 * 3) 大选区（近似）：既有行级/单元格级样式、选区又超阈值时，不再逐格证明一致性，
 *    返回空样式（ribbon 控件保持原状态）。样式指示是提示性 UI，为它扫描几十万格
 *    不值得（Excel 同场景也只认活动单元格格式）。
 * 路径 1)/2) 与旧实现逐位等价（含尾部预留空白行列的遍历语义）；3) 是有意取舍。
 * @return 代表样式
 */
DATableCellStyle DADataOperateOfDataFrameWidget::getCurrentCellStyle() const
{
    DATableCellStyle result;
    if (!mStyleManager || !mModel) {
        return result;
    }
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (!sm) {
        return result;
    }
    const QItemSelection sel = sm->selection();
    if (sel.isEmpty()) {
        return result;
    }
    // 选区格数与选中列集合（区间运算，不物化索引）
    qint64 cellCount   = 0;
    QSet< int > selCols;
    for (const QItemSelectionRange& r : sel) {
        cellCount += qint64(r.width()) * qint64(r.height());
        for (int c = r.left(); c <= r.right(); ++c) {
            selCols.insert(c);
        }
    }
    const int windowStart   = mModel->getCacheWindowStartRow();
    const bool rowStyleHit  = selectionIntersectsRowStyles(sel, windowStart);
    const bool cellStyleHit = rowStyleHit ? false : selectionIntersectsCellStyles(sel, windowStart);
    if (!rowStyleHit && !cellStyleHit) {
        // 1) 快路径：每格最终样式 = 列级样式（或无），一致性由选中列的列样式决定
        //（尾部预留空白列没有列样式，参与比较后与旧实现的逐格遍历等价）
        CellStyleConsensus acc;
        for (int c : std::as_const(selCols)) {
            acc.add(mStyleManager->hasColumnStyle(c) ? mStyleManager->getColumnStyle(c) : DATableCellStyle());
        }
        return acc.take();
    }
    if (cellCount > c_styleExactScanCellLimit) {
        // 3) 大选区 + 行/格级样式：放弃精确一致性（见函数注释）
        return result;
    }
    // 2) 小选区：逐格流式比对（范围含尾部预留空白行列，与旧实现一致）
    CellStyleConsensus acc;
    for (const QItemSelectionRange& r : sel) {
        for (int row = r.top(); row <= r.bottom(); ++row) {
            const int actualRow = toActualRow(row);
            for (int col = r.left(); col <= r.right(); ++col) {
                acc.add(mStyleManager->resolveCellStyle(actualRow, col));
            }
        }
    }
    return acc.take();
}

/**
 * @brief 选区是否与行级样式相交（稀疏键表判定，不展开选区）
 *
 * 样式键是数据源绝对行号，减窗口起始行换算回窗口内逻辑行后与区间比对；
 * 全 invalid 的空样式不参与叠加（mergeFrom 只覆盖 valid 属性），视同不相交。
 * @param sel 选区区间列表
 * @param windowStart 当前窗口起始行
 * @return 相交返回 true
 */
bool DADataOperateOfDataFrameWidget::selectionIntersectsRowStyles(const QItemSelection& sel, int windowStart) const
{
    if (!mStyleManager) {
        return false;
    }
    const QList< int > styledRows = mStyleManager->styledRows();
    for (int actualRow : styledRows) {
        if (mStyleManager->getRowStyle(actualRow).isNull()) {
            continue;
        }
        const int viewRow = actualRow - windowStart;
        for (const QItemSelectionRange& r : sel) {
            if (viewRow >= r.top() && viewRow <= r.bottom()) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief 选区是否与单元格级样式相交（稀疏键表判定，不展开选区）
 * @param sel 选区区间列表
 * @param windowStart 当前窗口起始行
 * @return 相交返回 true
 */
bool DADataOperateOfDataFrameWidget::selectionIntersectsCellStyles(const QItemSelection& sel, int windowStart) const
{
    if (!mStyleManager) {
        return false;
    }
    const QList< QPair< int, int > > styledCells = mStyleManager->styledCells();
    for (const QPair< int, int >& key : styledCells) {
        if (mStyleManager->getCellStyle(key.first, key.second).isNull()) {
            continue;
        }
        const int viewRow = key.first - windowStart;
        for (const QItemSelectionRange& r : sel) {
            if (viewRow >= r.top() && viewRow <= r.bottom() && key.second >= r.left() && key.second <= r.right()) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief 收集当前选中区涉及的列号（用于列级显示格式操作）
 *
 * 优先用整列选中（getFullySelectedDataframeColumns），否则取选中单元格涉及的列
 * （getSelectedDataframeCoumns，去重）。两者都空返回空列表。
 * @param ensureInDataframe 是否限定在 dataframe 范围内
 * @return 列号列表
 */
QList< int > collectSelectedColumns(const DADataOperateOfDataFrameWidget* w, bool ensureInDataframe)
{
    QList< int > cols = w->getFullySelectedDataframeColumns(ensureInDataframe);
    if (cols.isEmpty()) {
        cols = w->getSelectedDataframeCoumns(ensureInDataframe);
    }
    return cols;
}

/**
 * @brief 设置列显示格式到选中列
 *
 * 列级操作：对选中区涉及的所有列设置格式（可撤销）。
 * @param fmt 显示格式
 */
void DADataOperateOfDataFrameWidget::setDisplayFormatToSelection(const DATableDisplayFormat& fmt)
{
    if (!mStyleManager) {
        return;
    }
    QList< int > cols = collectSelectedColumns(this, false);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    std::unique_ptr< DACommandTableColumnFormat > cmd(new DACommandTableColumnFormat(mStyleManager));
    for (int col : std::as_const(cols)) {
        DATableDisplayFormat oldFmt =
            mStyleManager->hasColumnFormat(col) ? mStyleManager->getColumnFormat(col) : DATableDisplayFormat();
        cmd->addChange(col, oldFmt, fmt);
    }
    push(cmd.release());
}

/**
 * @brief 清除选中列的显示格式
 *
 * 仅清除显示格式，保留 bg/fg/font 样式。仅对有格式的列记录变更。
 */
void DADataOperateOfDataFrameWidget::clearDisplayFormatFromSelection()
{
    if (!mStyleManager) {
        return;
    }
    QList< int > cols = collectSelectedColumns(this, false);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    std::unique_ptr< DACommandTableColumnFormat > cmd(new DACommandTableColumnFormat(mStyleManager));
    for (int col : std::as_const(cols)) {
        if (mStyleManager->hasColumnFormat(col)) {
            DATableDisplayFormat oldFmt = mStyleManager->getColumnFormat(col);
            cmd->addChange(col, oldFmt, DATableDisplayFormat());  // invalid → 清除
        }
    }
    if (cmd->isEmpty()) {
        return;
    }
    push(cmd.release());
}

/**
 * @brief 获取选中列代表的显示格式
 *
 * 收集选中列各自的列级格式，若全部一致（含全 invalid）返回该格式，
 * 否则返回 invalid（ribbon 格式下拉框回退到 General）。
 * @return 代表格式
 */
DATableDisplayFormat DADataOperateOfDataFrameWidget::getCurrentColumnDisplayFormat() const
{
    DATableDisplayFormat result;  // invalid
    if (!mStyleManager) {
        return result;
    }
    QList< int > cols = collectSelectedColumns(this, false);
    if (cols.isEmpty()) {
        return result;
    }
    QList< DATableDisplayFormat > fmts;
    for (int col : std::as_const(cols)) {
        fmts.append(mStyleManager->hasColumnFormat(col) ? mStyleManager->getColumnFormat(col)
                                                        : DATableDisplayFormat());
    }
    const DATableDisplayFormat& first = fmts.first();
    for (int i = 1; i < fmts.size(); ++i) {
        if (!(fmts[ i ] == first)) {
            return result;  // 不一致 → invalid
        }
    }
    return first;
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
    return mSortSpec;
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
    if (mData.isNull()) {
        return false;
    }
    DAData d                 = mData;
    const DATableDataSource* ts = d.tableSource();
    return !(ts && !ts->isTableEditable());
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

/**
 * @brief 按列排序（重置排序规格为「仅该列」）
 *
 * 排序入口 = 列头右键菜单（DADataHeaderView::ColumnAction::SortAscending/SortDescending
 * → APP 接线到本方法）。只读/惰性源 daWarning 提示一次；拾取模式下不执行。
 * @param logicalIndex 列逻辑索引
 * @param order 排序方向
 */
void DADataOperateOfDataFrameWidget::sortColumn(int logicalIndex, Qt::SortOrder order)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.column", [logicalIndex]() {
        return QStringLiteral("column=%1").arg(logicalIndex);
    });
    applySortAction(logicalIndex, order, false);
}

/**
 * @brief 追加为次级排序列（保留既有规格；该列已在规格中则切换其方向）
 * @param logicalIndex 列逻辑索引
 * @param order 排序方向
 */
void DADataOperateOfDataFrameWidget::addSecondarySortColumn(int logicalIndex, Qt::SortOrder order)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.secondaryColumn", [logicalIndex]() {
        return QStringLiteral("column=%1").arg(logicalIndex);
    });
    applySortAction(logicalIndex, order, true);
}

/**
 * @brief 排序动作统一入口（菜单驱动）：守卫 → 构造规格 → applySortSpec
 *
 * 规格构造规则（与原点击交互一致，入口改为菜单）：
 * - addKey=false：重置为「仅该列，指定方向」；
 * - addKey=true：该列不在规格中 → 追加（指定方向）；已在 → 切换其方向；
 * - 每次动作 = 一个撤销单元。
 * @param logicalIndex 列逻辑索引
 * @param order 排序方向
 * @param addKey 是否保留既有规格追加
 * @return 成功 push 返回 true
 */
bool DADataOperateOfDataFrameWidget::applySortAction(int logicalIndex, Qt::SortOrder order, bool addKey)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.action", [this]() {
        return QStringLiteral("keys=%1").arg(mSortSpec.size());
    });
    if (mHeaderPickMode) {
        return false;  // 拾取会话中不排序（与点击互斥语义一致）
    }
    if (logicalIndex < 0 || logicalIndex >= dataColumnCount()) {
        return false;  // 尾部预留空白列不排序
    }
    if (!isTableEditable()) {
        daWarning << tr("This data source is read-only and cannot be sorted.");  // cn:该数据源为只读，无法排序
        return false;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || dataRowCount() <= 1) {
        return false;  // 非 dataframe 或无可排序的行
    }
    QVector< QPair< int, Qt::SortOrder > > newSpec = mSortSpec;
    if (addKey) {
        int idx = -1;
        for (int i = 0; i < newSpec.size(); ++i) {
            if (newSpec.at(i).first == logicalIndex) {
                idx = i;
                break;
            }
        }
        if (idx >= 0) {
            newSpec[ idx ].second = (newSpec.at(idx).second == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            newSpec.append(qMakePair(logicalIndex, order));
        }
    } else {
        newSpec.clear();
        newSpec.append(qMakePair(logicalIndex, order));
    }
    return applySortSpec(newSpec);
}

/**
 * @brief 应用排序规格（构造命令 + 回调接线 + push）
 *
 * 命令生效后按设计稿 §4.1 顺序联动：先样式键重映射（directionalCallback 里
 * styleManager()->onRowsReordered(最近应用置换)）→ 再模型刷新（notifyDataChanged
 * 清块缓存 + 行头 index 变更通知）→ 最后按置换修正选区、视口回顶。
 * 排序路径计时走慢日志嵌套作用域（超阈值才写 slow.log，验收口径：10 万行 ≤300 ms）。
 * @param newSpec 新排序规格
 * @return 成功 push 返回 true；算子失败（如混合类型列）提示并保持原规格返回 false
 */
bool DADataOperateOfDataFrameWidget::applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& newSpec)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.applySpec", [this, &newSpec]() {
        return QStringLiteral("rows=%1 keys=%2").arg(dataRowCount()).arg(newSpec.size());
    });
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return false;
    }
    QVector< QPair< int, bool > > keys;
    keys.reserve(newSpec.size());
    for (const auto& k : newSpec) {
        keys.append(qMakePair(k.first, k.second == Qt::AscendingOrder));
    }
    std::unique_ptr< DACommandDataFrame_sortRows > cmd(new DACommandDataFrame_sortRows(df, keys));
    QPointer< DADataTableModel > modle          = mModel;
    QPointer< DADataTableView > view            = ui->tableView;
    DACommandDataFrame_sortRows* cmdRaw         = cmd.get();
    DATableStyleManager* styleMgr               = mStyleManager;
    DATableValidationStore* vaStore             = mValidationStore;
    DATableNoteStore* noteStore                 = mNoteStore;
    DADataOperateOfDataFrameWidget* self        = this;
    // ① 样式键重映射（先于模型刷新；exec/undo 均传最近应用的置换，undo 传逆置换即等价按逆置换重映射）
    cmd->setDirectionalCallBack([ styleMgr, vaStore, noteStore, cmdRaw ](bool isUndo) {
        Q_UNUSED(isUndo);
        if (styleMgr) {
            styleMgr->onRowsReordered(cmdRaw->lastAppliedPermutation());
        }
        // 批注与样式同源（005 T1.26）：同一置换重映射，「批注跟着行走」
        if (noteStore) {
            noteStore->onRowsReordered(cmdRaw->lastAppliedPermutation());
        }
        // 排序移动行位置 → 唯一性违规缓存的行号失效（005 T1.25），保守全失效
        if (vaStore) {
            vaStore->invalidateUniqueCache();
        }
    });
    // ② 模型刷新 → ③ 选区跟随 → 视口回顶（回调范式照 paste 的逐格 notifyDataChanged 写法）
    cmd->setCallBack([ modle, view, cmdRaw, self ]() {
        // 排序不改变列值集合 → 条件格式列统计不失效（003 性能决策）：
        // 统计失效挂点走 cellValueChanged 精确信号，排序路径不发该信号，天然不失效
        // 各阶段用嵌套慢日志作用域计时：只在超过各自阈值时写入 slow.log
        if (modle) {
            DA_SLOW_LOG_SCOPE("dagui.table.sort.notify");
            const int rows = modle->actualRowCount();
            const int cols = modle->columnCount();
            modle->notifyDataChanged(0, 0, qMax(0, rows - 1), qMax(0, cols - 1));
            // 行头显示 index 值，排序后 index 随行移动，需通知垂直表头重取
            modle->notifyHeaderDataChanged(Qt::Vertical, 0, qMax(0, rows - 1));
        }
        if (view) {
            {
                DA_SLOW_LOG_SCOPE("dagui.table.sort.remap");
                self->remapSelectionByPermutation(cmdRaw->lastAppliedPermutation());
            }
            DA_SLOW_LOG_SCOPE("dagui.table.sort.showRow");
            view->showActualRow(0);  // 视口回顶（行位置整体变了，含 undo/redo）
        }
        // 排序是破坏式重排：置工程未保存标记（exec/undo 都发，与 004 dataModified 口径一致；
        // 修复 002 遗留缺口——此前排序从不标脏，排序后关工程不提示保存）
        Q_EMIT self->dataModified();
    });
    // 排序是整表操作（大表算子+刷新可达秒级）：全程忙等待光标（RAII 恢复），避免 UI 假死观感
    DAWaitCursorScoped waitCursor;
    bool execOk = false;
    {
        DA_SLOW_LOG_SCOPE("dagui.table.sort.exec");
        execOk = cmd->exec();
    }
    if (!execOk) {
        // 算子报错（如 object 列混合类型不可比较）：排序规格回滚到变更前
        daWarning << tr("Cannot sort: the column contains values of incomparable mixed types.");  // cn:无法排序：该列包含不可比较的混合类型
        return false;
    }
    {
        DA_SLOW_LOG_SCOPE("dagui.table.sort.push");
        push(cmd.release());
    }
    mSortSpec = newSpec;
    forEachPaneHeader([ this ](DADataHeaderView* hh) { hh->setSortSpec(mSortSpec); });
    return true;
}

/**
 * @brief 按最近应用的置换修正选区（选中行随排序跟随）
 *
 * appliedPerm 为新→旧置换（new[i]=old[appliedPerm[i]]），旧行号 old 的新位置 =
 * inv[old]（逆置换）。行数不变、滑动窗未平移，回调时视图选中的位置号即重排前的
 * 数据行号，直接按逆置换映射到新位置重建选区；映射出窗口的行跳过。
 *
 * **性能约束（2026-10-18 修复）**：禁止逐格重建——Qt 的「整列选择」本身就会产生
 * 每行一个 range 的选区（2 万行窗口 = 2 万个 range），逐格映射再 ClearAndSelect
 * 会让视图对上万散落 range 逐个做脏区合并，实测 3.2 万行表耗时 6~19 秒。
 * 这里的算法：先按列跨度 (left,right) 分组归并行区间，再分两条路径——
 * 1. 整窗快速路径：归并后覆盖全部模型行（典型=整列选择）→ 保持整列选中。
 *    置换是双射，「列仍被选中」与 Excel 排序后的视觉语义一致（滚出窗口的新位置
 *    本就不可见；换入窗口的行随列视为选中）；
 * 2. 一般路径：逐行映射 → 排序 → 游程压缩，每个连续段一个 range；总段数超过
 *    c_maxRemapSelRuns 时降级为只保留当前格映射（响应性优先，qDebug 说明）。
 * @param appliedPerm 最近应用的置换（exec=正置换 / undo=逆置换）
 */
void DADataOperateOfDataFrameWidget::remapSelectionByPermutation(const QVector< int >& appliedPerm)
{
    // 游程压缩后的 range 数上限：超过则视为「巨大散落选区」，降级为只映射当前格
    static constexpr int c_maxRemapSelRuns = 2048;
    DADataTableView* view          = ui->tableView;
    QItemSelectionModel* sm        = view ? view->selectionModel() : nullptr;
    QAbstractItemModel* m          = view ? view->model() : nullptr;
    if (!sm || !m || appliedPerm.isEmpty() || !sm->hasSelection()) {
        return;
    }
    const QVector< int > inv = DATableKeyRemap::inversePermutation(appliedPerm);
    const int modelRows      = m->rowCount();
    auto mapRow              = [ this, view, &inv ](int viewRow) -> int {
        const int oldActual = toActualRow(viewRow);
        if (oldActual < 0 || oldActual >= inv.size() || inv.at(oldActual) < 0) {
            return -1;
        }
        return view->toLogicalRow(inv.at(oldActual));  // 目标行不在当前滑动窗内时返回 -1
    };
    // 按列跨度分组收集行区间（const 引用迭代，不触发 COW）
    const QItemSelection& oldSel = sm->selection();
    QMap< QPair< int, int >, QVector< QPair< int, int > > > spanRows;
    for (const QItemSelectionRange& r : oldSel) {
        spanRows[ qMakePair(r.left(), r.right()) ].append(qMakePair(r.top(), r.bottom()));
    }
    QItemSelection newSel;
    int totalRuns     = 0;
    bool fallback     = false;
    for (auto it = spanRows.constBegin(); it != spanRows.constEnd(); ++it) {
        const int left  = it.key().first;
        const int right = it.key().second;
        // 行区间排序 + 归并
        QVector< QPair< int, int > > iv = it.value();
        std::sort(iv.begin(), iv.end());
        QVector< QPair< int, int > > merged;
        merged.reserve(iv.size());
        for (int k = 0; k < iv.size(); ++k) {
            const QPair< int, int >& p = iv.at(k);
            if (!merged.isEmpty() && p.first <= merged.last().second + 1) {
                merged.last().second = qMax(merged.last().second, p.second);
            } else {
                merged.append(p);
            }
        }
        // 整窗快速路径：选区覆盖全部模型行（典型=整列选择）→ 保持整列
        if (modelRows > 0 && merged.size() == 1 && merged.at(0).first == 0 && merged.at(0).second == modelRows - 1) {
            newSel.select(m->index(0, left), m->index(modelRows - 1, right));
            continue;
        }
        // 一般路径：逐行映射（出窗跳过）→ 排序 → 游程压缩
        QVector< int > mapped;
        for (int k = 0; k < merged.size(); ++k) {
            for (int row = merged.at(k).first; row <= merged.at(k).second; ++row) {
                const int nr = mapRow(row);
                if (nr >= 0) {
                    mapped.append(nr);
                }
            }
        }
        std::sort(mapped.begin(), mapped.end());
        int i = 0;
        while (i < mapped.size()) {
            int j = i;
            while (j + 1 < mapped.size() && mapped.at(j + 1) == mapped.at(j) + 1) {
                ++j;
            }
            newSel.select(m->index(mapped.at(i), left), m->index(mapped.at(j), right));
            ++totalRuns;
            i = j + 1;
            if (totalRuns > c_maxRemapSelRuns) {
                fallback = true;
                break;
            }
        }
        if (fallback) {
            break;
        }
    }
    if (fallback) {
        // 巨大散落选区降级：放弃细粒度保留（重建上万散落段会让视图卡死数秒），只保留当前格映射
        newSel.clear();
        qDebug().noquote() << QStringLiteral("DADataOperateOfDataFrameWidget::remapSelectionByPermutation: "
                                             "selection too scattered (> %1 runs), only current index remapped")
                                  .arg(c_maxRemapSelRuns);
    }
    QModelIndex newCur;
    const QModelIndex cur = sm->currentIndex();
    if (cur.isValid()) {
        const int newLogical = mapRow(cur.row());
        if (newLogical >= 0) {
            newCur = m->index(newLogical, cur.column());
        }
    }
    sm->select(newSel, QItemSelectionModel::ClearAndSelect);
    if (newCur.isValid()) {
        sm->setCurrentIndex(newCur, QItemSelectionModel::NoUpdate);
    }
}

/**
 * @brief 打开查找/替换浮动条（懒建，非模态，页级持有）
 * @param expandReplace true 展开替换行（Ctrl+H），false 仅查找（Ctrl+F）
 */
void DADataOperateOfDataFrameWidget::showFindReplace(bool expandReplace)
{
    if (!mFindReplaceDialog) {
        mFindReplaceDialog = new DADialogFindReplace(this);
        connect(mFindReplaceDialog, &DADialogFindReplace::searchRequested, this, &DADataOperateOfDataFrameWidget::onFindSearchRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::nextRequested, this, &DADataOperateOfDataFrameWidget::onFindNextRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::previousRequested, this, &DADataOperateOfDataFrameWidget::onFindPrevRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::replaceRequested, this, &DADataOperateOfDataFrameWidget::onFindReplaceRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::replaceAllRequested, this, &DADataOperateOfDataFrameWidget::onFindReplaceAllRequested);
        connect(mFindReplaceDialog, &DADialogFindReplace::closed, this, &DADataOperateOfDataFrameWidget::onFindDialogClosed);
    }
    mFindReplaceDialog->activate(expandReplace);
    updateFindCountLabel();
}

/**
 * @brief 关闭查找/替换浮动条（清高亮经 closed → onFindDialogClosed）
 */
void DADataOperateOfDataFrameWidget::closeFindReplace()
{
    if (mFindReplaceDialog && mFindReplaceDialog->isVisible()) {
        mFindReplaceDialog->close();
    }
}

int DADataOperateOfDataFrameWidget::findHitCount() const
{
    return mFindHits.size();
}

int DADataOperateOfDataFrameWidget::currentFindHitIndex() const
{
    return mFindHitIndex;
}

/**
 * @brief 打开条件格式规则编辑对话框（003 T1.5）
 *
 * 作用列 = 当前选区覆盖的列集合（无选区回退当前格所在列）；对话框编辑期间
 * 用 store 的「预览规则集」实时叠加（不落库），确定后按 resultRules 与 store
 * 现值的差异生成增/删/改/移子命令的父命令入页撤销栈（D4：一切写操作入栈；
 * 条件格式不写 DataFrame，红线3），取消还原。惰性/只读源上色阶与数据条
 * 在对话框内置灰并给出说明（§1.4-2），单元格规则正常。
 */
void DADataOperateOfDataFrameWidget::showConditionalFormatDialog(int initialType)
{
    if (!mConditionalFormatStore) {
        // 开发诊断：无元数据注册表（正常 APP 链路不会出现）
        qWarning() << "DADataOperateOfDataFrameWidget::showConditionalFormatDialog: metadata registry unavailable";
        return;
    }
    QList< int > cols = getSelectedDataframeCoumns();
    if (cols.isEmpty()) {
        const int c = getSelectedOneDataframeColumn();
        if (c >= 0) {
            cols.append(c);
        }
    }
    if (cols.isEmpty()) {
        daWarning << tr("Please select a column first");  // cn:请先选择列
        return;
    }
    // 列名：dataframe 用 columns()；惰性表等无 dataframe 时退回模型表头文本
    QStringList columnNames;
    DAPyDataFrame df = getDataframe();
    if (!df.isNone()) {
        columnNames = df.columns();
    } else if (mModel) {
        const int cc = mModel->columnCount();
        for (int c = 0; c < cc; ++c) {
            columnNames.append(mModel->headerData(c, Qt::Horizontal).toString());
        }
    }
    DADialogConditionalFormat dlg(mConditionalFormatStore, this);
    dlg.setup(columnNames, cols, isColumnStatsSourceAvailable(), initialType);
    if (QDialog::Accepted == dlg.exec()) {
        QUndoCommand* cmd = buildConditionalFormatCommands(dlg.resultRules());
        if (cmd) {
            push(cmd);
        }
    }
}

/**
 * @brief 清除选中列的条件格式规则（一次命令，可撤销）
 */
void DADataOperateOfDataFrameWidget::clearConditionalFormatFromSelection()
{
    if (!mConditionalFormatStore) {
        return;
    }
    const QList< int > cols = getSelectedDataframeCoumns();
    if (cols.isEmpty()) {
        daWarning << tr("Please select a column first");  // cn:请先选择列
        return;
    }
    QUndoCommand* parent = new QUndoCommand(tr("Clear Conditional Format Rules"));  // cn:清除条件格式规则
    bool any             = false;
    for (int col : cols) {
        if (mConditionalFormatStore->hasRules(col)) {
            new DACommandConditionalFormat_clearColumn(mConditionalFormatStore, col, parent);
            any = true;
        }
    }
    if (!any) {
        delete parent;
        daInfo << tr("The selected columns have no conditional format rules");  // cn:所选列没有条件格式规则
        return;
    }
    push(parent);
}

/**
 * @brief 条件格式入口作用列
 *
 * 选区覆盖列；无选区回退当前格所在列；空列表 = 无有效列（调用方提示）。
 * @return 作用列（升序不保证，按选区返回序）
 */
QList< int > DADataOperateOfDataFrameWidget::conditionalFormatScopeColumns() const
{
    QList< int > cols = getSelectedDataframeCoumns();
    if (cols.isEmpty()) {
        const int c = getSelectedOneDataframeColumn();
        if (c >= 0) {
            cols.append(c);
        }
    }
    return cols;
}

/**
 * @brief 给每个作用列追加一条规则（预设一键施加）
 *
 * 新规则集 = 落库规则 + 追加项，经 buildConditionalFormatCommands 差异生成撤销命令入栈
 * （D4：一切写操作入栈；条件格式不写 DataFrame，红线3）。
 * @param rule 预设规则
 */
void DADataOperateOfDataFrameWidget::applyConditionalFormatRuleToScope(const DATableConditionalFormatRule& rule)
{
    if (!mConditionalFormatStore) {
        return;
    }
    const QList< int > cols = conditionalFormatScopeColumns();
    if (cols.isEmpty()) {
        daWarning << tr("Please select a column first");  // cn:请先选择列
        return;
    }
    QHash< int, QVector< DATableConditionalFormatRule > > newRules;
    for (int c : cols) {
        QVector< DATableConditionalFormatRule > v = mConditionalFormatStore->columnRules(c);
        v.append(rule);
        newRules.insert(c, v);
    }
    if (QUndoCommand* cmd = buildConditionalFormatCommands(newRules)) {
        push(cmd);
    }
}

/**
 * @brief 突出显示单元格规则/重复值快速对话框：确定后施加到作用列
 * @param cond 条件（重复/唯一在对话框内可切换）
 */
void DADataOperateOfDataFrameWidget::showQuickCellRuleDialog(DATableConditionalFormatRule::Condition cond)
{
    DADialogConditionalFormatQuickRule dlg(cond, this);
    if (QDialog::Accepted == dlg.exec()) {
        applyConditionalFormatRuleToScope(dlg.resultRule());
    }
}

/**
 * @brief 清除全表条件格式规则（一次命令，可撤销）
 */
void DADataOperateOfDataFrameWidget::clearAllConditionalFormatRules()
{
    if (!mConditionalFormatStore) {
        return;
    }
    const QList< int > cols = mConditionalFormatStore->columnsWithRules();
    if (cols.isEmpty()) {
        daInfo << tr("The table has no conditional format rules");  // cn:本表没有条件格式规则
        return;
    }
    QUndoCommand* parent = new QUndoCommand(tr("Clear Conditional Format Rules"));  // cn:清除条件格式规则
    for (int c : cols) {
        new DACommandConditionalFormat_clearColumn(mConditionalFormatStore, c, parent);
    }
    push(parent);
}

/**
 * @brief 填充条件格式菜单（WPS 式预设入口）
 *
 * 结构（分组间分隔符）：数据条▸ / 色阶▸ ‖ 突出显示单元格规则▸ ‖ 清除规则▸ ‖ 自定义设置...。
 * 数据条/色阶子菜单为预设缩略图一键施加 + 「其他规则...」进自定义对话框；
 * 突出显示子菜单含大于/小于/介于/等于/文本包含（快速对话框）、发生日期▸（十项一键）、
 * 重复值...（快速对话框）与「其他规则...」；惰性/只读源上数据条与色阶子菜单置灰。
 * @param menu 目标菜单（先 clear 再填充；Ribbon 与右键子菜单共用）
 */
void DADataOperateOfDataFrameWidget::populateConditionalFormatMenu(QMenu* menu)
{
    if (!menu) {
        return;
    }
    using CfRule  = DATableConditionalFormatRule;
    using CfCond  = DATableConditionalFormatRule::Condition;
    using CfType  = DATableConditionalFormatRule::Type;
    const QIcon cfIcon(QStringLiteral(":/DAGui/icon/conditional-format.svg"));
    const bool scaleBarAvailable = isColumnStatsSourceAvailable();
    menu->clear();

    // 预设色名（与 DAConditionalFormatPresets 数据条六色同序）
    const QStringList barColorNames { tr("Blue"),    // cn:蓝色
                                      tr("Green"),    // cn:绿色
                                      tr("Red"),      // cn:红色
                                      tr("Yellow"),   // cn:黄色
                                      tr("Cyan"),     // cn:青色
                                      tr("Purple") };  // cn:紫色

    // —— 数据条▸（渐变 6 + 实心 6 + 其他规则）——
    QMenu* barMenu = menu->addMenu(cfIcon, tr("Data Bars"));  // cn:数据条
    barMenu->setEnabled(scaleBarAvailable);
    {
        const QList< DACFDataBarPreset > presets = DAConditionalFormatPresets::dataBarPresets();
        QAction* head = barMenu->addAction(tr("Gradient Fill"));  // cn:渐变填充
        head->setEnabled(false);
        for (int i = 0; i < 6 && i < presets.size(); ++i) {
            const DACFDataBarPreset ps = presets.at(i);
            QAction* a = barMenu->addAction(DAConditionalFormatPresets::dataBarThumb(ps.color, ps.gradient),
                                            tr("%1 Gradient Fill").arg(barColorNames.value(i)));  // cn:%1渐变填充
            connect(a, &QAction::triggered, this, [ this, ps ]() {
                CfRule r;
                r.type       = CfType::DataBar;
                r.barColor   = ps.color;
                r.barGradient = true;
                applyConditionalFormatRuleToScope(r);
            });
        }
        barMenu->addSeparator();
        QAction* head2 = barMenu->addAction(tr("Solid Fill"));  // cn:实心填充
        head2->setEnabled(false);
        for (int i = 6; i < presets.size(); ++i) {
            const DACFDataBarPreset ps = presets.at(i);
            QAction* a = barMenu->addAction(DAConditionalFormatPresets::dataBarThumb(ps.color, ps.gradient),
                                            tr("%1 Solid Fill").arg(barColorNames.value(i - 6)));  // cn:%1实心填充
            connect(a, &QAction::triggered, this, [ this, ps ]() {
                CfRule r;
                r.type     = CfType::DataBar;
                r.barColor = ps.color;
                applyConditionalFormatRuleToScope(r);
            });
        }
        barMenu->addSeparator();
        QAction* other = barMenu->addAction(tr("Other Rules..."));  // cn:其他规则...
        connect(other, &QAction::triggered, this, [ this ]() {
            showConditionalFormatDialog(static_cast< int >(CfType::DataBar));
        });
    }

    // —— 色阶▸（12 预设 + 其他规则）——
    QMenu* scaleMenu = menu->addMenu(cfIcon, tr("Color Scales"));  // cn:色阶
    scaleMenu->setEnabled(scaleBarAvailable);
    {
        const QList< DACFColorScalePreset > presets = DAConditionalFormatPresets::colorScalePresets();
        for (const DACFColorScalePreset& ps : presets) {
            QAction* a = scaleMenu->addAction(DAConditionalFormatPresets::colorScaleThumb(ps.colors),
                                              tr("%1-Color Scale").arg(ps.colors.size()));  // cn:%1色色阶
            connect(a, &QAction::triggered, this, [ this, ps ]() {
                CfRule r;
                r.type          = CfType::ColorScale;
                r.threeColorScale = (ps.colors.size() >= 3);
                if (ps.colors.size() >= 3) {
                    r.scaleMinColor = ps.colors.at(0);
                    r.scaleMidColor = ps.colors.at(1);
                    r.scaleMaxColor = ps.colors.at(2);
                } else if (ps.colors.size() == 2) {
                    r.scaleMinColor = ps.colors.at(0);
                    r.scaleMaxColor = ps.colors.at(1);
                }
                applyConditionalFormatRuleToScope(r);
            });
        }
        scaleMenu->addSeparator();
        QAction* other = scaleMenu->addAction(tr("Other Rules..."));  // cn:其他规则...
        connect(other, &QAction::triggered, this, [ this ]() {
            showConditionalFormatDialog(static_cast< int >(CfType::ColorScale));
        });
    }
    menu->addSeparator();

    // —— 突出显示单元格规则▸ ——
    QMenu* hlMenu = menu->addMenu(cfIcon, tr("Highlight Cells Rules"));  // cn:突出显示单元格规则
    {
        QAction* aGreater = hlMenu->addAction(tr("Greater Than..."));  // cn:大于...
        connect(aGreater, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Greater); });
        QAction* aLess = hlMenu->addAction(tr("Less Than..."));  // cn:小于...
        connect(aLess, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Less); });
        QAction* aBetween = hlMenu->addAction(tr("Between..."));  // cn:介于...
        connect(aBetween, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Between); });
        QAction* aEqual = hlMenu->addAction(tr("Equal To..."));  // cn:等于...
        connect(aEqual, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Equal); });
        QAction* aContains = hlMenu->addAction(tr("Text Contains..."));  // cn:文本包含...
        connect(aContains, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Contains); });
        // 发生日期▸：十项一键施加（默认橙红底白字样式）
        QMenu* dateMenu = hlMenu->addMenu(tr("A Date Occurring"));  // cn:发生日期
        const QList< QPair< CfRule::DateOccurrence, QString > > dateItems {
            { CfRule::DateOccurrence::Yesterday, tr("Yesterday") },                // cn:昨天
            { CfRule::DateOccurrence::Today, tr("Today") },                        // cn:今天
            { CfRule::DateOccurrence::Tomorrow, tr("Tomorrow") },                  // cn:明天
            { CfRule::DateOccurrence::Last7Days, tr("In the Last 7 Days") },       // cn:最近 7 天
            { CfRule::DateOccurrence::LastWeek, tr("Last Week") },                 // cn:上周
            { CfRule::DateOccurrence::ThisWeek, tr("This Week") },                 // cn:本周
            { CfRule::DateOccurrence::NextWeek, tr("Next Week") },                 // cn:下周
            { CfRule::DateOccurrence::LastMonth, tr("Last Month") },               // cn:上月
            { CfRule::DateOccurrence::ThisMonth, tr("This Month") },               // cn:本月
            { CfRule::DateOccurrence::NextMonth, tr("Next Month") },               // cn:下月
        };
        for (const auto& item : dateItems) {
            QAction* a = dateMenu->addAction(item.second);
            connect(a, &QAction::triggered, this, [ this, item ]() {
                CfRule r;
                r.type      = CfType::CellRule;
                r.condition = CfCond::DateOccurring;
                r.value1    = static_cast< int >(item.first);
                const DACFHighlightStyle def = DAConditionalFormatPresets::highlightStyles().value(0);
                if (def.hasBackground()) {
                    r.style.setBackground(QBrush(def.background));
                }
                if (def.hasForeground()) {
                    r.style.setForeground(def.foreground);
                }
                applyConditionalFormatRuleToScope(r);
            });
        }
        QAction* aDup = hlMenu->addAction(tr("Duplicate Values..."));  // cn:重复值...
        connect(aDup, &QAction::triggered, this, [ this ]() { showQuickCellRuleDialog(CfCond::Duplicate); });
        hlMenu->addSeparator();
        QAction* aOther = hlMenu->addAction(tr("Other Rules..."));  // cn:其他规则...
        connect(aOther, &QAction::triggered, this, [ this ]() {
            showConditionalFormatDialog(static_cast< int >(CfType::CellRule));
        });
    }
    menu->addSeparator();

    // —— 清除规则▸ ——
    QMenu* clearMenu = menu->addMenu(tr("Clear Rules"));  // cn:清除规则
    {
        QAction* aSel = clearMenu->addAction(tr("Clear Rules from Selected Columns"));  // cn:清除所选列的规则
        connect(aSel, &QAction::triggered, this, &DADataOperateOfDataFrameWidget::clearConditionalFormatFromSelection);
        QAction* aAll = clearMenu->addAction(tr("Clear Rules from Entire Sheet"));  // cn:清除全表的规则
        connect(aAll, &QAction::triggered, this, &DADataOperateOfDataFrameWidget::clearAllConditionalFormatRules);
    }
    menu->addSeparator();

    // —— 自定义设置...（规则管理对话框）——
    QAction* aManage = menu->addAction(cfIcon, tr("Custom Rules..."));  // cn:自定义设置...
    connect(aManage, &QAction::triggered, this, [ this ]() { showConditionalFormatDialog(); });
}

/**
 * @brief 色阶/数据条的数据源可用性（§1.4-2）
 *
 * 判定走 DAData 持有的 DATableDataSource（不新造平行接口）：惰性加载
 * （isLazyLoaded，无法全量列统计）或不可编辑（isTableEditable 为假，数据库表）
 * 均不可用；无 tableSource 的普通 dataframe 可用。
 * @return 色阶/数据条可用返回 true
 */
bool DADataOperateOfDataFrameWidget::isColumnStatsSourceAvailable() const
{
    if (mData.isNull()) {
        return false;
    }
    DAData d                    = mData;
    const DATableDataSource* ts = d.tableSource();
    return !(ts && (ts->isLazyLoaded() || !ts->isTableEditable()));
}

/**
 * @brief 规则集差异 → 撤销命令（父命令聚合，旧新对象记录范式 D4）
 *
 * 每列独立 diff：新集为空 → clearColumn；旧集为空 → 逐条 add；同元素多重集
 * 仅顺序不同 → move 序列；其余 → 位置式 diff（前缀 modify + 尾部 add/remove）。
 * undo/redo 结果对象与差异前/后完全一致（单测钉住）。
 * @param newRules 对话框结果（列 → 规则列表）
 * @return 父命令；无任何差异返回 nullptr（调用方不推空命令）
 */
QUndoCommand* DADataOperateOfDataFrameWidget::buildConditionalFormatCommands(
    const QHash< int, QVector< DATableConditionalFormatRule > >& newRules) const
{
    QUndoCommand* parent = new QUndoCommand(tr("Change Conditional Format Rules"));  // cn:更改条件格式规则
    bool any             = false;
    QList< int > cols    = newRules.keys();
    std::sort(cols.begin(), cols.end());
    for (int col : std::as_const(cols)) {
        // 预览已在对话框 accept 时清除，columnRules 返回落库值
        const QVector< DATableConditionalFormatRule > oldRules = mConditionalFormatStore->columnRules(col);
        const QVector< DATableConditionalFormatRule > newCol   = newRules.value(col);
        if (oldRules == newCol) {
            continue;
        }
        any = true;
        if (newCol.isEmpty()) {
            new DACommandConditionalFormat_clearColumn(mConditionalFormatStore, col, parent);
            continue;
        }
        if (oldRules.isEmpty()) {
            for (int i = 0; i < newCol.size(); ++i) {
                new DACommandConditionalFormat_addRule(mConditionalFormatStore, col, newCol.at(i), i, parent);
            }
            continue;
        }
        // 纯重排检测（同元素多重集）：move 序列（保持"移动"语义，与对话框上移/下移对应）
        QVector< DATableConditionalFormatRule > rest = oldRules;
        bool permutation                             = true;
        for (const DATableConditionalFormatRule& r : newCol) {
            int found = -1;
            for (int i = 0; i < rest.size(); ++i) {
                if (rest.at(i) == r) {
                    found = i;
                    break;
                }
            }
            if (found < 0) {
                permutation = false;
                break;
            }
            rest.removeAt(found);
        }
        if (permutation && rest.isEmpty()) {
            QVector< DATableConditionalFormatRule > cur = oldRules;
            for (int i = 0; i < newCol.size(); ++i) {
                int j = i;
                while (j < cur.size() && !(cur.at(j) == newCol.at(i))) {
                    ++j;
                }
                if (j > i && j < cur.size()) {
                    new DACommandConditionalFormat_moveRule(mConditionalFormatStore, col, j, i, parent);
                    const DATableConditionalFormatRule r = cur.takeAt(j);
                    cur.insert(i, r);
                }
            }
            continue;
        }
        // 位置式 diff：公共前缀 modify + 尾部 add（新更长）/ remove（旧更长，从尾到头保持索引有效）
        const int n = qMin(oldRules.size(), newCol.size());
        for (int i = 0; i < n; ++i) {
            if (!(oldRules.at(i) == newCol.at(i))) {
                new DACommandConditionalFormat_modifyRule(mConditionalFormatStore, col, i, oldRules.at(i), newCol.at(i), parent);
            }
        }
        for (int i = n; i < newCol.size(); ++i) {
            new DACommandConditionalFormat_addRule(mConditionalFormatStore, col, newCol.at(i), i, parent);
        }
        for (int i = oldRules.size() - 1; i >= n; --i) {
            new DACommandConditionalFormat_removeRule(mConditionalFormatStore, col, i, parent);
        }
    }
    if (!any) {
        delete parent;
        return nullptr;
    }
    return parent;
}

void DADataOperateOfDataFrameWidget::onFindSearchRequested()
{
    runFindScan();
}

void DADataOperateOfDataFrameWidget::onFindPrevRequested()
{
    if (mFindHits.isEmpty()) {
        // 无命中（如刚打开未扫描）：先扫一次再导航
        runFindScan();
        if (mFindHits.isEmpty()) {
            return;
        }
        gotoFindHit(mFindHits.size() - 1);
        return;
    }
    gotoFindHit(mFindHitIndex - 1);
}

void DADataOperateOfDataFrameWidget::onFindNextRequested()
{
    if (mFindHits.isEmpty()) {
        runFindScan();
        if (mFindHits.isEmpty()) {
            return;
        }
        gotoFindHit(0);
        return;
    }
    gotoFindHit(mFindHitIndex + 1);
}

void DADataOperateOfDataFrameWidget::onFindReplaceRequested()
{
    applyFindReplace(false);
}

void DADataOperateOfDataFrameWidget::onFindReplaceAllRequested()
{
    applyFindReplace(true);
}

void DADataOperateOfDataFrameWidget::onFindDialogClosed()
{
    // 清查找高亮（按来源清 Find，不影响 Validation 层）与命中状态；
    // 序号自增使任何在途扫描结果作废（关闭即弃）
    ++mFindRequestSerial;
    mFindHits.clear();
    mFindHitIndex  = -1;
    mFindTotalHits = 0;
    if (mHighlightOverlay) {
        mHighlightOverlay->clear(DATableHighlightOverlay::Source::Find);
    }
}

/**
 * @brief 执行一次查找扫描（硬约束①②③的页面侧落点）
 *
 * ①去抖在对话框侧（QTimer 350ms）；②请求序号：进入即自增，结果返回时序号非最新即丢弃
 * （不排队、过期即弃）；③上限截断在算子侧（max_hits 默认 10 万），totalHits 带真实总数。
 * 扫描同步执行 + DAWaitCursorScoped 忙等待光标（RAII 保证异常/提前返回也恢复）。
 * 命中灌入 overlay(Source::Find, Level::Hit)，当前命中单独 Level::CurrentHit。
 */
void DADataOperateOfDataFrameWidget::runFindScan()
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.findScan", [this]() {
        return QStringLiteral("text_len=%1").arg(mLastFindOptions.keyword.size());
    });
    if (!mFindReplaceDialog) {
        return;
    }
    const int serial                             = ++mFindRequestSerial;  // 硬约束②：本次请求序号
    mLastFindOptions                             = mFindReplaceDialog->options();
    const DADialogFindReplace::FindOptions& opt  = mLastFindOptions;
    // 清旧命中与高亮
    mFindHits.clear();
    mFindHitIndex  = -1;
    mFindTotalHits = 0;
    mHighlightOverlay->clear(DATableHighlightOverlay::Source::Find);
    if (opt.keyword.isEmpty()) {
        updateFindCountLabel();
        return;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        updateFindCountLabel();
        return;
    }
    DAWaitCursorScoped waitCursor;  // 忙等待光标（§4.2 进度反馈），RAII 恢复
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    const DAPyScriptsDataFrame::SearchDataResult res =
        py.searchData(df,
                      opt.keyword,
                      opt.caseSensitive,
                      opt.useRegex,
                      opt.wholeCell,
                      findScanRowRange(opt.selectionOnly),
                      findScanColRange(opt.selectionOnly));
    if (serial != mFindRequestSerial) {
        return;  // 过期结果丢弃（期间又发起了新请求/关闭）
    }
    if (!res.ok) {
        mFindReplaceDialog->setSearchFailed(true);  // 无效正则等
        updateFindCountLabel();
        return;
    }
    mFindTotalHits = res.totalHits;
    const int n    = qMin(res.rows.size(), res.cols.size());
    mFindHits.reserve(n);
    for (int i = 0; i < n; ++i) {
        mFindHits.append(qMakePair(res.rows.at(i), res.cols.at(i)));
    }
    mHighlightOverlay->set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, mFindHits);
    if (!mFindHits.isEmpty()) {
        gotoFindHit(0);  // 首条设为当前命中并定位（内部会更新计数标签）
    } else {
        updateFindCountLabel();
    }
}

/**
 * @brief 导航到第 idx 条命中（循环），更新当前命中高亮并滚动定位
 * @param idx 目标序号（越界自动回绕）
 */
void DADataOperateOfDataFrameWidget::gotoFindHit(int idx)
{
    const int n = mFindHits.size();
    if (n == 0) {
        mFindHitIndex = -1;
        mHighlightOverlay->clear(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit);
        updateFindCountLabel();
        return;
    }
    mFindHitIndex              = ((idx % n) + n) % n;  // 循环导航
    const QPair< int, int > hit = mFindHits.at(mFindHitIndex);
    // 当前命中单独一条 Level::CurrentHit（覆盖该格的 Hit 级别）
    mHighlightOverlay->set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit, { hit });
    if (mLastFindOptions.selectionOnly) {
        // 选区范围模式：只滚动定位、不选中命中格（选区是扫描/替换的范围基准，
        // 不能被导航收缩，002 回归点）；当前命中靠 CurrentHit 高亮辨识
        locateActualCell(hit.first, hit.second, /*select=*/false);
    } else {
        // 滚动定位 + 选中（命中来自全表扫描，可能落在隐藏列——单入口负责取消隐藏）
        locateActualCell(hit.first, hit.second);
    }
    updateFindCountLabel();
}

/**
 * @brief 扫描行范围：全表 → nullopt；选区 → 选中行包围盒（数据源绝对行号）
 * @param selectionOnly 范围选项是否为「当前选区」
 * @return 行闭区间或 nullopt
 */
std::optional< QPair< int, int > > DADataOperateOfDataFrameWidget::findScanRowRange(bool selectionOnly) const
{
    if (!selectionOnly) {
        return std::nullopt;
    }
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (!sm || !sm->hasSelection()) {
        return std::nullopt;  // 无选区按全表处理
    }
    int minR = INT_MAX;
    int maxR = INT_MIN;
    // 按选区区间取包围盒（全选卡顿修复：不再 selectedIndexes 物化整个选区；
    // toActualRow 随视图行单调，区间顶/底的映射即最小/最大）
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        minR = qMin(minR, toActualRow(r.top()));
        maxR = qMax(maxR, toActualRow(r.bottom()));
    }
    if (maxR < minR) {
        return std::nullopt;
    }
    return qMakePair(minR, maxR);
}

/**
 * @brief 扫描列范围：全表 → nullopt；选区 → 选中列包围盒
 * @param selectionOnly 范围选项是否为「当前选区」
 * @return 列闭区间或 nullopt
 */
std::optional< QPair< int, int > > DADataOperateOfDataFrameWidget::findScanColRange(bool selectionOnly) const
{
    if (!selectionOnly) {
        return std::nullopt;
    }
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (!sm || !sm->hasSelection()) {
        return std::nullopt;
    }
    int minC = INT_MAX;
    int maxC = INT_MIN;
    // 按选区区间取包围盒（全选卡顿修复：不再 selectedIndexes 物化整个选区）
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        minC = qMin(minC, r.left());
        maxC = qMax(maxC, r.right());
    }
    if (maxC < minC) {
        return std::nullopt;
    }
    return qMakePair(minC, maxC);
}

/**
 * @brief 更新计数标签（含上限截断提示，硬约束③）
 */
void DADataOperateOfDataFrameWidget::updateFindCountLabel()
{
    if (!mFindReplaceDialog) {
        return;
    }
    mFindReplaceDialog->setHitCount(mFindHitIndex + 1, mFindHits.size(), mFindTotalHits, mFindTotalHits > mFindHits.size());
}

/**
 * @brief 应用替换（all=false 替换当前命中这一格 / true 替换扫描范围全部）
 *
 * 流程：先用同一匹配语义预扫拿变更格数（真实命中总数=将变更格数）→ 超阈值
 * （c_replaceUndoCellLimit）弹「不可撤销」确认，确认后执行但不入栈（§4.11 降级路径，
 * 不违反 D4）→ 否则 findReplace 就地替换 + 构造 DACommandDataFrame_replace push
 * （一次全部替换 = 一个撤销单元）→ 刷新模型 → 重新扫描刷新命中。
 * @param all 是否全部替换
 */
void DADataOperateOfDataFrameWidget::applyFindReplace(bool all)
{
    if (!mFindReplaceDialog) {
        return;
    }
    const DADialogFindReplace::FindOptions opt = mFindReplaceDialog->options();
    if (opt.keyword.isEmpty()) {
        return;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    if (!isTableEditable()) {
        daWarning << tr("This data source is read-only and cannot be modified.");  // cn:该数据源为只读，无法修改
        return;
    }
    // 解析替换范围：全部替换 → 扫描范围；替换当前 → 当前命中单格
    std::optional< QPair< int, int > > rowRange;
    std::optional< QPair< int, int > > colRange;
    if (all) {
        rowRange = findScanRowRange(opt.selectionOnly);
        colRange = findScanColRange(opt.selectionOnly);
    } else {
        if (mFindHitIndex < 0 || mFindHitIndex >= mFindHits.size()) {
            // 无当前命中：先扫一次（如刚打开直接点替换）
            runFindScan();
            if (mFindHits.isEmpty()) {
                daInfo << tr("No matches found");  // cn:未找到匹配项
                return;
            }
        }
        const QPair< int, int > hit = mFindHits.at(mFindHitIndex);
        rowRange                    = qMakePair(hit.first, hit.first);
        colRange                    = qMakePair(hit.second, hit.second);
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    // 预扫变更格数（与替换同一匹配语义，命中总数=将变更格数）
    int pendingCount = 0;
    {
        DAWaitCursorScoped waitCursor;
        const DAPyScriptsDataFrame::SearchDataResult probe =
            py.searchData(df, opt.keyword, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange);
        if (!probe.ok) {
            mFindReplaceDialog->setSearchFailed(true);  // 无效正则等
            return;
        }
        pendingCount = probe.totalHits;
    }
    if (pendingCount <= 0) {
        daInfo << tr("No matches found");  // cn:未找到匹配项
        return;
    }
    bool undoable = true;
    if (pendingCount > c_replaceUndoCellLimit) {
        const auto btn = QMessageBox::question(this,
                                               tr("Replace All"),  // cn:全部替换
                                               tr("Too many cells will be changed (%1). This operation cannot be undone. Continue?")
                                                   .arg(pendingCount),  // cn:改动量过大（%1 格），本次替换将不可撤销，是否继续？
                                               QMessageBox::Yes | QMessageBox::No,
                                               QMessageBox::No);
        if (btn != QMessageBox::Yes) {
            return;
        }
        undoable = false;
    }
    QVector< int > rows;
    QVector< int > cols;
    pybind11::list oldValues;
    pybind11::list newValues;
    bool okReplace = false;
    {
        DAWaitCursorScoped waitCursor;
        if (undoable) {
            okReplace = py.findReplace(df, opt.keyword, opt.replaceText, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange, &rows, &cols, &oldValues, &newValues);
        } else {
            // 降级路径：只执行替换，不记录清单（不可撤销、不入栈）
            okReplace = py.findReplace(df, opt.keyword, opt.replaceText, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange);
        }
    }
    if (!okReplace) {
        mFindReplaceDialog->setSearchFailed(true);
        return;
    }
    if (!undoable) {
        // 统计失效挂点（003）：降级路径未记录清单，保守失效全部列统计
        if (mConditionalFormatStore) {
            mConditionalFormatStore->invalidateAllStats();
        }
        notifyReplaceChanged(QVector< int >(), QVector< int >());  // 未记录清单 → 全表刷新
        Q_EMIT replaceApplied(pendingCount);
        return;
    }
    if (rows.isEmpty()) {
        daInfo << tr("No matches found");  // cn:未找到匹配项
        return;
    }
    // 正常路径：数据已被 findReplace 就地替换，构造命令持有变更清单后 push
    //（DACommandWithRedoCount 首次 redo 跳过，不重复替换；undo/redo 经 da_write_cells 批量写回）
    std::unique_ptr< DACommandDataFrame_replace > cmd(new DACommandDataFrame_replace(df, rows, cols, oldValues, newValues));
    QPointer< DADataTableModel > modle    = mModel;
    DACommandDataFrame_replace* cmdRaw    = cmd.get();
    // 统计失效挂点（003）：替换改变列值集合，失效变更清单覆盖列的统计（exec/undo 都触发）
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    cmd->setCallBack([ modle, cmdRaw, cfStore ]() {
        const QVector< int >& rs = cmdRaw->changedRows();
        const QVector< int >& cs = cmdRaw->changedCols();
        if (cfStore) {
            QList< int > colsList;
            colsList.reserve(cs.size());
            for (int c : cs) {
                colsList.append(c);
            }
            const QList< int > uniqueCols = toUniqueSorted(colsList);
            for (int c : uniqueCols) {
                cfStore->invalidateColumnStats(c);
            }
        }
        if (!modle) {
            return;
        }
        if (rs.isEmpty()) {
            return;
        }
        int r0 = rs.at(0), r1 = rs.at(0), c0 = cs.at(0), c1 = cs.at(0);
        for (int r : rs) {
            r0 = qMin(r0, r);
            r1 = qMax(r1, r);
        }
        for (int c : cs) {
            c0 = qMin(c0, c);
            c1 = qMax(c1, c);
        }
        modle->notifyDataChanged(r0, c0, r1, c1);
    });
    push(cmd.release());
    notifyReplaceChanged(rows, cols);
    Q_EMIT replaceApplied(rows.size());
}

/**
 * @brief 替换生效后刷新模型（按变更范围一次性通知，避免逐格）
 * @param rows 变更行（空 → 全表刷新，降级路径未记录清单时用）
 * @param cols 变更列
 */
void DADataOperateOfDataFrameWidget::notifyReplaceChanged(const QVector< int >& rows, const QVector< int >& cols)
{
    if (!mModel) {
        return;
    }
    if (rows.isEmpty()) {
        mModel->notifyDataChanged(0, 0, qMax(0, mModel->actualRowCount() - 1), qMax(0, mModel->columnCount() - 1));
    } else {
        int r0 = rows.at(0), r1 = rows.at(0), c0 = cols.at(0), c1 = cols.at(0);
        for (int r : rows) {
            r0 = qMin(r0, r);
            r1 = qMax(r1, r);
        }
        for (int c : cols) {
            c0 = qMin(c0, c);
            c1 = qMax(c1, c);
        }
        mModel->notifyDataChanged(r0, c0, r1, c1);
    }
    // 计算列挂点（004 T1.23）：替换改变列值 → 依赖变更列的计算列标 Stale
    //（rows 为空 = 无变更，不标）
    if (!rows.isEmpty()) {
        markComputedColumnsStaleForColumns(QList< int >(cols.begin(), cols.end()));
    }
    runFindScan();  // 替换后重新扫描刷新命中（§WP2 步骤 6）
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
 * @brief 列位置索引 → 列名（越界返回空串）
 */
QString DADataOperateOfDataFrameWidget::columnNameAt(int actualCol) const
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || actualCol < 0) {
        return QString();
    }
    const QList< QString > cols = df.columns();
    return (actualCol < cols.size()) ? cols.at(actualCol) : QString();
}

/**
 * @brief 数据变更后把依赖这些列（或目标列自身被改）的计算列标待重算（R8 手动重算）
 *
 * 只标状态不求值；排序路径不调用本函数（R13 行顺序变化不影响列值）。
 * @param actualCols 变更列位置（空列表 = 无操作）
 */
void DADataOperateOfDataFrameWidget::markComputedColumnsStaleForColumns(const QList< int >& actualCols)
{
    if (!mComputedColumnStore || mComputedColumnStore->isEmpty() || actualCols.isEmpty()) {
        return;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone()) {
        return;
    }
    const QList< QString > cols = df.columns();
    QStringList names;
    names.reserve(actualCols.size());
    for (int c : actualCols) {
        if (c >= 0 && c < cols.size()) {
            names.append(cols.at(c));
        }
    }
    mComputedColumnStore->markStaleForChangedColumns(names);
}

/**
 * @brief 整表操作保守把全部计算列标待重算（拿不到变更列信息时用，R8）
 */
void DADataOperateOfDataFrameWidget::markAllComputedColumnsStale()
{
    if (!mComputedColumnStore || mComputedColumnStore->isEmpty()) {
        return;
    }
    mComputedColumnStore->markAllStale();
}

/**
 * @brief store 变更后刷新表头（底色经模型 headerDataChanged，ƒx 角标经视口 update）
 */
void DADataOperateOfDataFrameWidget::refreshComputedColumnHeaders()
{
    if (mModel) {
        mModel->notifyHeaderDataChanged(Qt::Horizontal, 0, qMax(0, mModel->columnCount() - 1));
    }
    // ƒx 角标经视口 update 重绘——覆盖全部窗格（006e 冻结窗格同刷）
    forEachPaneHeader([](DADataHeaderView* hh) { hh->viewport()->update(); });
}

/**
 * @brief 求值定义并入栈重算命令（创建/编辑/重算共用管线，D4：重算必须入栈）
 *
 * 流程：构建 DACommandDataFrame_evalColumn → 接线回调 → cmd->exec()（首次求值 +
 * 捕获旧列快照；失败 → 标失效 + 提示，不写数据 R10）→ push（首次 redo 跳过）。
 * 定义/状态联动经 directional 回调（与数据同一撤销单元）：
 * - 创建：undo 移除定义，redo 恢复定义（Valid）；
 * - 重算/编辑：undo 恢复原定义与原状态，redo 写入新定义并置 Valid。
 * @param def 定义（targetColumnName/expr/dependsOn 已就绪）
 * @param isNewColumn true=创建路径（目标列求值前不存在）
 * @return 成功入栈返回 true
 */
bool DADataOperateOfDataFrameWidget::applyComputedColumnEval(const DAComputedColumn& def, bool isNewColumn)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !mComputedColumnStore) {
        return false;
    }
    auto cmd = std::make_unique< DACommandDataFrame_evalColumn >(df, def.targetColumnName, def.expr);
    QPointer< DADataTableModel > modle     = mModel;
    DATableComputedColumnStore* ccStore    = mComputedColumnStore;
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    const QString target = def.targetColumnName;
    // 编辑/重算场景：捕获原条目（定义+状态），undo 整体还原
    DATableComputedColumnStore::Entry prevEntry;
    const bool hasPrev = ccStore->entryOf(target, prevEntry);
    cmd->setCallBack([ modle, cfStore, self, target, isNewColumn ]() {
        if (modle) {
            if (isNewColumn) {
                // 创建/撤销创建都改变列结构 → 结构刷新（cacheShape + refreshWindow）
                modle->notifyColumnsInserted({ 0 });
            } else {
                // 重算/撤销重算只改列值
                const DAPyDataFrame mdf = modle->getData().toDataFrame();
                if (!mdf.isNone()) {
                    const QList< QString > cols = mdf.columns();
                    const int pos               = cols.indexOf(target);
                    if (pos >= 0) {
                        modle->notifyColumnChanged(pos);
                    }
                    if (cfStore) {
                        cfStore->invalidateColumnStats(pos);
                    }
                }
            }
        }
        if (self) {
            Q_EMIT self->dataModified();
        }
    });
    cmd->setDirectionalCallBack([ ccStore, def, isNewColumn, hasPrev, prevEntry ](bool isUndo) {
        if (!ccStore) {
            return;
        }
        if (isNewColumn) {
            if (isUndo) {
                ccStore->removeDefinition(def.targetColumnName);
            } else {
                ccStore->addDefinition(def, DATableComputedColumnStore::State::Valid);
            }
        } else {
            if (isUndo) {
                if (hasPrev) {
                    ccStore->addDefinition(prevEntry.def, prevEntry.state);
                }
            } else {
                ccStore->addDefinition(def, DATableComputedColumnStore::State::Valid);
            }
        }
    });
    {
        DAWaitCursorScoped waitCursor;  // 求值可能长（大表整列计算），忙等待光标
        if (!cmd->exec()) {
            // 求值失败：标失效 + 提示，不静默清空列数据（R9/R10）
            if (!isNewColumn) {
                mComputedColumnStore->setState(def.targetColumnName, DATableComputedColumnStore::State::Invalid);
            }
            daWarning << tr("Failed to evaluate the formula of computed column \"%1\": %2")
                             .arg(def.targetColumnName, cmd->lastErrorMessage());  // cn:计算列“%1”公式求值失败：%2
            return false;
        }
    }
    push(cmd.release());
    return true;
}

/**
 * @brief 打开计算列对话框（col<0 新建；col>=0 编辑该列既有公式）
 *
 * 只读/惰性源不可写数据，直接拦截。接受后求值入栈（创建 = 一个撤销单元，
 * Ctrl+Z 删列 + 移除定义）。
 * @param col 列位置索引（编辑模式）
 */
void DADataOperateOfDataFrameWidget::showComputedColumnDialog(int col)
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !mComputedColumnStore) {
        // 开发诊断：非 dataframe（只读惰性表）或无元数据注册表
        qWarning() << "DADataOperateOfDataFrameWidget::showComputedColumnDialog: dataframe or metadata registry unavailable";
        return;
    }
    if (!isTableEditable()) {
        daWarning << tr("The table is read-only and does not support computed columns");  // cn:只读表格不支持计算列
        return;
    }
    // 编辑模式：解析目标列既有定义（非计算列 → 走新建流程，不阻塞用户）
    DAComputedColumn editDef;
    if (col >= 0) {
        const QString name = columnNameAt(col);
        DATableComputedColumnStore::Entry entry;
        if (!name.isEmpty() && mComputedColumnStore->entryOf(name, entry)) {
            editDef = entry.def;
        }
    }
    DADialogComputedColumn dlg(df, this);
    if (editDef.isValid()) {
        dlg.setEditMode(editDef);
    }
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    DAComputedColumn def(dlg.targetColumnName(), dlg.expressionBody());
    def.dependsOn = dlg.dependencies();
    if (def.isSelfReferencing()) {
        // 对话框已拦截，双保险（存储层校验，计划 §3.3-4）
        daWarning << tr("The formula must not reference the target column itself");  // cn:公式不得引用目标列自身
        return;
    }
    const bool isNew = !mComputedColumnStore->hasDefinition(def.targetColumnName);
    applyComputedColumnEval(def, isNew);
}

/**
 * @brief 重算单个计算列（列头右键「重新计算」，R8 手动重算，入栈 D4）
 * @param col 列位置索引
 * @return 成功入栈返回 true
 */
bool DADataOperateOfDataFrameWidget::recomputeComputedColumn(int col)
{
    if (!mComputedColumnStore) {
        return false;
    }
    const QString name = columnNameAt(col);
    if (name.isEmpty()) {
        return false;
    }
    const DAComputedColumn def = mComputedColumnStore->definition(name);
    if (!def.isValid()) {
        daWarning << tr("Column \"%1\" has no computed formula").arg(name);  // cn:列“%1”没有计算公式
        return false;
    }
    return applyComputedColumnEval(def, false);
}

/**
 * @brief 重算全部待重算/失效的计算列（Ribbon「全部重算」）
 *
 * 每列一个独立命令、合并进一个宏命令 = 一个撤销单元（Ctrl+Z 一次回退全部重算）。
 * 求值顺序按定义插入顺序（创建顺序天然满足「先算被依赖列」；编辑造成的逆序依赖
 * v1 不做拓扑排序，用户可再点一次全部重算收敛）。
 * @return 成功重算的列数
 */
int DADataOperateOfDataFrameWidget::recomputeAllStaleComputedColumns()
{
    if (!mComputedColumnStore || mComputedColumnStore->isEmpty()) {
        return 0;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !isTableEditable()) {
        return 0;
    }
    const QList< DATableComputedColumnStore::Entry > entries = mComputedColumnStore->entries();
    QUndoCommand* macro = new QUndoCommand(tr("Recompute All Computed Columns"));  // cn:重算全部计算列
    int done            = 0;
    DAWaitCursorScoped waitCursor;  // 多列求值可能长，忙等待光标
    for (const DATableComputedColumnStore::Entry& e : entries) {
        if (!e.def.enabled || e.state == DATableComputedColumnStore::State::Valid) {
            continue;  // 只重算 Stale / Invalid
        }
        auto* cmd = new DACommandDataFrame_evalColumn(df, e.def.targetColumnName, e.def.expr, macro);
        QPointer< DADataTableModel > modle     = mModel;
        DATableComputedColumnStore* ccStore    = mComputedColumnStore;
        DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
        QPointer< DADataOperateOfDataFrameWidget > self(this);
        const QString target                   = e.def.targetColumnName;
        const DATableComputedColumnStore::Entry prevEntry = e;
        cmd->setCallBack([ modle, cfStore, self, target ]() {
            if (modle) {
                const DAPyDataFrame mdf = modle->getData().toDataFrame();
                if (!mdf.isNone()) {
                    const QList< QString > cols = mdf.columns();
                    const int pos               = cols.indexOf(target);
                    if (pos >= 0) {
                        modle->notifyColumnChanged(pos);
                    }
                    if (cfStore) {
                        cfStore->invalidateColumnStats(pos);
                    }
                }
            }
            if (self) {
                Q_EMIT self->dataModified();
            }
        });
        cmd->setDirectionalCallBack([ ccStore, prevEntry ](bool isUndo) {
            if (ccStore) {
                ccStore->addDefinition(prevEntry.def, isUndo ? prevEntry.state : DATableComputedColumnStore::State::Valid);
            }
        });
        if (!cmd->exec()) {
            // 求值失败：标失效 + 提示，命令从宏中移除（析构即脱离父命令）
            mComputedColumnStore->setState(e.def.targetColumnName, DATableComputedColumnStore::State::Invalid);
            daWarning << tr("Failed to evaluate the formula of computed column \"%1\": %2")
                             .arg(e.def.targetColumnName, cmd->lastErrorMessage());  // cn:计算列“%1”公式求值失败：%2
            delete cmd;
            continue;
        }
        ++done;
    }
    if (done == 0) {
        delete macro;
        return 0;
    }
    // 数据已在上面逐列 exec 完成；子命令均为 DACommandWithRedoCount（首次 redo 跳过），
    // push 宏命令不会重复求值
    push(macro);
    return done;
}

/**
 * @brief 移除某列的公式定义（保留列数据，列头右键「移除公式」；可撤销）
 * @param col 列位置索引
 * @return 成功入栈返回 true
 */
bool DADataOperateOfDataFrameWidget::removeComputedColumnFormula(int col)
{
    if (!mComputedColumnStore) {
        return false;
    }
    const QString name = columnNameAt(col);
    if (name.isEmpty() || !mComputedColumnStore->hasDefinition(name)) {
        return false;
    }
    auto cmd = std::make_unique< DACommandComputedColumn_removeDefinition >(mComputedColumnStore, name);
    if (!cmd->isCaptured()) {
        return false;
    }
    push(cmd.release());  // 普通 QUndoCommand：push 即执行 redo（移除定义）
    return true;
}

/**
 * @brief 列改名前的计算列影响面确认（004 T1.23 R20）
 *
 * 统计引用了将被改名列（作依赖或自身是计算列目标）的定义数；>0 时提示
 * 「有 N 个计算列引用它，将随改名批量更新」，用户确认后继续。
 * @param renames 改名对（old→new）
 * @return 继续返回 true（无引用恒 true）
 */
bool DADataOperateOfDataFrameWidget::confirmComputedColumnRename(const QList< QPair< QString, QString > >& renames) const
{
    if (!mComputedColumnStore || mComputedColumnStore->isEmpty() || renames.isEmpty()) {
        return true;
    }
    QStringList affected;
    for (const auto& p : renames) {
        const QStringList refs = mComputedColumnStore->definitionsReferencing(p.first);
        for (const QString& t : refs) {
            if (!affected.contains(t)) {
                affected.append(t);
            }
        }
        if (mComputedColumnStore->hasDefinition(p.first) && !affected.contains(p.first)) {
            affected.append(p.first);
        }
    }
    if (affected.isEmpty()) {
        return true;
    }
    const auto answer = QMessageBox::question(
        const_cast< DADataOperateOfDataFrameWidget* >(this),
        tr("Rename Column"),  // cn:重命名列
        tr("%1 computed column(s) reference the renamed column(s): %2. Their references will be updated automatically. Continue?")
            .arg(affected.size())
            .arg(affected.join(QStringLiteral(", "))),  // cn:有 %1 个计算列引用被改名的列：%2。其引用将自动批量更新，是否继续？
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    return answer == QMessageBox::Yes;
}

/**
 * @brief 把计算列定义迁移挂进改名命令的 directional 回调（与改名同一撤销单元）
 * @param cmd 改名命令（exec 前接线）
 * @param renames 改名对（old→new）
 */
void DADataOperateOfDataFrameWidget::wireComputedColumnRenameMigration(
    DACommandDataFrame_renameColumns* cmd, const QList< QPair< QString, QString > >& renames) const
{
    if (!cmd || !mComputedColumnStore || renames.isEmpty()) {
        return;
    }
    DATableComputedColumnStore* ccStore = mComputedColumnStore;
    cmd->setDirectionalCallBack([ ccStore, renames ](bool isUndo) {
        for (const auto& p : renames) {
            if (isUndo) {
                ccStore->renameColumnReferences(p.second, p.first);
            } else {
                ccStore->renameColumnReferences(p.first, p.second);
            }
        }
    });
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
    mSortSpec.clear();
    forEachPaneHeader([ this ](DADataHeaderView* hh) { hh->setSortSpec(mSortSpec); });
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
    QHash< int, QVector< QPair< int, int > > > res;
    const QList< QPoint > cells = getSelectedDataframeCells();
    if (cells.isEmpty()) {
        return res;
    }
    // col -> 升序去重行
    QHash< int, QList< int > > colRows;
    for (const QPoint& p : cells) {
        colRows[ p.y() ].append(p.x());
    }
    for (auto it = colRows.constBegin(); it != colRows.constEnd(); ++it) {
        QList< int > rows = it.value();
        std::sort(rows.begin(), rows.end());
        rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
        QVector< QPair< int, int > > runs;
        int runStart = rows.first();
        int prev     = rows.first();
        for (int i = 1; i < rows.size(); ++i) {
            if (rows[ i ] == prev + 1) {
                prev = rows[ i ];
                continue;
            }
            runs.append(qMakePair(runStart, prev));
            runStart = rows[ i ];
            prev     = rows[ i ];
        }
        runs.append(qMakePair(runStart, prev));
        res.insert(it.key(), runs);
    }
    return res;
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
    QPointer< DADataTableModel > modle = mModel;
    DATableConditionalFormatStore* cfStore = mConditionalFormatStore;
    QPointer< DADataOperateOfDataFrameWidget > self(this);
    cmd->setCallBack([ modle, rows, cols, cfStore, self ]() {
        if (modle) {
            const auto size = qMin(rows.size(), cols.size());
            for (int i = 0; i < size; ++i) {
                modle->notifyDataChanged(rows[ i ], cols[ i ]);
            }
        }
        if (cfStore) {
            const QList< int > uniqueCols = toUniqueSorted(cols);
            for (int c : uniqueCols) {
                cfStore->invalidateColumnStats(c);
            }
        }
        if (self) {
            self->markComputedColumnsStaleForColumns(cols);
        }
    });
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
 * @brief 规则显示文本（报告与列表共用）：列（第 n 个）· 类型 参数 — 名称
 * @param rule 规则
 * @return 已翻译的显示文本
 */
QString DADataOperateOfDataFrameWidget::validationRuleDisplayText(const DATableValidationRule& rule) const
{
    QString typeName;
    switch (rule.type) {
    case DATableValidationRule::Type::NumericRange:
        typeName = tr("Numeric range");  // cn:数值区间
        break;
    case DATableValidationRule::Type::DateRange:
        typeName = tr("Date range");  // cn:日期区间
        break;
    case DATableValidationRule::Type::EnumSet:
        typeName = tr("Allowed values (enum)");  // cn:枚举集合
        break;
    case DATableValidationRule::Type::NotEmpty:
        typeName = tr("Not empty");  // cn:非空
        break;
    case DATableValidationRule::Type::Unique:
        typeName = tr("Unique");  // cn:唯一
        break;
    case DATableValidationRule::Type::Regex:
        typeName = tr("Regex (full match)");  // cn:正则（整格匹配）
        break;
    }
    QString colText = rule.columnName;
    if (rule.columnOccurrence > 0) {
        colText = tr("%1 (occurrence %2)").arg(rule.columnName).arg(rule.columnOccurrence + 1);  // cn:%1（第 %2 个同名列）
    }
    QString text = QStringLiteral("%1 · %2").arg(colText, typeName);
    const QString params = rule.paramSummary();
    if (!params.isEmpty()) {
        text += QLatin1Char(' ') + params;
    }
    if (!rule.label.isEmpty()) {
        text += QStringLiteral(" — ") + rule.label;
    }
    return text;
}

/**
 * @brief 打开校验规则编辑对话框（005 T1.25）
 *
 * 一窗两栏编辑工作规则集；确定后与 store 现值对比，有差异才生成一条
 * DACommandValidationRule_setRules 入页撤销栈（旧新对象记录，D4；无差异不推空命令）。
 * 规则变化后既有违规高亮可能与新规则不一致 → 清 Validation 来源高亮（重跑由用户触发）。
 */
void DADataOperateOfDataFrameWidget::showValidationRulesDialog()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !mValidationStore) {
        return;
    }
    DADialogValidationRules dlg(df.columns(), mValidationStore->rules(), this);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    const QList< DATableValidationRule > oldRules = mValidationStore->rules();
    const QList< DATableValidationRule > newRules = dlg.resultRules();
    if (oldRules == newRules) {
        return;
    }
    push(new DACommandValidationRule_setRules(mValidationStore, oldRules, newRules));
    clearValidationHighlights();
}

/**
 * @brief 执行校验求值（只读，红线2：不改数据、不入撤销栈）
 *
 * 全部启用规则一次下发 da_validate_dataframe（空值判定/比较全在 pandas，红线1）；
 * 唯一性规则命中 store 缓存则不重算（§4.9，打点 uniquePythonEvals 可测）。列名 +
 * 第 n 个同名列解析列位置（R20），列缺失的规则标记 columnMissing 跳过。结果装载
 * 瞬态高亮层 Source::Validation（Error→ViolationError / Warning→ViolationWarning，
 * 来源隔离：不动查找命中）。
 * @param elapsedMs 非空时输出求值耗时（毫秒）
 * @return 逐规则结果（含列缺失标记与违规行清单）
 */
QList< DADataOperateOfDataFrameWidget::ValidationRuleOutcome > DADataOperateOfDataFrameWidget::evaluateValidationRules(int* elapsedMs)
{
    QList< ValidationRuleOutcome > outcomes;
    if (elapsedMs) {
        *elapsedMs = 0;
    }
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !mValidationStore) {
        return outcomes;
    }
    const QList< int > enabledIdx = mValidationStore->enabledRuleIndexes();
    if (enabledIdx.isEmpty()) {
        return outcomes;
    }
    const QList< DATableValidationRule > all = mValidationStore->rules();
    const QList< QString > colNames          = df.columns();
    // 列名 + 第 n 个同名列 → 列位置（R20 重名消歧，与算子同一解析语义）
    auto resolvePos = [&colNames](const QString& name, int occurrence) -> int {
        int seen = 0;
        for (int i = 0; i < colNames.size(); ++i) {
            if (colNames[ i ] == name) {
                if (seen == occurrence) {
                    return i;
                }
                ++seen;
            }
        }
        return -1;
    };
    QList< DAPyScriptsDataFrame::ValidationRuleSpec > specs;
    QElapsedTimer timer;
    timer.start();
    for (int ruleIdx : std::as_const(enabledIdx)) {
        const DATableValidationRule r = all[ ruleIdx ];
        ValidationRuleOutcome oc;
        oc.rule           = r;
        oc.columnPosition = resolvePos(r.columnName, r.columnOccurrence);
        oc.columnMissing  = oc.columnPosition < 0;
        const int outcomePos = outcomes.size();
        outcomes.append(oc);
        if (oc.columnMissing) {
            continue;
        }
        // 唯一性规则命中缓存 → 不重算（§4.9：O(n) 的 pandas 操作按列缓存重复集合）
        if (r.type == DATableValidationRule::Type::Unique && mValidationStore->uniqueCacheValid(r)) {
            outcomes[ outcomePos ].rows = mValidationStore->uniqueCacheRows(r);
            continue;
        }
        if (r.type == DATableValidationRule::Type::Unique) {
            mValidationStore->countUniquePythonEval();
        }
        DAPyScriptsDataFrame::ValidationRuleSpec sp;
        sp.index      = outcomePos;
        sp.column     = r.columnName;
        sp.occurrence = r.columnOccurrence;
        switch (r.type) {
        case DATableValidationRule::Type::NumericRange:
            sp.type       = QStringLiteral("numeric-range");
            sp.numericMin = r.numericMin;
            sp.numericMax = r.numericMax;
            sp.hasMin     = r.hasNumericMin;
            sp.hasMax     = r.hasNumericMax;
            break;
        case DATableValidationRule::Type::DateRange:
            sp.type    = QStringLiteral("date-range");
            sp.dateMin = r.dateMin.isValid() ? r.dateMin.toString(Qt::ISODateWithMs) : QString();
            sp.dateMax = r.dateMax.isValid() ? r.dateMax.toString(Qt::ISODateWithMs) : QString();
            break;
        case DATableValidationRule::Type::EnumSet:
            sp.type       = QStringLiteral("enum");
            sp.enumValues = r.enumValues;
            break;
        case DATableValidationRule::Type::NotEmpty:
            sp.type = QStringLiteral("not-empty");
            break;
        case DATableValidationRule::Type::Unique:
            sp.type = QStringLiteral("unique");
            break;
        case DATableValidationRule::Type::Regex:
            sp.type         = QStringLiteral("regex");
            sp.regexPattern = r.regexPattern;
            break;
        }
        specs.append(sp);
    }
    QList< DAPyScriptsDataFrame::ValidationRuleResult > results;
    QString err;
    if (!DAPyScripts::getDataFrame().validateDataFrame(df, specs, &results, &err)) {
        daWarning << tr("Validation failed: %1").arg(err);  // cn:数据校验失败：%1
        return QList< ValidationRuleOutcome >();
    }
    for (const DAPyScriptsDataFrame::ValidationRuleResult& res : std::as_const(results)) {
        if (res.index >= 0 && res.index < outcomes.size()) {
            outcomes[ res.index ].rows = res.rows;
            // 唯一性结果写缓存（数据变更后由失效挂点清空）
            if (outcomes[ res.index ].rule.type == DATableValidationRule::Type::Unique) {
                mValidationStore->setUniqueCache(outcomes[ res.index ].rule, res.rows);
            }
        }
    }
    if (elapsedMs) {
        *elapsedMs = static_cast< int >(timer.elapsed());
    }
    // 违规装载瞬态高亮层（Source::Validation；来源隔离：不碰 Find 的集合，红线2）
    QVector< QPair< int, int > > errCells, warnCells;
    for (const ValidationRuleOutcome& oc : std::as_const(outcomes)) {
        if (oc.columnMissing) {
            continue;
        }
        QVector< QPair< int, int > >& target =
            (oc.rule.level == DATableValidationRule::Level::Error) ? errCells : warnCells;
        target.reserve(target.size() + oc.rows.size());
        for (int row : oc.rows) {
            target.append(qMakePair(row, oc.columnPosition));
        }
    }
    if (mHighlightOverlay) {
        mHighlightOverlay->set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, errCells);
        mHighlightOverlay->set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, warnCells);
    }
    return outcomes;
}

/**
 * @brief 执行校验并弹出报告对话框（非模态，点击样例行定位）
 *
 * 无启用规则时提示并直接打开规则编辑对话框；报告样例行上限 50 条/规则（完整清单
 * 走高亮与导出）。定位走 007 的定位单入口 locateActualCell。
 */
void DADataOperateOfDataFrameWidget::runValidationAndShowReport()
{
    DAPyDataFrame df = getDataframe();
    if (df.isNone() || !mValidationStore) {
        return;
    }
    if (mValidationStore->enabledRuleIndexes().isEmpty()) {
        daInfo << tr("No enabled validation rules yet — add rules first");  // cn:暂无启用的校验规则——请先添加规则
        showValidationRulesDialog();
        return;
    }
    int elapsedMs = 0;
    const QList< ValidationRuleOutcome > outcomes = evaluateValidationRules(&elapsedMs);
    if (outcomes.isEmpty()) {
        return;  // 求值失败（已提示）
    }
    QList< DADialogValidationReport::Entry > entries;
    entries.reserve(outcomes.size());
    const int c_sampleLimit = 50;
    for (const ValidationRuleOutcome& oc : std::as_const(outcomes)) {
        DADialogValidationReport::Entry e;
        e.ruleText       = validationRuleDisplayText(oc.rule);
        e.isError        = oc.rule.level == DATableValidationRule::Level::Error;
        e.columnPosition = oc.columnPosition;
        e.columnMissing  = oc.columnMissing;
        e.rows           = oc.rows;
        const int sampleN = qMin(oc.rows.size(), c_sampleLimit);
        for (int i = 0; i < sampleN; ++i) {
            const int row = oc.rows[ i ];
            QString valTxt;
            if (!oc.columnMissing && oc.columnPosition >= 0) {
                valTxt = df.iat(static_cast< std::size_t >(row), static_cast< std::size_t >(oc.columnPosition)).toString();
            }
            e.rowTexts.append(tr("Row %1: %2").arg(row + 1).arg(valTxt));  // cn:行 %1：%2
        }
        entries.append(e);
    }
    // 已有报告窗口先关（避免多次校验叠加多个非模态窗）
    if (mValidationReport) {
        mValidationReport->close();
    }
    DADialogValidationReport* dlg = new DADialogValidationReport(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    mValidationReport = dlg;
    dlg->setResults(entries, mValidationStore->enabledRuleIndexes().size(), elapsedMs);
    QPointer< DADataTableView > view = ui->tableView;
    connect(dlg, &DADialogValidationReport::locateCellRequested, this, [ this, view ](int row, int col) {
        // 007：报告样例行定位走定位单入口（隐藏列自动取消隐藏 + 冻结区处置）
        if (view && col >= 0) {
            locateActualCell(row, col);
        }
    });
    connect(dlg, &DADialogValidationReport::clearHighlightsRequested, this, [ this ]() {
        clearValidationHighlights();
    });
    dlg->show();
}

/**
 * @brief 清除违规高亮（只清 Source::Validation，查找命中不受影响——来源隔离，红线2）
 */
void DADataOperateOfDataFrameWidget::clearValidationHighlights()
{
    if (mHighlightOverlay) {
        mHighlightOverlay->clear(DATableHighlightOverlay::Source::Validation);
    }
}

/**
 * @brief 列改名确认：提示「有 N 条校验规则引用它」，返回是否继续（无引用直接 true）
 * @param renames 改名对（old→new，仅变化的）
 * @return 继续返回 true
 */
bool DADataOperateOfDataFrameWidget::confirmValidationRuleRename(const QList< QPair< QString, QString > >& renames) const
{
    if (!mValidationStore || mValidationStore->isEmpty() || renames.isEmpty()) {
        return true;
    }
    int total = 0;
    QStringList cols;
    for (const auto& p : std::as_const(renames)) {
        const int n = mValidationStore->rulesReferencing(p.first);
        if (n > 0) {
            total += n;
            cols.append(p.first);
        }
    }
    if (total == 0) {
        return true;
    }
    const auto answer = QMessageBox::question(
        const_cast< DADataOperateOfDataFrameWidget* >(this),
        tr("Rename Column"),  // cn:重命名列
        tr("%1 validation rule(s) reference the renamed column(s): %2. Their references will be updated automatically. Continue?")
            .arg(total)
            .arg(cols.join(QStringLiteral(", "))),  // cn:有 %1 条校验规则引用被改名的列：%2。其引用将自动批量更新，是否继续？
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    return answer == QMessageBox::Yes;
}

/**
 * @brief 把校验规则引用迁移挂进改名命令的 directional 回调（与改名同一撤销单元，R20）
 *
 * 链式包裹既有回调（计算列迁移先挂）：setDirectionalCallBack 是覆盖语义，
 * 直接再设会丢掉计算列迁移，故取旧回调包进新回调。
 * @param cmd 改名命令（exec 前接线）
 * @param renames 改名对（old→new）
 */
void DADataOperateOfDataFrameWidget::wireValidationRuleRenameMigration(
    DACommandDataFrame_renameColumns* cmd, const QList< QPair< QString, QString > >& renames) const
{
    if (!cmd || !mValidationStore || renames.isEmpty()) {
        return;
    }
    DATableValidationStore* vaStore                          = mValidationStore;
    DACallBackInterface::DirectionalCallBack prevDirectional = cmd->getDirectionalCallBack();
    cmd->setDirectionalCallBack([ prevDirectional, vaStore, renames ](bool isUndo) {
        if (prevDirectional) {
            prevDirectional(isUndo);
        }
        for (const auto& p : std::as_const(renames)) {
            if (isUndo) {
                vaStore->renameColumnReferences(p.second, p.first);
            } else {
                vaStore->renameColumnReferences(p.first, p.second);
            }
        }
    });
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
    if (!ui || !ui->tableView) {
        return false;
    }
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (!sm) {
        return false;
    }
    const QModelIndex cur = sm->currentIndex();
    if (!cur.isValid()) {
        return false;
    }
    const int aRow = toActualRow(cur.row());
    const int aCol = cur.column();
    if (aRow < 0 || aRow >= dataRowCount() || aCol < 0 || aCol >= dataColumnCount()) {
        return false;
    }
    if (actualRow) {
        *actualRow = aRow;
    }
    if (actualCol) {
        *actualCol = aCol;
    }
    return true;
}

/**
 * @brief 打开当前单元格的批注编辑弹层（无批注=插入，有批注=编辑且弹层内可删除）
 */
void DADataOperateOfDataFrameWidget::showNoteEditorForCurrentCell()
{
    if (!mNoteStore) {
        return;
    }
    int row = -1, col = -1;
    if (!currentActualCell(&row, &col)) {
        daWarning << tr("Please select a cell to annotate");  // cn:请先选中要批注的单元格
        return;
    }
    editNoteAtCell(row, col);
}

/**
 * @brief 对给定格弹出编辑弹层并落地结果（插入/编辑共用）
 *
 * 弹层跟随单元格右下角定位（先 showActualRow 保证行在滑动窗内，visualRect 转全局
 * 坐标，出屏由弹层回夹）。保存 → 新增/编辑命令（旧新条目含时间戳记录，D4）；
 * 弹层内点删除或文本清空 → 删除命令；文本未变不推命令。批注不进 DataFrame（红线3）。
 * @param actualRow 数据源绝对行
 * @param actualCol 列位置
 */
void DADataOperateOfDataFrameWidget::editNoteAtCell(int actualRow, int actualCol)
{
    if (!mNoteStore) {
        return;
    }
    DATableNoteEditDialog dlg(this);
    const DATableNoteStore::Note oldNote = mNoteStore->note(actualRow, actualCol);
    dlg.setNoteText(oldNote.text);
    dlg.setExistingNote(!oldNote.text.isEmpty());
    // 弹层位置跟随单元格右下角
    QPoint corner;
    if (DADataTableView* tv = ui->tableView) {
        tv->showActualRow(actualRow);
        QAbstractItemModel* m = tv->model();
        int startRow          = 0;
        if (auto* cm = qobject_cast< DAAbstractCacheWindowTableModel* >(m)) {
            startRow = cm->getCacheWindowStartRow();
        }
        const QModelIndex idx = m ? m->index(actualRow - startRow, actualCol) : QModelIndex();
        if (idx.isValid()) {
            const QRect vr = tv->visualRect(idx);
            corner         = tv->viewport()->mapToGlobal(vr.bottomRight());
        }
    }
    if (corner.isNull()) {
        corner = QCursor::pos();
    }
    dlg.popupNear(corner);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    if (dlg.isDeleteRequested()) {
        if (oldNote.text.isEmpty()) {
            return;  // 本来就没有批注
        }
        push(new DACommandTableNote_set(mNoteStore, actualRow, actualCol, oldNote, DATableNoteStore::Note()));
        return;
    }
    const QString newText = dlg.noteText().trimmed();
    if (newText == oldNote.text) {
        return;  // 无变化不推命令
    }
    if (newText.isEmpty()) {
        // 文本清空 = 删除批注（批注没有"空内容"状态）
        if (oldNote.text.isEmpty()) {
            return;
        }
        push(new DACommandTableNote_set(mNoteStore, actualRow, actualCol, oldNote, DATableNoteStore::Note()));
        return;
    }
    DATableNoteStore::Note newNote;
    newNote.text      = newText;
    const QDateTime now = QDateTime::currentDateTime();
    newNote.createdAt = oldNote.text.isEmpty() ? now : oldNote.createdAt;
    newNote.updatedAt = now;
    push(new DACommandTableNote_set(mNoteStore, actualRow, actualCol, oldNote, newNote));
}

/**
 * @brief 删除当前单元格批注（命令可撤销）
 * @return 成功入栈返回 true；无当前格或无批注返回 false
 */
bool DADataOperateOfDataFrameWidget::deleteNoteAtCurrentCell()
{
    if (!mNoteStore) {
        return false;
    }
    int row = -1, col = -1;
    if (!currentActualCell(&row, &col)) {
        daWarning << tr("Please select a cell whose note is to be deleted");  // cn:请先选中要删除批注的单元格
        return false;
    }
    const DATableNoteStore::Note oldNote = mNoteStore->note(row, col);
    if (oldNote.text.isEmpty()) {
        daInfo << tr("The selected cell has no note");  // cn:选中单元格没有批注
        return false;
    }
    push(new DACommandTableNote_set(mNoteStore, row, col, oldNote, DATableNoteStore::Note()));
    return true;
}

/**
 * @brief 清除本表全部批注（一次命令，可撤销）
 * @return 清除的批注数（无批注返回 0，不推空命令）
 */
int DADataOperateOfDataFrameWidget::clearAllNotes()
{
    if (!mNoteStore) {
        return 0;
    }
    const int n = mNoteStore->count();
    if (n == 0) {
        daInfo << tr("This table has no notes");  // cn:本表没有批注
        return 0;
    }
    push(new DACommandTableNote_clearAll(mNoteStore));
    return n;
}
