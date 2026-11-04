#ifndef DAAPPTABLEACTIONHANDLER_H
#define DAAPPTABLEACTIONHANDLER_H
#include <QObject>
#include <QList>
#include "DAAppActionHandlerDeps.h"
#include "numpy/DAPyDType.h"  // 槽参数按值传递，moc 需要完整类型
#include "Table/Metadata/DATableDisplayFormat.h"  // onTableDisplayFormatCategoryChanged 用嵌套枚举

class QAction;
class QMenu;

namespace DA
{
class DATableCellStyle;
class DADataOperateOfDataFrameWidget;
class DADataManagerTreeWidget;
class DAPyDType;
class DADialogTableDisplayFormat;

/**
 * @brief 表格域动作分发器：Ribbon 表格 / 数据工具 / 样式面板与表头、数据管理树右键的动作
 *
 * 从 DAAppController 抽出的分发器，只负责把动作转成对当前数据页的调用；
 * 翻译上下文沿用 DA::DAAppController（.ts 译文已存在，拆类不改上下文）。
 */
class DAAppTableActionHandler : public QObject
{
    Q_OBJECT
public:
    explicit DAAppTableActionHandler(const DAAppActionHandlerDeps& deps, QObject* par = nullptr);
    // 建立动作绑定与数据管理树右键菜单
    void initConnections();

public Q_SLOTS:
    // 在指定页面重命名列（表头右键直入口）：弹输入框 → w->renameColumn；
    // 与 onActionRenameColumnTriggered 的差异：不经焦点检查、不依赖选区
    void renameColumnOnPage(DADataOperateOfDataFrameWidget* w, int col);
    // 复制列名到剪贴板（表头右键）
    void onActionCopyColumnNameTriggered();
    // 跳转到最大值（表头右键）
    void onActionGotoMaxTriggered();
    // 跳转到最小值（表头右键）
    void onActionGotoMinTriggered();
    // 显示列统计信息（表头右键）
    void onActionShowColumnDescribeTriggered();
    // 刷新「快速绘图」动作的可用状态（有选区才可点）
    void updateQuickPlotActionState();
    // 刷新「排序」动作（面板大按钮与下拉各项）的可用状态（有可编辑表格页才可点）
    void updateSortActionState();
    // 在 DataFrame 操作窗口中选中指定列（replace 语义，供树右键与列头菜单信号接线共用）
    void selectColumnInDataFrameWidget(DADataOperateOfDataFrameWidget* w, int col);
    // 为数据管理树注入右键菜单（series 节点=列操作菜单，共用列操作 action；
    // 数据集条目=重命名/移除/导出菜单，复用 ribbon 数据标签页 action）
    // 注：DataFrame 列头右键菜单已迁入 DAGui 的 DADataHeaderView（R12），
    // controller 只保留 columnActionRequested 信号接线（见 onDataOperatePageCreated）
    void setupDataManagerTreeContextMenu(DADataManagerTreeWidget* w);
    // 数据页反向上报（由 DAAppController::onDataOperatePageCreated 接线）
    void onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DA::DAPyDType dt);
    void onTableStyleCurrentChanged(const DATableCellStyle& style);
    void onTableDisplayFormatCurrentChanged(const DATableDisplayFormat& fmt);

private Q_SLOTS:
    // 列数据类型改变
    void onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt);
    // 移除选中行
    void onActionRemoveRowTriggered();
    // 移除选中列
    void onActionRemoveColumnTriggered();
    // 移除单元格内容
    void onActionRemoveCellTriggered();
    // 插入一行
    void onActionInsertRowTriggered();
    // 在选中位置上面插入一行
    void onActionInsertRowAboveTriggered();
    // 在选中位置右边插入一列
    void onActionInsertColumnRightTriggered();
    // 在选中位置左边插入一列
    void onActionInsertColumnLeftTriggered();
    // dataframe列重命名
    void onActionManageColumnsTriggered();
    // dataframe单列重命名（表头右键）
    void onActionRenameColumnTriggered();
    // 快速绘图（Ribbon「列」面板 + 列头右键）：用当前选区打开表内图表浮窗
    void onActionQuickPlotTriggered();
    // 表内查找/替换（002 T1.4，Ribbon「编辑」面板；唤出当前数据页浮动条）
    void onActionFindTriggered();
    void onActionReplaceTriggered();
    // 条件格式（003 T1.5，Ribbon「Table Style」面板；与表体右键菜单共用页面入口）
    void onActionConditionalFormatTriggered();
    // 计算列（004 T1.23，Ribbon「Data Tools」面板；与表体右键菜单共用页面入口）
    void onActionComputedColumnTriggered();
    // 全部重算计算列（004 T1.23，Ribbon「Data Tools」面板）
    void onActionRecomputeAllComputedColumnsTriggered();
    // 分列 / 合并列（004 T1.9，Ribbon「Data Tools」面板；与表体右键菜单共用页面入口）
    void onActionSplitColumnTriggered();
    void onActionMergeColumnsTriggered();
    // 分组聚合（004 T1.22，Ribbon「Data Tools」面板；与表体右键菜单共用页面入口）
    void onActionGroupByTriggered();
    // 转置 / 宽转长（004 T1.27，Ribbon「Data Tools」面板；与表体右键菜单共用页面入口）
    void onActionTransposeTriggered();
    void onActionMeltTriggered();
    // 填充（005 T1.24，Ribbon「Edit」面板下拉；与表体右键「填充」子菜单、Ctrl+D 共用页面入口）
    void onActionFillDownTriggered();
    void onActionFillUpTriggered();
    void onActionFillForwardTriggered();
    void onActionFillBackwardTriggered();
    void onActionFillSeriesTriggered();
    // 数据校验（005 T1.25，Ribbon「Data Tools」面板；与表体右键菜单共用页面入口）
    void onActionValidationRulesTriggered();
    void onActionRunValidationTriggered();
    // 单元格批注（005 T1.26，Ribbon「Edit」面板下拉；与表体右键菜单共用页面入口）
    void onActionNoteTriggered();
    // 列视图（006 T1.8，Ribbon「View」面板；与列头右键「列」子菜单共用页面入口）
    void onActionFreezeColumnsTriggered();
    void onActionHideColumnTriggered();
    void onActionShowAllColumnsTriggered();
    void onActionColumnViewDialogTriggered();
    void onActionFreezeFirstColumnTriggered();
    void onActionFreezeColumnsPromptTriggered();
    void onActionUnfreezeColumnsTriggered();
    // 定位（007 T1.7，Ribbon「View」面板；Ctrl+G 在 DADataTableView 红线5）
    void onActionGotoCellTriggered();
    // 排序（Ribbon「Sort」面板；与列头右键排序组共用页面入口）
    void onActionSortTriggered();           // 打开自定义排序对话框
    void onActionSortAscendingTriggered();  // 按当前列升序
    void onActionSortDescendingTriggered(); // 按当前列降序
    void onActionSortClearTriggered();      // 清除排序（按索引恢复原始行序）
    void onActionDeleteNoteTriggered();
    void onActionClearNotesTriggered();
    // 填充共用的列右键菜单项（数据管理树 series 节点右键用）
    void populateColumnContextMenu(QMenu& menu);
    // 选中列转换为数值
    void onActionCastToNumTriggered();
    // 选中列转换为文字
    void onActionCastToStringTriggered();
    // 选中列转换为日期
    void onActionCastToDatetimeTriggered();
    // 选中列转换为索引
    void onActionChangeToIndexTriggered();
    // 表格样式
    void onTableStyleFillColorChanged(const QColor& c);
    void onTableStyleFontChanged(const QFont& f);
    void onTableStyleFontColorChanged(const QColor& c);
    void onActionClearStyleSelectedTriggered();
    void onActionClearStyleAllTriggered();
    // 表格列显示格式
    void onTableDisplayFormatCategoryChanged(DATableDisplayFormat::Category c);
    void onActionTableFormatCellsTriggered();

private:
    DAAppActionHandlerDeps mDeps;
    DADialogTableDisplayFormat* mDialogTableDisplayFormat { nullptr };  ///< 表格显示格式对话框（堆分配复用）
};
}  // namespace DA

#endif  // DAAPPTABLEACTIONHANDLER_H
