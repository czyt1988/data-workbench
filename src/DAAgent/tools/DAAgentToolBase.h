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

    DACoreInterface* core() const { return m_core; }
    QString getOwnerModule() const override { return "DAAgent"; }

protected:
    // 数据管理器接口
    DADataManagerInterface* dataMgr() const
    {
        return m_core ? m_core->getDataManagerInterface() : nullptr;
    }

    // findData 返回 DAData（按值），不是 DAAbstractData*
    DAData findData(const QString& name) const
    {
        auto* mgr = dataMgr();
        return mgr ? mgr->findData(name) : DAData();
    }

    // getAllDatas 返回 QList<DAData>（值列表），遍历时用 const 引用
    QList< DAData > allDatas() const
    {
        auto* mgr = dataMgr();
        return mgr ? mgr->getAllDatas() : QList< DAData >();
    }

    // 图表操作窗口（位于 DAGui 模块），是访问 figure/chart 的入口
    DAChartOperateWidget* chartOperateWidget() const
    {
        if (!m_core) return nullptr;
        auto* ui = m_core->getUiInterface();
        if (!ui) return nullptr;
        auto* dock = ui->getDockingArea();
        if (!dock) return nullptr;
        return dock->getChartOperateWidget();
    }

    // 当前活动 figure，无活动 figure 返回 nullptr
    DAFigureWidget* currentFigure() const
    {
        auto* oper = chartOperateWidget();
        return oper ? oper->getCurrentFigure() : nullptr;
    }

    // 当前活动 chart，无活动 chart 返回 nullptr
    DAChartWidget* currentChart() const
    {
        auto* oper = chartOperateWidget();
        return oper ? oper->getCurrentChart() : nullptr;
    }

    // chart_id 映射：空或 "current" 使用当前活动图表，否则按标题/索引在当前 figure 中查找
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

    // 标准错误响应——工具执行失败时统一返回 {success:false, error:"..."}
    QJsonObject errorResponse(const QString& error) const
    {
        QJsonObject resp;
        resp["success"] = false;
        resp["error"]   = error;
        return resp;
    }

    // 标准成功响应——携带数据载荷
    QJsonObject successResponse(const QJsonObject& data) const
    {
        QJsonObject resp;
        resp["success"] = true;
        resp["data"]    = data;
        return resp;
    }

    // 标准成功响应——仅携带消息
    QJsonObject successResponse(const QString& message) const
    {
        QJsonObject resp;
        resp["success"] = true;
        resp["message"] = message;
        return resp;
    }

protected:
    DACoreInterface* m_core;
};
}  // namespace DA
