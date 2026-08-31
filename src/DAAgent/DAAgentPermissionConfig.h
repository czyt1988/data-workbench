// DAAgentPermissionConfig.h
#pragma once
#include "DAAgentAPI.h"
#include "DAAgentPermissionRule.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <optional>

namespace DA
{
/**
 * @brief 代码危险模式清单（code_patterns 的结构化形态）
 *
 * deny：静态规则直接拒绝（无需判官）；escalate：交判官裁决的升级模式。
 * 每条为一个正则表达式字符串（Python 侧 re 语法）。
 */
struct DAAgent_API DAAgentCodePatterns
{
    QStringList deny;      ///< 直接拒绝的正则清单
    QStringList escalate;  ///< 升级判官裁决的正则清单

    // JSON 投影（agent-permissions.json 与 init 协议共用；镜像 DAAgentPermissionRule::toJson）
    QJsonObject toJson() const
    {
        QJsonObject o;
        o["deny"]     = QJsonArray::fromStringList(deny);
        o["escalate"] = QJsonArray::fromStringList(escalate);
        return o;
    }
    static DAAgentCodePatterns fromJson(const QJsonObject& o)
    {
        DAAgentCodePatterns p;
        const QJsonArray denyArr = o.value("deny").toArray();
        for (const QJsonValue& v : denyArr) {
            if (v.isString())
                p.deny.append(v.toString());
        }
        const QJsonArray escArr = o.value("escalate").toArray();
        for (const QJsonValue& v : escArr) {
            if (v.isString())
                p.escalate.append(v.toString());
        }
        return p;
    }

    bool operator==(const DAAgentCodePatterns& other) const
    {
        return deny == other.deny && escalate == other.escalate;
    }
    bool operator!=(const DAAgentCodePatterns& other) const { return !(*this == other); }
};

/**
 * @brief 权限配置 DTO（getPermissionConfig/setPermissionConfig 的载荷）
 *
 * 原 QJsonObject 键集的结构体化：5 个标量为 std::optional 稀疏字段
 *（engaged = 显式设置，对应原 contains 守卫语义——未携带的标量不受
 * setPermissionConfig 影响）；rules/codePatterns/tierOverrides 为值类型
 * 部分，setPermissionConfig 时整体替换（原 JSON 协议中三者也总是全量
 * round-trip）。标量持久化于 agent-config.json 的 permission 分组，
 * 值类型部分持久化于 agent-permissions.json（由 DAAgentPermissionManager 管理）。
 */
class DAAgent_API DAAgentPermissionConfig
{
public:
    // ---- 模式（yolo/auto/manual，未设置默认 yolo） ----
    QString mode() const { return mMode.value_or(QStringLiteral("yolo")); }
    bool modeSet() const { return mMode.has_value(); }
    void setMode(const QString& v) { mMode = v; }
    // ---- gated_tools 批准后执行超时（默认 600s） ----
    int toolApprovalTimeoutSec() const { return mToolApprovalTimeoutSec.value_or(600); }
    bool toolApprovalTimeoutSecSet() const { return mToolApprovalTimeoutSec.has_value(); }
    void setToolApprovalTimeoutSec(int v) { mToolApprovalTimeoutSec = v; }
    // ---- 判官模型（空=未配置，auto 模式 code_exec 走判官裁决） ----
    QString judgeModel() const { return mJudgeModel.value_or(QString()); }
    bool judgeModelSet() const { return mJudgeModel.has_value(); }
    void setJudgeModel(const QString& v) { mJudgeModel = v; }
    // ---- 判官单次调用超时（默认 30s） ----
    int judgeTimeoutSec() const { return mJudgeTimeoutSec.value_or(30); }
    bool judgeTimeoutSecSet() const { return mJudgeTimeoutSec.has_value(); }
    void setJudgeTimeoutSec(int v) { mJudgeTimeoutSec = v; }
    // ---- manual 模式是否拦截应用内修改工具（默认 false） ----
    bool manualBlockInappTools() const { return mManualBlockInappTools.value_or(false); }
    bool manualBlockInappToolsSet() const { return mManualBlockInappTools.has_value(); }
    void setManualBlockInappTools(bool v) { mManualBlockInappTools = v; }

    // ---- 值类型部分（非稀疏字段本体 + engage 标志：未 engage 时 setPermissionConfig 不动该项，
    //      保持原 JSON contains 守卫语义；设置页总是全量携带三项） ----
    // 路径规则（顺序敏感，首条命中即返回）
    QList< DAAgentPermissionRule > rules() const { return mRules; }
    void setRules(const QList< DAAgentPermissionRule >& v)
    {
        mRules    = v;
        mRulesSet = true;
    }
    bool rulesSet() const { return mRulesSet; }
    // 代码危险模式
    DAAgentCodePatterns codePatterns() const { return mCodePatterns; }
    void setCodePatterns(const DAAgentCodePatterns& v)
    {
        mCodePatterns    = v;
        mCodePatternsSet = true;
    }
    bool codePatternsSet() const { return mCodePatternsSet; }
    // 工具分级覆盖表 {tool: tier}
    QHash< QString, QString > tierOverrides() const { return mTierOverrides; }
    void setTierOverrides(const QHash< QString, QString >& v)
    {
        mTierOverrides    = v;
        mTierOverridesSet = true;
    }
    bool tierOverridesSet() const { return mTierOverridesSet; }

private:
    std::optional< QString > mMode;
    std::optional< int > mToolApprovalTimeoutSec;
    std::optional< QString > mJudgeModel;
    std::optional< int > mJudgeTimeoutSec;
    std::optional< bool > mManualBlockInappTools;
    QList< DAAgentPermissionRule > mRules;
    bool mRulesSet = false;
    DAAgentCodePatterns mCodePatterns;
    bool mCodePatternsSet = false;
    QHash< QString, QString > mTierOverrides;
    bool mTierOverridesSet = false;
};
}  // namespace DA
