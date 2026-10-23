#include "DAAbstractChartAddWidgetBase.h"

namespace DA
{

DAAbstractChartAddWidgetBase::DAAbstractChartAddWidgetBase(QWidget* par) : QWidget(par)
{
}

DAAbstractChartAddWidgetBase::~DAAbstractChartAddWidgetBase()
{
}

/**
 * @brief 设置数据管理器
 * @param dmgr 数据管理器，允许为空
 */
void DAAbstractChartAddWidgetBase::setDataManager(DADataManager* dmgr)
{
    if (mDataManager == dmgr) {
        return;
    }
    mDataManager = dmgr;
    Q_EMIT dataManagerChanged(dmgr);
}

/**
 * @brief 获取数据管理器
 * @return 未设置时返回 nullptr
 */
DADataManager* DAAbstractChartAddWidgetBase::getDataManager() const
{
    return mDataManager;
}

}  // end DA
