#include "DAAbstractChart3DAddItemWidget.h"
namespace DA
{

DAAbstractChart3DAddItemWidget::DAAbstractChart3DAddItemWidget(QWidget* par) : QWidget(par)
{
}

DAAbstractChart3DAddItemWidget::~DAAbstractChart3DAddItemWidget()
{
}

/**
 * @brief 设置datamanager
 * @param dmgr
 */
void DAAbstractChart3DAddItemWidget::setDataManager(DADataManager* dmgr)
{
    if (mDataManager == dmgr) {
        return;
    }
    mDataManager = dmgr;
    Q_EMIT dataManagerChanged(dmgr);
}

/**
 * @brief 获取datamanager
 * @return
 */
DADataManager* DAAbstractChart3DAddItemWidget::getDataManager() const
{
    return mDataManager;
}

}  // end DA
