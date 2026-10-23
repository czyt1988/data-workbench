#include "DAAbstractChartAddItemWidget.h"
namespace DA
{
DAAbstractChartAddItemWidget::DAAbstractChartAddItemWidget(QWidget* par) : DAAbstractChartAddWidgetBase(par)
{
}

DAAbstractChartAddItemWidget::~DAAbstractChartAddItemWidget()
{
}

/**
 * @brief 批量创建的默认实现：取单个 createPlotItem 的结果
 * @return 单元素列表或空列表
 */
QList< QwtPlotItem* > DAAbstractChartAddItemWidget::createPlotItems()
{
    QwtPlotItem* item = createPlotItem();
    if (nullptr == item) {
        return {};
    }
    return { item };
}

/**
 * @brief 坐标轴标题推荐的默认实现：不提供
 */
bool DAAbstractChartAddItemWidget::getAxisTitleHint(QString& xTitle, QString& yTitle) const
{
    Q_UNUSED(xTitle)
    Q_UNUSED(yTitle)
    return false;
}

}  // end namespace of DA
