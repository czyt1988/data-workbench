#include "DAWaitCursorScoped.h"
#include <QApplication>

namespace DA
{

DAWaitCursorScoped::DAWaitCursorScoped() : DACursorScoped(QCursor(Qt::WaitCursor))
{
}

DAWaitCursorScoped::~DAWaitCursorScoped()
{
    QApplication::restoreOverrideCursor();
}
}  // end namespace DA
