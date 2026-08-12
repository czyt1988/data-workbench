#ifndef DAPYTHONSIGNALHANDLER_H
#define DAPYTHONSIGNALHANDLER_H
#include <QObject>
#include <DAPyBindQtGlobal.h>
#include <functional>
#include <memory>
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
    DA_DECLARE_PRIVATE(DAPythonSignalHandler)

public:
    explicit DAPythonSignalHandler(QObject* parent = nullptr);
    ~DAPythonSignalHandler() override;

    // 删除拷贝构造和赋值操作符
    DAPythonSignalHandler(const DAPythonSignalHandler&)            = delete;
    DAPythonSignalHandler& operator=(const DAPythonSignalHandler&) = delete;

    // 从Python线程调用，请求在主线程执行函数
    void callInMainThread(std::function< void() > func);

    // 清理所有待执行的函数
    void clearPendingFunctions();

Q_SIGNALS:
    /**
     * @brief 内部信号，用于触发主线程执行
     * @param funcWrapperId 函数包装器的唯一ID
     */
    void executeRequested(int funcWrapperId);

private Q_SLOTS:
    // 在主线程执行的槽函数
    void onExecuteRequested(int funcWrapperId);

public:
    // 函数包装器，用于存储待执行的函数
    class FunctionWrapper
    {
    public:
        explicit FunctionWrapper(std::function< void() > func) : mFunc(func)
        {
        }
        void execute()
        {
            if (mFunc)
                mFunc();
        }

    private:
        std::function< void() > mFunc;
    };

    // 使用智能指针管理函数包装器
    using FunctionWrapperPtr = std::shared_ptr< FunctionWrapper >;
};

}  // end DA
#endif  // DAPYTHONSIGNALHANDLER_H
