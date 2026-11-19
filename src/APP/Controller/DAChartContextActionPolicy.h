#ifndef DACHARTCONTEXTACTIONPOLICY_H
#define DACHARTCONTEXTACTIONPOLICY_H

#include <array>
#include <cstddef>

namespace DA
{
/**
 * @brief 绘图上下文里「当前激活子图类型」的策略输入
 *
 * 与 DAFigureCurrentSubChart 同值但独立定义：本策略是 APP 内部纯函数，只依赖标准库，
 * 便于 APPRefactorTest 直接编译（不引入 DAFigure/qwt 依赖）。
 */
enum class DAChartSubChartKind
{
    None = 0,  ///< 没有可操作的子图（无 figure / figure 里没有子图）
    Chart2D,   ///< 2D 子图
    Chart3D    ///< 3D 子图
};

/**
 * @brief 绘图动作分组：同组动作的可用性一致
 *
 * 分组是「动作语义」而不是「QAction 指针」，策略函数因此不依赖 Qt；由
 * DAAppChartActionHandler 负责把分组映射到具体 QAction/QActionGroup。
 */
enum class DAChartActionGroup
{
    Chart2DInteraction = 0,  ///< 2D 交互：网格/缩放/平移/拾取/图例/应用到所有绘图
    ChartEditorTools,        ///< 图表编辑器工具：指针/选框/辅助标记
    ResizeSubChart,          ///< 调整子图大小（SubChartEditor）
    DataProbe,               ///< 数据探针
    FigureTheme,             ///< 绘图主题、复制绘图到剪贴板（figure 级）
    ExportChart,             ///< 导出当前子图
    AddFigure,               ///< 新建绘图窗口
    NewXYAxis,               ///< 新建坐标系
    AddSeries,               ///< 添加绘图入口：曲线/散点/柱状/统计图/3D 系列
    Count
};

/**
 * @brief 分组动作的可用性
 *
 * 当前没有需要隐藏的动作（所有入口保持可见，只切换可用性）；若后续需要隐藏入口，
 * 在这里扩展 Hidden 并让 apply 侧处理可见性。
 */
enum class DAChartActionState
{
    Enabled = 0,
    Disabled
};

/**
 * @brief 策略输入
 */
struct DAChartContextActionInput
{
    DAChartSubChartKind subChart { DAChartSubChartKind::None };
    bool fromProvider { false };    ///< 当前绘图上下文是否由插件设置上下文提供者接管
    bool gridLayout { false };      ///< 目标 figure 是否处于保留式网格布局
};

/**
 * @brief 策略输出：每个动作分组的可用性
 */
struct DAChartContextActionPlan
{
    std::array< DAChartActionState, static_cast< std::size_t >(DAChartActionGroup::Count) > states;

    DAChartContextActionPlan();
    DAChartActionState stateOf(DAChartActionGroup group) const;
    bool isEnabled(DAChartActionGroup group) const;
};

/**
 * @brief 解析绘图动作可用性计划（无副作用的纯函数）
 * @param input 子图类型 / 来源 / figure 形态
 * @return 各动作分组的可用性
 */
DAChartContextActionPlan resolveChartContextActionPlan(const DAChartContextActionInput& input);

}  // namespace DA
#endif  // DACHARTCONTEXTACTIONPOLICY_H