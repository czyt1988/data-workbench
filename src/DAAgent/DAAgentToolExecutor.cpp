// DAAgentToolExecutor.cpp
#include "DAAgentToolExecutor.h"
#include "DAAgentBridge.h"
#include <QQueue>
#include <QPointer>
#include <QTimer>

namespace DA
{

// ===========================================================================
// PrivateData
// ===========================================================================
class DAAgentToolExecutor::PrivateData
{
    DA_DECLARE_PUBLIC(DAAgentToolExecutor)
public:
    explicit PrivateData(DAAgentToolExecutor* p);

    /// 队列项：桥用 QPointer——shutdown/崩溃退役 deleteLater 后自动置空，
    /// 出队存活检查据此取消，无悬垂指针
    struct Entry {
        QPointer<DAAgentBridge> bridge;
        QString callId;
        QString toolName;
        QJsonObject args;
        QString subagentId;
    };

    QQueue<Entry> mQueue;
    bool mPumpScheduled = false;  ///< 已挂 singleShot(0) 泵调度
    bool mExecuting = false;      ///< 重入防护（工具同步执行期间不再开泵）
};

DAAgentToolExecutor::PrivateData::PrivateData(DAAgentToolExecutor* p) : q_ptr(p)
{
}

// ===========================================================================
// ctor
// ===========================================================================

/**
 * @brief 构造函数
 * @param parent 父对象（Module 持有）
 */
DAAgentToolExecutor::DAAgentToolExecutor(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数（此处 PrivateData 为完整类型，unique_ptr 可析构）
 */
DAAgentToolExecutor::~DAAgentToolExecutor() = default;

// ===========================================================================
// 公共方法
// ===========================================================================

/**
 * @brief 入队工具调用（桥权限门放行后调用）
 * @param bridge 发起调用的桥（执行与结果回传的宿主）
 * @param callId 工具调用 ID（tool_result 回传配对）
 * @param toolName 工具名
 * @param args 工具参数
 * @param subagentId 子 agent 任务 id（主 agent 调用为空）
 * @return 队列位置（1-based，含本条；供 tool_exec_queued/UI 排队态上报）
 */
int DAAgentToolExecutor::enqueue(DAAgentBridge* bridge, const QString& callId, const QString& toolName,
                                 const QJsonObject& args, const QString& subagentId)
{
    DA_D(d);
    if (!bridge) {
        return 0;
    }
    PrivateData::Entry e;
    e.bridge     = bridge;
    e.callId     = callId;
    e.toolName   = toolName;
    e.args       = args;
    e.subagentId = subagentId;
    d->mQueue.enqueue(e);
    const int position = d->mQueue.size();
    // 泵经 singleShot(0) 调度：让当前 stdout 读取回调尽快返回（同旧
    // executeTool 投递语义），批内多条 tool_call 全部入队后统一串行执行
    if (!d->mPumpScheduled && !d->mExecuting) {
        d->mPumpScheduled = true;
        QTimer::singleShot(0, this, [this]() {
            DA_D(d);
            d->mPumpScheduled = false;
            // 串行排空：工具同步执行（主线程亲和），执行期间新入队的条目
            // 由本循环继续消费；mExecuting 防工具内部 processEvents 重入开泵
            while (!d->mQueue.isEmpty()) {
                const PrivateData::Entry entry = d->mQueue.dequeue();
                // 存活/停止检查（审计 12b/12c）：桥死亡（崩溃/退役）或用户
                // Stop 后不再执行已排队调用——副作用不为死进程发生，
                // Python 侧等待槽已由退出/停止路径统一唤醒，无需回传结果
                if (!entry.bridge || !entry.bridge->isRunning() || entry.bridge->isStopRequested()) {
                    qInfo() << "DAAgentToolExecutor: queued tool call cancelled (bridge dead or stopping), tool="
                            << entry.toolName << "callId=" << entry.callId;
                    continue;
                }
                d->mExecuting = true;
                entry.bridge->runQueuedTool(entry.callId, entry.toolName, entry.args, entry.subagentId);
                d->mExecuting = false;
            }
        });
    }
    return position;
}

/**
 * @brief 当前等待执行的队列长度（不含执行中）
 * @return 队列长度
 */
int DAAgentToolExecutor::queueLength() const
{
    DA_DC(d);
    return d->mQueue.size();
}

/**
 * @brief 清空队列（shutdown 用）
 */
void DAAgentToolExecutor::clear()
{
    DA_D(d);
    d->mQueue.clear();
}

} // namespace DA
