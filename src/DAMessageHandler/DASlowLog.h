#ifndef DASLOWLOG_H
#define DASLOWLOG_H
#include "DAMessageHandlerGlobal.h"
// Qt
#include <QString>
// stl
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <utility>

namespace DA
{

class DASlowLogCounter;

/**
 * @brief 慢日志运行期配置与登记中心（全部为线程安全的静态接口）
 *
 * 慢日志用于常态记录超过阈值的操作，输出走 qCWarning(DA_SLOW)（category
 * 为 "daworkbench.slow"），由 DALogger 分流到独立的 slow.log 文件，
 * 不进 UI 消息队列、不需要国际化，内容保持纯英文。
 *
 * 编译期由顶层 CMake 的 DA_ENABLE_SLOW_LOG 宏控制（Debug 自动开启）：
 * 宏关闭时所有 DA_SLOW_LOG_* 埋点宏展开为空，标签字符串与计时代码不生成；
 * 本类自身始终编译进 DAMessageHandler，保证测试与分流逻辑在任何配置下可用。
 *
 * 运行期只认环境变量（启动后解析一次，reloadConfig 仅供测试）：
 * - DA_SLOW_LOG              总开关，off/0/false/no 视为关闭，默认 on
 * - DA_SLOW_LOG_MS           全局默认阈值（毫秒），默认 50
 * - DA_SLOW_LOG_MS_<tag>     按标签覆盖阈值，如 DA_SLOW_LOG_MS_dagui.table.sort.applySpec=200
 * - DA_SLOW_LOG_AGGREGATE_MS 计数器与事件聚合输出周期（毫秒），默认 1000
 * - DA_SLOW_LOG_STALL_MS     看门狗判定主线程卡顿的阈值（毫秒），默认 500
 *
 * @see DASlowLogScope
 * @see DALogger
 */
class DAMESSAGEHANDLER_API DASlowLog
{
public:
    // 作用域位置信息（看门狗读取主线程当前作用域用）
    struct ScopeInfo
    {
        const char* tag  = nullptr;  ///< 标签（字符串字面量，静态生命周期）
        const char* file = nullptr;  ///< __FILE__
        int line         = 0;        ///< __LINE__
        const char* func = nullptr;  ///< Q_FUNC_INFO
    };

    static bool isEnabled();                        ///< 运行期总开关
    static qint64 defaultThresholdMs();             ///< 全局默认阈值（毫秒）
    static qint64 thresholdMs(const char* tag);     ///< 查标签覆盖表，无覆盖返回默认值
    static qint64 minThresholdMs();                 ///< 默认阈值与全部覆盖中的最小值（事件循环快速筛选用）
    static qint64 aggregateIntervalMs();            ///< 聚合输出周期（毫秒）
    static qint64 stallThresholdMs();               ///< 看门狗卡顿阈值（毫秒）

    static DASlowLogCounter* counter(const char* tag);  ///< 计数器注册（线程安全，调用方应缓存返回值）
    static void flushCounters();                        ///< 输出并清零全部计数器（由应用层定时调用）

    static void setMainThreadId(std::thread::id id);    ///< 由 DAApplication 构造时调用一次
    static bool isMainThread();

    static ScopeInfo currentMainThreadScope();      ///< 看门狗读取主线程栈顶作用域（可能为空）

    static bool inEmission();                       ///< 当前线程是否正在慢日志输出路径中（重入保护）
    static void emitScopeRecord(const char* tag,
                                qint64 elapsedNs,
                                qint64 selfNs,
                                int depth,
                                const char* file,
                                int line,
                                const char* func,
                                const QString& detail,
                                const char* threadLabel = nullptr);  ///< 输出一条作用域/单事件记录
    static void emitCounterRecord(const char* tag, quint64 count, quint64 totalNs, quint64 maxNs);   ///< 输出一条计数器聚合记录
    static void emitEventAggregate(const char* eventType, quint64 count, quint64 totalNs, quint64 maxNs);  ///< 输出一条事件聚合记录

    static void reloadConfig();                     ///< 强制重新解析环境变量（仅供单元测试，生产代码不调用）
};

/**
 * @brief RAII 作用域计时器：构造启动计时，析构时超阈值则输出一条慢日志
 *
 * 通过线程局部固定容量栈跟踪嵌套：输出 ms（总耗时）与 self_ms（扣除子作用域
 * 后的自身耗时），depth 为嵌套深度（最外层为 0）。栈满（64 层）后停止压栈，
 * 计时与输出仍继续。仅主线程会把栈顶帧发布给看门狗读取。
 *
 * 一般不直接使用本类，而是使用 DA_SLOW_LOG_SCOPE* 宏（见文件末尾）。
 */
class DAMESSAGEHANDLER_API DASlowLogScope
{
public:
    DASlowLogScope(const char* tag, const char* file, int line, const char* func);
    DASlowLogScope(const char* tag, qint64 thresholdMs, const char* file, int line, const char* func);
    ~DASlowLogScope();

    // 设置惰性详情回调：仅在超阈值输出时调用一次
    template < typename Fn >
    void setDetail(Fn&& fn)
    {
        mDetail = std::forward< Fn >(fn);
    }

    DASlowLogScope(const DASlowLogScope&)            = delete;
    DASlowLogScope& operator=(const DASlowLogScope&) = delete;

private:
    void begin(qint64 thresholdMs);

    const char* mTag;
    const char* mFile;
    const char* mFunc;
    int mLine;
    qint64 mThresholdNs;
    qint64 mStartNs;
    int mFrameIndex;  ///< 所在线程局部栈的帧下标；-1 表示未压栈（关闭或栈满）
    bool mActive;
    std::function< QString() > mDetail;
};

/**
 * @brief 高频路径计数器：原子累加调用次数/总耗时/最大单次耗时，由 flushCounters 周期输出
 */
class DAMESSAGEHANDLER_API DASlowLogCounter
{
    friend class DASlowLog;
public:
    explicit DASlowLogCounter(const char* tag);
    void add(qint64 ns);  ///< 原子累加一次采样

private:
    std::string mTag;
    std::atomic< quint64 > mCount { 0 };
    std::atomic< quint64 > mTotalNs { 0 };
    std::atomic< quint64 > mMaxNs { 0 };
};

/**
 * @brief 计数器作用域计时：析构时把耗时累加进 DASlowLogCounter（运行期关闭时零计时）
 */
class DAMESSAGEHANDLER_API DASlowLogCounterScope
{
public:
    explicit DASlowLogCounterScope(DASlowLogCounter* counter);
    ~DASlowLogCounterScope();

    DASlowLogCounterScope(const DASlowLogCounterScope&)            = delete;
    DASlowLogCounterScope& operator=(const DASlowLogCounterScope&) = delete;

private:
    DASlowLogCounter* mCounter;
    std::chrono::steady_clock::time_point mStart;
};

/**
 * @brief DA_SLOW_LOG_CALL 的实现载体：计时执行 fn 并原样返回其结果
 */
template < typename Fn >
decltype(auto) slowLogCall(const char* tag, const char* file, int line, const char* func, Fn&& fn)
{
    DASlowLogScope scope(tag, file, line, func);
    return std::forward< Fn >(fn)();
}

}  // namespace DA

//===================================================
// 埋点宏
//
// 编译期开关 DA_ENABLE_SLOW_LOG 由顶层 CMakeLists.txt 注入
// （Debug 配置自动开启；-DDA_ENABLE_SLOW_LOG=ON 强制全配置开启）。
// 关闭时全部宏展开为空（DA_SLOW_LOG_CALL 展开为原表达式），
// 标签字符串、计时器与分支都不生成，发布版零残留。
//
// 标签命名统一为 <层>.<模块>.<对象>.<动作>，如 dagui.table.sort.applySpec；
// 标签同时是阈值覆盖键（DA_SLOW_LOG_MS_<tag>），必须保持稳定、不带动态参数。
//===================================================
#if defined(DA_ENABLE_SLOW_LOG)

#define DA_SLOW_LOG_CONCAT_INNER(a, b) a##b
#define DA_SLOW_LOG_CONCAT(a, b) DA_SLOW_LOG_CONCAT_INNER(a, b)

#define DA_SLOW_LOG_SCOPE_IMPL(tag, var) \
    ::DA::DASlowLogScope var(tag, __FILE__, __LINE__, Q_FUNC_INFO)

// 作用域计时，使用全局默认阈值（DA_SLOW_LOG_MS，缺省 50ms）
#define DA_SLOW_LOG_SCOPE(tag) \
    DA_SLOW_LOG_SCOPE_IMPL(tag, DA_SLOW_LOG_CONCAT(da_slow_scope_, __COUNTER__))

// 作用域计时，显式指定阈值（毫秒）
#define DA_SLOW_LOG_SCOPE_MS(tag, ms) \
    ::DA::DASlowLogScope DA_SLOW_LOG_CONCAT(da_slow_scope_, __COUNTER__)( \
        tag, (ms), __FILE__, __LINE__, Q_FUNC_INFO)

#define DA_SLOW_LOG_SCOPE_DETAIL_IMPL(tag, detail, var) \
    ::DA::DASlowLogScope var(tag, __FILE__, __LINE__, Q_FUNC_INFO); \
    var.setDetail(detail)

// 作用域计时 + 惰性详情：detail 必须是可调用对象，仅超阈值时调用一次
#define DA_SLOW_LOG_SCOPE_DETAIL(tag, detail) \
    DA_SLOW_LOG_SCOPE_DETAIL_IMPL(tag, detail, DA_SLOW_LOG_CONCAT(da_slow_scope_, __COUNTER__))

// 测量单个表达式并保留返回值
#define DA_SLOW_LOG_CALL(tag, expr) \
    ::DA::slowLogCall(tag, __FILE__, __LINE__, Q_FUNC_INFO, [&]() -> decltype(auto) { return (expr); })

#define DA_SLOW_LOG_COUNTER_IMPL(tag, var, ptr) \
    static ::DA::DASlowLogCounter* const ptr = ::DA::DASlowLog::counter(#tag); \
    ::DA::DASlowLogCounterScope var(ptr)

// 高频路径计数器聚合。注意：tag 以裸标识符形式书写（不带引号），
// 如 DA_SLOW_LOG_COUNTER(dagui.model.actualData)，宏内部完成字符串化；
// 计数器指针缓存在函数局部静态变量中，后续调用只有一次魔法静态守卫检查
#define DA_SLOW_LOG_COUNTER(tag) \
    DA_SLOW_LOG_COUNTER_IMPL(tag, \
                             DA_SLOW_LOG_CONCAT(da_slow_counter_scope_, __COUNTER__), \
                             DA_SLOW_LOG_CONCAT(da_slow_counter_, __COUNTER__))

#else

#define DA_SLOW_LOG_SCOPE(tag) ((void)0)
#define DA_SLOW_LOG_SCOPE_MS(tag, ms) ((void)0)
#define DA_SLOW_LOG_SCOPE_DETAIL(tag, detail) ((void)0)
#define DA_SLOW_LOG_CALL(tag, expr) (expr)
#define DA_SLOW_LOG_COUNTER(tag) ((void)0)

#endif

#endif  // DASLOWLOG_H
