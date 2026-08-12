#pragma once
#include "DAAgentToolBase.h"   // 瘦身后的基类（plan-04）
#include <QObject>
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartOperateWidget.h"

namespace DA
{
/**
 * @brief 图表工具基类（plan-04 接收 DAAgentToolBase 搬出的图表方法）
 *
 * 继承瘦身后的 DAAgentToolBase（数据 + 响应方法），并补全 7 个图表访问
 * 方法（chartOperateWidget / currentFigure / currentChart / findFigureByName
 * / createFigure / findChart / enableAutoScale），实现体逐字照搬自旧
 * DAAgentToolBase.h，依赖 mCore->getUiInterface()->getDockingArea()->
 * getChartOperateWidget() 链路。本类无导出宏，方法为 protected 非虚，
 * 编译进插件 DLL，8 个图表工具（同 DLL）可见即可调用。
 */
class DAAgentChartToolBase : public DAAgentToolBase
{
    Q_OBJECT
public:
    using DAAgentToolBase::DAAgentToolBase;  // inherit constructor

protected:
    /// @brief 获取图表操作窗口（位于 DAGui 模块），是访问 figure/chart 的入口
    DAChartOperateWidget* chartOperateWidget() const;
    /// @brief 获取当前活动 figure，无活动 figure 返回 nullptr
    DAFigureWidget* currentFigure() const;
    /// @brief 获取当前活动 chart，无活动 chart 返回 nullptr
    DAChartWidget* currentChart() const;
    /// @brief 按名称查找 figure（按标签页 tab text 匹配，未找到或名称空返回 nullptr）
    DAFigureWidget* findFigureByName(const QString& name) const;
    /// @brief 创建新 figure 并设置为当前活动 figure
    DAFigureWidget* createFigure(const QString& name) const;
    /// @brief 按 chart_id 查找图表（空或 "current" 用当前活动图表，否则按标题/整数索引）
    DAChartWidget* findChart(const QString& chartId, const QString& figureName = QString()) const;
    /// @brief 重新启用图表坐标轴自动缩放，确保数据可见（撤销 createChart 的 setAxisScale 锁定）
    void enableAutoScale(DAChartWidget* chart) const;
};
}  // namespace DA
