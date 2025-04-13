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

void DAAbstractChartAddItemWidget::setCurrentData(const DAData& d)
{
    if (mData != d) {
        mData = d;
        Q_EMIT currentDataChanged(d);
    }
}

const DAData& DAAbstractChartAddItemWidget::getCurrentData() const
{
    return mData;
}

/**
 * @brief 可以通过此函数进行主动更新
 */
void DAAbstractChartAddItemWidget::updateData()
{
}

}  // end namespace of DA
