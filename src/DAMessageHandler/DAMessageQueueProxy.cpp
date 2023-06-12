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
class DAMessageQueueProxy::PrivateData
{
    DA_DECLARE_PUBLIC(DAMessageQueueProxy)
public:
    enum EmitSignalType
    {
        SignalQueueAppended    = 0,
        SignalQueueSizeChanged = 1
    };
    PrivateData(DAMessageQueueProxy* p);
    ~PrivateData();
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
    bool mIsLazyEmit { true };                       ///< 信号是否惰性触发
    std::unique_ptr< QTimer > mTimer;                ///< 信号发射间隔
    int mEmitIntervalms { 1000 };                    ///发射间隔
    bool mNeedEmitSignalQueueAppended { false };     ///< 标记需要发射信号
    bool mNeedEmitSignalQueueSizeChanged { false };  ///< 标记需要发射信号
    bool mDelayCreateTimerBeforeEventLoopUp { false };  ///< 这个是标记timer需要创建，但由于app的事件循环还未建立，因此要延迟create
};

/**
 * @brief 一个线程安全的队列，所有的@sa DAGlobalMessageQueue 都会向这个队列注册
 *
 * 这个使用单例，避免全局变量的初始化顺序问题导致异常
 */
class DAThreadSafeMessageQueue_Private
{
private:
    DAThreadSafeMessageQueue_Private() : mCapacity(1000)
    {
    }

public:
    static DAThreadSafeMessageQueue_Private& getInstance()
    {
        static DAThreadSafeMessageQueue_Private s_queue;
        return s_queue;
    }

    int size() const
    {
        QMutexLocker lc(&mMutex);
        return mMessages.size();
    }

    DAMessageLogItem at(int index) const
    {
        QMutexLocker lc(&mMutex);
        return mMessages.value(index);
    }

    void append(const DAMessageLogItem& item)
    {
        bool needNotifySizeChanged = false;

        {
            QMutexLocker lc(&mMutex);
            if (mMessages.size() >= mCapacity) {
                mMessages.pop_front();
            } else {
                //通知队列的尺寸变化了
                needNotifySizeChanged = true;
            }
            mMessages.append(item);
        }
        //通知队列有东西插入了
        {
            QMutexLocker lc(&mMutexNotifys);
            if (needNotifySizeChanged) {
                for (DAMessageQueueProxy::PrivateData* p : qAsConst(mNotifys)) {
                    p->queueSizeChanged();
                }
            }
            for (DAMessageQueueProxy::PrivateData* p : qAsConst(mNotifys)) {
                p->queueAppended();
            }
        }
    }

    void setCapacity(int c)
    {
        QMutexLocker lc(&mMutex);
        mCapacity = c;
    }

    int getCapacity()
    {
        QMutexLocker lc(&mMutex);
        return mCapacity;
    }

    void registerNotify(DAMessageQueueProxy::PrivateData* p)
    {
        QMutexLocker lc(&mMutexNotifys);
        mNotifys.append(p);
    }

    void unregisterNotify(DAMessageQueueProxy::PrivateData* p)
    {
        QMutexLocker lc(&mMutexNotifys);
        mNotifys.removeAll(p);
    }

    void clear()
    {
        QMutexLocker lc(&mMutex);
        int oldsize = mMessages.size();
        mMessages.clear();

        if (oldsize != 0) {
            QMutexLocker lcNotifys(&mMutexNotifys);
            for (DAMessageQueueProxy::PrivateData* p : qAsConst(mNotifys)) {
                p->queueSizeChanged();
            }
        }
    }

private:
    mutable QMutex mMutex;
    mutable QMutex mMutexNotifys;
    QList< DAMessageLogItem > mMessages;                  ///< 消息
    int mCapacity;                                        ///< 容量
    QList< DAMessageQueueProxy::PrivateData* > mNotifys;  ///< 等待通知的
};

//===================================================
// DAMessageQueueProxyPrivate
//===================================================
DAMessageQueueProxy::PrivateData::PrivateData(DAMessageQueueProxy* p) : q_ptr(p)
{
    DAThreadSafeMessageQueue_Private::getInstance().registerNotify(this);
    //默认是惰性发射
    buildTimer(mEmitIntervalms);
}

DAMessageQueueProxy::PrivateData::~PrivateData()
{
    DAThreadSafeMessageQueue_Private::getInstance().unregisterNotify(this);
}

void DAMessageQueueProxy::PrivateData::queueAppended()
{
    if (isLazyEmit()) {
        //惰性发射仅仅做标记
        //        delayCreateCheck();
        mNeedEmitSignalQueueAppended = true;
    } else {
        //非惰性发射立即发射信号
        q_ptr->emitSignal(SignalQueueAppended);
    }
}

void DAMessageQueueProxy::PrivateData::queueSizeChanged()
{
    if (isLazyEmit()) {
        //惰性发射仅仅做标记
        //        delayCreateCheck();
        mNeedEmitSignalQueueSizeChanged = true;
    } else {
        //非惰性发射立即发射信号
        q_ptr->emitSignal(SignalQueueSizeChanged);
    }
}

void DAMessageQueueProxy::PrivateData::buildTimer(int intervalms)
{
    if (QApplication::startingUp() || QApplication::closingDown()) {
        //如果app还未启动,或者已经关闭不发射信号
        mDelayCreateTimerBeforeEventLoopUp = true;
        return;
    }
    if (!mTimer) {
        //如果timer没有建立，就建立一个timer
        mTimer.reset(new QTimer());
        mTimer->setInterval(intervalms);
        QObject::connect(mTimer.get(), &QTimer::timeout, q_ptr, &DAMessageQueueProxy::onTimeout);
        mTimer->start();
    }
}

void DAMessageQueueProxy::PrivateData::setEmitInterval(int ms)
{
    mEmitIntervalms = ms;
    if (mTimer) {
        mTimer->setInterval(ms);
    }
}

int DAMessageQueueProxy::PrivateData::getEmitInterval() const
{
    return mEmitIntervalms;
}

void DAMessageQueueProxy::PrivateData::setLazyEmit(bool on)
{
    mIsLazyEmit = on;
    if (on) {
        if (nullptr == mTimer) {
            buildTimer(mEmitIntervalms);
        } else {
            if (mTimer->interval() != mEmitIntervalms) {
                mTimer->setInterval(mEmitIntervalms);
            }
        }
    } else {
        //不是惰性发射就删除timer
        mTimer.reset(nullptr);
    }
}

bool DAMessageQueueProxy::PrivateData::isLazyEmit() const
{
    return mIsLazyEmit;
}

void DAMessageQueueProxy::PrivateData::delayCreateCheck()
{
    if (mDelayCreateTimerBeforeEventLoopUp) {
        if (mIsLazyEmit) {
            mDelayCreateTimerBeforeEventLoopUp = false;
            buildTimer(mEmitIntervalms);
        }
    }
}
//===================================================
// DAMessageQueueProxy
//===================================================
DAMessageQueueProxy::DAMessageQueueProxy(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
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
    DAThreadSafeMessageQueue_Private::getInstance().append(item);
}

DAMessageLogItem DAMessageQueueProxy::at(int index) const
{
    return DAThreadSafeMessageQueue_Private::getInstance().at(index);
}

int DAMessageQueueProxy::size() const
{
    return DAThreadSafeMessageQueue_Private::getInstance().size();
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
    DAThreadSafeMessageQueue_Private::getInstance().clear();
}

/**
 * @brief 设置全局队列的容量
 * @param c
 */
void DAMessageQueueProxy::setGlobalQueueCapacity(int c)
{
    DAThreadSafeMessageQueue_Private::getInstance().setCapacity(c);
}

/**
 * @brief 获取全局队列的容量
 * @return
 */
int DAMessageQueueProxy::getGlobalQueueCapacity()
{
    return DAThreadSafeMessageQueue_Private::getInstance().getCapacity();
}

/**
 * @brief 计时器到达
 */
void DAMessageQueueProxy::onTimeout()
{
    if (d_ptr->mNeedEmitSignalQueueAppended) {
        d_ptr->mNeedEmitSignalQueueAppended = false;
        emit messageQueueAppended();
    }
    if (d_ptr->mNeedEmitSignalQueueSizeChanged) {
        d_ptr->mNeedEmitSignalQueueSizeChanged = false;
        emit messageQueueSizeChanged(DAThreadSafeMessageQueue_Private::getInstance().size());
    }
}

void DAMessageQueueProxy::emitSignal(int type)
{
    if (QApplication::startingUp() || QApplication::closingDown()) {
        //如果app还未启动,或者已经关闭不发射信号
        return;
    }
    switch (type) {
    case DAMessageQueueProxy::PrivateData::SignalQueueAppended:
        emit messageQueueAppended();
        break;
    case DAMessageQueueProxy::PrivateData::SignalQueueSizeChanged:
        emit messageQueueSizeChanged(DAThreadSafeMessageQueue_Private::getInstance().size());
        break;
    default:
        break;
    }
}

}  // namespace DA
