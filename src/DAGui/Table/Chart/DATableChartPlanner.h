#ifndef DATABLECHARTPLANNER_H
#define DATABLECHARTPLANNER_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include <QList>
#include <QString>

namespace DA
{

/**
 * @brief X 轴候选（浮窗「X 轴」下拉的一项）
 */
struct DAGUI_API DATableChartXOption
{
    int column { -1 };          ///< request.columns 下标；-1 表示行号
    bool asCategory { false };  ///< true = 把该低基数数值列当分类轴（出柱状图）

    bool isRowIndex() const { return column < 0; }
    bool equals(const DATableChartXOption& other) const
    {
        return column == other.column && asCategory == other.asCategory;
    }
};

/**
 * @brief 快速绘图的列角色判定与图型能力矩阵
 *
 * 规则来源：实施方案 §4.1（角色判定）、§4.2（能力矩阵）、D11（X 轴下拉）。
 * 只做纯计算，不碰界面；失败原因以枚举返回，由界面层翻译。
 */
class DAGUI_API DATableChartPlanner
{
public:
    // 判定单列角色：优先走 DATableDataSource::tableColumnType，类型不可判定时取少量样本推断
    static DATableColumnRole detectRole(const DAData& data, int dataColumn, int sampleRowLimit = 50);
    // 组装一个请求需要的列信息（按传入的数据列号顺序）
    static DATableChartColumn makeColumn(const DAData& data, int dataColumn, int occurrence = 0);
    // 按能力矩阵规划可选图型与默认规格
    static DATableChartPlan plan(const DATableChartRequest& request);
    // 给定图型生成默认规格（表格侧/浮窗切图型时使用）
    static DATableChartSpec defaultSpec(const DATableChartRequest& request, DATableChartType type);
    // 当前请求是否支持指定图型
    static bool supports(const DATableChartRequest& request, DATableChartType type);
    // X 轴下拉候选（多列数值/时间场景）；不适用时返回空列表（界面隐藏下拉）
    static QList< DATableChartXOption > xAxisCandidates(const DATableChartRequest& request,
                                                        DATableChartType type);
    // 把 spec 的 X 轴切换到候选项（保留其它字段）；用于界面下拉联动
    static void applyXOption(const DATableChartRequest& request,
                             DATableChartSpec& spec,
                             const DATableChartXOption& option);
    // 分类聚合口径的 Python 估计器名（mean/min/max/...）
    static QString aggregationKey(DATableChartAggregation aggregation);
    // 分类聚合口径的全部取值（界面下拉用）
    static QList< DATableChartAggregation > allAggregations();

private:
    // 低基数数值列的唯一值数（<= limit 时返回该值，未知/超限返回 -1）
    static int probeUniqueCount(const DATableChartRequest& request, int dataColumn, int limit);
};

}  // namespace DA

#endif  // DATABLECHARTPLANNER_H
