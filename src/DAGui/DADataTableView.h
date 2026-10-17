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
 * （002 起追加键位，本页现状只保留 Ctrl+C）。
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
    // 构建表体右键菜单（v1：剪切/复制/粘贴；调用方负责弹出与销毁）
    QMenu* buildContextMenu(const QModelIndex& at);

Q_SIGNALS:
    // 表体右键菜单表意信号（APP 接线到既有剪切/复制/粘贴槽逻辑）
    void contextMenuCutRequested();
    void contextMenuCopyRequested();
    void contextMenuPasteRequested();

protected:
    // Ctrl+C / Ctrl+Insert 复制选中单元格到剪贴板
    void keyPressEvent(QKeyEvent* event) override;
    // 表体右键菜单
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // 数据源是否可编辑（判定路径同 DADataTableModel::actualFlags 的只读源分支）
    bool isDataSourceEditable() const;
};
}
#endif  // DADATATABLEVIEW_H
