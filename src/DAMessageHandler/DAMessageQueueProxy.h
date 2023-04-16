#ifndef DAMESSAGEQUEUEPROXY_H
#define DAMESSAGEQUEUEPROXY_H
#include "DAMessageHandlerGlobal.h"
#include "DAMessageLogItem.h"
#include <QObject>
namespace DA
{

/**
 * @brief 这是一个全局的日志队列，所有的qdebug相关的消息都会推入这个队列中
 *
 * 此队列提供一个惰性的信号，所谓惰性信号是指不会一直触发，而是会限制触发的频度，避免过多的消息导致
 * 系统缓慢，默认每个信号间隔1s，可以通过setSignalEmitInterval来改变
 *
 * @note 由于使用了惰性信号，此类的信号只会触发一个，例如@sa messageQueueSizeChanged 是在队列尺寸发生变化时触发，
 * 而@sa messageQueueAppended 是在有消息插入的时候触发，理论上，@sa messageQueueSizeChanged 触发，那么必然@sa
 * messageQueueAppended 会被触发，但是，由于内部会有一个全局的间隔时间限制了信号的触发频率，因此，@sa
 * messageQueueSizeChanged 触发后 并不会触发@sa messageQueueAppended
 */
class DAMESSAGEHANDLER_API DAMessageQueueProxy : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAMessageQueueProxy)
    friend class DAThreadSafeMessageQueue_Private;

public:
    DAMessageQueueProxy(QObject* par = nullptr);
    ~DAMessageQueueProxy();
    //设置信号发射间隔
    void setEmitInterval(int ms);
    int getEmitInterval() const;

    //插入一个日志内容
    void append(const DAMessageLogItem& item);

    //消息内容
    DAMessageLogItem at(int index) const;

    //尺寸
    int size() const;

    //设置惰性触发
    void setLazyEmit(bool on = true);
    bool isLazyEmit() const;

    //清空队列
    void clear();

    //设置全局队列的容量
    static void setGlobalQueueCapacity(int c);

    //获取全局队列的尺寸
    static int getGlobalQueueCapacity();
private slots:
    void onTimeout();
signals:
    /**
     * @brief 有消息插入
     * @note 此信号触发@sa messageQueueSizeChanged 信号就不会触发，这两个是互斥
     * @note 正常会先触发@sa messageQueueSizeChanged 信号，等全局队列缓冲满后才会触发@sa messageQueueAppended 信号
     */
    void messageQueueAppended();
    /**
     * @brief 消息的队列尺寸发生了变化
     * @param newSize 消息队列的尺寸
     * @note 此信号触发@sa messageQueueAppended 信号就不会触发，这两个是互斥
     * @note 正常会先触发@sa messageQueueSizeChanged 信号，等全局队列缓冲满后才会触发@sa messageQueueAppended 信号
     */
    void messageQueueSizeChanged(int newSize);

protected:
    void emitSignal(int type);
};
}  // namespace DA
#endif  // DAGLOBALLOGQUEUE_H
