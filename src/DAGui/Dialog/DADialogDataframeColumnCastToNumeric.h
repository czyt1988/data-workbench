#ifndef DADIALOGDATAFRAMECOLUMNCASTTONUMERIC_H
#define DADIALOGDATAFRAMECOLUMNCASTTONUMERIC_H

#include <QDialog>
#include "DADialogPythonArgs.h"
#include "DAGuiAPI.h"
namespace Ui
{
class DADialogDataframeColumnCastToNumeric;
}
namespace DA
{
/**
 * @brief 针对da_dataframe.da_cast_to_num函数的参数设置对话框
 */
class DAGUI_API DADialogDataframeColumnCastToNumeric : public DADialogPythonArgs
{
    Q_OBJECT

public:
    explicit DADialogDataframeColumnCastToNumeric(QWidget* parent = nullptr);
    ~DADialogDataframeColumnCastToNumeric();
    pybind11::dict getArgs() const override;
    QString getArgErrorsValue() const;
    QString getArgDowncastValue() const;

private:
    Ui::DADialogDataframeColumnCastToNumeric* ui;
};
}  // end of namespace DA
#endif  // DADIALOGDATAFRAMECOLUMNCASTTONUMERIC_H
