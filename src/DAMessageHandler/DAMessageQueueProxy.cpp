#include "DAMessageQueueProxy.h"
// stl
#include <memory>
// Qt
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QTimer>
#include <QApplication>
namespace DA
{
class DAMessageQueueProxyPrivate
{
    DA_IMPL_PUBLIC(DAMessageQueueProxy)
public:
    enum EmitSignalType
    {
        SignalQueueAppended    = 0,
        SignalQueueSizeChanged = 1
    };
    DAMessageQueueProxyPrivate(DAMessageQueueProxy* p);
    ~DAMessageQueueProxyPrivate();
    //_DAThreadSafeMessageQueue通知DAGlobalMessageQueuePrivate，队列有东西插入
    void queueAppended();

    //_DAThreadSafeMessageQueue通知DAGlobalMessageQueuePrivate，队列的尺寸变化了
    void queueSizeChanged();

    //构建timer
    void buildTimer(int intervalms);

    //设置发射间隔
    void setEmitInterval(int ms);
    int getEmitInterval() const;
    //设置惰性发射
    void setLazyEmit(bool on);
    bool isLazyEmit() const;
    //这个是为了避免事件循环未启动就创建timer
    void delayCreateCheck();

public:
    bool _isLazyEmit;                      ///< 信号是否惰性触发
    std::unique_ptr< QTimer > _timer;      ///< 信号发射间隔
    int _emitIntervalms;                   ///发射间隔
    bool _needEmitSignalQueueAppended;     ///< 标记需要发射信号
    bool _needEmitSignalQueueSizeChanged;  ///< 标记需要发射信号
    bool _delayCreateTimerBeforeEventLoopUp;  ///< 这个是标记timer需要创建，但由于app的事件循环还未建立，因此要延迟create
};

/**
 * @brief 一个线程安全的队列，所有的@sa DAGlobalMessageQueue 都会向这个队列注册
 *
 * 这个使用单例，避免全局变量的初始化顺序问题导致异常
 */
class _DAThreadSafeMessageQueue
{
private:
    _DAThreadSafeMessageQueue() : _capacity(1000)
    {
    }

public:
    static _DAThreadSafeMessageQueue& getInstance()
    {
        static _DAThreadSafeMessageQueue s_queue;
        return s_queue;
    }

    int size() const
    {
        QMutexLocker lc(&_mutex);
        return _messages.size();
    }

    DAMessageLogItem at(int index) const
    {
        QMutexLocker lc(&_mutex);
        return _messages.value(index);
    }

    void append(const DAMessageLogItem& item)
    {
        bool needNotifySizeChanged = false;

        {
            QMutexLocker lc(&_mutex);
            if (_messages.size() >= _capacity) {
                _messages.pop_front();
            } else {
                //通知队列的尺寸变化了
                needNotifySizeChanged = true;
            }
            _messages.append(item);
        }
        //通知队列有东西插入了
        {
            QMutexLocker lc(&_mutexNotifys);
            if (needNotifySizeChanged) {
                for (DAMessageQueueProxyPrivate* p : qAsConst(_notifys)) {
                    p->queueSizeChanged();
                }
            }
            for (DAMessageQueueProxyPrivate* p : qAsConst(_notifys)) {
                p->queueAppended();
            }
        }
    }

    void setCapacity(int c)
    {
        QMutexLocker lc(&_mutex);
        _capacity = c;
    }

    int getCapacity()
    {
        QMutexLocker lc(&_mutex);
        return _capacity;
    }

    void registerNotify(DAMessageQueueProxyPrivate* p)
    {
        QMutexLocker lc(&_mutexNotifys);
        _notifys.append(p);
    }

    void unregisterNotify(DAMessageQueueProxyPrivate* p)
    {
        QMutexLocker lc(&_mutexNotifys);
        _notifys.removeAll(p);
    }

    void clear()
    {
        QMutexLocker lc(&_mutex);
        int oldsize = _messages.size();
        _messages.clear();

        if (oldsize != 0) {
            QMutexLocker lcNotifys(&_mutexNotifys);
            for (DAMessageQueueProxyPrivate* p : qAsConst(_notifys)) {
                p->queueSizeChanged();
            }
        }
    }

private:
    mutable QMutex _mutex;
    mutable QMutex _mutexNotifys;
    QList< DAMessageLogItem > _messages;            ///< 消息
    int _capacity;                                  ///< 容量
    QList< DAMessageQueueProxyPrivate* > _notifys;  ///< 等待通知的
};

//===================================================
// DAMessageQueueProxyPrivate
//===================================================
DAMessageQueueProxyPrivate::DAMessageQueueProxyPrivate(DAMessageQueueProxy* p)
    : q_ptr(p), _isLazyEmit(true), _emitIntervalms(1000), _delayCreateTimerBeforeEventLoopUp(false)
{
    _DAThreadSafeMessageQueue::getInstance().registerNotify(this);
    //默认是惰性发射
    buildTimer(_emitIntervalms);
}

DAMessageQueueProxyPrivate::~DAMessageQueueProxyPrivate()
{
    _DAThreadSafeMessageQueue::getInstance().unregisterNotify(this);
}

void DAMessageQueueProxyPrivate::queueAppended()
{
    if (isLazyEmit()) {
        //惰性发射仅仅做标记
        delayCreateCheck();
        _needEmitSignalQueueAppended = true;
    } else {
        //非惰性发射立即发射信号
        q_ptr->emitSignal(SignalQueueAppended);
    }
}

void DAMessageQueueProxyPrivate::queueSizeChanged()
{
    if (isLazyEmit()) {
        //惰性发射仅仅做标记
        delayCreateCheck();
        _needEmitSignalQueueSizeChanged = true;
    } else {
        //非惰性发射立即发射信号
        q_ptr->emitSignal(SignalQueueSizeChanged);
    }
}

void DAMessageQueueProxyPrivate::buildTimer(int intervalms)
{
    if (QApplication::startingUp() || QApplication::closingDown()) {
        //如果app还未启动,或者已经关闭不发射信号
        _delayCreateTimerBeforeEventLoopUp = true;
        return;
    }
    _timer.reset(new QTimer());
    _timer->setInterval(intervalms);
    QObject::connect(_timer.get(), &QTimer::timeout, q_ptr, &DAMessageQueueProxy::onTimeout);
    _timer->start();
}

void DAMessageQueueProxyPrivate::setEmitInterval(int ms)
{
    _emitIntervalms = ms;
    if (_timer) {
        _timer->setInterval(ms);
    }
}

int DAMessageQueueProxyPrivate::getEmitInterval() const
{
    return _emitIntervalms;
}

void DAMessageQueueProxyPrivate::setLazyEmit(bool on)
{
    _isLazyEmit = on;
    if (on) {
        if (nullptr == _timer) {
            buildTimer(_emitIntervalms);
        } else {
            if (_timer->interval() != _emitIntervalms) {
                _timer->setInterval(_emitIntervalms);
            }
        }
    } else {
        //不是惰性发射就删除timer
        _timer.reset(nullptr);
    }
}

bool DAMessageQueueProxyPrivate::isLazyEmit() const
{
    return _isLazyEmit;
}

void DAMessageQueueProxyPrivate::delayCreateCheck()
{
    if (_delayCreateTimerBeforeEventLoopUp) {
        if (_isLazyEmit) {
            _delayCreateTimerBeforeEventLoopUp = false;
            buildTimer(_emitIntervalms);
        }
    }
}
//===================================================
// DAMessageQueueProxy
//===================================================
DAMessageQueueProxy::DAMessageQueueProxy(QObject* par) : QObject(par), d_ptr(new DAMessageQueueProxyPrivate(this))
{
}

DAMessageQueueProxy::~DAMessageQueueProxy()
{
}

/**
 * @brief 设置信号发射间隔
 *
 * 在设置了lazyEmit才会起作用
 * @param ms
 */
void DAMessageQueueProxy::setEmitInterval(int ms)
{
    d_ptr->setEmitInterval(ms);
}
/**
 * @brief 获取信号发射间隔，次间隔只有在设置了惰性触发时起作用
 * @return
 */
int DAMessageQueueProxy::getEmitInterval() const
{
    return d_ptr->getEmitInterval();
}

void DAMessageQueueProxy::append(const DAMessageLogItem& item)
{
    _DAThreadSafeMessageQueue::getInstance().append(item);
}

DAMessageLogItem DAMessageQueueProxy::at(int index) const
{
    return _DAThreadSafeMessageQueue::getInstance().at(index);
}

int DAMessageQueueProxy::size() const
{
    return _DAThreadSafeMessageQueue::getInstance().size();
}

/**
 * @brief 设置信号惰性触发
 * @param on
 */
void DAMessageQueueProxy::setLazyEmit(bool on)
{
    d_ptr->setLazyEmit(on);
}

/**
 * @brief 判断信号是否惰性触发
 * @return
 */
bool DAMessageQueueProxy::isLazyEmit() const
{
    return d_ptr->isLazyEmit();
}

/**
 * @brief 清空队列
 */
void DAMessageQueueProxy::clear()
{
    _DAThreadSafeMessageQueue::getInstance().clear();
}

/**
 * @brief 设置全局队列的容量
 * @param c
 */
void DAMessageQueueProxy::setGlobalQueueCapacity(int c)
{
    _DAThreadSafeMessageQueue::getInstance().setCapacity(c);
}

/**
 * @brief 获取全局队列的容量
 * @return
 */
int DAMessageQueueProxy::getGlobalQueueCapacity()
{
    return _DAThreadSafeMessageQueue::getInstance().getCapacity();
}

/**
 * @brief 计时器到达
 */
void DAMessageQueueProxy::onTimeout()
{
    if (d_ptr->_needEmitSignalQueueAppended) {
        emit messageQueueAppended();
    }
    if (d_ptr->_needEmitSignalQueueSizeChanged) {
        emit messageQueueSizeChanged(_DAThreadSafeMessageQueue::getInstance().size());
    }
}

void DAMessageQueueProxy::emitSignal(int type)
{
    if (QApplication::startingUp() || QApplication::closingDown()) {
        //如果app还未启动,或者已经关闭不发射信号
        return;
    }
    switch (type) {
    case DAMessageQueueProxyPrivate::SignalQueueAppended:
        emit messageQueueAppended();
        break;
    case DAMessageQueueProxyPrivate::SignalQueueSizeChanged:
        emit messageQueueSizeChanged(_DAThreadSafeMessageQueue::getInstance().size());
        break;
    default:
        break;
    }
}

}  // namespace DA
