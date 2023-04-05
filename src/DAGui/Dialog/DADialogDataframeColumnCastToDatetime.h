#ifndef DADIALOGDATAFRAMECOLUMNCASTTODATETIME_H
#define DADIALOGDATAFRAMECOLUMNCASTTODATETIME_H

#include "DADialogPythonArgs.h"
#include "DAGuiAPI.h"
namespace Ui
{
class DADialogDataframeColumnCastToDatetime;
}
namespace DA
{
/**
 * @brief to_datetime参数设置
 */
class DAGUI_API DADialogDataframeColumnCastToDatetime : public DADialogPythonArgs
{
    Q_OBJECT

public:
    explicit DADialogDataframeColumnCastToDatetime(QWidget* parent = nullptr);
    ~DADialogDataframeColumnCastToDatetime();
    pybind11::dict getArgs() const override;
    QString getArgErrorsValue() const;
    QString getArgUnitValue() const;
    QString getArgOriginValue() const;

protected:
    void changeEvent(QEvent* e);

private slots:
    void on_toolButtonHowEditFormat_clicked();

    void on_radioButtonUnix_clicked(bool checked);

    void on_radioButtonJulian_clicked(bool checked);

private:
    Ui::DADialogDataframeColumnCastToDatetime* ui;
};
}  // end of namespace DA
#endif  // DADIALOGDATAFRAMECOLUMNCASTTODATETIME_H
