#ifndef DAPYTHONSIGNALHANDLER_H
#define DAPYTHONSIGNALHANDLER_H
#include <QObject>
#include <QMap>
#include <DAPyBindQtGlobal.h>
#include <functional>
#include <memory>
#include <mutex>
#include "DAPyBindQtGlobal.h"
namespace DA
{
/**
 * @brief Python线程到Qt主线程的通信处理器
 *
 * 这个类允许Python线程通过信号槽机制安全地调用Qt主线程中的函数
 * 非单例模式，由主窗口或其他容器管理生命周期
 */
class DAPYBINDQT_API DAPythonSignalHandler : public QObject
{
    Q_OBJECT

public:
    explicit DAPythonSignalHandler(QObject* parent = nullptr);
    virtual ~DAPythonSignalHandler();

    // 删除拷贝构造和赋值操作符
    DAPythonSignalHandler(const DAPythonSignalHandler&)            = delete;
    DAPythonSignalHandler& operator=(const DAPythonSignalHandler&) = delete;

    /**
     * @brief 从Python线程调用，请求在主线程执行函数
     * @param func 要在主线程执行的函数
     *
     * 这个函数是线程安全的，可以从任何线程调用
     */
    void callInMainThread(std::function< void() > func);

    /**
     * @brief 清理所有待执行的函数
     *
     * 在主窗口销毁前调用，确保所有函数都被清理
     */
    void clearPendingFunctions();

Q_SIGNALS:
    /**
     * @brief 内部信号，用于触发主线程执行
     * @param funcWrapperId 函数包装器的唯一ID
     */
    void executeRequested(int funcWrapperId);

private Q_SLOTS:
    /**
     * @brief 在主线程执行的槽函数
     * @param funcWrapperId 函数包装器的ID
     */
    void onExecuteRequested(int funcWrapperId);

public:
    // 函数包装器，用于存储待执行的函数
    class FunctionWrapper
    {
    public:
        explicit FunctionWrapper(std::function< void() > func) : m_func(func)
        {
        }
        void execute()
        {
            if (m_func)
                m_func();
        }

    private:
        std::function< void() > m_func;
    };

    // 使用智能指针管理函数包装器
    using FunctionWrapperPtr = std::shared_ptr< FunctionWrapper >;

private:
    // 存储函数包装器的映射，键是唯一ID
    QMap< int, FunctionWrapperPtr > m_functionMap;

    // 线程安全保护
    std::mutex m_mutex;

    // 下一个函数包装器的ID
    int m_nextFuncId { 0 };

    // 是否正在销毁中
    bool m_destroying { false };
};
}  // end DA
#endif  // DAPYTHONSIGNALHANDLER_H
