#ifndef DAWORKBENCHABOUTDIALOG_H
#define DAWORKBENCHABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class DAWorkbenchAboutDialog;
}
namespace DA{
class DAWorkbenchAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DAWorkbenchAboutDialog(QWidget *parent = nullptr);
    ~DAWorkbenchAboutDialog();
private:
    void makeAboutInfo();
private:
    Ui::DAWorkbenchAboutDialog *ui;
};
}
#endif // DAWORKBENCHABOUTDIALOG_H
