#include "Chart/DAStatsPlotRequest.h"
#include <QObject>

namespace DA
{

DAStatsPlotRequest::~DAStatsPlotRequest() {}

/**
 * @brief 基类级校验
 *
 * 校验三层：目标 Figure/Chart 非空、数据源为 DataFrame。
 * 子类 isValid() 在此基础上补充本类型必需字段（列名非空等）。
 * @return 基类级全部有效返回 true
 */
bool DAStatsPlotRequest::isValidBase() const
{
    return nullptr != figure && nullptr != chart && data.isDataFrame();
}

/**
 * @brief 基类级错误信息
 * @return 全部有效时返回空串；否则返回第一个命中的可读错误（国际化）
 */
QString DAStatsPlotRequest::baseErrorString() const
{
    if (nullptr == figure || nullptr == chart) {
        return QObject::tr("No figure/chart available for statistical plot");  // cn: 没有可用于统计绘图的图表窗口
    }
    if (!data.isDataFrame()) {
        return QObject::tr("Cannot resolve the data source for statistical plot; "
                           "please ensure a dataframe is selected in the settings window");  // cn: 无法确定统计绘图的数据源，请确保在设置窗口中已选择数据表
    }
    return QString();
}

}  // namespace DA
