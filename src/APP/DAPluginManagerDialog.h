#ifndef FCPLUGINMANAGERDIALOG_H
#define FCPLUGINMANAGERDIALOG_H

#include <QtWidgets/QDialog>
#include "DAGlobals.h"
namespace Ui
{
class DAPluginManagerDialog;
}

namespace DA
{
class DAAppPluginManager;

/**
 * @brief 插件管理对话框
 *
 * 列出plugins目录下的C++插件与pyplugins目录下的Python节点包，
 * 通过勾选框启用/禁用插件：
 * - C++插件点"应用/确定"后立即热加载/热卸载并持久化到.pluginignore
 * - Python节点包持久化到pyplugins/.pluginignore，下次启动生效
 */
class DAPluginManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DAPluginManagerDialog(DAAppPluginManager* mgr, QWidget* parent = nullptr);
    virtual ~DAPluginManagerDialog() override;

protected:
    void changeEvent(QEvent* e);
    // 关闭前应用勾选变更
    void accept() override;

private Q_SLOTS:
    // 应用按钮
    void onApplyClicked();

private:
    void init();
    // 重建插件列表（勾选状态与当前生效状态一致）
    void updatePluginList();
    // 应用全部勾选变更（热插拔并持久化），完成后刷新列表
    void applyChanges();

private:
    Ui::DAPluginManagerDialog* ui;
    DAAppPluginManager* mPluginMgr { nullptr };
};
}  // namespace DA
#endif  // FCPLUGINMANAGERDIALOG_H
