#ifndef DADATATABLEVIEW_H
#define DADATATABLEVIEW_H
#include "DAGuiAPI.h"
#include "DACacheWindowTableView.h"
#include "Models/DADataTableModel.h"
#include "DAData.h"

class QMenu;
class QContextMenuEvent;
class QShowEvent;

namespace DA
{
class DATableNoteStore;
/**
 * @brief 数据表格视图
 *
 * 表体右键菜单与快捷键的集中落点（设计稿 §3.5）：菜单内容（哪些项、顺序、启用规则）
 * 单点定义在本类（DAGui），QAction 自建、不共享 APP 的 action 对象；点击后发表意信号
 * 交 APP 接线到既有槽逻辑，视图内不新增执行逻辑。表格快捷键一律在 keyPressEvent 处理
 * （Ctrl+C/Ctrl+F/Ctrl+H/Ctrl+D/Esc 在本类处理；未处理的键一律转发基类 DACacheWindowTableView，
 * 恢复整表语义键 Ctrl+G/Ctrl+Home/Ctrl+End/PageUp/PageDown/Home/End——002 追加查找替换族，
 * Ribbon action 不设 shortcut，红线5）。
 */
class DAGUI_API DADataTableView : public DACacheWindowTableView
{
    Q_OBJECT
public:
    explicit DADataTableView(QWidget* parent = nullptr);
    ~DADataTableView();
    DADataTableModel* getDataModel() const;
    // 设置datafarme
    void setData(const DAData& d);
    DAData getData() const;
    // 把选中单元格按包围矩形拼成 Tab/换行分隔的文本写入剪贴板，读 Qt::DisplayRole（随显示格式）
    // 主页剪贴板路由 actionCopy 表格分支也调用此函数
    bool copySelectionToClipboard();
    // 构建表体右键菜单（v1：剪切/复制/粘贴；002：查找/替换；003：条件格式；004：数据工具；
    // 005：填充/校验/批注；调用方负责弹出与销毁）
    QMenu* buildContextMenu(const QModelIndex& at);
    // 注入批注子存储（005 T1.26：右键菜单「插入/编辑/删除批注」的启用规则查当前格
    // 有无批注；非拥有，可空——页面构造时注入，随元数据束比视图长寿）
    void setNoteStore(DATableNoteStore* store);

Q_SIGNALS:
    // 表体右键菜单表意信号（APP 接线到既有剪切/复制/粘贴槽逻辑）
    void contextMenuCutRequested();
    void contextMenuCopyRequested();
    void contextMenuPasteRequested();
    // 查找/替换入口（002 T1.4）：Ctrl+F/Ctrl+H 快捷键与右键菜单共用，页面据此唤出浮动条；
    // prefillFromCurrentCell：右键菜单入口置真（浮动条查找框预填当前单元格内容），
    // 快捷键入口置假（保持空框/旧词）
    void findRequested(bool prefillFromCurrentCell);
    void replaceRequested(bool prefillFromCurrentCell);
    // 定位入口（007 T1.7）：Ctrl+G 与滚动条右键「定位到行」共用，页面据此唤出定位对话框
    void gotoRequested();
    // Esc 关闭查找条（焦点在表格时；对话框自身焦点下的 Esc 由对话框处理）
    void findBarCloseRequested();
    // 条件格式入口（003 T1.5）：表体右键菜单唤出规则编辑对话框（列头菜单不放，R16）
    void conditionalFormatRequested();
    // 计算列入口（004 T1.23）：表体右键菜单与 Ribbon「Data Tools」面板同入口
    void computedColumnRequested();
    // 分列/合并列入口（004 T1.9）：表体右键菜单与 Ribbon「Data Tools」面板同入口
    //（启用规则：分列=单列选区、合并=多列选区，见 buildContextMenu）
    void splitColumnRequested();
    void mergeColumnsRequested();
    // 分组聚合入口（004 T1.22）：产出新数据集（非破坏），dataframe 数据即可用
    void groupByRequested();
    // 转置 / 宽转长入口（004 T1.27）：默认产出新数据集（R14），dataframe 数据即可用
    void transposeRequested();
    void meltRequested();
    // 填充入口（005 T1.24）：Ctrl+D（向下）与表体右键「填充」子菜单共用，
    // 页面据此调 fillSelection/showFillSeriesDialog（选区语义按列独立，求值在 Python）
    void fillDownRequested();
    void fillUpRequested();
    void fillForwardRequested();
    void fillBackwardRequested();
    void fillSeriesRequested();
    // 数据校验入口（005 T1.25）：表体右键菜单与 Ribbon「Data Tools」面板同入口
    void validationRulesRequested();
    void runValidationRequested();
    // 单元格批注入口（005 T1.26）：表体右键菜单与 Ribbon「编辑」面板同入口；
    // 插入/编辑共用页面 showNoteEditorForCurrentCell（弹层按有无批注自适应）
    void noteInsertRequested();
    void noteEditRequested();
    void noteDeleteRequested();
    void noteClearAllRequested();

protected:
    // Ctrl+C/Ctrl+X/Ctrl+V/Ctrl+F/Ctrl+H/Esc 在本类处理；未处理的键一律转发基类（恢复整表语义键
    // Ctrl+G/Ctrl+Home/Ctrl+End/PageUp/PageDown/Home/End）。表格快捷键唯一挂点，
    // Ribbon action 不设 shortcut（红线5）
    void keyPressEvent(QKeyEvent* event) override;
    // ShortcutOverride：本类 keyPressEvent 处理的标准键先在覆盖阶段认领，
    // 防止其它 dock 控件注册的窗口级快捷键把按键抢走（键事件不再到达本类）
    bool event(QEvent* e) override;
    // 左上角角格（QTableCornerButton）右缘热区拖拽改行头宽（与行头右缘热区同语义）
    bool eventFilter(QObject* watched, QEvent* e) override;
    void showEvent(QShowEvent* event) override;
    // 定位到行（007 T1.7）：覆写为只发 gotoRequested 表意信号，本类不弹 QInputDialog
    //（滚动条右键经 onGotoRowRequested → promptGotoRow 虚派发走同一条链）
    void promptGotoRow() override;
    // 表体右键菜单
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // 角格事件过滤器懒装（QTableCornerButton 由 QTableView 首布局创建）
    void installCornerButtonFilter();
    DATableNoteStore* mNoteStore { nullptr };  ///< 非拥有（批注子存储，005 T1.26，可空）
    bool mCornerResizing { false };            ///< 角格右缘行头宽拖拽进行中
    int mCornerResizeStartX { 0 };             ///< 拖拽起点全局 x
    int mCornerResizeStartWidth { 0 };         ///< 拖拽起点行头宽
    bool mCornerFilterInstalled { false };     ///< 角格事件过滤器已装（防重复）
    QWidget* mCornerButton { nullptr };        ///< 左上角角格（QTableCornerButton，非拥有）
    // 数据源是否可编辑（判定路径同 DADataTableModel::actualFlags 的只读源分支）
    bool isDataSourceEditable() const;
    // 当前选区覆盖的数据列数（去重；尾部预留空白列不计，004 分列/合并启用规则用）
    int selectedDataColumnCount() const;
    // 是否存在某数据列拥有 >=2 行的连续选中行区间（005 填充启用规则：
    // 不足两行时向下/向上/ffill/bfill 无从发生，菜单置灰，计划 §3.3-1）
    bool hasMultiRowSelectedRun() const;
};
}
#endif  // DADATATABLEVIEW_H
