#include "DAWaitCursorScoped.h"
#include <QApplication>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAWaitCursorScoped
//===================================================
DAWaitCursorScoped::DAWaitCursorScoped()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

DAWaitCursorScoped::~DAWaitCursorScoped()
{
    QApplication::restoreOverrideCursor();
}

void DAWaitCursorScoped::setWaitCursor()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void DAWaitCursorScoped::setCursor(const QCursor& cur)
{
    QApplication::setOverrideCursor(cur);
}

void DAWaitCursorScoped::release()
{
    QApplication::restoreOverrideCursor();
}
