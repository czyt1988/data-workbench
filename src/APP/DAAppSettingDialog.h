#ifndef DAAPPSETTINGDIALOG_H
#define DAAPPSETTINGDIALOG_H
#include "DAGuiAPI.h"
#include <QDialog>
#include "DASettingDialog.h"

namespace DA
{
class DAAppCore;
class DAAppUI;
class AppMainWindow;
class DAAppConfig;
/**
 * @brief 程序的默认设置对话框
 */
class DAAppSettingDialog : public DASettingDialog
{
    Q_OBJECT
public:
    explicit DAAppSettingDialog(QWidget* parent = nullptr);
    // 配置与核心均由 AppMainWindow 注入（依赖注入，不使用全局单例）
    void buildUI(DAAppConfig* config, DAAppCore* core);
    virtual ~DAAppSettingDialog() override;
};
}

#endif  // DADIALOGAPPSETTING_H
