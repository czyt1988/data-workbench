// DAAgentPermissionManager.h
#pragma once
#include "DAAgentAPI.h"
#include "DAAgentPermissionRule.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QJsonObject>
#include "DAGlobals.h"

namespace DA
{
/**
 * @brief Agent 权限引擎：模式决策 / 工具分级 / 路径策略 / 会话记忆（母文档 §6.1）
 *
 * 非 QObject（镜像 DAAgentSessionStore 的 PIMPL 风格，无信号槽需求）。
 * 单一执法点：DAAgentBridge::executeTool 调用 decide() 产出 Allow/Deny/Ask，
 * 策略状态（模式/规则/变量）由此处统一维护，配置经 agent-permissions.json
 *（规则/危险模式/分级覆盖，原子写）与 agent-config.ini（模式/超时/判官/开关）持久化。
 *
 * 三模式语义（母文档 §1）：
 *   - yolo：除硬 deny（系统目录，全模式生效）外全部放行
 *   - auto：read/inapp_mutate 放行；file_write 走路径策略；code_exec 走
 *     safety 裁决（判官未配置时 allow/uncertain/缺失一律降级 ask，D1 [v2.1]）
 *   - manual：file_write/code_exec 每次 ask；inapp_mutate 默认放行
 *     （manual_block_inapp_tools=true 时 ask）
 * unknown 分级（A3 [v2.1]）：auto/manual 一律 ask，yolo 放行。
 */
class DAAgent_API DAAgentPermissionManager
{
public:
    /// 决策结果动作
    enum Action {
        Allow,  ///< 放行执行
        Deny,   ///< 拒绝（合成 deny 结果回传）
        Ask     ///< 弹审批卡（HITL）
    };

    /// decide() 的完整决策结果
    struct Decision {
        Action action = Ask;
        QString tier;    ///< 工具分级名（read/inapp_mutate/file_write/code_exec/unknown）
        QString reason;  ///< deny 时的完整 error 文本（直接进工具结果）
    };

    // 工具分级名常量
    static QString tierRead();         ///< "read"
    static QString tierInappMutate();  ///< "inapp_mutate"
    static QString tierFileWrite();    ///< "file_write"
    static QString tierCodeExec();     ///< "code_exec"
    static QString tierUnknown();      ///< "unknown"

    // 模式名常量
    static QString modeYolo();    ///< "yolo"
    static QString modeAuto();    ///< "auto"
    static QString modeManual();  ///< "manual"

    // code_exec deny 固定脱敏文本（不向 LLM 泄露规则细节）
    static QString codeDenyMessage();
    // 系统目录硬 deny 文本（与原 isPathSafe 工具侧文案一致，保基线）
    static QString systemPathDenyMessage();

    DAAgentPermissionManager();
    ~DAAgentPermissionManager();

    // ---- 持久化（agent-permissions.json，原子写 tmp+rename） ----
    // 配置文件绝对路径（与 agent-config.ini 同目录）
    QString configFilePath() const;
    // 读取配置；文件缺失时播种（ensureDefaultRules）并落盘；存在时解析 + 硬 deny 强制回填
    bool load();
    // 落盘当前规则/危险模式/分级覆盖
    bool save() const;
    // 播种默认规则（§9.1：4 条 ${workspace}/** allow + 4 条系统目录硬 deny + 危险模式种子）
    void ensureDefaultRules();

    // ---- 规则/危险模式/分级覆盖 ----
    // 当前规则列表（顺序敏感，首条命中即返回）
    QList< DAAgentPermissionRule > rules() const;
    // 整体替换规则（load 回填与测试用）
    void setRules(const QList< DAAgentPermissionRule >& rules);
    // 代码危险模式 {deny:[...], escalate:[...]}（Python 侧计划二消费）
    QJsonObject codePatterns() const;
    // 设置代码危险模式
    void setCodePatterns(const QJsonObject& patterns);
    // 分级覆盖表 {tool: tier}
    QJsonObject tierOverrides() const;
    // 设置分级覆盖表
    void setTierOverrides(const QJsonObject& overrides);

    // ---- 模式（agent-config.ini agent/permission_mode） ----
    // 当前模式（非法值回退 auto）
    QString mode() const;
    // 写入模式（仅接受 yolo/auto/manual）
    void setMode(const QString& mode);

    // ---- 工具分级（内置表 → 参数约定回退 → unknown，tier_overrides 最高优先） ----
    // 返回工具的分级名
    QString tierOf(const QString& tool, const QJsonObject& params) const;
    // gated_tools：文件写入 + 代码执行全量（Python 长超时清单，A9）
    static QStringList gatedTools();

    // ---- 核心决策（母文档 §4 矩阵 [v2.1]） ----
    // 按模式×分级×安全裁决产出 Allow/Deny/Ask
    Decision decide(const QString& tool, const QJsonObject& params, const QJsonObject& safety) const;
    // 路径策略评估：按规则顺序首条命中返回其动作，无命中返回 Ask
    Action evaluatePath(const QString& tool, const QString& normalizedAbsPath, QString* reason = nullptr) const;
    // 提取并规范化工具参数中的路径（file_path 优先，回退 path/output_path/report_path；
    // run_script 相对路径按工作区根解析）；无路径参数返回空串
    QString resolveToolPath(const QString& tool, const QJsonObject& params) const;

    // ---- 会话记忆（A5 [v2.1]：仅 file_write，code_exec 永不记忆） ----
    // 记录批准的路径前缀（scopeKey 为规范化目录前缀）
    void rememberSession(const QString& tool, const QString& scopeKey);
    // 判断路径是否命中本会话已批准的前缀
    bool isRemembered(const QString& tool, const QString& normalizedAbsPath) const;
    // 清空会话记忆（切换会话/进程退出/崩溃恢复）
    void clearSessionMemory();
    // 从工具参数推导记忆前缀（规范化父目录）；无路径返回空串
    QString sessionScopeKey(const QString& tool, const QJsonObject& params) const;

    // ---- 变量解析（${workspace}/${project}/${data}/${exe}/${home}） ----
    // 设置脚本工作区根目录（DAAppProject::getScriptWorkspaceDir 注入）
    void setWorkspaceRoot(const QString& dir);
    // 当前脚本工作区根目录（规范化，未注入为空）
    QString workspaceRoot() const;
    // 设置工程文件所在目录（${project} 变量）
    void setProjectDir(const QString& dir);
    // 当前工程目录（规范化，无工程为空）
    QString projectDir() const;
    // 全部变量当前值（匹配时解析 ${var} 用）
    QHash< QString, QString > variables() const;

    // ---- ini 派生配置 ----
    // 判官是否已配置（agent/judge_model 非空；D1 兜底判据）
    bool judgeConfigured() const;
    // 判官模型名（可空）
    QString judgeModel() const;
    // 判官超时秒（默认 30）
    int judgeTimeoutSec() const;
    // 审批超时秒（gated_tools 长超时，默认 600）
    int toolApprovalTimeoutSec() const;
    // manual 模式是否拦截应用内修改工具（默认 false）
    bool manualBlockInappTools() const;

    // ---- 设置页 round-trip ----
    // 汇总全部权限配置（模式/超时/开关/判官/规则/危险模式/分级覆盖）
    QJsonObject getConfig() const;
    // 按 contains 守卫写入（镜像 setLLMConfig 风格），含规则落盘
    void setConfig(const QJsonObject& config);

private:
    DA_DECLARE_PRIVATE(DAAgentPermissionManager)
};
} // namespace DA
