#ifndef DASTATUSBAR_H
#define DASTATUSBAR_H
#include <QStatusBar>
namespace DA
{
class DAStatusBarWidget;

class DAStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    DAStatusBar(QWidget* par = nullptr);
    DAStatusBarWidget* getStatusWidget() const;

private:
    DAStatusBarWidget* mStatusWidget { nullptr };
};
}

#endif  // DASTATUSBAR_H
