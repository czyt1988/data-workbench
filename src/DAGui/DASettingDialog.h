#ifndef DASETTINGDIALOG_H
#define DASETTINGDIALOG_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "DASettingWidget.h"
namespace Ui
{
class DASettingDialog;
}

namespace DA
{
/**
 * @brief 设置对话框
 */
class DAGUI_API DASettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DASettingDialog(QWidget* parent = nullptr);
    ~DASettingDialog();
    DASettingWidget* settingWidget() const;
    //获取改变的页面
    QList< DAAbstractSettingPage* > getChanggedPages() const;
public Q_SLOTS:
    //设置页面
    void setPage(int index);
private Q_SLOTS:
    void onPushButtonOKClicked();
    void onPushButtonApplyClicked();
Q_SIGNALS:
    /**
     * @brief 配置改变或者应用，需要保存的信号
     */
    void needSave();

private:
    Ui::DASettingDialog* ui;
};
}

#endif  // DASETTINGDIALOG_H
