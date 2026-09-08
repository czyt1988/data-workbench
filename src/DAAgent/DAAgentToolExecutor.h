// DAAgentToolExecutor.h
#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>
#include "DAAgentAPI.h"
#include "DAGlobals.h"

namespace DA
{
class DAAgentBridge;

/**
 * @brief 全局工具执行队列（决策点 2 方案 c，审计问题 12）
 *
 * 并发会话下所有桥的工具调用汇入同一队列，主线程串行出队执行（保持既有
 * 主线程亲和：run_code 需嵌入式解释器 GIL、图表类工具需 GUI 线程）。
 * 相对旧"singleShot(0) 直执行"模型的增强：
 * - 排队状态可见：enqueue 返回队列位置，桥经 tool_exec_queued 协议消息
 *   上报 Python、经 agentToolQueued 信号上报 UI（可解释的等待而非假超时）；
 * - 取消语义：出队/执行前检查桥存活与停止标志（isRunning/isStopRequested），
 *   崩溃或用户 Stop 后已排队未执行的调用被取消——副作用不再为死进程发生
 *   （审计 12b/12c）；
 * - 跨会话限流：全局串行天然满足"run_code 类长任务同一时刻只一个会话执行"。
 *
 * 归属：DAAgentModule 持有单实例（initialize 创建），attachBridge 注入各桥；
 * 桥未注入执行器时退化为直执行（独立使用 Bridge 的场景/协议级测试）。
 * 队列项持 QPointer<DAAgentBridge>——桥对象销毁自动置空，无悬垂指针。
 */
class DAAgent_API DAAgentToolExecutor : public QObject
{
    Q_OBJECT
public:
    // 构造函数
    explicit DAAgentToolExecutor(QObject* parent = nullptr);
    // 析构函数（PIMPL：unique_ptr<PrivateData> 需完整类型处定义）
    virtual ~DAAgentToolExecutor() override;

    // 入队（桥权限门放行后调用）；返回队列位置（1-based，含本条）
    int enqueue(DAAgentBridge* bridge, const QString& callId, const QString& toolName,
                const QJsonObject& args, const QString& subagentId);
    // 当前等待执行的队列长度（不含执行中）
    int queueLength() const;
    // 清空队列（shutdown 用；条目直接丢弃——桥即将停止，存活检查也会取消）
    void clear();

    DA_DECLARE_PRIVATE(DAAgentToolExecutor)
};
} // namespace DA
