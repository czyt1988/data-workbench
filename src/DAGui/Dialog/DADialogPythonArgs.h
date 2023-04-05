#ifndef DADIALOGPYTHONARGS_H
#define DADIALOGPYTHONARGS_H
#include <QDialog>
#include "DAPybind11InQt.h"
#include "DAGuiAPI.h"
namespace DA
{
/**
 * @brief 用于维护python args的基类dialog
 */
class DAGUI_API DADialogPythonArgs : public QDialog
{
    Q_OBJECT
public:
    DADialogPythonArgs(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~DADialogPythonArgs();
    //获取args参数
    virtual pybind11::dict getArgs() const = 0;
};
}  // end of namespace DA
#endif  // DADIALOGPYTHONARGS_H
