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
    void buildUI(DAAppConfig* config);
    ~DAAppSettingDialog();
};
}

#endif  // DADIALOGAPPSETTING_H
