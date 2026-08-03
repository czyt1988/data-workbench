#pragma once
#include "DAAgentAPI.h"
#include "DAAbstractAgentTool.h"
#include "DACoreInterface.h"
#include "DAData.h"
#include "DADataManagerInterface.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
// 图表访问链路所需头文件，DAChartOperateWidget 位于 DAGui 模块
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartOperateWidget.h"

namespace DA
{
/**
 * @brief 平台内置工具通用基类
 *
 * 提供 DADataManager / DAChartOperateWidget / DAFigureWidget / DAChartWidget 的便捷访问方法，
 * 以及统一的 errorResponse / successResponse 响应构造。
 * 所有平台内置工具（数据类、绘图类、文件/报告类）均继承此类。
 */
class DAAgentToolBase : public QObject, public DAAbstractAgentTool
{
    Q_OBJECT
public:
    DAAgentToolBase(DACoreInterface* core, QObject* parent = nullptr)
        : QObject(parent), DAAbstractAgentTool(), m_core(core) {}

    /// @brief 返回核心接口指针
    DACoreInterface* core() const { return m_core; }
    /// @copydoc DAAbstractAgentTool::getOwnerModule
    QString getOwnerModule() const override { return "DAAgent"; }

protected:
    /// @brief 获取数据管理器接口
    DADataManagerInterface* dataMgr() const
    {
        return m_core ? m_core->getDataManagerInterface() : nullptr;
    }

    /// @brief 按名称查找数据，返回 DAData（按值）
    DAData findData(const QString& name) const
    {
        auto* mgr = dataMgr();
        return mgr ? mgr->findData(name) : DAData();
    }

    /// @brief 获取所有数据的值列表
    QList< DAData > allDatas() const
    {
        auto* mgr = dataMgr();
        return mgr ? mgr->getAllDatas() : QList< DAData >();
    }

    /** @brief 获取图表操作窗口（位于 DAGui 模块），是访问 figure/chart 的入口 */
    DAChartOperateWidget* chartOperateWidget() const
    {
        if (!m_core) return nullptr;
        auto* ui = m_core->getUiInterface();
        if (!ui) return nullptr;
        auto* dock = ui->getDockingArea();
        if (!dock) return nullptr;
        return dock->getChartOperateWidget();
    }

    /// @brief 获取当前活动 figure，无活动 figure 返回 nullptr
    DAFigureWidget* currentFigure() const
    {
        auto* oper = chartOperateWidget();
        return oper ? oper->getCurrentFigure() : nullptr;
    }

    /// @brief 获取当前活动 chart，无活动 chart 返回 nullptr
    DAChartWidget* currentChart() const
    {
        auto* oper = chartOperateWidget();
        return oper ? oper->getCurrentChart() : nullptr;
    }

    /** @brief 按 chart_id 查找图表
     *
     * chart_id 为空或 "current" 时使用当前活动图表，否则按标题或整数索引在当前 figure 中查找。
     */
    DAChartWidget* findChart(const QString& chartId) const
    {
        if (chartId.isEmpty() || chartId == "current") {
            return currentChart();
        }
        auto* fig = currentFigure();
        if (!fig) {
            return nullptr;
        }
        // 优先按标题匹配
        for (DAChartWidget* c : fig->getCharts()) {
            if (c && c->getChartTitle() == chartId) {
                return c;
            }
        }
        // 其次尝试作为整数索引解析
        bool ok = false;
        int idx = chartId.toInt(&ok);
        if (ok) {
            QList< DAChartWidget* > charts = fig->getCharts();
            if (idx >= 0 && idx < charts.size()) {
                return charts[ idx ];
            }
        }
        return nullptr;
    }

    /// @brief 构造标准错误响应 {success:false, error:"..."}
    QJsonObject errorResponse(const QString& error) const
    {
        QJsonObject resp;
        resp["success"] = false;
        resp["error"]   = error;
        return resp;
    }

    /// @brief 构造标准成功响应，携带数据载荷
    QJsonObject successResponse(const QJsonObject& data) const
    {
        QJsonObject resp;
        resp["success"] = true;
        resp["data"]    = data;
        return resp;
    }

    /// @brief 构造标准成功响应，仅携带消息
    QJsonObject successResponse(const QString& message) const
    {
        QJsonObject resp;
        resp["success"] = true;
        resp["message"] = message;
        return resp;
    }

protected:
    /// @brief 核心接口指针
    DACoreInterface* m_core;
};
}  // namespace DA
