#ifndef DADATATABLEVIEW_H
#define DADATATABLEVIEW_H
#include "DAGuiAPI.h"
#include "DACacheWindowTableView.h"
#include "Models/DADataTableModel.h"
#include "DAData.h"
namespace DA
{
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

protected:
    // Ctrl+C / Ctrl+Insert 复制选中单元格到剪贴板
    void keyPressEvent(QKeyEvent* event) override;

private:
};
}
#endif  // DADATATABLEVIEW_H
