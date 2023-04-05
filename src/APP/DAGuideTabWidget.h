#ifndef DAGUIDETABWIDGET_H
#define DAGUIDETABWIDGET_H
#include <QTabWidget>
#include "DAGlobals.h"
namespace DA
{
class DAGuideTabWidget : public QTabWidget
{
public:
    DAGuideTabWidget(QWidget* p = nullptr);
};
}  // end of namespace DA
#endif  // DAGUIDETABWIDGET_H
