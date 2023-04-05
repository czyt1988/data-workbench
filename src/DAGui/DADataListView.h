#ifndef DADATALISTWIDGET_H
#define DADATALISTWIDGET_H
#include <QTreeView>
#include "DAGuiAPI.h"
namespace DA
{
class DAGUI_API DADataListView : public QTreeView
{
public:
    DADataListView(QWidget* p = nullptr);
};
}  // end of namespace DA
#endif  // DADATALIST_H
