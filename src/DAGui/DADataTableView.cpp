#include "DADataTableView.h"
namespace DA
{
DADataTableView::DADataTableView(QWidget* parent) : DACacheWindowTableView(parent)
{
}

DADataTableView::~DADataTableView()
{
}

DADataTableModel* DADataTableView::getDataModel() const
{
    return qobject_cast< DADataTableModel* >(model());
}

void DADataTableView::setData(const DAData& d)
{
    DADataTableModel* m = getDataModel();
    if (!m) {
        qWarning() << tr("DADataTableView must set model first");  // cn:你需要先设置模型
        return;
    }
    m->setData(d);
}

DAData DADataTableView::getData() const
{
    DADataTableModel* m = getDataModel();
    if (m) {
        return m->getData();
    }
    return DAData();
}
}
