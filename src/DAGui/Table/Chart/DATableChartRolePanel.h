#ifndef DATABLECHARTROLEPANEL_H
#define DATABLECHARTROLEPANEL_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include "DAGlobals.h"
#include <QList>
#include <QObject>

namespace DA
{
class DAPropertyPanelWidget;
class DATableChartRolePanelPrivate;

/**
 * @brief 快速绘图的角色设置行生成器（v5）
 *
 * 把「角色标签 + 列下拉」行写进宿主设置页的通用属性面板（DAPropertyPanelWidget 的分组内嵌面板），
 * 行的外观、行高与属性名宽度由属性面板统一，与图表/工作流设置页保持一致；
 * 任何改动立即写回内部规格并发 specEdited，由宿主即时重绘。
 *
 * 复用边界（实施方案 §5）：本类只写角色与聚合选择，不嵌入任何添加绘图向导页面；
 * 取数与图元构建仍由 DAChartQuickPlot 统一完成。
 */
class DAGUI_API DATableChartRolePanel : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DATableChartRolePanel)
public:
    explicit DATableChartRolePanel(QObject* parent = nullptr);
    ~DATableChartRolePanel() override;

    // 设置承载角色行的属性面板（设置页「数据角色」分组的内嵌面板；非拥有）
    void setRowPanel(DAPropertyPanelWidget* panel);
    // 当前承载面板（未设置返回 nullptr）
    DAPropertyPanelWidget* rowPanel() const;

    // 设置请求与规格（会按规格图型重建角色行）
    void setContext(const DATableChartRequest& request, const DATableChartSpec& spec);
    // 当前请求（已包含设置区手动追加的列）与规格
    const DATableChartRequest& request() const;
    const DATableChartSpec& spec() const;

    // 列的显示名（重名列追加序号）
    static QString columnDisplayName(const DATableChartColumn& column);

Q_SIGNALS:
    // 任一角色/聚合/分箱发生变化（已写回内部规格）
    void specEdited();

private:
    // 重建角色行（延迟调用，避免在下拉框自己的信号栈上销毁它）
    void rebuildRows();
    // 移除当前全部角色行（deleteLater 释放，可在信号栈上安全调用）
    void clearRows();
};

}  // namespace DA

#endif  // DATABLECHARTROLEPANEL_H
