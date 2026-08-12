#pragma once
#include "DAAgentAPI.h"
#include "DAAbstractAgentTool.h"
#include "DACoreInterface.h"
#include "DAData.h"
#include "DADataManagerInterface.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

namespace DA
{
/**
 * @brief 平台内置工具通用基类（瘦身版，plan-04）
 *
 * 仅提供 DADataManager 的便捷访问方法，以及统一的 errorResponse /
 * successResponse 响应构造。数据类工具继承本类即可获得数据访问能力，
 * 不依赖 DAGui。图表访问方法（chartOperateWidget / currentFigure 等 7 个）
 * 已搬迁至插件 DAAgentChartToolBase（plugins/DAAgentTools），绘图类工具
 * 继承 DAAgentChartToolBase 以获得图表访问能力。
 */
class DAAgent_API DAAgentToolBase : public QObject, public DAAbstractAgentTool
{
    Q_OBJECT
public:
    DAAgentToolBase(DACoreInterface* core, QObject* parent = nullptr)
        : QObject(parent), DAAbstractAgentTool(), mCore(core) {}

    // 返回核心接口指针
    DACoreInterface* core() const { return mCore; }
    /// @copydoc DAAbstractAgentTool::getOwnerModule
    QString getOwnerModule() const override { return "DAAgent"; }

protected:
    // 获取数据管理器接口
    DADataManagerInterface* dataMgr() const
    {
        return mCore ? mCore->getDataManagerInterface() : nullptr;
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
    ///< 核心接口指针
    DACoreInterface* mCore;
};
}  // namespace DA
