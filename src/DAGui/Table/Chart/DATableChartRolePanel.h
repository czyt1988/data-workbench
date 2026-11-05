#ifndef DATABLECHARTROLEPANEL_H
#define DATABLECHARTROLEPANEL_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include "DAGlobals.h"
#include <QWidget>

namespace DA
{
class DATableChartRolePanelPrivate;

/**
 * @brief 快速绘图的轻量角色设置面板（v3）
 *
 * 按当前图型生成「角色标签 + 列下拉」行（2D/3D 统一），列下拉可换整表任意列；
 * 任何改动立即写回内部规格并发 specEdited，由浮窗即时重绘（不弹模态对话框）。
 *
 * 复用边界（实施方案 §5）：本类只写角色与聚合选择，不嵌入任何添加绘图向导页面；
 * 取数与图元构建仍由 DAChartQuickPlot 统一完成。
 */
class DAGUI_API DATableChartRolePanel : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DATableChartRolePanel)
public:
    explicit DATableChartRolePanel(QWidget* parent = nullptr);
    ~DATableChartRolePanel() override;

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
};

}  // namespace DA

#endif  // DATABLECHARTROLEPANEL_H
