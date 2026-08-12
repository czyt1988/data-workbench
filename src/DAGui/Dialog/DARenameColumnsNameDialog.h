#ifndef DARENAMECOLUMNSNAMEDIALOG_H
#define DARENAMECOLUMNSNAMEDIALOG_H
#include <QtWidgets/QDialog>
#include <QStandardItemModel>
#include "DAGuiAPI.h"
namespace Ui
{
class DARenameColumnsNameDialog;
}
namespace DA
{
/**
 * @brief 表头改名对话框
 */
class DAGUI_API DARenameColumnsNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DARenameColumnsNameDialog(QWidget* parent = nullptr);
    ~DARenameColumnsNameDialog();
    //设置数据名
    void setDataName(const QString& name);
    QString getDataName() const;
    //设置数据列名
    void setColumnsName(const QList< QString >& names);
    QList< QString > getColumnsName() const;

private:
    QList< QString > getColumnsNameInternal() const;

protected:
    void changeEvent(QEvent* e) override;

private Q_SLOTS:
    void on_pushButtonOK_clicked();

private:
    Ui::DARenameColumnsNameDialog* ui;
    QStandardItemModel* mModel;
    QList< QString > mNewCols;
};
}  // end of namespace DA
#endif  // DARENAMECOLUMNSNAMEDIALOG_H
