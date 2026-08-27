#ifndef DAWORKBENCHABOUTDIALOG_H
#define DAWORKBENCHABOUTDIALOG_H

#include <QColor>
#include <QDialog>

namespace Ui {
class DAWorkbenchAboutDialog;
}
namespace DA {
/**
 * @brief 关于对话框，展示软件简介、联系方式以及C++/Python第三方库清单与协议
 */
class DAWorkbenchAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DAWorkbenchAboutDialog(QWidget* parent = nullptr);
    virtual ~DAWorkbenchAboutDialog() override;

private:
    void setupUiStyle();
    void makeAboutInfo();
    QString makeOverviewSection() const;
    QString makeCppLibrarySection() const;
    QString makePythonLibrarySection() const;
    QColor dimTextColor() const;
    QColor zebraColor() const;

private:
    Ui::DAWorkbenchAboutDialog* ui;
};
}  // namespace DA
#endif  // DAWORKBENCHABOUTDIALOG_H
