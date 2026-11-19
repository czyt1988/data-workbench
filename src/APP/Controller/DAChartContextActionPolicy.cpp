#include "DAChartContextActionPolicy.h"

namespace DA
{
namespace
{
constexpr std::size_t kActionGroupCount = static_cast< std::size_t >(DAChartActionGroup::Count);

void setState(DAChartContextActionPlan& plan, DAChartActionGroup group, DAChartActionState state)
{
    plan.states[ static_cast< std::size_t >(group) ] = state;
}

DAChartActionState availability(bool enabled)
{
    return enabled ? DAChartActionState::Enabled : DAChartActionState::Disabled;
}
}  // namespace

DAChartContextActionPlan::DAChartContextActionPlan()
{
    states.fill(DAChartActionState::Enabled);
}

DAChartActionState DAChartContextActionPlan::stateOf(DAChartActionGroup group) const
{
    const std::size_t index = static_cast< std::size_t >(group);
    if (index >= kActionGroupCount) {
        return DAChartActionState::Disabled;
    }
    return states[ index ];
}

bool DAChartContextActionPlan::isEnabled(DAChartActionGroup group) const
{
    return DAChartActionState::Enabled == stateOf(group);
}

/**
 * @brief 解析绘图动作可用性计划
 *
 * 判定口径（越靠前的维度约束越强）：
 * 1. 子图类型：只有 2D 子图激活时 2D 交互与图表编辑器工具可用；3D/None 一律置灰。
 * 2. figure 级动作（主题/复制/导出）2D/3D 都可用，但需要一个激活子图（None 时置灰）。
 * 3. 来源：插件 provider 接管期间，结构性动作（新建绘图/坐标系/调整子图大小/数据探针）
 *    与主「绘图」标签页的添加绘图入口一律置灰——它们会作用到宿主绘图区或与插件 figure
 *    的布局/生命周期不兼容。
 * 4. figure 形态：网格布局下「调整子图大小」「新建坐标系」不可用（几何由网格轨道决定）。
 * @param input 子图类型 / 来源 / figure 形态
 * @return 各动作分组的可用性计划
 */
DAChartContextActionPlan resolveChartContextActionPlan(const DAChartContextActionInput& input)
{
    DAChartContextActionPlan plan;
    const bool is2D      = (DAChartSubChartKind::Chart2D == input.subChart);
    const bool hasFigure = (DAChartSubChartKind::None != input.subChart);

    // 1. 2D 子图交互与图表编辑器工具
    setState(plan, DAChartActionGroup::Chart2DInteraction, availability(is2D));
    setState(plan, DAChartActionGroup::ChartEditorTools, availability(is2D));

    // 2. figure 级动作：有激活子图即可用（3D 导出走位图路径）
    setState(plan, DAChartActionGroup::FigureTheme, availability(hasFigure));
    setState(plan, DAChartActionGroup::ExportChart, availability(hasFigure));

    // 3. 结构性动作：provider 接管期间一律不可用
    setState(plan, DAChartActionGroup::AddFigure, availability(!input.fromProvider));
    setState(plan, DAChartActionGroup::AddSeries, availability(!input.fromProvider));

    // 4. 子图几何相关：网格布局与 provider 接管期间不可用
    const bool geometryEditable = is2D && !input.gridLayout && !input.fromProvider;
    setState(plan, DAChartActionGroup::ResizeSubChart, availability(geometryEditable));
    setState(plan, DAChartActionGroup::NewXYAxis, availability(!input.gridLayout && !input.fromProvider));
    setState(plan, DAChartActionGroup::DataProbe, availability(is2D && !input.fromProvider));
    return plan;
}

}  // namespace DA