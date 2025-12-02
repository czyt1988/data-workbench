#ifndef FCPLUGINMANAGERDIALOG_H
#define FCPLUGINMANAGERDIALOG_H

#include <QtWidgets/QDialog>
#include <QStandardItemModel>
#include "DAGlobals.h"
namespace Ui
{
class DAPluginManagerDialog;
}

namespace DA
{
class DAAppPluginManager;
class DAPluginManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DAPluginManagerDialog(DAAppPluginManager* mgr, QWidget* parent = nullptr);
    ~DAPluginManagerDialog();

protected:
    void changeEvent(QEvent* e);

private:
    void init();

private:
    Ui::DAPluginManagerDialog* ui;
    DAAppPluginManager* mPluginMgr { nullptr };
};
}  // namespace DA
#endif  // FCPLUGINMANAGERDIALOG_H
