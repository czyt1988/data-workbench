#ifndef DACHARTLISTWIDGET_H
#define DACHARTLISTWIDGET_H
#include <QListView>
#include "DAGuiAPI.h"
namespace DA
{
class DAGUI_API DAChartListView : public QListView
{
public:
    DAChartListView(QWidget* p = nullptr);
};
}  // end of namespace DA
#endif  // DACHARTWIDGETLIST_H
