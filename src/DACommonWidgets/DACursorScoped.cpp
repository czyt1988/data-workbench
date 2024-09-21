#include "DACursorScoped.h"
#include <QApplication>
namespace DA
{

/**
 * @brief 传入图标并全局设置为当前图标
 * @param cur
 */
DACursorScoped::DACursorScoped(const QCursor& cur)
{
    setCursor(cur);
}

DACursorScoped::~DACursorScoped()
{
    release();
}

void DACursorScoped::setCursor(const QCursor& cur)
{
    QApplication::setOverrideCursor(cur);
}

/**
 * @brief 提前恢复图标
 */
void DACursorScoped::release()
{
    QApplication::restoreOverrideCursor();
}
}  // end namespace DA
