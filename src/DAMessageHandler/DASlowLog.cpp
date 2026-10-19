#include "DASlowLog.h"
#include "DALogCategory.h"
// Qt
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QThread>
// stl
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace DA
{

namespace
{

//===================================================
// 运行期配置（环境变量解析一次，只读缓存；reloadConfig 仅供测试）
//===================================================

struct DASlowLogConfig
{
    bool enabled            = true;
    qint64 defaultThresholdMs = 50;
    qint64 minThresholdMs     = 50;
    qint64 aggregateIntervalMs = 1000;
    qint64 stallThresholdMs    = 500;
    // 透明比较器：查询时用 string_view，不分配内存
    std::map< std::string, qint64, std::less<> > tagOverrides;
};

std::atomic< const DASlowLogConfig* > sConfig { nullptr };  // 指向的对象故意泄漏，避免退出期析构竞争
std::mutex sConfigMutex;

qint64 envToThresholdMs(const char* name, qint64 defaultValue)
{
    const QByteArray v = qgetenv(name);
    if (v.isEmpty()) {
        return defaultValue;
    }
    bool ok  = false;
    qint64 n = v.toLongLong(&ok);
    // 非法值（非数字、负数）回退默认值
    if (!ok || n < 0) {
        return defaultValue;
    }
    return n;
}

DASlowLogConfig* parseConfigFromEnv()
{
    DASlowLogConfig* cfg  = new DASlowLogConfig();
    const QByteArray sw   = qgetenv("DA_SLOW_LOG");
    if (!sw.isEmpty()) {
        const QByteArray lower = sw.toLower();
        cfg->enabled           = !(lower == "off" || lower == "0" || lower == "false" || lower == "no");
    }
    cfg->defaultThresholdMs  = envToThresholdMs("DA_SLOW_LOG_MS", 50);
    cfg->aggregateIntervalMs = envToThresholdMs("DA_SLOW_LOG_AGGREGATE_MS", 1000);
    cfg->stallThresholdMs    = envToThresholdMs("DA_SLOW_LOG_STALL_MS", 500);
    // 按标签覆盖：扫描 DA_SLOW_LOG_MS_<tag>
    const QString prefix = QStringLiteral("DA_SLOW_LOG_MS_");
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QStringList keys        = env.keys();
    for (const QString& key : keys) {
        if (key.startsWith(prefix) && key.size() > prefix.size()) {
            bool ok  = false;
            qint64 n = env.value(key).toLongLong(&ok);
            if (ok && n >= 0) {
                cfg->tagOverrides.emplace(key.mid(prefix.size()).toStdString(), n);
            }
        }
    }
    cfg->minThresholdMs = cfg->defaultThresholdMs;
    for (const auto& kv : cfg->tagOverrides) {
        cfg->minThresholdMs = qMin(cfg->minThresholdMs, kv.second);
    }
    return cfg;
}

const DASlowLogConfig* config()
{
    const DASlowLogConfig* c = sConfig.load(std::memory_order_acquire);
    if (!c) {
        std::lock_guard< std::mutex > lk(sConfigMutex);
        c = sConfig.load(std::memory_order_relaxed);
        if (!c) {
            c = parseConfigFromEnv();
            sConfig.store(c, std::memory_order_release);
        }
    }
    return c;
}

//===================================================
// 线程局部作用域栈（固定容量，不做动态扩容）
//===================================================

constexpr int kMaxScopeDepth = 64;

struct ScopeFrame
{
    const char* tag    = nullptr;
    const char* file   = nullptr;
    int line           = 0;
    const char* func   = nullptr;
    qint64 thresholdNs = 0;
    qint64 startNs     = 0;
    qint64 childNs     = 0;  ///< 子作用域累计耗时（纳秒）
};

struct ScopeStack
{
    int depth = 0;
    ScopeFrame frames[kMaxScopeDepth];
};

ScopeStack& tlsScopeStack()
{
    static thread_local ScopeStack sStack;
    return sStack;
}

// 主线程栈顶帧发布给看门狗；帧位于线程局部固定数组中，主线程阻塞期间不会被销毁，
// 帧内只有字符串字面量与整型，无悬垂风险
std::atomic< const ScopeFrame* > sMainThreadFrame { nullptr };

std::atomic< bool > sMainThreadIdSet { false };
std::thread::id sMainThreadId;

// 输出路径重入守卫：日志系统内部再触发日志时直接跳过
thread_local bool tlsInEmission = false;

qint64 nowNs()
{
    return std::chrono::duration_cast< std::chrono::nanoseconds >(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

qint64 nsToMs(qint64 ns)
{
    return (ns + 500000) / 1000000;
}

// __FILE__ 截成文件名
const char* baseFileName(const char* path)
{
    if (!path) {
        return "-";
    }
    const char* base = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

// Q_FUNC_INFO 精简成 命名空间::类::函数 形式，便于 grep（无 '(' 时原样返回）
QString cleanedFuncName(const char* func)
{
    if (!func || !*func) {
        return QStringLiteral("-");
    }
    const QByteArray s(func);
    int paren = s.indexOf('(');
    QByteArray head = (paren > 0) ? s.left(paren) : s;
    int space = head.lastIndexOf(' ');
    if (space < 0) {
        space = head.lastIndexOf('*');  // 返回类型带指针（如 int* Foo::bar）
    }
    return QString::fromUtf8((space >= 0) ? head.mid(space + 1) : head);
}

QString currentThreadLabel()
{
    if (DASlowLog::isMainThread()) {
        return QStringLiteral("main");
    }
    return QString::number(reinterpret_cast< quintptr >(QThread::currentThreadId()), 16);
}

// 统一出口：守卫 + qCWarning(DA_SLOW)，由 DALogger 分流到 slow.log
void emitLine(const QString& line)
{
    if (tlsInEmission) {
        return;
    }
    tlsInEmission = true;
    qCWarning(DA_SLOW).noquote() << line;
    tlsInEmission = false;
}

//===================================================
// 计数器登记（注册表只增不减，故意泄漏避免退出期竞争）
//===================================================

std::mutex& counterRegistryMutex()
{
    static std::mutex sMutex;
    return sMutex;
}

std::vector< std::unique_ptr< DASlowLogCounter > >& counterRegistry()
{
    static auto& sRegistry = *new std::vector< std::unique_ptr< DASlowLogCounter > >();
    return sRegistry;
}

}  // namespace

//===================================================
// DASlowLog
//===================================================

bool DASlowLog::isEnabled()
{
    return config()->enabled;
}

qint64 DASlowLog::defaultThresholdMs()
{
    return config()->defaultThresholdMs;
}

qint64 DASlowLog::thresholdMs(const char* tag)
{
    const DASlowLogConfig* c = config();
    if (tag && *tag) {
        auto it = c->tagOverrides.find(std::string_view(tag));
        if (it != c->tagOverrides.end()) {
            return it->second;
        }
    }
    return c->defaultThresholdMs;
}

qint64 DASlowLog::minThresholdMs()
{
    return config()->minThresholdMs;
}

qint64 DASlowLog::aggregateIntervalMs()
{
    return config()->aggregateIntervalMs;
}

qint64 DASlowLog::stallThresholdMs()
{
    return config()->stallThresholdMs;
}

DASlowLogCounter* DASlowLog::counter(const char* tag)
{
    if (!tag || !*tag) {
        return nullptr;
    }
    std::lock_guard< std::mutex > lk(counterRegistryMutex());
    auto& registry = counterRegistry();
    for (const auto& up : registry) {
        if (up->mTag == tag) {
            return up.get();
        }
    }
    registry.emplace_back(new DASlowLogCounter(tag));
    return registry.back().get();
}

void DASlowLog::flushCounters()
{
    std::lock_guard< std::mutex > lk(counterRegistryMutex());
    auto& registry = counterRegistry();
    for (const auto& up : registry) {
        DASlowLogCounter* c       = up.get();
        const quint64 count       = c->mCount.exchange(0, std::memory_order_relaxed);
        const quint64 totalNs     = c->mTotalNs.exchange(0, std::memory_order_relaxed);
        const quint64 maxNs       = c->mMaxNs.exchange(0, std::memory_order_relaxed);
        if (count == 0) {
            continue;
        }
        emitCounterRecord(c->mTag.c_str(), count, totalNs, maxNs);
    }
}

void DASlowLog::setMainThreadId(std::thread::id id)
{
    sMainThreadId = id;
    sMainThreadIdSet.store(true, std::memory_order_release);
}

bool DASlowLog::isMainThread()
{
    return sMainThreadIdSet.load(std::memory_order_acquire) && std::this_thread::get_id() == sMainThreadId;
}

DASlowLog::ScopeInfo DASlowLog::currentMainThreadScope()
{
    const ScopeFrame* f = sMainThreadFrame.load(std::memory_order_acquire);
    ScopeInfo info;
    if (f) {
        info.tag  = f->tag;
        info.file = f->file;
        info.line = f->line;
        info.func = f->func;
    }
    return info;
}

bool DASlowLog::inEmission()
{
    return tlsInEmission;
}

void DASlowLog::emitScopeRecord(const char* tag,
                                qint64 elapsedNs,
                                qint64 selfNs,
                                int depth,
                                const char* file,
                                int line,
                                const char* func,
                                const QString& detail,
                                const char* threadLabel)
{
    QString lineStr = QStringLiteral("[slow] tag=%1 ms=%2 self_ms=%3 depth=%4 thread=%5 file=%6 line=%7 func=%8")
                          .arg(QString::fromUtf8(tag ? tag : "-"))
                          .arg(nsToMs(elapsedNs))
                          .arg(nsToMs(selfNs < 0 ? 0 : selfNs))
                          .arg(depth)
                          .arg(threadLabel ? QString::fromLatin1(threadLabel) : currentThreadLabel())
                          .arg(QString::fromUtf8(baseFileName(file)))
                          .arg(line)
                          .arg(cleanedFuncName(func));
    if (!detail.isEmpty()) {
        lineStr += QStringLiteral(" detail=\"%1\"").arg(detail);
    }
    emitLine(lineStr);
}

void DASlowLog::emitCounterRecord(const char* tag, quint64 count, quint64 totalNs, quint64 maxNs)
{
    emitLine(QStringLiteral("[slow] tag=%1 count=%2 total_ms=%3 max_ms=%4 window_ms=%5")
                 .arg(QString::fromUtf8(tag ? tag : "-"))
                 .arg(count)
                 .arg(nsToMs(totalNs))
                 .arg(nsToMs(maxNs))
                 .arg(aggregateIntervalMs()));
}

void DASlowLog::emitEventAggregate(const char* eventType, quint64 count, quint64 totalNs, quint64 maxNs)
{
    emitLine(QStringLiteral("[slow] tag=qt.event.aggregate type=%1 count=%2 total_ms=%3 max_ms=%4 window_ms=%5")
                 .arg(QString::fromUtf8(eventType ? eventType : "-"))
                 .arg(count)
                 .arg(nsToMs(totalNs))
                 .arg(nsToMs(maxNs))
                 .arg(aggregateIntervalMs()));
}

void DASlowLog::reloadConfig()
{
    std::lock_guard< std::mutex > lk(sConfigMutex);
    const DASlowLogConfig* c = parseConfigFromEnv();
    sConfig.store(c, std::memory_order_release);  // 旧配置故意泄漏
}

//===================================================
// DASlowLogScope
//===================================================

DASlowLogScope::DASlowLogScope(const char* tag, const char* file, int line, const char* func)
    : mTag(tag), mFile(file), mFunc(func), mLine(line), mThresholdNs(0), mStartNs(0), mFrameIndex(-1), mActive(false)
{
    begin(DASlowLog::thresholdMs(tag));
}

DASlowLogScope::DASlowLogScope(const char* tag, qint64 thresholdMs, const char* file, int line, const char* func)
    : mTag(tag), mFile(file), mFunc(func), mLine(line), mThresholdNs(0), mStartNs(0), mFrameIndex(-1), mActive(false)
{
    begin(thresholdMs);
}

void DASlowLogScope::begin(qint64 thresholdMs)
{
    mThresholdNs = thresholdMs * 1000000;
    if (!DASlowLog::isEnabled() || DASlowLog::inEmission()) {
        // 运行期关闭或处于输出路径中：不做任何记录
        return;
    }
    mActive  = true;
    mStartNs = nowNs();
    ScopeStack& stack = tlsScopeStack();
    if (stack.depth < kMaxScopeDepth) {
        mFrameIndex    = stack.depth;
        ScopeFrame& f  = stack.frames[mFrameIndex];
        f.tag          = mTag;
        f.file         = mFile;
        f.line         = mLine;
        f.func         = mFunc;
        f.thresholdNs  = mThresholdNs;
        f.startNs      = mStartNs;
        f.childNs      = 0;
        ++stack.depth;
        if (DASlowLog::isMainThread()) {
            sMainThreadFrame.store(&f, std::memory_order_release);
        }
    } else {
        // 栈满：停止发布栈帧，计时与输出继续
        mFrameIndex = -1;
    }
}

DASlowLogScope::~DASlowLogScope()
{
    if (!mActive) {
        return;
    }
    const qint64 elapsedNs = nowNs() - mStartNs;
    qint64 childNs         = 0;
    int depth              = 0;
    ScopeStack& stack      = tlsScopeStack();
    if (mFrameIndex >= 0) {
        childNs = stack.frames[mFrameIndex].childNs;
        depth   = mFrameIndex;
        stack.depth = mFrameIndex;  // 弹栈（正常嵌套即 depth-1）
        if (DASlowLog::isMainThread()) {
            sMainThreadFrame.store(mFrameIndex > 0 ? &stack.frames[mFrameIndex - 1] : nullptr,
                                   std::memory_order_release);
        }
        if (mFrameIndex > 0) {
            stack.frames[mFrameIndex - 1].childNs += elapsedNs;
        }
    } else {
        depth = stack.depth;  // 栈满未压帧：以当前深度记录
    }
    if (elapsedNs >= mThresholdNs) {
        const QString detail = mDetail ? mDetail() : QString();
        DASlowLog::emitScopeRecord(mTag, elapsedNs, elapsedNs - childNs, depth, mFile, mLine, mFunc, detail, nullptr);
    }
    mDetail = nullptr;  // 释放详情 lambda 持有的捕获引用
}

//===================================================
// DASlowLogCounter / DASlowLogCounterScope
//===================================================

DASlowLogCounter::DASlowLogCounter(const char* tag) : mTag(tag ? tag : "")
{
}

void DASlowLogCounter::add(qint64 ns)
{
    const quint64 v = static_cast< quint64 >(ns < 0 ? 0 : ns);
    mCount.fetch_add(1, std::memory_order_relaxed);
    mTotalNs.fetch_add(v, std::memory_order_relaxed);
    quint64 prev = mMaxNs.load(std::memory_order_relaxed);
    while (prev < v && !mMaxNs.compare_exchange_weak(prev, v, std::memory_order_relaxed)) {
        // prev 被 compare_exchange_weak 更新为当前值，继续循环
    }
}

DASlowLogCounterScope::DASlowLogCounterScope(DASlowLogCounter* counter) : mCounter(nullptr)
{
    if (counter && DASlowLog::isEnabled() && !DASlowLog::inEmission()) {
        mCounter = counter;
        mStart   = std::chrono::steady_clock::now();
    }
}

DASlowLogCounterScope::~DASlowLogCounterScope()
{
    if (!mCounter) {
        return;
    }
    const qint64 ns = std::chrono::duration_cast< std::chrono::nanoseconds >(
                          std::chrono::steady_clock::now() - mStart)
                          .count();
    mCounter->add(ns);
}

}  // namespace DA
