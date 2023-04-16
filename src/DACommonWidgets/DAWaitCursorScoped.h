#ifndef DAWAITCURSORSCOPED_H
#define DAWAITCURSORSCOPED_H
#include <QCursor>
#include "DACommonWidgetsAPI.h"
namespace DA
{
/**
 * @brief 一个等待鼠标的保护类
 */
class DACOMMONWIDGETS_API DAWaitCursorScoped
{
public:
    DAWaitCursorScoped();
    ~DAWaitCursorScoped();
    void setWaitCursor();
    void setCursor(const QCursor& cur = QCursor(Qt::WaitCursor));
    void release();
};
}  // namespace DA

#ifndef DA_WAIT_CURSOR_SCOPED
#define DA_WAIT_CURSOR_SCOPED()                                                                                        \
    DAWaitCursorScoped __da__wait__cursor;                                                                             \
    Q_UNUSED(__da__wait__cursor)
#endif
#ifndef DA_WAIT_CURSOR_SCOPED_NS
#define DA_WAIT_CURSOR_SCOPED_NS()                                                                                     \
    DA::DAWaitCursorScoped __da__wait__cursor;                                                                         \
    Q_UNUSED(__da__wait__cursor)
#endif
#endif  // DAWAITCURSORSCOPED_H
