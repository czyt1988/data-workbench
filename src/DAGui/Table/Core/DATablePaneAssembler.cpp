#include "Table/Core/DATablePaneAssembler.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Controllers/DATableViewStateController.h"
// qt
#include <QHeaderView>
// DAGui
#include "Table/Views/DADataHeaderView.h"
#include "Table/Views/DADataTableView.h"
#include "Table/Views/DATableStyleItemDelegate.h"
#include "Table/Metadata/DATableNoteStore.h"
#include "Table/Metadata/DATableComputedColumnStore.h"
#include "Table/Metadata/DATableViewStateStore.h"
#include "Table/Models/DAAbstractCacheWindowTableModel.h"
#include "Table/Views/DATablePaneContainer.h"

namespace DA
{
//===================================================
// DATablePaneAssembler
//===================================================

DATablePaneAssembler::DATablePaneAssembler(const DATablePageContext& ctx,
                                           DATableViewStateController* viewState,
                                           DATableStyleItemDelegate* styleDelegate)
    : mCtx(ctx), mViewState(viewState), mStyleDelegate(styleDelegate)
{
}

/**
 * @brief 装配一个窗格（006b 的窗格装配唯一路径 · 装配段）
 *
 * 1) 表头替换（DADataHeaderView：列头右键菜单/带修饰键点击信号/行头宽拖拽）；
 * 2) 模型与委托（同一 delegate 实例注入多个视图，delegate 属页面）；
 *    冻结窗格共享主窗格的 selectionModel（006e，顺序：先 setModel 再共享）；
 * 3) 批注子存储注入视图（右键菜单「插入/编辑/删除批注」的启用规则查当前格）；
 * 4) 水平表头：计算列 store 注入（ƒx 角标 + 列头菜单计算列组）+ 接线；
 * 5) 垂直表头：行头宽拖拽/统一行高接线。
 * 表头替换必须在 setModel 之前（QTableView::setModel 会把模型接到"当前"表头上）。
 * 主窗格与 006e 懒建的冻结窗格都走这里；漏一处 = 该窗格某操作没反应。
 * @param pane 目标窗格
 */
void DATablePaneAssembler::assemble(DADataTableView* pane) const
{
    if (!pane) {
        return;
    }
    // 1) 表头替换：用 DADataHeaderView。行高/Interactive 设置在 DADataHeaderView(Vertical)
    //    构造内重建（参照 DACacheWindowTableView 构造对默认行头的同款设置）
    pane->setHorizontalHeader(new DADataHeaderView(Qt::Horizontal, pane));
    pane->setVerticalHeader(new DADataHeaderView(Qt::Vertical, pane));
    // 2) 模型与委托；冻结窗格共享主窗格的 selectionModel（顺序：先 setModel 再共享）
    pane->setModel(mCtx.model());
    if (pane != mCtx.mainView()) {
        pane->setSelectionModel(mCtx.mainView()->selectionModel());
    }
    pane->setItemDelegate(mStyleDelegate);
    // 3) 批注子存储注入视图
    pane->setNoteStore(mCtx.noteStore());
    // 4) 水平表头接线
    if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(pane->horizontalHeader())) {
        wireHorizontalHeader(hh, pane);
    }
    // 5) 垂直表头接线（需要窗格做「窗口内行号 → 绝对行号」换算）
    if (DADataHeaderView* vh = qobject_cast< DADataHeaderView* >(pane->verticalHeader())) {
        wireVerticalHeader(vh, pane);
    }
}

/**
 * @brief 水平表头接线（计算列 store/冻结查询/列宽采集/列序拖动/表头高）
 *
 * @param hh 水平表头
 * @param pane 归属窗格（列宽采集用触发窗格，不再写死主窗格）
 */
void DATablePaneAssembler::wireHorizontalHeader(DADataHeaderView* hh, DADataTableView* pane) const
{
    // 计算列 store 注入（ƒx 角标 + 列头菜单计算列组）
    hh->setComputedColumnStore(mCtx.computedColumnStore());
    // 冻结状态查询：列头菜单据此显示「冻结此列」或「取消冻结此列」（视图状态控制器纯查表）
    DATableViewStateController* vs = mViewState;
    hh->setFrozenColumnChecker([ vs ](int logical) { return vs && vs->isColumnInFrozenArea(logical); });
    // 列宽采集：用触发信号的那个窗格的表头（008 期3 起采集经视图状态控制器）
    QObject::connect(hh, &QHeaderView::sectionResized, hh, [ this, pane ](int logicalIndex, int, int newSize) {
        if (mViewState) {
            mViewState->onHeaderSectionResized(pane, logicalIndex, newSize);
        }
    });
    // 列序拖动落库（006c）：必须在表头替换之后（hh 是新表头）；每装一个窗格连一份
    if (DATablePaneContainer* pc = mCtx.paneContainer()) {
        QObject::connect(hh, &QHeaderView::sectionMoved, pc, &DATablePaneContainer::onPaneSectionMoved);
    }
    // 表头高拖拽落库（底边热区）：写视图状态 + 套到各窗格（冻结窗格行对齐要求同高）
    QObject::connect(hh, &DADataHeaderView::horizontalHeaderHeightChanged, hh, [ this ](int height) {
        if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
            vsStore->setHorizontalHeaderHeight(height);
        }
        if (mViewState) {
            mViewState->applyHeaderHeightToPanes(height);
        }
    });
}

/**
 * @brief 垂直表头接线（行头宽拖拽/单行行高/行头右键/搬窗重投影）
 *
 * @param vh 垂直表头
 * @param pane 归属窗格（窗口内行号 → 绝对行号换算）
 */
void DATablePaneAssembler::wireVerticalHeader(DADataHeaderView* vh, DADataTableView* pane) const
{
    // 行头宽拖拽 → 写视图状态 + 冻结几何重算（006e）
    QObject::connect(vh, &DADataHeaderView::rowHeaderWidthChanged, vh, [ this ](int width) {
        if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
            vsStore->setRowHeaderWidth(width);
        }
        if (DATablePaneContainer* pc = mCtx.paneContainer()) {
            pc->updateFrozenGeometry();
        }
    });
    // 单行行高拖拽落库：窗口内行号 → 绝对行号，写逐行覆盖（store 信号驱动重投影）
    QObject::connect(vh, &DADataHeaderView::rowHeightChanged, vh, [ this, pane ](int logicalIndex, int height) {
        if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
            vsStore->setRowHeightOverride(pane->toActualRow(logicalIndex), height);
        }
    });
    // 行头右键「设置整表行高…」：转发到窗格的既有表意信号（页面弹输入框）
    QObject::connect(vh, &DADataHeaderView::rowHeightPromptRequested, pane, &DADataTableView::rowHeightPromptRequested);
    // 滑动窗搬窗 / 插删行列后：段号与绝对行号的映射变了，把逐行覆盖重新落到正确的段
    if (DAAbstractCacheWindowTableModel* cacheModel = pane->getCacheModel()) {
        QObject::connect(cacheModel,
                         &DAAbstractCacheWindowTableModel::cacheWindowChanged,
                         pane,
                         [ this, pane ](int, int) {
                             if (mViewState) {
                                 mViewState->applyRowHeightOverridesToPane(pane);
                             }
                         });
        QObject::connect(cacheModel,
                         &DAAbstractCacheWindowTableModel::tableStructureChanged,
                         pane,
                         [ this, pane ]() {
                             if (mViewState) {
                                 mViewState->applyRowHeightOverridesToPane(pane);
                             }
                         });
    }
}
}  // end of namespace DA
