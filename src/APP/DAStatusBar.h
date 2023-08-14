#ifndef DASTATUSBAR_H
#define DASTATUSBAR_H
#include <QStatusBar>
namespace DA
{

class DAStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    DAStatusBar(QWidget* par = nullptr);
};
}

#endif  // DASTATUSBAR_H
