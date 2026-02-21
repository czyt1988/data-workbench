#include "DAAbstractPlotItemSettingWidget.h"
#include "plot/QImPlotItemNode.h"

/**
 * @def DAAbstractChartItemSettingWidget_ReturnWhenItemNull
 * @brief 如果getPlotItem返回空则return，此宏经常用于DAAbstractChartItemSettingWidget之类的槽函数进行判断是否设置了item
 *
 */
#ifndef DAAbstractChartItemSettingWidget_ReturnWhenItemNull
#define DAAbstractChartItemSettingWidget_ReturnWhenItemNull                                                            \
    do {                                                                                                               \
        if (nullptr == getPlotItem()) {                                                                                \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#endif

namespace DA
{
DAAbstractPlotItemSettingWidget::DAAbstractPlotItemSettingWidget(QWidget* parent) : QWidget(parent)
{
}

DAAbstractPlotItemSettingWidget::~DAAbstractPlotItemSettingWidget()
{
}

/**
 * @brief 设置plotitem
 *
 * 注意，如果重载，必须保证调用了DAAbstractChartItemSettingWidget::setPlotItem(item),
 * 否则getPlotItem会失效，同时无法自动处理item的detach
 * @param item
 */
void DAAbstractPlotItemSettingWidget::setPlotItem(QIM::QImPlotItemNode* item)
{
    mPlotItem = item;
    updateUI(item);
}

/**
 * @brief setPlotItem之后调用的虚函数
 * @param item
 * @return
 */
QIM::QImPlotItemNode* DAAbstractPlotItemSettingWidget::getPlotItem() const
{
    return mPlotItem.data();
}

/**
 * @brief 判断是否有item
 * @return
 */
bool DAAbstractPlotItemSettingWidget::isHaveItem() const
{
    return (!mPlotItem.isNull());
}

/**
 * @brief 判断当前item是否是对应的rtti，如果没有item也返回false
 * @param rtti
 * @return
 */
bool DAAbstractPlotItemSettingWidget::checkItemType(int type) const
{
    if (!isHaveItem()) {
        return false;
    }
    return (mPlotItem->type() == type);
}

void DAAbstractPlotItemSettingWidget::updateUI(QIM::QImPlotItemNode* item)
{
    Q_UNUSED(item);
}


}
