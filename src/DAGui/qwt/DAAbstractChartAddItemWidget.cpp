#include "DAAbstractChartAddItemWidget.h"
#include <QDebug>
namespace DA
{
DAAbstractChartAddItemWidget::DAAbstractChartAddItemWidget(QWidget* par) : QWidget(par)
{
}

DAAbstractChartAddItemWidget::~DAAbstractChartAddItemWidget()
{
}

void DAAbstractChartAddItemWidget::setDataManager(DADataManager* dmgr)
{
    if (mDataManager != dmgr) {
        mDataManager = dmgr;
        Q_EMIT dataManagerChanged(dmgr);
    }
}

DADataManager* DAAbstractChartAddItemWidget::getDataManager() const
{
    return mDataManager;
}

}  // end namespace of DA
