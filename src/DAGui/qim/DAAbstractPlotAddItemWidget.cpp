#include "DAAbstractPlotAddItemWidget.h"
#include <QDebug>
namespace DA
{
DAAbstractPlotAddItemWidget::DAAbstractPlotAddItemWidget(QWidget* par) : QWidget(par)
{
}

DAAbstractPlotAddItemWidget::~DAAbstractPlotAddItemWidget()
{
}

void DAAbstractPlotAddItemWidget::setDataManager(DADataManager* dmgr)
{
    if (mDataManager != dmgr) {
        mDataManager = dmgr;
        Q_EMIT dataManagerChanged(dmgr);
    }
}

DADataManager* DAAbstractPlotAddItemWidget::getDataManager() const
{
    return mDataManager;
}

}  // end namespace of DA
