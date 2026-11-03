#ifndef DACHARTQUICKPLOT_H
#define DACHARTQUICKPLOT_H
#include "DAGuiAPI.h"
#include "Table/Chart/DATableChartTypes.h"
#include <QList>
#include <QString>

class QwtPlotItem;

namespace DA
{
class DAChartWidget;
class DAChartOperateWidget;

/**
 * @brief 按规格构建图元并挂到图表的唯一编排器
 *
 * 浮窗预览与「转入绘图区」共用同一份「请求 + 规格 + 采样 + 图元构建」：
 * 浮窗直接渲染到自己的 DAChartWidget；转入时用同一份规格重新构建图元，
 * 挂到正式 chart。这样不存在第二套建图逻辑，也不会把浮窗的 chart 直接 reparent。
 *
 * 数据侧约定：
 * - 折线/散点/直方图按 DAChartSeriesSampler 采样；
 * - 分类聚合/箱线图能精确就精确（内存源阈值内走全量），否则采样并在
 *   DATableChartBuildInfo 标注近似；
 * - 调用方必须持有 Python GIL（与统计向导的约定一致）。
 */
class DAGUI_API DAChartQuickPlot
{
public:
    /**
     * @brief 把规格渲染到指定图表
     * @param request 表格侧构造的请求
     * @param spec 绘图规格
     * @param chart 目标图表（浮窗或正式图表）
     * @param info 输出：采样/缺失值/Top N 等附加信息
     * @param error 输出：失败原因（已翻译，可直接展示）
     * @param clearExisting true 时先清空图表现有图元（浮窗重画）；追加到当前图表时传 false
     * @param createdItems 输出：本次新建并已 attach 的图元（供转入时登记撤销命令）
     * @return 成功返回 true
     */
    static bool render(const DATableChartRequest& request,
                       const DATableChartSpec& spec,
                       DAChartWidget* chart,
                       DATableChartBuildInfo* info = nullptr,
                       QString* error = nullptr,
                       bool clearExisting = true,
                       QList< QwtPlotItem* >* createdItems = nullptr);

    /**
     * @brief 转入绘图区
     *
     * appendToCurrent=false：新建 Figure 标签页（默认），「创建 figure + 建 chart + 挂图元」
     * 打包成新图自己的撤销宏，一次 Ctrl+Z 撤销整个转入；
     * appendToCurrent=true：把图元追加进当前 chart（同样登记撤销命令）。
     * @param chartOperate 绘图区容器
     * @return 承载图表的 DAChartWidget，失败返回 nullptr
     */
    static DAChartWidget* transferToFigure(const DATableChartRequest& request,
                                           const DATableChartSpec& spec,
                                           DAChartOperateWidget* chartOperate,
                                           bool appendToCurrent,
                                           DATableChartBuildInfo* info = nullptr,
                                           QString* error = nullptr);
};

}  // namespace DA

#endif  // DACHARTQUICKPLOT_H
