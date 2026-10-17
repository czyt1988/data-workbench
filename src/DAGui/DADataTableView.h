#ifndef DADATATABLEVIEW_H
#define DADATATABLEVIEW_H
#include "DAGuiAPI.h"
#include "DACacheWindowTableView.h"
#include "Models/DADataTableModel.h"
#include "DAData.h"

class QMenu;
class QContextMenuEvent;

namespace DA
{
/**
 * @brief 数据表格视图
 *
 * 表体右键菜单与快捷键的集中落点（设计稿 §3.5）：菜单内容（哪些项、顺序、启用规则）
 * 单点定义在本类（DAGui），QAction 自建、不共享 APP 的 action 对象；点击后发表意信号
 * 交 APP 接线到既有槽逻辑，视图内不新增执行逻辑。表格快捷键一律在 keyPressEvent 处理
 * （现状：Ctrl+C 复制、Ctrl+F 查找、Ctrl+H 替换、Esc 关查找条——002 追加后三键，
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
    // 构建表体右键菜单（v1：剪切/复制/粘贴；002：查找/替换；003：条件格式；调用方负责弹出与销毁）
    QMenu* buildContextMenu(const QModelIndex& at);

Q_SIGNALS:
    // 表体右键菜单表意信号（APP 接线到既有剪切/复制/粘贴槽逻辑）
    void contextMenuCutRequested();
    void contextMenuCopyRequested();
    void contextMenuPasteRequested();
    // 查找/替换入口（002 T1.4）：Ctrl+F/Ctrl+H 快捷键与右键菜单共用，页面据此唤出浮动条
    void findRequested();
    void replaceRequested();
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

protected:
    // Ctrl+C / Ctrl+Insert 复制；Ctrl+F 查找；Ctrl+H 替换；Ctrl+D 向下填充（005）；
    // Esc 关查找条（快捷键唯一挂点，红线5）
    void keyPressEvent(QKeyEvent* event) override;
    // 表体右键菜单
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
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
