#ifndef DADIALOGAPPSETTING_H
#define DADIALOGAPPSETTING_H
#include "DAGuiAPI.h"
#include <QDialog>

namespace Ui
{
class DADialogAppSetting;
}
namespace DA
{
class DAGUI_API DADialogAppSetting : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogAppSetting(QWidget* parent = nullptr);
    ~DADialogAppSetting();
private slots:
    void onSettingApply();
    void onAccepted();

private:
    Ui::DADialogAppSetting* ui;
};
}

#endif  // DADIALOGAPPSETTING_H
